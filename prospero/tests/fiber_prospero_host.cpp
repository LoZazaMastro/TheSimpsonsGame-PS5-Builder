// Actual PS5 Fiber implementation, with the matched SDK's virtual-range
// implementation and its Linux kernel model. These checks are not PS5 runs.
#include <rex/thread/fiber.h>
#include <ps5platform/kernel.h>
#include <ps5platform/shm.h>
extern "C" {
#include "host_kernel.h"
}

#include <array>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

using rex::thread::Fiber;
constexpr size_t kUnit = 65536;
constexpr size_t kMinimumStack = 16 * 1024 * 1024;
static thread_local void* last_reservation;
static thread_local size_t last_reservation_bytes;
static std::atomic<unsigned> reserve_calls;

extern "C" int __real_ps5_vrange_reserve(size_t, void*, size_t, void**);
extern "C" int __wrap_ps5_vrange_reserve(size_t bytes, void* hint, size_t alignment, void** out) {
  reserve_calls.fetch_add(1, std::memory_order_relaxed);
  const int result = __real_ps5_vrange_reserve(bytes, hint, alignment, out);
  if (!result) {
    last_reservation = *out;
    last_reservation_bytes = bytes;
  }
  return result;
}

#define CHECK(expression) do { if (!(expression)) { \
  std::fprintf(stderr, "FAIL line %u: %s\n", unsigned(__LINE__), #expression); \
  std::abort(); } } while (0)

static uint32_t Mxcsr() { return _mm_getcsr(); }
static uint16_t X87() {
  uint16_t out;
  __asm__ __volatile__("fnstcw %0" : "=m"(out));
  return out;
}
static void SetFp(uint32_t mxcsr, uint16_t x87) {
  _mm_setcsr(mxcsr);
  __asm__ __volatile__("fldcw %0" : : "m"(x87));
}
static void CheckNoResources() {
  ps5_shm_stats stats{};
  ps5_shm_live(&stats);
  CHECK(stats.ranges == 0 && stats.range_bytes == 0);
  CHECK(host_direct_allocations() == 0);
}

// The helper itself controls all six nonvolatile integer registers, including
// RBP. A suspended call really has to preserve them, not just C++ locals the
// optimizer may put on the stack or recompute.
extern "C" unsigned fiber_test_registers(void (*call)(void*), void* argument);
__asm__(
    ".text\n.globl fiber_test_registers\n.type fiber_test_registers,@function\n"
    "fiber_test_registers:\n"
    "pushq %rbp\npushq %rbx\npushq %r12\npushq %r13\npushq %r14\npushq %r15\n"
    "subq $8,%rsp\nmovq %rdi,%rax\nmovq %rsi,%rdi\n"
    "movabsq $0x1122334455667788,%rbx\nmovabsq $0x2233445566778899,%rbp\n"
    "movabsq $0x33445566778899aa,%r12\nmovabsq $0x445566778899aabb,%r13\n"
    "movabsq $0x5566778899aabbcc,%r14\nmovabsq $0x66778899aabbccdd,%r15\n"
    "call *%rax\nxorl %eax,%eax\n"
    "movabsq $0x1122334455667788,%r10\ncmpq %r10,%rbx\nje 1f\norl $1,%eax\n1:\n"
    "movabsq $0x2233445566778899,%r10\ncmpq %r10,%rbp\nje 2f\norl $2,%eax\n2:\n"
    "movabsq $0x33445566778899aa,%r10\ncmpq %r10,%r12\nje 3f\norl $4,%eax\n3:\n"
    "movabsq $0x445566778899aabb,%r10\ncmpq %r10,%r13\nje 4f\norl $8,%eax\n4:\n"
    "movabsq $0x5566778899aabbcc,%r10\ncmpq %r10,%r14\nje 5f\norl $16,%eax\n5:\n"
    "movabsq $0x66778899aabbccdd,%r10\ncmpq %r10,%r15\nje 6f\norl $32,%eax\n6:\n"
    "addq $8,%rsp\npopq %r15\npopq %r14\npopq %r13\npopq %r12\npopq %rbx\npopq %rbp\nret\n"
    ".size fiber_test_registers,.-fiber_test_registers\n");

static void Switch(void* target) { Fiber::SwitchTo(static_cast<Fiber*>(target)); }

__attribute__((noinline, target("avx2"))) static unsigned LiveVector(unsigned value, Fiber* target) {
  const float seed = static_cast<float>(value);
  const __m256 live = _mm256_setr_ps(seed + 0.25f, seed + 1.25f, seed + 2.25f, seed + 3.25f,
                                   seed + 4.25f, seed + 5.25f, seed + 6.25f, seed + 7.25f);
  const unsigned registers = fiber_test_registers(Switch, target);
  alignas(32) float out[8];
  _mm256_store_ps(out, live);
  for (unsigned i = 0; i != 8; ++i) CHECK(out[i] == seed + static_cast<float>(i) + 0.25f);
  return registers;
}

