// Exercise the actual PS5 SDK _setjmp.o and the real guest_jump.h macro.
// The runner supplies the SDK's real 96-byte jmp_buf declaration and renames
// the object's symbols, leaving its machine-code bytes unchanged, so this
// test never interposes Linux libc's own setjmp/longjmp implementation.
#include "guest_jump.h"

#include <array>
#include <atomic>
#include <climits>
#include <cstdio>
#include <cstring>
#include <thread>
#include <xmmintrin.h>

static_assert(sizeof(jmp_buf) == 96);
static_assert(sizeof(PPCJumpBuffer) == 96);
static_assert(alignof(PPCJumpBuffer) == 16);

static void Require(bool condition, const char* message) {
  if (!condition) {
    std::fprintf(stderr, "FAIL %s\n", message);
    std::abort();
  }
}

static void CheckPattern(const void* ptr, size_t count, const char* message) {
  const auto* bytes = static_cast<const unsigned char*>(ptr);
  for (size_t i = 0; i < count; ++i) Require(bytes[i] == 0xa5, message);
}

static void CheckUnusedBufferBytes(const PPCJumpBuffer& buffer) {
  const auto* bytes = reinterpret_cast<const unsigned char*>(&buffer.value);
  CheckPattern(bytes + 66, 2, "reserved bytes before MXCSR changed");
  CheckPattern(bytes + 72, 24, "SDK save wrote beyond its 72-byte state");
}

struct alignas(16) GuardedBuffer {
  std::array<unsigned char, 16> before;
  PPCJumpBuffer buffer;
  std::array<unsigned char, 16> after;
};

static void CheckBoundsAndReturns() {
  // Heap allocation mirrors the real native ownership. This frame remains
  // live throughout both returns; nothing nontrivial is skipped by longjmp.
  auto* cell = new GuardedBuffer;
  std::memset(cell, 0xa5, sizeof(*cell));
  if (_setjmp(cell->buffer.value) == 0) {
    CheckPattern(cell->before.data(), cell->before.size(), "leading canary changed");
    CheckPattern(cell->after.data(), cell->after.size(), "trailing canary changed");
    CheckUnusedBufferBytes(cell->buffer);
    ___longjmp(cell->buffer.value, -23);
  }
  CheckPattern(cell->before.data(), cell->before.size(), "leading canary changed on restore");
  CheckPattern(cell->after.data(), cell->after.size(), "trailing canary changed on restore");
  CheckUnusedBufferBytes(cell->buffer);
  delete cell;
}

[[gnu::noinline]] static void DeepGuestJump(uint32_t key, int value, unsigned depth) {
  volatile uint64_t frame[32];
  for (unsigned i = 0; i < 32; ++i) frame[i] = 0x1122334400000000ull + depth + i;
  // An opaque use keeps the stack frames real with the optimized test flags.
  __asm__ __volatile__("" : : "m"(frame) : "memory");
  if (depth) DeepGuestJump(key, value, depth - 1);
  ppc_longjmp(key, value);
}

static void CheckGuestReturn(int requested) {
  const uint32_t key = 0x12345678;
  volatile uint64_t live_value = 0x8877665544332211ull;
  const int returned = ppc_setjmp(key);
  if (returned == 0) DeepGuestJump(key, requested, 12);
  Require(returned == (requested == 0 ? 1 : requested), "wrong guest jump return value");
  Require(live_value == 0x8877665544332211ull, "live stack frame was not restored");
}

static void CheckMapRehash() {
  auto& map = get_jmp_buf_map();
  map.clear();
  const uint32_t saved_key = 0xabcdef01;
  auto* const buffer = &map[saved_key];
  std::memset(&buffer->value, 0xa5, sizeof(buffer->value));
  const int returned = ppc_setjmp(saved_key);
  if (returned == 0) {
    for (uint32_t i = 0; i < 4096; ++i) {
      auto& neighbor = map[i];
      std::memset(&neighbor.value, 0xa5, sizeof(neighbor.value));
    }
    map.rehash(16384);
    Require(&map.at(saved_key) == buffer, "rehash moved the saved map value");
    CheckUnusedBufferBytes(*buffer);
    ppc_longjmp(saved_key, 71);
  }
  Require(returned == 71, "map jump did not resume through the real macro");
  CheckUnusedBufferBytes(*buffer);
  for (uint32_t i = 0; i < 4096; ++i) {
    CheckPattern(&map.at(i).value, sizeof(jmp_buf), "neighbor map buffer changed");
  }
  map.clear();
}

static uint16_t ReadX87Control() {
  uint16_t value;
  __asm__ __volatile__("fnstcw %0" : "=m"(value));
  return value;
}

static void WriteX87Control(uint16_t value) {
  __asm__ __volatile__("fldcw %0" : : "m"(value));
}

