// Real production recorder, with injectable interruption and I/O failures.
#include <rex/platform/trace_prospero.h>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>

static std::atomic<bool> forbid_allocations{false};
static std::atomic<bool> writer_held{false}, release_writer{false};
static int hook_mode;
static jmp_buf abandoned_writer;
static unsigned write_mode, write_calls, clock_calls;
static bool fail_clock;

extern "C" void* __real_malloc(size_t);
extern "C" void* __real_calloc(size_t, size_t);
extern "C" void* __real_realloc(void*, size_t);
extern "C" ssize_t __real_write(int, const void*, size_t);
extern "C" int __real_clock_gettime(clockid_t, timespec*);
extern "C" void* __wrap_malloc(size_t n) {
  if (forbid_allocations.load()) _exit(90);
  return __real_malloc(n);
}
extern "C" void* __wrap_calloc(size_t n, size_t m) {
  if (forbid_allocations.load()) _exit(90);
  return __real_calloc(n, m);
}
extern "C" void* __wrap_realloc(void* p, size_t n) {
  if (forbid_allocations.load()) _exit(90);
  return __real_realloc(p, n);
}
extern "C" ssize_t __wrap_write(int fd, const void* p, size_t n) {
  ++write_calls;
  if (write_mode == 2 || (write_mode == 1 && write_calls <= 2)) {
    errno = EINTR;
    return -1;
  }
  if (write_mode == 1 && write_calls == 3 && n > 17) n = 17;
  return __real_write(fd, p, n);
}
extern "C" int __wrap_clock_gettime(clockid_t id, timespec* time) {
  ++clock_calls;
  if (fail_clock) { errno = ENOTSUP; return -1; }
  return __real_clock_gettime(id, time);
}
extern "C" void prospero_trace_test_after_claim(uint64_t ticket) {
  if (ticket != 1) return;
  if (hook_mode == 1) longjmp(abandoned_writer, 1);
  if (hook_mode == 2) {
    writer_held.store(true);
    while (!release_writer.load()) std::this_thread::yield();
  }
}

static void Check(bool condition) { if (!condition) _exit(89); }
static void Event(uint64_t id) {
  prospero_trace_event(PROSPERO_TRACE_ALLOC_BEGIN, 0xfedcba9876543210ull,
      id, ~id, id ^ 0x55aa55aa55aa55aaull, id + 1);
}
static void Dump(int fd) {
  errno = EDOM;
  Check(prospero_trace_dump_fd(fd) == 1);
  Check(errno == EDOM);
  Check(prospero_trace_dump_fd(fd) == 0);
}

int main(int argc, char** argv) {
  Check(argc == 3);
  const char* mode = argv[1];
  const int fd = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT, 0600);
  Check(fd >= 0);
  prospero_trace_set_output_fd(fd);
  prospero_trace_set_image_anchors(0x401234, 0x1234);
  if (!std::strcmp(mode, "concurrent")) {
    constexpr unsigned count = 8;
    std::thread workers[count];
    std::atomic<unsigned> ready{0}, done{0};
    std::atomic<bool> start{false}, finish{false};
    for (unsigned t = 0; t < count; ++t) {
      workers[t] = std::thread([&, t] {
        ready.fetch_add(1);
        while (!start.load()) std::this_thread::yield();
        for (unsigned i = 0; i < 12000; ++i) Event((uint64_t(t) << 32) | i);
        done.fetch_add(1);
        while (!finish.load()) std::this_thread::yield();
      });
    }
    while (ready.load() != count) std::this_thread::yield();
    forbid_allocations.store(true);
    start.store(true);
    while (done.load() != count) std::this_thread::yield();
    Check(clock_calls == 0);
    Dump(fd);
    forbid_allocations.store(false);
    finish.store(true);
    for (auto& worker : workers) worker.join();
  } else if (!std::strcmp(mode, "abandoned")) {
    forbid_allocations.store(true);
    hook_mode = 1;
    if (setjmp(abandoned_writer) == 0) Event(0);
    hook_mode = 0;
    for (unsigned i = 0; i < 32768; ++i) Event(i + 1);
    Dump(fd);
    forbid_allocations.store(false);
  } else if (!std::strcmp(mode, "interrupted")) {
    hook_mode = 2;
    std::thread held([] { Event(0); });
    while (!writer_held.load()) std::this_thread::yield();
    forbid_allocations.store(true);
    for (unsigned i = 0; i < 20000; ++i) Event(i + 1);
    Dump(fd); // Must finish while the writer is still suspended.
    forbid_allocations.store(false);
    release_writer.store(true);
    held.join();
  } else if (!std::strncmp(mode, "raw", 3)) {
    const size_t page = size_t(sysconf(_SC_PAGESIZE));
    constexpr size_t usable = 128 * 1024;
    auto* area = static_cast<unsigned char*>(mmap(nullptr, usable + 2 * page, PROT_NONE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    Check(area != MAP_FAILED);
    auto* data = area + page;
    Check(mprotect(data, usable, PROT_READ | PROT_WRITE) == 0);
    for (size_t i = 0; i < usable; ++i) data[i] = static_cast<unsigned char>(i);
    const uintptr_t low = reinterpret_cast<uintptr_t>(data);
    uintptr_t rsp = low + 32;
    if (std::strcmp(mode, "raw-unknown")) prospero_trace_register_stack(low, usable);
    if (!std::strcmp(mode, "raw-end")) rsp = low + usable - 15;
    if (!std::strcmp(mode, "raw-outside")) rsp = UINTPTR_MAX;
    if (!std::strcmp(mode, "raw-unregistered")) {
      prospero_trace_unregister_stack();
      Check(munmap(area, usable + 2 * page) == 0);
      area = nullptr;
    }
    forbid_allocations.store(true);
    errno = EDOM;
    Check(prospero_trace_dump_stack_fd(fd, rsp) == 1 && errno == EDOM);
    Check(prospero_trace_dump_stack_fd(fd, rsp) == 0);
    forbid_allocations.store(false);
    prospero_trace_unregister_stack();
    if (area) Check(munmap(area, usable + 2 * page) == 0);
  } else {
    forbid_allocations.store(true);
    if (!std::strcmp(mode, "eintr")) write_mode = 1;
    if (!std::strcmp(mode, "iofail")) write_mode = 2;
    if (!std::strcmp(mode, "clockfail")) fail_clock = true;
    Check(prospero_trace_dump_fd(-1) == 0);
    errno = EDOM;
    for (unsigned i = 0; i < 1000; ++i) Event(i);
    Check(errno == EDOM && clock_calls == 0);
    if (!std::strcmp(mode, "checkpoint") || !std::strcmp(mode, "clockfail")) {
      prospero_trace_checkpoint(110, 1, 2, 3);
      prospero_trace_write_line("METRICS test=1\n", 15);
      prospero_trace_write_line(nullptr, 10);
      prospero_trace_write_line(reinterpret_cast<const char*>(1), 1537);
      Check(errno == EDOM);
    }
    Dump(fd);
    if (write_mode == 2) Check(write_calls == 4);
    if (!std::strcmp(mode, "checkpoint") || !std::strcmp(mode, "clockfail")) {
      const unsigned calls = write_calls;
      prospero_trace_checkpoint(999, 0, 0, 0);
      prospero_trace_write_line("MUST_NOT_WRITE\n", 15);
      Check(write_calls == calls && errno == EDOM);
    }
    forbid_allocations.store(false);
  }
  Check(close(fd) == 0);
  std::printf("PASS trace %s\n", mode);
}