struct Run {
  Fiber* root;
  Fiber* fiber = nullptr;
  unsigned iterations;
  unsigned visits = 0;
  uint32_t creation_mxcsr;
  uint16_t creation_x87;
  uintptr_t stack_low = 0;
  uintptr_t stack_high = 0;
};

static void Repeated(void* opaque) {
  auto& run = *static_cast<Run*>(opaque);
  CHECK(Fiber::Current() == run.fiber);
  CHECK(Mxcsr() == run.creation_mxcsr && X87() == run.creation_x87);
  volatile uint64_t canary[256];
  const auto here = reinterpret_cast<uintptr_t>(&canary);
  CHECK(here >= run.stack_low && here + sizeof(canary) <= run.stack_high);
  for (unsigned i = 0; i != 256; ++i) canary[i] = 0x5a170000u + i;
  for (unsigned i = 0; i != run.iterations; ++i) {
    const uint32_t mxcsr = (run.creation_mxcsr & ~0x603fu) | 0x4000u | (i % 32u);
    const uint16_t x87 = (run.creation_x87 & ~uint16_t{0x0c00}) | 0x0800u;
    SetFp(mxcsr, x87);
    ++run.visits;
    CHECK(LiveVector(i, run.root) == 0);
    CHECK(Fiber::Current() == run.fiber);
    CHECK(Mxcsr() == mxcsr && X87() == x87);
    for (unsigned j = 0; j != 256; ++j) CHECK(canary[j] == 0x5a170000u + j);
  }
  // Exercise unwinding entirely within the fiber's own stack.
  try { throw std::runtime_error("fiber local exception"); }
  catch (const std::runtime_error& error) { CHECK(std::strcmp(error.what(), "fiber local exception") == 0); }
  ++run.visits;
  Fiber::SwitchTo(run.root);
  std::abort();
}

static void RunSwitches(unsigned iterations) {
  const uint32_t original_mxcsr = Mxcsr();
  const uint16_t original_x87 = X87();
  Fiber* root = Fiber::ConvertCurrentThread();
  CHECK(root && Fiber::Current() == root);
  CHECK(Fiber::ConvertCurrentThread() == root);
  Run run{root, nullptr, iterations, 0,
          (original_mxcsr & ~0x603fu) | 0x6003u,
          static_cast<uint16_t>((original_x87 & ~uint16_t{0x0c00}) | 0x0c00u)};
  SetFp(run.creation_mxcsr, run.creation_x87);
  run.fiber = Fiber::Create(256 * 1024, Repeated, &run);
  CHECK(run.fiber);
  CHECK(last_reservation_bytes == kMinimumStack + 2 * kUnit);
  run.stack_low = reinterpret_cast<uintptr_t>(last_reservation) + kUnit;
  run.stack_high = run.stack_low + kMinimumStack;
  for (unsigned i = 0; i != iterations + 1; ++i) {
    const uint32_t mxcsr = (original_mxcsr & ~0x603fu) | 0x2000u | (i % 32u);
    const uint16_t x87 = static_cast<uint16_t>((original_x87 & ~uint16_t{0x0c00}) | 0x0400u);
    SetFp(mxcsr, x87);
    Fiber::SwitchTo(root);  // Same-context switch must be a no-op.
    CHECK(LiveVector(i, run.fiber) == 0);
    CHECK(Fiber::Current() == root);
    CHECK(Mxcsr() == mxcsr && X87() == x87);
    CHECK(run.visits == i + 1);
  }
  run.fiber->Destroy();
  CHECK(Fiber::Current() == root);
  root->Destroy();
  CHECK(Fiber::Current() == nullptr);
  SetFp(original_mxcsr, original_x87);
}

struct DeepRun { Fiber* root; unsigned depth = 0; };
__attribute__((noinline)) static void DeepFrame(DeepRun& run, unsigned depth) {
  volatile uint64_t page[2048];
  for (unsigned i = 0; i != 2048; ++i) page[i] = 0x9e370000u + depth * 2048 + i;
  if (depth) DeepFrame(run, depth - 1);
  else { run.depth = 64; Fiber::SwitchTo(run.root); }
  for (unsigned i = 0; i != 2048; ++i) CHECK(page[i] == 0x9e370000u + depth * 2048 + i);
}
static void Deep(void* opaque) {
  auto& run = *static_cast<DeepRun*>(opaque);
  DeepFrame(run, 63);  // More than 1 MiB of live frames, beyond old 256 KiB.
  run.depth = 65;
  Fiber::SwitchTo(run.root);
  std::abort();
}
static void CheckDeepStack() {
  auto* root = Fiber::ConvertCurrentThread();
  DeepRun run{root};
  auto* fiber = Fiber::Create(1, Deep, &run);
  CHECK(root && fiber);
  Fiber::SwitchTo(fiber);
  CHECK(run.depth == 64);
  Fiber::SwitchTo(fiber);
  CHECK(run.depth == 65);
  fiber->Destroy(); root->Destroy(); CheckNoResources();
}

