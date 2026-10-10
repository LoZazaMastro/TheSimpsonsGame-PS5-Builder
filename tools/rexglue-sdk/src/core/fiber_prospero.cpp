/**
 * @file        rex/core/fiber_prospero.cpp
 * @brief       PS5 cooperative fibers with explicit SysV contexts and guarded stacks
 * @license     BSD-3-Clause
 * Copyright (c) 2026 The Simpsons Game Recompiled PS5 contributors.
 */

#include <rex/platform.h>
#if REX_PLATFORM_PROSPERO

#include <rex/thread/fiber.h>

#include <ps5platform/kernel.h>
#include <ps5platform/shm.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <new>

#if !REX_ARCH_AMD64
#error The PS5 cooperative fiber backend requires the x86-64 SysV ABI.
#endif

// A normal C ABI call is the suspension point. Save the six nonvolatile
// integer registers, the return stack/instruction pointers, and FP control.
// MXCSR is saved in full so PPC rounding/status state stays with the fiber.
// x87 data registers and XMM/YMM registers are caller-saved in this ABI.
// No native getcontext/makecontext/swapcontext layout or syscall is involved.
extern "C" void prospero_fiber_switch(void* save, const void* load);
__asm__(
    ".text\n"
    ".p2align 4\n"
    ".globl prospero_fiber_switch\n"
    ".hidden prospero_fiber_switch\n"
    ".type prospero_fiber_switch,@function\n"
    "prospero_fiber_switch:\n"
    "  leaq 8(%rsp), %rax\n"
    "  movq %rax, 0(%rdi)\n"
    "  movq (%rsp), %rax\n"
    "  movq %rax, 8(%rdi)\n"
    "  movq %rbx, 16(%rdi)\n"
    "  movq %rbp, 24(%rdi)\n"
    "  movq %r12, 32(%rdi)\n"
    "  movq %r13, 40(%rdi)\n"
    "  movq %r14, 48(%rdi)\n"
    "  movq %r15, 56(%rdi)\n"
    "  stmxcsr 64(%rdi)\n"
    "  fnstcw 68(%rdi)\n"
    "  ldmxcsr 64(%rsi)\n"
    "  fldcw 68(%rsi)\n"
    "  movq 16(%rsi), %rbx\n"
    "  movq 24(%rsi), %rbp\n"
    "  movq 32(%rsi), %r12\n"
    "  movq 40(%rsi), %r13\n"
    "  movq 48(%rsi), %r14\n"
    "  movq 56(%rsi), %r15\n"
    "  movq 0(%rsi), %rsp\n"
    "  jmp *8(%rsi)\n"
    ".size prospero_fiber_switch,.-prospero_fiber_switch\n");

