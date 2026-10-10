/*
 * Matched PS5 SDK allocator wrappers with bounded event recording.
 * Derived from mihawk-99/PS5_PayloadSDK b5efad528e0ac1b8289f39d72a0e890ff68ff0a6,
 * platform/src/heap_wrap.c, Copyright (C) 2026 Mihawk, GPL-3.0-or-later.
 *
 * The marked implementation body differs only in function linkage/names.
 * Ownership routing, eight-arena selection in heap.c, native fallback,
 * realloc copy rules and error behavior are unchanged. Public wrappers add
 * entry/exit events with the actual caller PC; recording does not allocate.
 * No guard allocator, extra owns/usable-size checks or payload reads are added.
 */
#include "ps5platform/heap.h"
#include <rex/platform/trace_prospero.h>
#include <stdio.h>
#include <sys/types.h>

void *__wrap_malloc(size_t);
void *__wrap_calloc(size_t, size_t);
void __wrap_free(void *);
size_t __wrap_malloc_usable_size(const void *);
void *__wrap_realloc(void *, size_t);
void *__wrap_reallocf(void *, size_t);
void *__wrap_reallocarray(void *, size_t, size_t);
int __wrap_posix_memalign(void **, size_t, size_t);
void *__wrap_aligned_alloc(size_t, size_t);
void *__wrap_memalign(size_t, size_t);
ssize_t __wrap_getdelim(char **, size_t *, int, FILE *);
ssize_t __wrap_getline(char **, size_t *, FILE *);

/* BEGIN MATCHED SDK IMPLEMENTATION */
/*
 * PS5 Platform - the title heap's wraps of libc's allocator
 * (include/ps5platform/heap.h).
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Apart from src/heap.c, so that a title with an allocator of its own can use
 * the heap (ps5_heap_malloc and the rest) without linking these.
 */
#include "ps5platform/heap.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

void ps5p_heap_count_fallback(void);

/* libc's allocator, through the linker's --wrap. malloc_usable_size is weak:
 * a title that does not reference it links without its wrap. */
void *__real_malloc(size_t bytes);
void *__real_calloc(size_t count, size_t bytes);
void *__real_realloc(void *pointer, size_t bytes);
void __real_free(void *pointer);
int __real_posix_memalign(void **out, size_t alignment, size_t bytes);
__attribute__((weak)) size_t __real_malloc_usable_size(const void *pointer);

static void *
prospero_impl_malloc(size_t bytes)
{
   void *const pointer = ps5_heap_malloc(bytes);
   if (pointer)
      return pointer;
   ps5p_heap_count_fallback();
   return __real_malloc(bytes);
}

static void *
prospero_impl_calloc(size_t count, size_t bytes)
{
   void *const pointer = ps5_heap_calloc(count, bytes);
   if (pointer)
      return pointer;
   ps5p_heap_count_fallback();
   return __real_calloc(count, bytes);
}

static void
prospero_impl_free(void *pointer)
{
   if (!pointer)
      return;
   if (ps5_heap_owns(pointer))
      ps5_heap_free(pointer);
   else
      __real_free(pointer);
}

static size_t
prospero_impl_malloc_usable_size(const void *pointer)
{
   if (!pointer)
      return 0;
   if (ps5_heap_owns(pointer))
      return ps5_heap_usable_size(pointer);
   return __real_malloc_usable_size ? __real_malloc_usable_size(pointer) : 0;
}

static void *
prospero_impl_realloc(void *pointer, size_t bytes)
{
   if (!pointer)
      return __wrap_malloc(bytes);
   /* FreeBSD's realloc gives a zero-size request a minimum-size object. */
   if (bytes == 0)
      bytes = 1;
   if (ps5_heap_owns(pointer)) {
      void *const moved = ps5_heap_realloc(pointer, bytes);
      if (moved)
         return moved;
      /* Direct memory is exhausted: move the block to libc. */
      void *const copy = __real_malloc(bytes);
      if (!copy)
         return NULL;
      ps5p_heap_count_fallback();
      const size_t kept = ps5_heap_usable_size(pointer);
      memcpy(copy, pointer, kept < bytes ? kept : bytes);
      ps5_heap_free(pointer);
      return copy;
   }
   void *const moved = __real_realloc(pointer, bytes);
   if (moved || !__real_malloc_usable_size)
      return moved;
   /* libc's heap is exhausted: move its block to the title heap. */
   void *const copy = ps5_heap_malloc(bytes);
   if (!copy)
      return NULL;
   const size_t kept = __real_malloc_usable_size(pointer);
   memcpy(copy, pointer, kept < bytes ? kept : bytes);
   __real_free(pointer);
   return copy;
}

static void *
prospero_impl_reallocf(void *pointer, size_t bytes)
{
   void *const moved = __wrap_realloc(pointer, bytes);
   if (!moved)
      __wrap_free(pointer);
   return moved;
}