static void CheckFpControls(unsigned rounding) {
  const uint32_t original_mxcsr = _mm_getcsr();
  const uint16_t original_x87 = ReadX87Control();
  // Keep all exceptions masked, vary rounding and the two denormal modes.
  const uint32_t saved_mxcsr =
      ((original_mxcsr | 0x1f80) & ~uint32_t{0xe07f}) | (rounding << 13) |
      ((rounding & 1) ? 0x40u : 0) | ((rounding & 2) ? 0x8000u : 0);
  const uint16_t saved_x87 =
      static_cast<uint16_t>((original_x87 & ~uint16_t{0x0c00}) | (rounding << 10));
  PPCJumpBuffer buffer;
  _mm_setcsr(saved_mxcsr);
  WriteX87Control(saved_x87);
  if (_setjmp(buffer.value) == 0) {
    _mm_setcsr((saved_mxcsr ^ 0xe040u) | 0x21u);
    WriteX87Control(static_cast<uint16_t>(saved_x87 ^ 0x0c00));
    ___longjmp(buffer.value, 1);
  }
  const uint32_t restored_mxcsr = _mm_getcsr();
  const uint16_t restored_x87 = ReadX87Control();
  _mm_setcsr(original_mxcsr);
  WriteX87Control(original_x87);
  Require((restored_mxcsr & ~0x3fu) == (saved_mxcsr & ~0x3fu),
          "MXCSR controls were not restored");
  Require((restored_mxcsr & 0x3fu) == 0x21u,
          "SDK contract preserves the current MXCSR exception flags");
  Require(restored_x87 == saved_x87, "x87 control was not restored");
}

extern "C" int CheckJumpRegisters(jmp_buf);
__asm__(R"(
.text
.globl CheckJumpRegisters
.type CheckJumpRegisters,@function
CheckJumpRegisters:
  pushq %rbp
  pushq %rbx
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15
  subq $24, %rsp
  movq %rdi, 0(%rsp)
  movq %rsp, 8(%rsp)
  movabsq $0x1111222233334444, %rbx
  movabsq $0x2222333344445555, %rbp
  movabsq $0x3333444455556666, %r12
  movabsq $0x4444555566667777, %r13
  movabsq $0x5555666677778888, %r14
  movabsq $0x6666777788889999, %r15
  call tested_sdk_setjmp
  testl %eax, %eax
  jnz .Ljump_resumed
  xorq %rbx, %rbx
  xorq %rbp, %rbp
  xorq %r12, %r12
  xorq %r13, %r13
  xorq %r14, %r14
  xorq %r15, %r15
  movq 0(%rsp), %rdi
  movl $37, %esi
  call tested_sdk_longjmp
  ud2
.Ljump_resumed:
  cmpl $37, %eax
  jne .Ljump_failure
  cmpq 8(%rsp), %rsp
  jne .Ljump_failure
  movabsq $0x1111222233334444, %rax
  cmpq %rax, %rbx
  jne .Ljump_failure
  movabsq $0x2222333344445555, %rax
  cmpq %rax, %rbp
  jne .Ljump_failure
  movabsq $0x3333444455556666, %rax
  cmpq %rax, %r12
  jne .Ljump_failure
  movabsq $0x4444555566667777, %rax
  cmpq %rax, %r13
  jne .Ljump_failure
  movabsq $0x5555666677778888, %rax
  cmpq %rax, %r14
  jne .Ljump_failure
  movabsq $0x6666777788889999, %rax
  cmpq %rax, %r15
  jne .Ljump_failure
  xorl %eax, %eax
  jmp .Ljump_done
.Ljump_failure:
  movl $1, %eax
.Ljump_done:
  addq $24, %rsp
  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %rbx
  popq %rbp
  ret
.size CheckJumpRegisters,.-CheckJumpRegisters
)");

static void CheckThreadLocalMaps() {
  constexpr size_t count = 4;
  std::array<std::thread, count> threads;
  std::array<uintptr_t, count> maps{};
  std::atomic<size_t> ready{0};
  for (size_t i = 0; i < count; ++i) {
    threads[i] = std::thread([&, i] {
      maps[i] = reinterpret_cast<uintptr_t>(&get_jmp_buf_map());
      ready.fetch_add(1, std::memory_order_release);
      while (ready.load(std::memory_order_acquire) < count) std::this_thread::yield();
      for (int v = 0; v < 100; ++v) CheckGuestReturn(v);
    });
  }
  for (auto& thread : threads) thread.join();
  for (size_t i = 0; i < count; ++i) {
    for (size_t j = i + 1; j < count; ++j) Require(maps[i] != maps[j], "TLS maps alias");
  }
}

int main() {
  CheckBoundsAndReturns();
  std::puts("PASS SDK buffer bounds: heap canaries and unused bytes unchanged");
  PPCJumpBuffer registers;
  Require(CheckJumpRegisters(registers.value) == 0,
          "nonvolatile registers, stack or instruction pointer not restored");
  std::puts("PASS RBX/RBP/R12/R13/R14/R15, stack pointer and resume instruction");
  for (int value : {0, 1, -23, INT_MIN, INT_MAX}) CheckGuestReturn(value);
  std::puts("PASS real guest macro: nested resume and zero/positive/negative return values");
  CheckMapRehash();
  std::puts("PASS live map value survives rehash; 4096 neighbor buffers unchanged");
  for (unsigned mode = 0; mode < 4; ++mode) CheckFpControls(mode);
  std::puts("PASS four MXCSR/x87 rounding modes, denormal controls and exception flags");
  CheckThreadLocalMaps();
  std::puts("PASS four concurrent thread-local maps and 400 guest jumps");
}
