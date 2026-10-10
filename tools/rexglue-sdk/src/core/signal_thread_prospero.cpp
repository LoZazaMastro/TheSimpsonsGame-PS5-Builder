// PS5 signal handlers must not re-enter LLVM's allocating emulated TLS runtime.
// BSD 3-Clause License. See the project LICENSE.
#include <rex/platform/signal_thread_prospero.h>
#include <rex/platform/trace_phases_prospero.h>

#include <atomic>
#include <cerrno>
#include <pthread.h>

namespace {
struct Slot {
  std::atomic<uintptr_t> owner{0};
  std::atomic<unsigned> ready{0};
  std::atomic<void*> context{nullptr};
  std::atomic<ProsperoSuspendWait> wait{nullptr};
  std::atomic<unsigned> seh_active{0};
  std::atomic<uint32_t> seh_code{0};
  std::atomic<uintptr_t> seh_address{0};
};
static_assert(std::atomic<uintptr_t>::is_always_lock_free &&
              std::atomic<unsigned>::is_always_lock_free &&
              std::atomic<void*>::is_always_lock_free &&
              std::atomic<ProsperoSuspendWait>::is_always_lock_free &&
              std::atomic<uint32_t>::is_always_lock_free);
constinit Slot slots[PROSPERO_SIGNAL_THREAD_CAPACITY]{};
pthread_once_t cleanup_once = PTHREAD_ONCE_INIT;
pthread_key_t cleanup_key;
int cleanup_status = EAGAIN;

uintptr_t CurrentThread() noexcept {
  // The native libkernel accessor also identifies trace recorder ownership.
  // Do not replace this with a C/C++ thread_local or __emutls_get_address.
  return (uintptr_t)pthread_self();
}

Slot* Find(uintptr_t owner) noexcept {
  if (!owner) return nullptr;
  for (Slot& slot : slots) {
    if (slot.owner.load(std::memory_order_acquire) == owner &&
        slot.ready.load(std::memory_order_acquire)) return &slot;
  }
  return nullptr;
}

void Clear(Slot& slot, uintptr_t owner) noexcept {
  // Only the owning current thread changes or clears its slot. A signal may
  // interrupt that thread, but cannot run concurrently with its interrupted
  // instructions. Other threads can only claim an owner==0 slot.
  if (!owner || slot.owner.load(std::memory_order_acquire) != owner) return;
  slot.ready.store(0, std::memory_order_release);
  slot.wait.store(nullptr, std::memory_order_relaxed);
  slot.context.store(nullptr, std::memory_order_relaxed);
  slot.seh_active.store(0, std::memory_order_relaxed);
  slot.seh_code.store(0, std::memory_order_relaxed);
  slot.seh_address.store(0, std::memory_order_relaxed);
  slot.owner.store(0, std::memory_order_release);
}

void Cleanup(void* opaque) noexcept {
  // Explicit earlier cleanup may have made this slot available to another
  // thread. The old key callback must not clear that new owner's state.
  if (opaque) Clear(*static_cast<Slot*>(opaque), CurrentThread());
}

void InitializeCleanup() noexcept {
  cleanup_status = pthread_key_create(&cleanup_key, Cleanup);
}

Slot* Acquire(int& error) noexcept {
  const uintptr_t owner = CurrentThread();
  if (Slot* existing = Find(owner)) return existing;
  if (!owner) { error = EINVAL; return nullptr; }
  error = pthread_once(&cleanup_once, InitializeCleanup);
  if (!error) error = cleanup_status;
  if (error) return nullptr;
  for (Slot& slot : slots) {
    uintptr_t empty = 0;
    if (!slot.owner.compare_exchange_strong(empty, owner, std::memory_order_acq_rel,
                                            std::memory_order_relaxed)) continue;
    // ready stays false until both the exit hook and all fields exist. A
    // signal in this interval observes no binding and never initializes one.
    slot.ready.store(0, std::memory_order_relaxed);
    slot.context.store(nullptr, std::memory_order_relaxed);
    slot.wait.store(nullptr, std::memory_order_relaxed);
    slot.seh_active.store(0, std::memory_order_relaxed);
    slot.seh_code.store(0, std::memory_order_relaxed);
    slot.seh_address.store(0, std::memory_order_relaxed);
    error = pthread_setspecific(cleanup_key, &slot);
    if (error) { Clear(slot, owner); return nullptr; }
    slot.ready.store(1, std::memory_order_release);
    return &slot;
  }
  error = ENOSPC;
  return nullptr;
}

int RegistrationError(int error) noexcept {
  prospero_trace_checkpoint(PROSPERO_PHASE_SIGNAL_THREAD_REGISTRATION_FAILED,
                           unsigned(error), PROSPERO_SIGNAL_THREAD_CAPACITY, 0);
  return error;
}
}  // namespace