static void *
prospero_impl_reallocarray(void *pointer, size_t count, size_t bytes)
{
   if (bytes != 0 && count > SIZE_MAX / bytes) {
      errno = ENOMEM;
      return NULL;
   }
   return __wrap_realloc(pointer, count * bytes);
}

static int
prospero_impl_posix_memalign(void **out, size_t alignment, size_t bytes)
{
   if (alignment < sizeof(void *) || (alignment & (alignment - 1)) != 0)
      return EINVAL;
   void *const pointer = ps5_heap_memalign(alignment, bytes);
   if (pointer) {
      *out = pointer;
      return 0;
   }
   ps5p_heap_count_fallback();
   return __real_posix_memalign(out, alignment, bytes);
}

static void *
prospero_impl_aligned_alloc(size_t alignment, size_t bytes)
{
   void *pointer = NULL;
   const int result =
      __wrap_posix_memalign(&pointer, alignment < sizeof(void *) ? sizeof(void *) : alignment, bytes);
   if (result != 0) {
      errno = result;
      return NULL;
   }
   return pointer;
}

static void *
prospero_impl_memalign(size_t alignment, size_t bytes)
{
   return __wrap_aligned_alloc(alignment, bytes);
}

/* libc's getdelim grows the caller's buffer with libc's realloc, which must
 * never see a title-heap block: this one grows it with the wrap. */
static ssize_t
prospero_impl_getdelim(char **line, size_t *capacity, int delimiter, FILE *stream)
{
   if (!line || !capacity || !stream) {
      errno = EINVAL;
      return -1;
   }
   if (!*line)
      *capacity = 0;
   size_t length = 0;
   for (;;) {
      const int c = fgetc(stream);
      if (c == EOF) {
         if (length == 0 || ferror(stream))
            return -1;
         break;
      }
      if (length + 2 > *capacity) {
         size_t grown = *capacity < 64 ? 128 : *capacity * 2;
         if (grown > (size_t)SSIZE_MAX) {
            errno = EOVERFLOW;
            return -1;
         }
         char *const moved = __wrap_realloc(*line, grown);
         if (!moved)
            return -1;
         *line = moved;
         *capacity = grown;
      }
      (*line)[length++] = (char)c;
      if (c == (unsigned char)delimiter)
         break;
   }
   (*line)[length] = '\0';
   return (ssize_t)length;
}

static ssize_t
prospero_impl_getline(char **line, size_t *capacity, FILE *stream)
{
   return __wrap_getdelim(line, capacity, '\n', stream);
}
/* END MATCHED SDK IMPLEMENTATION */

/* Event schema (a,b,c,d):
 * malloc/calloc begin=(0,bytes,count,api), end=(result,bytes,count,api).
 * realloc begin=(old,bytes,count,api), end=(result,bytes,old,api).
 * free begin/end=(pointer,0,0,api).
 * align begin=(out-slot-or-0,bytes,alignment,api),
 *       end=(result,bytes,error-code,api).
 * heap_detail usable begin=(pointer,0,0,0x10000|api),
 *                    end=(pointer,usable,0,0x20000|api).
 * heap_detail line begin=(line-slot,capacity-slot,stream,0x10000|api),
 *                  end=(line-slot,result,stream,0x20000|api).
 * API ids: malloc1 calloc2 free3 usable_size4 realloc5 reallocf6,
 * reallocarray7 posix_memalign8 aligned_alloc9 memalign10 getdelim11 getline12.
 * Nested calls retain their own events. They share the opaque thread id and
 * expose the intermediate wrapper PC, while the outer BEGIN retains the
 * original title/native caller. No stack walk or context probing is used.
 */
#define TRACE_CALLER const uintptr_t caller = (uintptr_t)__builtin_return_address(0)
#define TRACE(kind, a, b, c, api) \
   prospero_trace_event((kind), caller, (uintptr_t)(a), (uintptr_t)(b), \
                       (uintptr_t)(c), (uintptr_t)(api))
#define TRACE_WRAPPER __attribute__((noinline))

TRACE_WRAPPER void *__wrap_malloc(size_t bytes)
{
   TRACE_CALLER;
   TRACE(PROSPERO_TRACE_ALLOC_BEGIN, 0, bytes, 1, 1);
   void *const result = prospero_impl_malloc(bytes);
   TRACE(PROSPERO_TRACE_ALLOC_END, result, bytes, 1, 1);
   return result;
}

TRACE_WRAPPER void *__wrap_calloc(size_t count, size_t bytes)
{
   TRACE_CALLER;
   TRACE(PROSPERO_TRACE_ALLOC_BEGIN, 0, bytes, count, 2);
   void *const result = prospero_impl_calloc(count, bytes);
   TRACE(PROSPERO_TRACE_ALLOC_END, result, bytes, count, 2);
   return result;
}

