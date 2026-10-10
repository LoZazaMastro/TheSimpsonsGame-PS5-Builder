// Bounded terminal diagnostics, independent of the allocator being diagnosed.
// BSD 3-Clause License. See the project LICENSE.
#include <rex/platform/trace_prospero.h>

#include <atomic>
#include <cerrno>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

namespace {

constexpr size_t kCapacity = 16384;
constexpr size_t kStackSlots = 512;
constexpr size_t kRawStackLimit = 64 * 1024;
constexpr uint64_t kBusy = uint64_t{1} << 63;
static_assert(std::atomic<uint64_t>::is_always_lock_free);
static_assert(std::atomic<uintptr_t>::is_always_lock_free);
static_assert(std::atomic<unsigned>::is_always_lock_free);
static_assert(std::atomic<int>::is_always_lock_free);
static_assert(sizeof(uintptr_t) == 8);

struct Slot {
  std::atomic<uint64_t> state{0};
  uintptr_t thread = 0;
  uintptr_t caller = 0;
  uintptr_t a = 0, b = 0, c = 0, d = 0;
  unsigned kind = 0;
};
struct Record {
  uint64_t sequence = 0;
  uintptr_t thread = 0, caller = 0;
  uintptr_t a = 0, b = 0, c = 0, d = 0;
  unsigned kind = 0;
};
struct StackSlot {
  std::atomic<uintptr_t> owner{0};
  std::atomic<uintptr_t> low{0}, high{0};
  std::atomic<unsigned> ready{0};
};

// Static zero-initialized storage: no constructors, malloc or TLS registration.
constinit Slot slots[kCapacity]{};
constinit Record snapshot[kCapacity]{};
constinit StackSlot stack_slots[kStackSlots]{};
constinit std::atomic<uint64_t> sequence{0};
constinit std::atomic<uint64_t> published{0}, overwritten{0};
constinit std::atomic<uint64_t> dropped_busy{0}, dropped_race{0}, dropped_stale{0};
constinit std::atomic<uint64_t> dropped_frozen{0}, dropped_exhausted{0};
constinit std::atomic<uint64_t> stack_registration_dropped{0};
constinit std::atomic<unsigned> frozen{0};
std::atomic_flag dump_started = ATOMIC_FLAG_INIT;
std::atomic_flag stack_dump_started = ATOMIC_FLAG_INIT;
constinit std::atomic<int> output_fd{-1};
constinit std::atomic<uintptr_t> runtime_anchor{0}, linked_anchor{0};
constinit std::atomic<unsigned> anchors_known{0};

uintptr_t CurrentThread() noexcept {
  // pthread_self has no failure/errno result and needs no application TLS.
  // pthread_t is a pointer in the PS5 SDK and an integer on the Linux test host.
  return (uintptr_t)pthread_self();
}

struct ClockSample { uint64_t nanoseconds; bool known; };
ClockSample MonotonicTime() noexcept {
  // Only checkpoints/dumps call this. Allocation events never make a clock
  // syscall or read a native timespec structure.
  struct timespec time{};
  if (clock_gettime(CLOCK_MONOTONIC, &time) != 0 || time.tv_sec < 0 ||
      time.tv_nsec < 0 || time.tv_nsec >= 1000000000) return {0, false};
  const uint64_t seconds = uint64_t(time.tv_sec), nanos = uint64_t(time.tv_nsec);
  if (seconds > (UINT64_MAX - nanos) / 1000000000) return {0, false};
  return {seconds * 1000000000 + nanos, true};
}

#if defined(PROSPERO_TRACE_TESTING)
extern "C" void prospero_trace_test_after_claim(uint64_t ticket);
#endif

uint64_t RecordEvent(unsigned kind, uintptr_t caller, uintptr_t a, uintptr_t b,
                     uintptr_t c, uintptr_t d) noexcept {
  if (frozen.load(std::memory_order_acquire)) {
    dropped_frozen.fetch_add(1, std::memory_order_relaxed);
    return 0;
  }
  const uint64_t ticket = sequence.fetch_add(1, std::memory_order_relaxed) + 1;
  if (ticket == 0 || ticket >= kBusy) {
    dropped_exhausted.fetch_add(1, std::memory_order_relaxed);
    return 0;
  }
  Slot& slot = slots[ticket % kCapacity];
  uint64_t previous = slot.state.load(std::memory_order_acquire);
  if (previous & kBusy) {
    dropped_busy.fetch_add(1, std::memory_order_relaxed);
    return 0;
  }
  // A producer delayed before claiming must not overwrite a newer ticket.
  if (previous >= ticket) {
    dropped_stale.fetch_add(1, std::memory_order_relaxed);
    return 0;
  }
  if (!slot.state.compare_exchange_strong(previous, kBusy | ticket,
                                          std::memory_order_acquire,
                                          std::memory_order_relaxed)) {
    dropped_race.fetch_add(1, std::memory_order_relaxed);
    return 0;
  }
  if (frozen.load(std::memory_order_acquire)) {
    slot.state.store(previous, std::memory_order_release);
    dropped_frozen.fetch_add(1, std::memory_order_relaxed);
    return 0;
  }
#if defined(PROSPERO_TRACE_TESTING)
  prospero_trace_test_after_claim(ticket);
#endif
  slot.thread = CurrentThread();
  slot.kind = kind;
  slot.caller = caller;
  slot.a = a;
  slot.b = b;
  slot.c = c;
  slot.d = d;
  slot.state.store(ticket, std::memory_order_release);
  published.fetch_add(1, std::memory_order_relaxed);
  if (previous) overwritten.fetch_add(1, std::memory_order_relaxed);
  return ticket;
}

struct Buffer {
  char* data;
  size_t capacity;
  size_t used = 0;
  void Character(char value) noexcept { if (used < capacity) data[used++] = value; }
  void Text(const char* value) noexcept { while (*value) Character(*value++); }
  void Unsigned(uint64_t value) noexcept {
    char digits[20];
    size_t count = 0;
    do { digits[count++] = char('0' + value % 10); value /= 10; } while (value);
    while (count) Character(digits[--count]);
  }
  void Hex(uint64_t value) noexcept {
    Text("0x");
    constexpr char digits[] = "0123456789abcdef";
    for (int shift = 60; shift >= 0; shift -= 4) Character(digits[(value >> shift) & 15]);
  }
};

bool WriteBounded(int fd, const char* data, size_t bytes) noexcept {
  size_t done = 0;
  for (unsigned attempts = 0; attempts < 4 && done < bytes; ++attempts) {
    const ssize_t count = write(fd, data + done, bytes - done);
    if (count > 0) done += size_t(count);
    else if (count == 0 || errno != EINTR) return false;
  }
  return done == bytes;
}

struct Output {
  int fd;
  char bytes[4096];
  size_t used = 0;
  bool good = true;
  explicit Output(int descriptor) noexcept : fd(descriptor) {}
  void Flush() noexcept {
    if (used && good) good = WriteBounded(fd, bytes, used);
    used = 0;
  }
  void Append(const char* text, size_t length) noexcept {
    for (size_t i = 0; i < length && good; ++i) {
      if (used == sizeof(bytes)) Flush();
      if (good) bytes[used++] = text[i];
    }
  }
};

const char* KindName(unsigned kind) noexcept {
  switch (kind) {
    case PROSPERO_TRACE_PHASE: return "phase";
    case PROSPERO_TRACE_ALLOC_BEGIN: return "alloc_begin";
    case PROSPERO_TRACE_ALLOC_END: return "alloc_end";
    case PROSPERO_TRACE_FREE_BEGIN: return "free_begin";
    case PROSPERO_TRACE_FREE_END: return "free_end";
    case PROSPERO_TRACE_REALLOC_BEGIN: return "realloc_begin";
    case PROSPERO_TRACE_REALLOC_END: return "realloc_end";
    case PROSPERO_TRACE_ALIGN_BEGIN: return "align_begin";
    case PROSPERO_TRACE_ALIGN_END: return "align_end";
    case PROSPERO_TRACE_HEAP_DETAIL: return "heap_detail";
    case PROSPERO_TRACE_THREAD: return "thread";
    case PROSPERO_TRACE_FIBER: return "fiber";
    case PROSPERO_TRACE_JUMP: return "jump";
    case PROSPERO_TRACE_HEAP_LINKS: return "heap_links";
    case PROSPERO_TRACE_HEAP_CHECK: return "heap_check";
    default: return "unknown";
  }
}

void SwapRecords(size_t a, size_t b) noexcept {
  const Record temporary = snapshot[a];
  snapshot[a] = snapshot[b];
  snapshot[b] = temporary;
}
void SiftDown(size_t parent, size_t count) noexcept {
  for (size_t child = parent * 2 + 1; child < count; child = parent * 2 + 1) {
    if (child + 1 < count && snapshot[child].sequence < snapshot[child + 1].sequence) ++child;
    if (snapshot[parent].sequence >= snapshot[child].sequence) return;
    SwapRecords(parent, child);
    parent = child;
  }
}
void SortRecords(size_t count) noexcept {
  // Iterative heapsort: bounded storage/work, no allocator or recursive stack.
  for (size_t start = count / 2; start; --start) SiftDown(start - 1, count);
  for (size_t end = count; end > 1; --end) {
    SwapRecords(0, end - 1);
    SiftDown(0, end - 1);
  }
}

size_t SnapshotRecords(size_t& incomplete, size_t& raced) noexcept {
  size_t count = 0;
  for (Slot& slot : slots) {
    uint64_t state = slot.state.load(std::memory_order_acquire);
    if (!state) continue;
    if (state & kBusy) { ++incomplete; continue; }
    // Claim before copying plain fields. A two-load seqlock alone would have
    // a C++ data race if an in-flight producer overwrote the record.
    if (!slot.state.compare_exchange_strong(state, kBusy | state,
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed)) {
      ++raced;
      continue;
    }
    snapshot[count++] = {state, slot.thread, slot.caller, slot.a, slot.b, slot.c, slot.d, slot.kind};
    slot.state.store(state, std::memory_order_release);
  }
  SortRecords(count);
  return count;
}

bool CurrentStack(uintptr_t thread, uintptr_t& low, uintptr_t& high) noexcept {
  for (StackSlot& slot : stack_slots) {
    if (slot.owner.load(std::memory_order_acquire) != thread ||
        !slot.ready.load(std::memory_order_acquire)) continue;
    const uintptr_t begin = slot.low.load(std::memory_order_relaxed);
    const uintptr_t end = slot.high.load(std::memory_order_relaxed);
    if (slot.owner.load(std::memory_order_acquire) == thread &&
        slot.ready.load(std::memory_order_acquire) && begin < end) {
      low = begin;
      high = end;
      return true;
    }
  }
  return false;
}

void PublishStack(StackSlot& slot, uintptr_t low, uintptr_t high) noexcept {
  // Only the owning current thread changes its range. A signal interrupting
  // this update sees ready=0 and omits raw memory reads.
  slot.ready.store(0, std::memory_order_release);
  slot.low.store(low, std::memory_order_relaxed);
  slot.high.store(high, std::memory_order_relaxed);
  slot.ready.store(1, std::memory_order_release);
}

}  // namespace

