// Per-thread state that PS5 signal handlers can inspect without emulated TLS.
// BSD 3-Clause License. See the project LICENSE.
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#define PROSPERO_SIGNAL_NOEXCEPT noexcept
extern "C" {
#else
#define PROSPERO_SIGNAL_NOEXCEPT
#endif

typedef void (*ProsperoSuspendWait)(void* context);

// Normal thread context only. The context must remain alive until unbound or
// cleared by that same thread. Existing different bindings are rejected.
// Returns zero on success, otherwise a positive errno-style error. A full
// registry never evicts a live thread. Call before publishing the running
// thread to code that can suspend it.
int prospero_signal_bind_suspend(void* context, ProsperoSuspendWait wait)
    PROSPERO_SIGNAL_NOEXCEPT;
void prospero_signal_unbind_suspend(void* expected_context) PROSPERO_SIGNAL_NOEXCEPT;

// Signal context: bounded lookup using pthread_self and lock-free atomics.
// No initialization, allocator, emutls, pthread key lookup, logging or lock.
// The registered callback must obey the same signal-context requirements.
// Returns one if a published binding was dispatched, otherwise zero.
// Deliberately not noexcept: cancellation of a suspended pthread may unwind
// through the wait callback and must still reach its native cleanup handlers.
int prospero_signal_dispatch_suspend(void);

// Idempotent cleanup of the calling thread. Must precede release of its bound
// context and TLS storage. A native pthread-key destructor is an additional
// exit fallback; it cannot clear a slot reused by another thread.
void prospero_signal_thread_clear(void) PROSPERO_SIGNAL_NOEXCEPT;

// The setter may initialize a slot and must run in ordinary thread context.
// Setting false on an unregistered thread is already satisfied. The getter
// and capture operation perform only signal-safe lookups and atomic access.
int prospero_signal_seh_set_active(int active) PROSPERO_SIGNAL_NOEXCEPT;
int prospero_signal_seh_active(void) PROSPERO_SIGNAL_NOEXCEPT;
int prospero_signal_seh_capture(uint32_t code, uintptr_t address) PROSPERO_SIGNAL_NOEXCEPT;
int prospero_signal_seh_read(uint32_t* code, uintptr_t* information0,
                            uintptr_t* information1) PROSPERO_SIGNAL_NOEXCEPT;

enum { PROSPERO_SIGNAL_THREAD_CAPACITY = 1024 };

#ifdef __cplusplus
}
#endif
#undef PROSPERO_SIGNAL_NOEXCEPT
