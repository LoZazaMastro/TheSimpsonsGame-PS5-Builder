// Test the production signal registry. Native pthreads/signals exercise normal
// operation and exit. Explicit link-time syscall seams additionally test the
// entire fixed capacity, publication interruption, key errors and stale exits.
// These seams model thread identities; they do not establish the PS5 kernel ABI.
#include <rex/platform/signal_thread_prospero.h>

#include <array>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

extern "C" void* signal_test_with_native_cleanup(void*);
struct signal_test_cleanup_start {
  void (*run)(void*);
  void (*cleanup)(void*);
  void* context;
};

namespace {
thread_local uintptr_t simulated_owner = 0;
bool fail_key_create = false;
bool fail_key_set = false;
bool interrupt_publication = false;
pthread_key_t registry_key;
void (*registry_cleanup)(void*) = nullptr;
std::atomic<void*> last_slot{nullptr};
volatile sig_atomic_t signal_kind = 0;
volatile sig_atomic_t signal_count = 0;
volatile sig_atomic_t signal_error = 0;
volatile sig_atomic_t inside_signal = 0;
volatile sig_atomic_t unsafe_signal_calls = 0;
std::atomic<unsigned> callbacks{0};

struct Context { uintptr_t marker; };
constexpr uintptr_t kMarker = UINT64_C(0x189abcde234567f0);

void Callback(void* pointer) {
  auto* context = static_cast<Context*>(pointer);
  if (context->marker != kMarker) signal_error = 10;
  callbacks.fetch_add(1, std::memory_order_relaxed);
}

void OnSignal(int) {
  inside_signal = 1;
  const int dispatched = prospero_signal_dispatch_suspend();
  if (signal_kind == 1) {
    if (dispatched || prospero_signal_seh_active() ||
        prospero_signal_seh_capture(42, 43)) signal_error = 1;
  } else if (signal_kind == 2) {
    if (dispatched != 1 || !prospero_signal_seh_active() ||
        !prospero_signal_seh_capture(42, 43)) signal_error = 2;
  }
  signal_count = signal_count + 1;
  inside_signal = 0;
}

void InstallSignal() {
  struct sigaction action{};
  action.sa_handler = OnSignal;
  sigemptyset(&action.sa_mask);
  assert(sigaction(SIGUSR1, &action, nullptr) == 0);
}

void CountUnsafe() {
  if (inside_signal) unsafe_signal_calls = unsafe_signal_calls + 1;
}

void CheckQuiet() {
  assert(prospero_signal_dispatch_suspend() == 0);
  assert(prospero_signal_seh_active() == 0);
  assert(prospero_signal_seh_capture(42, 43) == 0);
}

void Basics() {
  Context context{kMarker}, other{kMarker};
  CheckQuiet();
  assert(prospero_signal_seh_set_active(0) == 0);
  assert(prospero_signal_bind_suspend(nullptr, Callback) == EINVAL);
  assert(prospero_signal_bind_suspend(&context, nullptr) == EINVAL);
  assert(prospero_signal_bind_suspend(&context, Callback) == 0);
  assert(prospero_signal_bind_suspend(&context, Callback) == 0);
  assert(prospero_signal_bind_suspend(&other, Callback) == EBUSY);
  assert(prospero_signal_seh_set_active(1) == 0);
  signal_kind = 2;
  raise(SIGUSR1);
  assert(signal_count == 1 && signal_error == 0 && callbacks == 1 && unsafe_signal_calls == 0);
  uint32_t code;
  uintptr_t information0, information1;
  assert(prospero_signal_seh_read(&code, &information0, &information1) == 1);
  assert(code == 42 && information0 == 0 && information1 == 43);
  assert(prospero_signal_seh_read(nullptr, &information0, &information1) == 0);
  prospero_signal_unbind_suspend(&other);
  assert(prospero_signal_dispatch_suspend() == 1);
  prospero_signal_unbind_suspend(&context);
  assert(prospero_signal_dispatch_suspend() == 0);
  assert(prospero_signal_bind_suspend(&other, Callback) == 0);
  prospero_signal_thread_clear();
  prospero_signal_thread_clear();
  CheckQuiet();
  code = 1; information0 = 1; information1 = 1;
  assert(prospero_signal_seh_read(&code, &information0, &information1) == 0);
  assert(code == 0 && information0 == 0 && information1 == 0);
  puts("PASS signal dispatch/capture, invalid and conflicting binds, precise unbind, idempotent cleanup");
}

void Publication() {
  Context context{kMarker};
  signal_kind = 1;
  interrupt_publication = true;
  assert(prospero_signal_bind_suspend(&context, Callback) == 0);
  interrupt_publication = false;
  assert(signal_count == 2 && signal_error == 0 && callbacks == 0 && unsafe_signal_calls == 0);
  assert(prospero_signal_dispatch_suspend() == 1);
  prospero_signal_thread_clear();
  puts("PASS signal before/after native key registration sees no incomplete binding and calls no TLS/key/once API");
}

void KeyErrors(bool creation) {
  Context context{kMarker};
  fail_key_create = creation;
  fail_key_set = !creation;
  assert(prospero_signal_bind_suspend(&context, Callback) == (creation ? EAGAIN : ENOMEM));
  CheckQuiet();
  if (creation) {
    // pthread_once completed: failure stays explicit; a broken key is never used.
    assert(prospero_signal_seh_set_active(1) == EAGAIN);
  } else {
    fail_key_set = false;
    assert(prospero_signal_bind_suspend(&context, Callback) == 0);
    assert(prospero_signal_dispatch_suspend() == 1);
    prospero_signal_thread_clear();
  }
  puts(creation ? "PASS native key creation failure is propagated, no false registration" :
                  "PASS native key publication failure rolls slot back and next attempt succeeds");
}

void Capacity() {
  Context context{kMarker};
  std::array<void*, PROSPERO_SIGNAL_THREAD_CAPACITY> saved{};
  for (unsigned i = 0; i < saved.size(); ++i) {
    simulated_owner = i + 1;
    assert(prospero_signal_bind_suspend(&context, Callback) == 0);
    assert(prospero_signal_seh_set_active(1) == 0);
    saved[i] = last_slot.load();
  }
  simulated_owner = saved.size() + 1;
  assert(prospero_signal_bind_suspend(&context, Callback) == ENOSPC);
  CheckQuiet();
  for (unsigned i = 0; i < saved.size(); ++i) {
    simulated_owner = i + 1;
    assert(prospero_signal_dispatch_suspend() == 1 && prospero_signal_seh_active());
  }
  simulated_owner = 1;
  prospero_signal_thread_clear();
  simulated_owner = saved.size() + 1;
  assert(prospero_signal_bind_suspend(&context, Callback) == 0);
  assert(last_slot.load() == saved[0]);
  simulated_owner = 1;
  registry_cleanup(saved[0]);  // Late key destructor from an old owner.
  simulated_owner = saved.size() + 1;
  assert(prospero_signal_dispatch_suspend() == 1);
  prospero_signal_thread_clear();
  for (unsigned i = 1; i < saved.size(); ++i) {
    simulated_owner = i + 1;
    registry_cleanup(saved[i]);
    CheckQuiet();
  }
  simulated_owner = 0;
  puts("PASS all 1024 slots, overflow keeps live owners intact, reuse and stale native destructor ownership");
}

void* ConcurrentThread(void*) {
  Context context{kMarker};
  for (unsigned repeat = 0; repeat < 300; ++repeat) {
    assert(prospero_signal_bind_suspend(&context, Callback) == 0);
    assert(prospero_signal_seh_set_active(1) == 0);
    assert(prospero_signal_dispatch_suspend() == 1);
    assert(prospero_signal_seh_capture(42, 43) == 1);
    if (repeat != 299) prospero_signal_thread_clear();
  }
  return nullptr;  // Last slot must be cleared by the native pthread key.
}

void Concurrent() {
  constexpr unsigned count = 24;
  std::array<pthread_t, count> threads;
  for (auto& thread : threads) assert(pthread_create(&thread, nullptr, ConcurrentThread, nullptr) == 0);
  for (auto thread : threads) assert(pthread_join(thread, nullptr) == 0);
  assert(callbacks == count * 300);
  // Fake identities are disjoint from native host pthread ids: all capacity
  // must remain available, proving exited native owners left no occupied slot.
  Capacity();
  puts("PASS 24 native threads, 7200 bind/capture/clear cycles and real native key cleanup");
}

struct CancelContext { sem_t wait; std::atomic<bool> waiting{false}; std::atomic<bool> cleaned{false}; };

void CancelWait(void* pointer) {
  auto& context = *static_cast<CancelContext*>(pointer);
  context.waiting.store(true, std::memory_order_release);
  while (sem_wait(&context.wait) != 0 && errno == EINTR) {}
}

void CancelCleanup(void* pointer) {
  prospero_signal_thread_clear();
  static_cast<CancelContext*>(pointer)->cleaned.store(true, std::memory_order_release);
}

void CancelThread(void* pointer) {
  assert(prospero_signal_bind_suspend(pointer, CancelWait) == 0);
  // Invoke through an actual host signal frame, without claiming the target
  // libc's unwind ABI. Dispatch must not add a noexcept termination boundary.
  raise(SIGUSR1);
}

void Cancellation() {
  CancelContext context;
  assert(sem_init(&context.wait, 0, 0) == 0);
  pthread_t thread;
  signal_test_cleanup_start start{CancelThread, CancelCleanup, &context};
  assert(pthread_create(&thread, nullptr, signal_test_with_native_cleanup, &start) == 0);
  while (!context.waiting.load(std::memory_order_acquire)) sched_yield();
  assert(pthread_cancel(thread) == 0);
  void* value;
  assert(pthread_join(thread, &value) == 0 && value == PTHREAD_CANCELED);
  assert(context.cleaned.load(std::memory_order_acquire));
  assert(sem_destroy(&context.wait) == 0);
  // Forced unwind bypasses the return half of OnSignal; reset its test-only flag.
  inside_signal = 0;
  Capacity();
  puts("PASS native host cancellation through dispatch/wait/signal reaches cleanup; PS5 unwind remains a target check");
}
}  // namespace