extern "C" void prospero_trace_event(unsigned kind, uintptr_t caller, uintptr_t a,
                                     uintptr_t b, uintptr_t c, uintptr_t d) noexcept {
  // Atomics and pthread_self do not change errno. In particular this hot path
  // does not ask the C/C++ TLS runtime for an errno or thread_local object.
  RecordEvent(kind, caller, a, b, c, d);
}

extern "C" void prospero_trace_set_image_anchors(uintptr_t runtime,
                                                  uintptr_t linked) noexcept {
  anchors_known.store(0, std::memory_order_release);
  runtime_anchor.store(runtime, std::memory_order_relaxed);
  linked_anchor.store(linked, std::memory_order_relaxed);
  anchors_known.store(runtime != 0 && runtime >= linked, std::memory_order_release);
}

extern "C" void prospero_trace_set_output_fd(int fd) noexcept {
  output_fd.store(fd, std::memory_order_release);
}

extern "C" void prospero_trace_write_line(const char* data, size_t bytes) noexcept {
  const int saved_errno = errno;
  const int fd = output_fd.load(std::memory_order_acquire);
  if (data && bytes <= 1536 && fd >= 0 && !frozen.load(std::memory_order_acquire)) {
    WriteBounded(fd, data, bytes);
  }
  errno = saved_errno;
}

