// BSD 3-Clause License. See the project LICENSE.
#include <rex/platform/fault_prospero.h>
#include <rex/platform/trace_phases_prospero.h>

#include <atomic>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace rex::platform {
namespace {

constinit std::atomic<int> fault_fd{-1};
static_assert(std::atomic<int>::is_always_lock_free);
// atomic_flag is guaranteed lock-free. Do not spin or wait in a signal.
std::atomic_flag terminal_fault_started = ATOMIC_FLAG_INIT;
std::atomic_flag abort_caller_started = ATOMIC_FLAG_INIT;
std::atomic_flag heap_failure_started = ATOMIC_FLAG_INIT;

struct Buffer {
  char* data;
  size_t capacity;
  size_t used = 0;

  void Character(char value) noexcept {
    if (used < capacity) data[used++] = value;
  }
  void Text(const char* text) noexcept {
    while (*text) Character(*text++);
  }
  void Hex(uint64_t value) noexcept {
    Text("0x");
    constexpr char digits[] = "0123456789abcdef";
    for (int shift = 60; shift >= 0; shift -= 4) {
      Character(digits[(value >> shift) & 15]);
    }
  }
  void Decimal(int32_t value) noexcept {
    uint64_t magnitude = value < 0 ? uint64_t(-int64_t(value)) : uint64_t(value);
    if (value < 0) Character('-');
    char digits[10];
    size_t count = 0;
    do {
      digits[count++] = char('0' + magnitude % 10);
      magnitude /= 10;
    } while (magnitude);
    while (count) Character(digits[--count]);
  }
};

bool WriteBounded(int fd, const char* data, size_t size) noexcept {
  size_t done = 0;
  // Account for EINTR/partial writes without an unbounded signal-handler loop.
  for (unsigned attempt = 0; attempt < 4 && done < size; ++attempt) {
    const ssize_t written = write(fd, data + done, size - done);
    if (written > 0) {
      done += size_t(written);
    } else if (written == 0 || errno != EINTR) {
      return false;
    }
  }
  return done == size;
}

}  // namespace

ProsperoFaultAccess ClassifyProsperoMemoryFault(int signal_number, uint64_t trap_number,
                                             uint64_t error, uint64_t page_fault_trap) noexcept {
  if (signal_number != SIGSEGV && signal_number != SIGBUS) return ProsperoFaultAccess::kUnhandled;
  if (signal_number == SIGBUS && trap_number != page_fault_trap) {
    return ProsperoFaultAccess::kUnhandled;
  }
  // SDK machine/pmap.h: PGEX_W=0x02, PGEX_I=0x10. An instruction fetch
  // cannot be recovered by making a guest data page readable/writable.
  if (error & 0x10) return ProsperoFaultAccess::kUnhandled;
  return error & 0x02 ? ProsperoFaultAccess::kWrite : ProsperoFaultAccess::kRead;
}

bool OpenProsperoFaultLog(const char* path) noexcept {
  if (fault_fd.load(std::memory_order_acquire) >= 0) return true;
  if (!path || path[0] != '/') {
    errno = EINVAL;
    return false;
  }
  // O_NONBLOCK avoids waiting on a FIFO masquerading as the log. A regular
  // file is still allowed to block inside the kernel while performing I/O;
  // the bound here is on calls and bytes, not on storage latency.
  const int descriptor = open(path, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC |
                                       O_NOFOLLOW | O_NONBLOCK, 0666);
  if (descriptor < 0) return false;
  struct stat status{};
  const int queried = fstat(descriptor, &status);
  if (queried != 0 || !S_ISREG(status.st_mode)) {
    const int failure = queried != 0 ? errno : EINVAL;
    close(descriptor);
    errno = failure;
    return false;
  }
  int unopened = -1;
  if (!fault_fd.compare_exchange_strong(unopened, descriptor, std::memory_order_release,
                                        std::memory_order_relaxed)) {
    close(descriptor);
    return true;
  }
  prospero_trace_set_output_fd(descriptor);
  return true;
}

bool ProsperoFaultLogAvailable() noexcept {
  return fault_fd.load(std::memory_order_acquire) >= 0;
}