static void Unused(void*) { std::abort(); }
static void CheckAllocationFailure() {
  const unsigned calls = reserve_calls.load();
  errno = 0;
  CHECK(Fiber::Create(std::numeric_limits<size_t>::max(), Unused, nullptr) == nullptr && errno == ENOMEM);
  CHECK(reserve_calls.load() == calls);
  errno = 0;
  CHECK(Fiber::Create(0, nullptr, nullptr) == nullptr && errno == EINVAL);
  CHECK(reserve_calls.load() == calls);
  for (int failure : {HOST_CALL_RESERVE, HOST_CALL_ALLOCATE, HOST_CALL_MAP}) {
    host_fail(failure, -1);
    CHECK(Fiber::Create(0, Unused, nullptr) == nullptr);
    host_fail(HOST_CALL_NONE, 0);
    CheckNoResources();
  }
  auto* larger = Fiber::Create(kMinimumStack + 4096, Unused, nullptr);
  CHECK(larger && last_reservation_bytes == kMinimumStack + 3 * kUnit);
  larger->Destroy(); CheckNoResources();
}

static void CheckForeignCleanup() {
  auto* main = Fiber::ConvertCurrentThread();
  Fiber* completed = nullptr;
  std::thread finished([&] { completed = Fiber::ConvertCurrentThread(); CHECK(completed); });
  finished.join();
  completed->Destroy();
  CHECK(Fiber::Current() == main);
  Fiber* movable = nullptr;
  std::thread maker([&] { movable = Fiber::Create(0, Unused, nullptr); CHECK(movable); });
  maker.join();
  movable->Destroy();
  CHECK(Fiber::Current() == main);
  main->Destroy(); CheckNoResources();
}

struct Migration { Fiber* return_to = nullptr; unsigned visits = 0; Fiber* self = nullptr; };
static void Migrating(void* opaque) {
  auto& run = *static_cast<Migration*>(opaque);
  const uint32_t own_mxcsr = (Mxcsr() & ~0x6000u) | 0x6000u;
  _mm_setcsr(own_mxcsr);
  for (unsigned i = 0; i != 2; ++i) {
    CHECK(Fiber::Current() == run.self);
    CHECK(Mxcsr() == own_mxcsr);
    ++run.visits;
    Fiber::SwitchTo(run.return_to);
  }
  std::abort();
}
static void CheckMigration() {
  Migration run;
  run.self = Fiber::Create(0, Migrating, &run);
  CHECK(run.self);
  for (unsigned i = 0; i != 2; ++i) {
    std::thread thread([&] {
      run.return_to = Fiber::ConvertCurrentThread();
      CHECK(run.return_to);
      const auto fp = Mxcsr();
      Fiber::SwitchTo(run.self);
      CHECK(Fiber::Current() == run.return_to && Mxcsr() == fp);
      run.return_to->Destroy();
    });
    thread.join();
    CHECK(run.visits == i + 1);
  }
  run.self->Destroy(); CheckNoResources();
}

static void DestroySelf(void*) { Fiber::Current()->Destroy(); }
static void Returns(void*) {}
static void Child(std::string_view which) {
  rlimit limit{0, 0}; setrlimit(RLIMIT_CORE, &limit);
  if (which == "null-switch") { Fiber::SwitchTo(nullptr); std::exit(2); }
  auto* root = Fiber::ConvertCurrentThread();
  CHECK(root);
  if (which == "return" || which == "destroy-current") {
    auto* fiber = Fiber::Create(0, which == "return" ? Returns : DestroySelf, nullptr);
    CHECK(fiber); Fiber::SwitchTo(fiber); std::exit(2);
  }
  auto* fiber = Fiber::Create(0, Unused, nullptr);
  CHECK(fiber);
  volatile auto* at = static_cast<unsigned char*>(last_reservation);
  if (which == "upper-guard") at += last_reservation_bytes - kUnit;
  *at = 0x42;
  std::exit(2);
}

int main(int argc, char** argv) {
  if (argc == 2) Child(argv[1]);
  RunSwitches(20000); CheckNoResources();
  std::puts("PASS 40002 switches: six callee-saved GPRs, MXCSR including status, x87 control, creation inheritance, TLS identity, stack canaries, self-switch and local exception");
  CheckDeepStack(); std::puts("PASS live stack depth exceeds 1 MiB and all frames survive suspension");
  CheckAllocationFailure(); std::puts("PASS overflow, invalid entry, reserve/allocate/map refusal cleanup, larger stack honored");
  CheckForeignCleanup(); std::puts("PASS completed native-thread descriptor cleanup does not clear caller TLS");
  CheckMigration(); std::puts("PASS inactive fiber resumes on a second thread, with current TLS and saved FP state");
  std::array<std::thread, 8> threads;
  for (auto& thread : threads) thread = std::thread([] { RunSwitches(2000); });
  for (auto& thread : threads) thread.join();
  CheckNoResources(); std::puts("PASS eight concurrent host threads with independent roots/fibers and 32016 switches");
  std::puts("PASS complete PS5 fiber host suite; console execution not performed");
}