extern "C" int prospero_signal_bind_suspend(void* context, ProsperoSuspendWait wait) noexcept {
  if (!context || !wait) return RegistrationError(EINVAL);
  int error = 0;
  Slot* slot = Acquire(error);
  if (!slot) return RegistrationError(error);
  const auto previous = slot->wait.load(std::memory_order_acquire);
  if (previous) {
    return previous == wait && slot->context.load(std::memory_order_relaxed) == context
               ? 0 : RegistrationError(EBUSY);
  }
  // Publish the context before its callback. The handler sees either no
  // callback or this complete pair. Only the current owner updates it.
  slot->context.store(context, std::memory_order_relaxed);
  slot->wait.store(wait, std::memory_order_release);
  prospero_trace_checkpoint(PROSPERO_PHASE_SIGNAL_THREAD_BOUND,
                           reinterpret_cast<uintptr_t>(context),
                           reinterpret_cast<uintptr_t>(wait), 0);
  return 0;
}

extern "C" void prospero_signal_unbind_suspend(void* expected_context) noexcept {
  if (Slot* slot = Find(CurrentThread())) {
    if (slot->context.load(std::memory_order_relaxed) != expected_context) return;
    slot->wait.store(nullptr, std::memory_order_release);
    slot->context.store(nullptr, std::memory_order_relaxed);
  }
}

extern "C" int prospero_signal_dispatch_suspend(void) {
  Slot* slot = Find(CurrentThread());
  if (!slot) return 0;
  const auto wait = slot->wait.load(std::memory_order_acquire);
  if (!wait) return 0;
  void* const context = slot->context.load(std::memory_order_relaxed);
  if (!context) return 0;
  wait(context);
  return 1;
}

extern "C" void prospero_signal_thread_clear(void) noexcept {
  const uintptr_t owner = CurrentThread();
  // Include an incompletely published slot. No key call is needed here, so
  // this also works before the emutls key destructor drains C++ destructors.
  if (owner) for (Slot& slot : slots) Clear(slot, owner);
}

extern "C" int prospero_signal_seh_set_active(int active) noexcept {
  Slot* slot = Find(CurrentThread());
  if (!slot && !active) return 0;
  int error = 0;
  if (!slot) slot = Acquire(error);
  if (!slot) return RegistrationError(error);
  slot->seh_active.store(active ? 1u : 0u, std::memory_order_release);
  return 0;
}

extern "C" int prospero_signal_seh_active(void) noexcept {
  Slot* slot = Find(CurrentThread());
  return slot ? int(slot->seh_active.load(std::memory_order_acquire)) : 0;
}

extern "C" int prospero_signal_seh_capture(uint32_t code, uintptr_t address) noexcept {
  Slot* slot = Find(CurrentThread());
  if (!slot || !slot->seh_active.load(std::memory_order_acquire)) return 0;
  slot->seh_address.store(address, std::memory_order_relaxed);
  slot->seh_code.store(code, std::memory_order_release);
  return 1;
}

extern "C" int prospero_signal_seh_read(uint32_t* code, uintptr_t* information0,
                                        uintptr_t* information1) noexcept {
  if (!code || !information0 || !information1) return 0;
  Slot* slot = Find(CurrentThread());
  if (!slot) { *code = 0; *information0 = 0; *information1 = 0; return 0; }
  *code = slot->seh_code.load(std::memory_order_acquire);
  *information0 = 0;
  *information1 = slot->seh_address.load(std::memory_order_relaxed);
  return 1;
}