namespace rex::thread {
namespace {

constexpr size_t kUnit = PS5_KERNEL_DIRECT_ALIGNMENT;
constexpr size_t kGuardBytes = kUnit;
// XThread uses 16 MiB for recompiled guest/native call frames. A Win32
// CreateFiber size is only its initial commit; the old POSIX vector instead
// imposed a fixed 256 KiB total for a default guest fiber, beside heap data.
constexpr size_t kMinimumStack = 16 * 1024 * 1024;
static_assert(kUnit && (kUnit & (kUnit - 1)) == 0);

[[noreturn]] void InvalidFiber() { std::abort(); }

void ReleaseReservation(void* base, size_t bytes) {
  // Never return direct memory while a failed unmap could leave an alias.
  if (ps5_vrange_release(base, bytes) != 0) InvalidFiber();
}

void ReleaseDirect(int64_t start, size_t bytes) {
  if (sceKernelReleaseDirectMemory(start, bytes) != 0) InvalidFiber();
}

}  // namespace

thread_local Fiber* Fiber::tls_current_ = nullptr;

Fiber* Fiber::ConvertCurrentThread() {
  if (tls_current_) return tls_current_;
  auto* f = new (std::nothrow) Fiber();
  if (!f) {
    errno = ENOMEM;
    return nullptr;
  }
  // The current registers will be saved by the first actual SwitchTo.
  f->is_thread_fiber_ = true;
  tls_current_ = f;
  return f;
}

Fiber* Fiber::Create(size_t stack_size, void (*entry)(void*), void* arg) {
  if (!entry) {
    errno = EINVAL;
    return nullptr;
  }
  const size_t requested = std::max(stack_size, kMinimumStack);
  constexpr size_t overhead = 2 * kGuardBytes + kUnit - 1;
  if (requested > std::numeric_limits<size_t>::max() - overhead) {
    errno = ENOMEM;
    return nullptr;
  }
  const size_t bytes = (requested + kUnit - 1) & ~(kUnit - 1);
  const size_t reservation_bytes = bytes + 2 * kGuardBytes;
  uint32_t initial_mxcsr;
  uint16_t initial_x87;
  __asm__ __volatile__("stmxcsr %0" : "=m"(initial_mxcsr));
  __asm__ __volatile__("fnstcw %0" : "=m"(initial_x87));
  auto* f = new (std::nothrow) Fiber();
  if (!f) {
    errno = ENOMEM;
    return nullptr;
  }
  void* reservation = nullptr;
  if (ps5_vrange_reserve(reservation_bytes, nullptr, kUnit, &reservation) != 0) {
    delete f;
    errno = ENOMEM;
    return nullptr;
  }
  const uintptr_t reservation_address = reinterpret_cast<uintptr_t>(reservation);
  if (!reservation || reservation_address % kUnit != 0 ||
      reservation_address > std::numeric_limits<uintptr_t>::max() - reservation_bytes) {
    if (reservation) ReleaseReservation(reservation, reservation_bytes);
    delete f;
    errno = ENOMEM;
    return nullptr;
  }
  int64_t direct_start = -1;
  if (sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), bytes, kUnit,
                                   PS5_KERNEL_DIRECT_TYPE_CPU, &direct_start) != 0) {
    ReleaseReservation(reservation, reservation_bytes);
    delete f;
    errno = ENOMEM;
    return nullptr;
  }
  void* const bottom = static_cast<uint8_t*>(reservation) + kGuardBytes;
  void* mapped = bottom;
  const int mapped_result = sceKernelMapDirectMemory(
      &mapped, bytes, PS5_KERNEL_PROT_CPU_READ | PS5_KERNEL_PROT_CPU_WRITE,
      PS5_KERNEL_MAP_FIXED, direct_start, kUnit);
  if (mapped_result != 0 || mapped != bottom) {
    if (mapped_result == 0 && sceKernelMunmap(mapped, bytes) != 0) InvalidFiber();
    ReleaseReservation(reservation, reservation_bytes);
    ReleaseDirect(direct_start, bytes);
    delete f;
    errno = ENOMEM;
    return nullptr;
  }

  f->stack_reservation_ = reservation;
  f->stack_reservation_bytes_ = reservation_bytes;
  f->stack_bytes_ = bytes;
  f->stack_direct_start_ = direct_start;
  f->entry_ = entry;
  f->arg_ = arg;
  // SysV function entry has (RSP + 8) aligned to 16. Trampoline never
  // returns, but a real synthetic return address bounds its initial frame.
  const uintptr_t top = reinterpret_cast<uintptr_t>(bottom) + bytes;
  const uintptr_t sp = (top & ~uintptr_t{15}) - sizeof(uintptr_t);
  *reinterpret_cast<uintptr_t*>(sp) = reinterpret_cast<uintptr_t>(&InvalidFiber);
  f->context_.rsp = sp;
  f->context_.rip = reinterpret_cast<uintptr_t>(&Fiber::Trampoline);
  f->context_.mxcsr = initial_mxcsr;
  f->context_.x87_control = initial_x87;
  return f;
}

void Fiber::Trampoline() noexcept {
  Fiber* const f = tls_current_;
  if (!f || !f->entry_) InvalidFiber();
  f->entry_(f->arg_);
  // Guest FiberEntryPoint switches to the main fiber before returning.
  // Returning from an arbitrary start routine cannot fall into unknown code.
  InvalidFiber();
}

void Fiber::SwitchTo(Fiber* target) {
  Fiber* const from = tls_current_;
  if (!from || !target) InvalidFiber();
  if (from == target) return;
  tls_current_ = target;
  prospero_fiber_switch(&from->context_, &target->context_);
}

void Fiber::Destroy() {
  if (tls_current_ == this) {
    if (!is_thread_fiber_) InvalidFiber();
    tls_current_ = nullptr;
  }
  // XThread can destroy its completed thread's descriptor from another
  // thread. Such cleanup must not clear that caller's own current fiber.
  if (stack_reservation_) {
    ReleaseReservation(stack_reservation_, stack_reservation_bytes_);
    ReleaseDirect(stack_direct_start_, stack_bytes_);
  }
  delete this;
}

}  // namespace rex::thread
#endif  // REX_PLATFORM_PROSPERO