TRACE_WRAPPER void __wrap_free(void *pointer)
{
   TRACE_CALLER;
   const uintptr_t before = (uintptr_t)pointer;
   TRACE(PROSPERO_TRACE_FREE_BEGIN, before, 0, 0, 3);
   prospero_impl_free(pointer);
   TRACE(PROSPERO_TRACE_FREE_END, before, 0, 0, 3);
}

TRACE_WRAPPER size_t __wrap_malloc_usable_size(const void *pointer)
{
   TRACE_CALLER;
   TRACE(PROSPERO_TRACE_HEAP_DETAIL, pointer, 0, 0, 0x10000 | 4);
   const size_t result = prospero_impl_malloc_usable_size(pointer);
   TRACE(PROSPERO_TRACE_HEAP_DETAIL, pointer, result, 0, 0x20000 | 4);
   return result;
}

TRACE_WRAPPER void *__wrap_realloc(void *pointer, size_t bytes)
{
   TRACE_CALLER;
   const uintptr_t before = (uintptr_t)pointer;
   TRACE(PROSPERO_TRACE_REALLOC_BEGIN, before, bytes, 1, 5);
   void *const result = prospero_impl_realloc(pointer, bytes);
   TRACE(PROSPERO_TRACE_REALLOC_END, result, bytes, before, 5);
   return result;
}

TRACE_WRAPPER void *__wrap_reallocf(void *pointer, size_t bytes)
{
   TRACE_CALLER;
   const uintptr_t before = (uintptr_t)pointer;
   TRACE(PROSPERO_TRACE_REALLOC_BEGIN, before, bytes, 1, 6);
   void *const result = prospero_impl_reallocf(pointer, bytes);
   TRACE(PROSPERO_TRACE_REALLOC_END, result, bytes, before, 6);
   return result;
}

TRACE_WRAPPER void *__wrap_reallocarray(void *pointer, size_t count, size_t bytes)
{
   TRACE_CALLER;
   const uintptr_t before = (uintptr_t)pointer;
   TRACE(PROSPERO_TRACE_REALLOC_BEGIN, before, bytes, count, 7);
   void *const result = prospero_impl_reallocarray(pointer, count, bytes);
   TRACE(PROSPERO_TRACE_REALLOC_END, result, bytes, before, 7);
   return result;
}

TRACE_WRAPPER int __wrap_posix_memalign(void **out, size_t alignment, size_t bytes)
{
   TRACE_CALLER;
   TRACE(PROSPERO_TRACE_ALIGN_BEGIN, out, bytes, alignment, 8);
   const int result = prospero_impl_posix_memalign(out, alignment, bytes);
   // Read the output only after the underlying API successfully wrote it.
   TRACE(PROSPERO_TRACE_ALIGN_END, result == 0 ? *out : NULL, bytes, result, 8);
   return result;
}

TRACE_WRAPPER void *__wrap_aligned_alloc(size_t alignment, size_t bytes)
{
   TRACE_CALLER;
   TRACE(PROSPERO_TRACE_ALIGN_BEGIN, 0, bytes, alignment, 9);
   void *const result = prospero_impl_aligned_alloc(alignment, bytes);
   TRACE(PROSPERO_TRACE_ALIGN_END, result, bytes, result ? 0 : errno, 9);
   return result;
}

TRACE_WRAPPER void *__wrap_memalign(size_t alignment, size_t bytes)
{
   TRACE_CALLER;
   TRACE(PROSPERO_TRACE_ALIGN_BEGIN, 0, bytes, alignment, 10);
   void *const result = prospero_impl_memalign(alignment, bytes);
   TRACE(PROSPERO_TRACE_ALIGN_END, result, bytes, result ? 0 : errno, 10);
   return result;
}

TRACE_WRAPPER ssize_t __wrap_getdelim(char **line, size_t *capacity, int delimiter, FILE *stream)
{
   TRACE_CALLER;
   TRACE(PROSPERO_TRACE_HEAP_DETAIL, line, capacity, stream, 0x10000 | 11);
   const ssize_t result = prospero_impl_getdelim(line, capacity, delimiter, stream);
   TRACE(PROSPERO_TRACE_HEAP_DETAIL, line, result, stream, 0x20000 | 11);
   return result;
}

TRACE_WRAPPER ssize_t __wrap_getline(char **line, size_t *capacity, FILE *stream)
{
   TRACE_CALLER;
   TRACE(PROSPERO_TRACE_HEAP_DETAIL, line, capacity, stream, 0x10000 | 12);
   const ssize_t result = prospero_impl_getline(line, capacity, stream);
   TRACE(PROSPERO_TRACE_HEAP_DETAIL, line, result, stream, 0x20000 | 12);
   return result;
}
