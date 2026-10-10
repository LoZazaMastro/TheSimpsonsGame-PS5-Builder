/*
 * PS5 Platform - dlmalloc 2.8.6 as the title heap's allocator (src/heap.c).
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Local configuration derived from mihawk-99/PS5_PayloadSDK, commit
 * b5efad528e0ac1b8289f39d72a0e890ff68ff0a6, platform/src/dlmalloc/mspace.c.
 * malloc.c derives from Doug Lea's MIT-0 release with a diagnostic hook
 * at the existing first-smallbin corruption check; see SOURCE.
 * Mapping, eight arenas and libc fallback remain in the matched SDK heap.
 */
#include <errno.h>
#include <stddef.h>
#include <rex/platform/trace_prospero.h>

void *ps5p_heap_map(size_t bytes);
int ps5p_heap_unmap(void *address, size_t bytes);

__attribute__((noreturn)) void prospero_heap_failure(
   const char *kind, const char *operation, unsigned line,
   void *mspace, void *pointer);

#define ONLY_MSPACES 1
/* Bypass dlmalloc's built-in lock selection. The state word has Sonic's
 * four-byte layout, with a deliberate sleeping backoff under contention. */
#define USE_LOCKS 2
#include "allocator_lock.h"
static MLOCK_T malloc_global_mutex = 0;

/* Preserve all corruption checks. The line is the macro invocation site;
 * several checks in a nested macro can share it, so it is not a branch ID. */
#define CORRUPTION_ERROR_ACTION(m) \
   prospero_heap_failure("corruption", __func__, (unsigned)__LINE__, (void *)(m), NULL)
#define USAGE_ERROR_ACTION(m, p) \
   prospero_heap_failure("usage", __func__, (unsigned)__LINE__, (void *)(m), (void *)(p))

/* Values come from the existing predicate. Do not re-read damaged links or
 * a chunk header merely for logging. The index gives the expected size;
 * the original request is the enclosing malloc/mspace_malloc's bytes.
 * caller is the allocator's immediate caller; the preceding wrapper BEGIN
 * event separately records the original title/native caller.
 */
#define DLMALLOC_UNLINK_FIRST_FAILURE(M, B, P, F, I, BACK, ADDR_OK) do { \
   const uintptr_t caller = (uintptr_t)__builtin_return_address(0); \
   prospero_trace_event(PROSPERO_TRACE_HEAP_LINKS, caller, (uintptr_t)(M), \
                       (uintptr_t)(B), (uintptr_t)(P), (uintptr_t)(F)); \
   prospero_trace_event(PROSPERO_TRACE_HEAP_CHECK, caller, (uintptr_t)(I), \
                       (uintptr_t)(BACK), (uintptr_t)bytes, \
                       ((uintptr_t)((ADDR_OK) ? 2u : 1u) << 32) | (unsigned)__LINE__); \
} while (0)

/* Each block records its mspace so cross-thread free/realloc use its owner. */
#define FOOTERS 1
#define HAVE_MORECORE 0
#define HAVE_MMAP 1
#define HAVE_MREMAP 0
#define MMAP(s) ps5p_heap_map(s)
#define DIRECT_MMAP(s) ps5p_heap_map(s)
#define MUNMAP(a, s) ps5p_heap_unmap((a), (s))
/* Released PS5 direct memory keeps old bytes. Even large calloc must clear. */
#define MMAP_CLEARS 0
#define MALLOC_ALIGNMENT ((size_t)32)
#define DEFAULT_GRANULARITY ((size_t)16 << 20)
#define DEFAULT_MMAP_THRESHOLD ((size_t)32 << 20)
#define MALLOC_FAILURE_ACTION errno = ENOMEM
#define NO_MALLINFO 1
#define NO_MALLOC_STATS 1
#define malloc_getpagesize ((size_t)0x10000)
#include "malloc.c"