extern "C" void prospero_trace_register_stack(uintptr_t low, size_t bytes) noexcept {
  const uintptr_t thread = CurrentThread();
  if (!thread || !low || !bytes || bytes > UINTPTR_MAX - low) {
    // A failed replacement must not leave a formerly valid range available
    // for a later raw read after its owner has changed or unmapped that stack.
    prospero_trace_unregister_stack();
    stack_registration_dropped.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  for (StackSlot& slot : stack_slots) {
    if (slot.owner.load(std::memory_order_acquire) == thread) {
      PublishStack(slot, low, low + bytes);
      return;
    }
  }
  for (StackSlot& slot : stack_slots) {
    uintptr_t empty = 0;
    if (slot.owner.compare_exchange_strong(empty, thread, std::memory_order_acq_rel,
                                           std::memory_order_relaxed)) {
      PublishStack(slot, low, low + bytes);
      return;
    }
  }
  stack_registration_dropped.fetch_add(1, std::memory_order_relaxed);
}

extern "C" void prospero_trace_unregister_stack(void) noexcept {
  const uintptr_t thread = CurrentThread();
  if (!thread) return;
  for (StackSlot& slot : stack_slots) {
    if (slot.owner.load(std::memory_order_acquire) == thread) {
      slot.ready.store(0, std::memory_order_release);
      slot.owner.store(0, std::memory_order_release);
    }
  }
}

extern "C" void prospero_trace_checkpoint(uintptr_t phase, uintptr_t b, uintptr_t c,
                                          uintptr_t d) noexcept {
  const int saved_errno = errno;
  const uintptr_t caller = reinterpret_cast<uintptr_t>(
      __builtin_extract_return_addr(__builtin_return_address(0)));
  const uint64_t ticket = RecordEvent(PROSPERO_TRACE_PHASE, caller, phase, b, c, d);
  const int fd = output_fd.load(std::memory_order_acquire);
  if (fd >= 0 && !frozen.load(std::memory_order_acquire)) {
    const ClockSample time = MonotonicTime();
    char text[448];
    Buffer out{text, sizeof(text)};
    out.Text("[PPSA74207] trace-checkpoint seq="); out.Unsigned(ticket);
    out.Text(" pid="); out.Unsigned(uint64_t(getpid()));
    out.Text(" monotonic_ns="); out.Unsigned(time.nanoseconds);
    out.Text(" clock_known="); out.Unsigned(time.known);
    out.Text(" thread="); out.Hex(CurrentThread());
    out.Text(" phase="); out.Unsigned(phase);
    out.Text(" caller="); out.Hex(caller);
    out.Text(" b="); out.Hex(b); out.Text(" c="); out.Hex(c); out.Text(" d="); out.Hex(d);
    out.Character('\n');
    WriteBounded(fd, text, out.used);
  }
  errno = saved_errno;
}

extern "C" int prospero_trace_dump_fd(int fd) noexcept {
  if (fd < 0) return 0;
  const int saved_errno = errno;
  if (dump_started.test_and_set(std::memory_order_relaxed)) { errno = saved_errno; return 0; }
  frozen.store(1, std::memory_order_release);
  const ClockSample time = MonotonicTime();
  size_t incomplete = 0, raced = 0;
  const size_t count = SnapshotRecords(incomplete, raced);
  Output output{fd};
  char text[1024];
  Buffer out{text, sizeof(text)};
  out.Text("[PPSA74207] TRACE BEGIN format=1 capacity="); out.Unsigned(kCapacity);
  out.Text(" pid="); out.Unsigned(uint64_t(getpid()));
  out.Text(" monotonic_ns="); out.Unsigned(time.nanoseconds);
  out.Text(" clock_known="); out.Unsigned(time.known);
  out.Text(" retained="); out.Unsigned(count);
  out.Text(" reserved="); out.Unsigned(sequence.load(std::memory_order_relaxed));
  out.Text(" published="); out.Unsigned(published.load(std::memory_order_relaxed));
  out.Text(" overwritten="); out.Unsigned(overwritten.load(std::memory_order_relaxed));
  out.Text(" incomplete="); out.Unsigned(incomplete);
  out.Text(" snapshot_raced="); out.Unsigned(raced);
  out.Text("\nTRACE dropped_busy="); out.Unsigned(dropped_busy.load(std::memory_order_relaxed));
  out.Text(" dropped_race="); out.Unsigned(dropped_race.load(std::memory_order_relaxed));
  out.Text(" dropped_stale="); out.Unsigned(dropped_stale.load(std::memory_order_relaxed));
  out.Text(" dropped_frozen="); out.Unsigned(dropped_frozen.load(std::memory_order_relaxed));
  out.Text(" dropped_exhausted="); out.Unsigned(dropped_exhausted.load(std::memory_order_relaxed));
  out.Text(" stack_registration_dropped="); out.Unsigned(stack_registration_dropped.load(std::memory_order_relaxed));
  const bool known = anchors_known.load(std::memory_order_acquire);
  const uintptr_t runtime = runtime_anchor.load(std::memory_order_relaxed);
  const uintptr_t linked = linked_anchor.load(std::memory_order_relaxed);
  out.Text("\nTRACE image_anchor_known="); out.Unsigned(known && runtime >= linked);
  out.Text(" runtime_anchor="); out.Hex(runtime);
  out.Text(" linked_anchor="); out.Hex(linked);
  out.Text(" load_bias="); out.Hex(known && runtime >= linked ? runtime - linked : 0);
  out.Text("\nTRACE ordering=reservation_sequence thread=opaque_pthread_handle addresses=absolute\n");
  output.Append(text, out.used);
  for (size_t i = 0; i < count && output.good; ++i) {
    const Record& record = snapshot[i];
    Buffer line{text, sizeof(text)};
    line.Text("T seq="); line.Unsigned(record.sequence);
    line.Text(" thread="); line.Hex(record.thread);
    line.Text(" kind="); line.Text(KindName(record.kind));
    line.Character('('); line.Unsigned(record.kind); line.Character(')');
    line.Text(" caller="); line.Hex(record.caller);
    line.Text(" a="); line.Hex(record.a); line.Text(" b="); line.Hex(record.b);
    line.Text(" c="); line.Hex(record.c); line.Text(" d="); line.Hex(record.d);
    line.Character('\n');
    output.Append(text, line.used);
  }
  Buffer footer{text, sizeof(text)};
  footer.Text("[PPSA74207] TRACE END retained="); footer.Unsigned(count);
  footer.Text(" incomplete="); footer.Unsigned(incomplete);
  footer.Text(" snapshot_raced="); footer.Unsigned(raced);
  footer.Text("\n");
  output.Append(text, footer.used);
  output.Flush();
  errno = saved_errno;
  return 1;
}

extern "C" int prospero_trace_dump_stack_fd(int fd, uintptr_t rsp) noexcept {
  if (fd < 0) return 0;
  const int saved_errno = errno;
  if (stack_dump_started.test_and_set(std::memory_order_relaxed)) { errno = saved_errno; return 0; }
  const uintptr_t thread = CurrentThread();
  uintptr_t low = 0, high = 0;
  const bool registered = CurrentStack(thread, low, high);
  const bool inside = registered && rsp >= low && rsp < high;
  Output output{fd};
  char text[512];
  Buffer header{text, sizeof(text)};
  header.Text("[PPSA74207] RAW STACK BEGIN thread="); header.Hex(thread);
  header.Text(" pid="); header.Unsigned(uint64_t(getpid()));
  header.Text(" registered="); header.Unsigned(registered);
  header.Text(" rsp="); header.Hex(rsp);
  header.Text(" low="); header.Hex(low); header.Text(" high="); header.Hex(high);
  header.Text(" limit_bytes="); header.Unsigned(kRawStackLimit);
  header.Text("\nRAW STACK is memory words inside known current-thread bounds, not a backtrace.\n");
  output.Append(text, header.used);
  size_t dumped = 0;
  if (inside && rsp <= UINTPTR_MAX - 7) {
    const uintptr_t start = (rsp + 7) & ~uintptr_t{7};
    const size_t available = high > start ? size_t(high - start) : 0;
    const size_t bytes = (available < kRawStackLimit ? available : kRawStackLimit) & ~size_t{7};
    for (size_t offset = 0; offset < bytes && output.good; offset += 32) {
      Buffer line{text, sizeof(text)};
      line.Text("S address="); line.Hex(start + offset);
      for (size_t word = 0; word < 32 && offset + word < bytes; word += 8) {
        const auto* source = reinterpret_cast<const volatile unsigned char*>(start + offset + word);
        uint64_t value = 0;
        for (unsigned byte = 0; byte < 8; ++byte) value |= uint64_t(source[byte]) << (byte * 8);
        line.Character(' '); line.Hex(value);
        dumped += 8;
      }
      line.Character('\n');
      output.Append(text, line.used);
    }
  } else {
    constexpr char unavailable[] = "RAW STACK unavailable: RSP is outside a registered live stack; no memory read.\n";
    output.Append(unavailable, sizeof(unavailable) - 1);
  }
  Buffer footer{text, sizeof(text)};
  footer.Text("[PPSA74207] RAW STACK END bytes="); footer.Unsigned(dumped); footer.Character('\n');
  output.Append(text, footer.used);
  output.Flush();
  errno = saved_errno;
  return 1;
}
