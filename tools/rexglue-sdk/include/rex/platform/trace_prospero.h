// Allocation-free PS5 terminal diagnostics. C-compatible for native wrappers.
// BSD 3-Clause License. See the project LICENSE.
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#define PROSPERO_TRACE_NOEXCEPT noexcept
#else
#define PROSPERO_TRACE_NOEXCEPT
#endif

enum ProsperoTraceKind {
  PROSPERO_TRACE_PHASE = 1,
  PROSPERO_TRACE_ALLOC_BEGIN = 2,
  PROSPERO_TRACE_ALLOC_END = 3,
  PROSPERO_TRACE_FREE_BEGIN = 4,
  PROSPERO_TRACE_FREE_END = 5,
  PROSPERO_TRACE_REALLOC_BEGIN = 6,
  PROSPERO_TRACE_REALLOC_END = 7,
  PROSPERO_TRACE_ALIGN_BEGIN = 8,
  PROSPERO_TRACE_ALIGN_END = 9,
  PROSPERO_TRACE_HEAP_DETAIL = 10,
  PROSPERO_TRACE_THREAD = 11,
  PROSPERO_TRACE_FIBER = 12,
  PROSPERO_TRACE_JUMP = 13,
  // heap_links: a=arena, b=bin, c=P, d=F; caller is mspace_malloc's
  // immediate return PC. The public wrapper callsite is in alloc_begin.
  PROSPERO_TRACE_HEAP_LINKS = 14,
  // heap_check: a=index, b=observed F->bk, c=request bytes,
  // d=(failure_flags<<32)|source_line. No diagnostic-only dereference.
  PROSPERO_TRACE_HEAP_CHECK = 15,
};

// Stable numeric kinds, four opaque pointer-sized arguments, absolute caller
// addresses. Does not dereference caller/arguments or allocate memory. Each
// producer attempts one slot claim; a busy/stale claim is counted and dropped.
// Uses pthread_self for an opaque host thread handle; no C++ thread_local data.
void prospero_trace_event(unsigned kind, uintptr_t caller, uintptr_t a,
                          uintptr_t b, uintptr_t c, uintptr_t d) PROSPERO_TRACE_NOEXCEPT;

// Normal startup only, when BOTH addresses of the same symbol are known.
// Never derive a bias from a page-alignment guess. Zero runtime address or a
// runtime address below its linked address leaves the image anchor unknown.
void prospero_trace_set_image_anchors(uintptr_t runtime_anchor,
                                      uintptr_t linked_anchor) PROSPERO_TRACE_NOEXCEPT;

// Normal startup: share the already opened regular fault-log descriptor.
// The trace code neither opens nor closes it. A negative descriptor disables
// persistent checkpoints without disabling the in-memory ring.
void prospero_trace_set_output_fd(int fd) PROSPERO_TRACE_NOEXCEPT;

// Register only a known mapped, writable stack range for the calling thread.
// Bounds are stored in a fixed table, with no heap or thread_local storage.
// Invalid/full registrations are counted; a missing range disables stack reads.
void prospero_trace_register_stack(uintptr_t low, size_t bytes) PROSPERO_TRACE_NOEXCEPT;
void prospero_trace_unregister_stack(void) PROSPERO_TRACE_NOEXCEPT;

// Record a phase and append one bounded persistent line without freezing the
// ring. Suitable for startup phases and throttled heartbeats, not every malloc.
void prospero_trace_checkpoint(uintptr_t phase, uintptr_t b, uintptr_t c,
                               uintptr_t d) PROSPERO_TRACE_NOEXCEPT;

// Append a caller-formatted bounded telemetry line. The caller owns the
// readable buffer for this call. No recording recursion; writes stop at crash.
void prospero_trace_write_line(const char* data, size_t bytes) PROSPERO_TRACE_NOEXCEPT;

// First terminal dump freezes further recording and writes retained events
// in reservation-sequence order through an already opened descriptor. There
// are no allocations, path operations, stdio calls, stack walks or waits on
// writers. Busy/incomplete slots are skipped and explicitly counted.
// Returns 1 for the first dump attempt, 0 if already attempted or fd is invalid.
int prospero_trace_dump_fd(int fd) PROSPERO_TRACE_NOEXCEPT;

// Separately guarded raw words, not a backtrace: at most 64 KiB from rsp to
// the registered live current-thread stack end. Unknown/out-of-range RSP is
// reported without reading memory. Intended for the abort/signal writer.
int prospero_trace_dump_stack_fd(int fd, uintptr_t rsp) PROSPERO_TRACE_NOEXCEPT;

#ifdef __cplusplus
}
#endif
#undef PROSPERO_TRACE_NOEXCEPT