size_t FormatProsperoFaultRecord(const ProsperoFaultRecord& record,
                               char* destination, size_t capacity) noexcept {
  if (!destination) return 0;
  Buffer out{destination, capacity};
  out.Text("[PPSA74207] terminal fault: signal="); out.Decimal(record.signal_number);
  out.Text(" si_code="); out.Decimal(record.signal_code);
  out.Text(" context_present="); out.Character(record.context_present ? '1' : '0');
  out.Text(" fault="); out.Hex(record.fault_address);
  out.Text("\nRIP="); out.Hex(record.rip);
  out.Text(" RSP="); out.Hex(record.rsp);
  out.Text(" RBP="); out.Hex(record.rbp);
  out.Text(" RFLAGS="); out.Hex(record.flags);
  out.Text(" trap="); out.Hex(record.trap_number);
  out.Text(" error="); out.Hex(record.page_fault_error);
  constexpr const char* names[] = {"RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI",
                                  "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15"};
  for (size_t index = 0; index < 16; ++index) {
    out.Character(index % 4 == 0 ? '\n' : ' ');
    out.Text(names[index]); out.Character('='); out.Hex(record.registers[index]);
  }
  out.Text("\nimage_base_known="); out.Character(record.image_base_known ? '1' : '0');
  out.Text(" image_base="); out.Hex(record.image_base);
  out.Text(" reporter="); out.Hex(record.reporter_address);
  out.Text("\nAddresses are absolute; no backtrace or guessed guest PC.\n");
  return out.used;
}

void WriteProsperoFaultRecord(const ProsperoFaultRecord& record) noexcept {
  const int saved_errno = errno;
  if (terminal_fault_started.test_and_set(std::memory_order_relaxed)) {
    errno = saved_errno;
    return;
  }
  char text[1024];
  const size_t length = FormatProsperoFaultRecord(record, text, sizeof(text));
  const int descriptor = fault_fd.load(std::memory_order_acquire);
  const int destination = descriptor >= 0 ? descriptor : STDERR_FILENO;
  if (descriptor < 0 || !WriteBounded(descriptor, text, length)) {
    // fd 2 is merely a backup: it may be closed or not retained by the loader.
    // Never pretend that it is a persistent kernel/file report.
    WriteBounded(STDERR_FILENO, text, length);
  }
  prospero_trace_event(PROSPERO_TRACE_PHASE, record.reporter_address,
      PROSPERO_PHASE_TERMINAL_SIGNAL,
      (uint64_t(uint32_t(record.signal_number)) << 32) | uint32_t(record.signal_code),
      record.fault_address, record.rip);
  prospero_trace_dump_fd(destination);
  prospero_trace_dump_stack_fd(destination, record.rsp);
  errno = saved_errno;
}

void WriteProsperoAbortCaller(uintptr_t return_address) noexcept {
  const int saved_errno = errno;
  if (abort_caller_started.test_and_set(std::memory_order_relaxed)) {
    errno = saved_errno;
    return;
  }
  char text[160];
  Buffer out{text, sizeof(text)};
  out.Text("[PPSA74207] abort caller=");
  out.Hex(return_address);
  out.Text(" (return address in linked code; separate from signal RIP)\n");
  const int descriptor = fault_fd.load(std::memory_order_acquire);
  const int destination = descriptor >= 0 ? descriptor : STDERR_FILENO;
  if (descriptor < 0 || !WriteBounded(descriptor, text, out.used)) {
    WriteBounded(STDERR_FILENO, text, out.used);
  }
  uintptr_t rsp;
  __asm__ __volatile__("movq %%rsp, %0" : "=r"(rsp));
  prospero_trace_event(PROSPERO_TRACE_PHASE, return_address,
      PROSPERO_PHASE_EXPLICIT_ABORT, return_address, rsp, 0);
  prospero_trace_dump_fd(destination);
  prospero_trace_dump_stack_fd(destination, rsp);
  errno = saved_errno;
}

void WriteProsperoHeapFailure(const char* kind, const char* operation, unsigned line,
                             uintptr_t arena, uintptr_t pointer) noexcept {
  const int saved_errno = errno;
  if (heap_failure_started.test_and_set(std::memory_order_relaxed)) {
    errno = saved_errno;
    return;
  }
  char text[384];
  Buffer out{text, sizeof(text)};
  out.Text("[PPSA74207] heap failure: kind=");
  out.Text(kind ? kind : "unknown");
  out.Text(" operation=");
  out.Text(operation ? operation : "unknown");
  out.Text(" line=");
  // Allocator source line numbers are below INT32_MAX; preserve unexpected
  // values as hexadecimal rather than narrowing an unsigned value.
  if (line <= 0x7fffffffU) out.Decimal(static_cast<int32_t>(line));
  else out.Hex(line);
  out.Text(" arena="); out.Hex(arena);
  out.Text(" pointer="); out.Hex(pointer);
  out.Text("\n");
  const int descriptor = fault_fd.load(std::memory_order_acquire);
  if (descriptor < 0 || !WriteBounded(descriptor, text, out.used)) {
    WriteBounded(STDERR_FILENO, text, out.used);
  }
  const auto caller = reinterpret_cast<uintptr_t>(
      __builtin_extract_return_addr(__builtin_return_address(0)));
  prospero_trace_event(PROSPERO_TRACE_HEAP_DETAIL, caller, arena, pointer, line, 0);
  errno = saved_errno;
}

}  // namespace rex::platform
