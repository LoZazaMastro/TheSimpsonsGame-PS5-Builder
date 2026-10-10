/**
 * @file        rex/thread/fiber.h
 * @brief       Host OS fiber primitive for cooperative context switching
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#pragma once

#include <rex/platform.h>
#include <cstddef>

#if REX_PLATFORM_LINUX || REX_PLATFORM_MAC
#include <ucontext.h>
#include <cstdint>
#include <vector>
#elif REX_PLATFORM_PROSPERO
#include <cstdint>
#endif

namespace rex::thread {

/// Host OS fiber primitive.
/// Each guest fiber gets one Fiber. Switching preserves the entire C++ call
/// stack, so mid-function resume works without any LR lookup.
struct Fiber {
  /// Convert the calling thread into a fiber.
  /// Must be called once on a thread before any SwitchTo.
  /// Returns a handle for the thread's current execution context.
  static Fiber* ConvertCurrentThread();

  /// Create a new fiber with its own host stack.
  /// entry(arg) is called when the fiber is first switched to.
  static Fiber* Create(size_t stack_size, void (*entry)(void*), void* arg);

  /// Suspend the current fiber and resume target.
  /// Returns when another fiber calls SwitchTo back to this one.
  static void SwitchTo(Fiber* target);

  /// Destroy this fiber. Must not be called while it is executing.
  void Destroy();

  /// Returns the fiber currently executing on this thread, or nullptr.
  static Fiber* Current() { return tls_current_; }

 private:
  static thread_local Fiber* tls_current_;

#if REX_PLATFORM_WIN32
  void* handle_ = nullptr;
  bool is_thread_fiber_ = false;
#elif REX_PLATFORM_PROSPERO
  // Cooperative SysV call boundary, not a signal/ucontext snapshot. Caller-
  // saved scalar and vector registers are spilled by the calling compiler.
  struct alignas(16) Context {
    uintptr_t rsp = 0;
    uintptr_t rip = 0;
    uintptr_t rbx = 0;
    uintptr_t rbp = 0;
    uintptr_t r12 = 0;
    uintptr_t r13 = 0;
    uintptr_t r14 = 0;
    uintptr_t r15 = 0;
    uint32_t mxcsr = 0;
    uint16_t x87_control = 0;
  } context_;
  static_assert(offsetof(Context, rsp) == 0 && offsetof(Context, rip) == 8 &&
                offsetof(Context, rbx) == 16 && offsetof(Context, rbp) == 24 &&
                offsetof(Context, r12) == 32 && offsetof(Context, r13) == 40 &&
                offsetof(Context, r14) == 48 && offsetof(Context, r15) == 56 &&
                offsetof(Context, mxcsr) == 64 && offsetof(Context, x87_control) == 68);

  // Created fibers have dedicated direct memory with unbacked guards at
  // both ends. A converted native thread continues using its own stack.
  void* stack_reservation_ = nullptr;
  size_t stack_reservation_bytes_ = 0;
  size_t stack_bytes_ = 0;
  int64_t stack_direct_start_ = -1;
  void (*entry_)(void*) = nullptr;
  void* arg_ = nullptr;
  bool is_thread_fiber_ = false;

  static void Trampoline() noexcept;
#elif REX_PLATFORM_LINUX || REX_PLATFORM_MAC
  ucontext_t context_{};
  std::vector<uint8_t> stack_;
  void (*entry_)(void*) = nullptr;
  void* arg_ = nullptr;
  bool is_thread_fiber_ = false;

  static void Trampoline();
#endif
};

}  // namespace rex::thread