extern "C" pthread_t __real_pthread_self() noexcept;
extern "C" int __real_pthread_key_create(pthread_key_t*, void (*)(void*)) noexcept;
extern "C" int __real_pthread_setspecific(pthread_key_t, const void*) noexcept;
extern "C" void* __real_pthread_getspecific(pthread_key_t) noexcept;
extern "C" int __real_pthread_once(pthread_once_t*, void (*)(void));

extern "C" pthread_t __wrap_pthread_self() noexcept {
  return simulated_owner ? static_cast<pthread_t>(simulated_owner) : __real_pthread_self();
}
extern "C" int __wrap_pthread_key_create(pthread_key_t* key, void (*cleanup)(void*)) noexcept {
  CountUnsafe();
  if (fail_key_create) return EAGAIN;
  const int result = __real_pthread_key_create(key, cleanup);
  if (!result) { registry_key = *key; registry_cleanup = cleanup; }
  return result;
}
extern "C" int __wrap_pthread_setspecific(pthread_key_t key, const void* pointer) noexcept {
  CountUnsafe();
  if (key == registry_key) {
    last_slot.store(const_cast<void*>(pointer));
    if (fail_key_set) return ENOMEM;
    if (interrupt_publication) raise(SIGUSR1);
  }
  const int result = __real_pthread_setspecific(key, pointer);
  if (key == registry_key && interrupt_publication) raise(SIGUSR1);
  return result;
}
extern "C" void* __wrap_pthread_getspecific(pthread_key_t key) noexcept {
  CountUnsafe();
  return __real_pthread_getspecific(key);
}
extern "C" int __wrap_pthread_once(pthread_once_t* once, void (*initialize)(void)) {
  CountUnsafe();
  return __real_pthread_once(once, initialize);
}

int main(int argc, char** argv) {
  assert(argc == 2);
  InstallSignal();
  if (!std::strcmp(argv[1], "basics")) Basics();
  else if (!std::strcmp(argv[1], "publication")) Publication();
  else if (!std::strcmp(argv[1], "key-create-error")) KeyErrors(true);
  else if (!std::strcmp(argv[1], "key-set-error")) KeyErrors(false);
  else if (!std::strcmp(argv[1], "capacity")) Capacity();
  else if (!std::strcmp(argv[1], "concurrent")) Concurrent();
  else if (!std::strcmp(argv[1], "cancel")) Cancellation();
  else assert(false);
}
