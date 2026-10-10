/*
 * PS5 Platform - a thread's default stack (include/ps5platform/libc.h).
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A thread whose creator asks for no stack size runs on 64 KiB, where the main
 * thread has 2 MiB, and a thread created asking for 2 MiB gets it (docs/PROBE.md,
 * "Threads"). Libraries start their threads without asking -- Mesa's queues, the
 * Vulkan CTS, libc++'s std::thread -- and code with larger frames than a
 * sixteenth of a desktop's default overruns them.
 *
 * libkernel takes a thread's stack from flexible memory, about 400 MiB for the
 * whole title, which 256 threads of 2 MiB exceed (the Vulkan CTS's concurrent
 * image copies start that many, and pthread_create failed). So a consumer that
 * links with --wrap for pthread_create, pthread_join and pthread_detach gets
 * these instead: a thread whose attributes place no stack of their own and ask
 * for less than the main thread's gets the main thread's 2 MiB, in direct
 * memory, with an unmapped guard page below it. Everything else the caller set
 * is kept, and a thread that places its own stack or asks for 2 MiB or more is
 * created as asked.
 *
 * A stack is freed once its thread has ended: pthread_join frees it, and a
 * detached thread's is freed by a reaper thread that joins it once its start
 * routine has returned (or it called pthread_exit). Such threads are created
 * joinable, so the reaper can; for the caller they are detached as asked. A
 * few freed stacks are kept for the next threads.
 *
 * Every thread created through the wrap starts with its creator's MXCSR, as a
 * Linux thread does, so a title that set the IEEE state (ps5platform/fp.h)
 * keeps it in its threads.
 *
 * Local adaptation for The Simpsons Game Recompiled (2026-10-09).
 * Source: mihawk-99/PS5_PayloadSDK, commit
 * b5efad528e0ac1b8289f39d72a0e890ff68ff0a6, platform/src/threads.c.
 * Unlike the pinned implementation described above, requested stacks of
 * 2 MiB and larger also use direct memory. The caller's usable size is kept;
 * only the physical backing rounds up to 64 KiB. Explicit caller-owned stacks
 * are untouched. The cache matches backing and guard sizes and holds at most
 * 64 MiB / 32 stacks. A failed direct allocation falls back to the same usable
 * size in native pthread_create; it never silently shrinks a 4/16 MiB stack.
 */
#define _GNU_SOURCE 1
/* pthread_attr_getstackaddr is deprecated, and the one call that says what
 * this needs. */
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "ps5platform/libc.h"

#include "placement.h"
#include "ps5platform/kernel.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../../tools/rexglue-sdk/include/rex/platform/trace_phases_prospero.h"

int __real_pthread_create(pthread_t *thread, const pthread_attr_t *attributes,
                          void *(*start)(void *), void *argument);
int __real_pthread_join(pthread_t thread, void **value);
int __real_pthread_detach(pthread_t thread);

#define STACK_GUARD PS5P_PAGE
#define STACK_CACHE 32
#define STACK_CACHE_BYTES ((size_t)64 << 20)
#define STACK_PROTECTION (PS5_KERNEL_PROT_CPU_READ | PS5_KERNEL_PROT_CPU_WRITE)

struct thread_stack {
   /* The reservation: the guard page, then the stack. */
   void *base;
   int64_t direct_start;
   size_t bytes;
   size_t guard_bytes;
};

struct thread_record {
   struct thread_record *next;
   pthread_t thread;
   struct thread_stack stack;
   void *(*start)(void *);
   void *argument;
   uint32_t mxcsr;
   bool detached;
   bool finished;
   bool reap_failed;
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t reaper_wake = PTHREAD_COND_INITIALIZER;
static struct thread_record *records;
static struct thread_stack cache[STACK_CACHE];
static unsigned cached;
static size_t cached_bytes;
static unsigned live_stacks;
static bool reaper_started;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static pthread_key_t finish_key;
static int finish_key_status = EAGAIN;

/* ------------------------------------------------------------- the stacks */

static bool
stack_map(struct thread_stack *stack, size_t bytes, size_t guard_bytes)
{
   void *base = NULL;
   if (ps5p_reserve_placed(bytes + guard_bytes, 0, PS5P_DIRECT_UNIT, -1, &base) != 0)
      return false;
   int64_t start = -1;
   if (sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), bytes, PS5P_DIRECT_UNIT,
                                     PS5_KERNEL_DIRECT_TYPE_CPU, &start) != 0) {
      sceKernelMunmap(base, bytes + guard_bytes);
      return false;
   }
   void *const at = (char *)base + guard_bytes;
   void *mapped = at;
   const int result = sceKernelMapDirectMemory(&mapped, bytes, STACK_PROTECTION,
                                               PS5_KERNEL_MAP_FIXED, start, PS5P_DIRECT_UNIT);
   if (result != 0 || mapped != at) {
      if (result == 0 && mapped != at)
         sceKernelMunmap(mapped, bytes);
      sceKernelReleaseDirectMemory(start, bytes);
      sceKernelMunmap(base, bytes + guard_bytes);
      return false;
   }
   stack->base = base;
   stack->direct_start = start;
   stack->bytes = bytes;
   stack->guard_bytes = guard_bytes;
   return true;
}

static void
stack_unmap(const struct thread_stack *stack)
{
   sceKernelMunmap(stack->base, stack->bytes + stack->guard_bytes);
   sceKernelReleaseDirectMemory(stack->direct_start, stack->bytes);
}

/* Called with the lock held. */
static bool
stack_take(struct thread_stack *stack, size_t bytes, size_t guard_bytes)
{
   for (unsigned i = 0; i < cached; ++i) {
      if (cache[i].bytes == bytes && cache[i].guard_bytes == guard_bytes) {
         *stack = cache[i];
         cache[i] = cache[--cached];
         cached_bytes -= stack->bytes;
         live_stacks++;
         return true;
      }
   }
   if (!stack_map(stack, bytes, guard_bytes))
      return false;
   live_stacks++;
   return true;
}

/* Called with the lock held. */
static void
stack_give(const struct thread_stack *stack)
{
   live_stacks--;
   if (cached < STACK_CACHE && stack->bytes <= STACK_CACHE_BYTES - cached_bytes &&
       stack->guard_bytes <= STACK_CACHE_BYTES) {
      cache[cached++] = *stack;
      cached_bytes += stack->bytes;
   } else {
      stack_unmap(stack);
   }
}

/* ------------------------------------------------------------ the records */

/* Called with the lock held; unlinks the record it returns. */
static struct thread_record *
record_take(struct thread_record *expected)
{
   for (struct thread_record **at = &records; *at; at = &(*at)->next) {
      if (*at == expected) {
         struct thread_record *const record = *at;
         *at = record->next;
         return record;
      }
   }
   return NULL;
}

static void *
reaper(void *unused)
{
   (void)unused;
   pthread_mutex_lock(&lock);
   for (;;) {
      struct thread_record *found = NULL;
      for (struct thread_record **at = &records; *at; at = &(*at)->next) {
         if ((*at)->detached && (*at)->finished && !(*at)->reap_failed) {
            found = *at;
            *at = found->next;
            break;
         }
      }
      if (!found) {
         pthread_cond_wait(&reaper_wake, &lock);
         continue;
      }
      pthread_mutex_unlock(&lock);
      const int result = __real_pthread_join(found->thread, NULL);
      pthread_mutex_lock(&lock);
      if (result == 0) {
         stack_give(&found->stack);
         free(found);
      } else {
         /* A failed join is not evidence that the stack is unused. Retain it
          * rather than unmapping a possibly running thread or retrying in a
          * tight loop. This exceptional record stays counted as live. */
         found->reap_failed = true;
         found->next = records;
         records = found;
      }
   }
   return NULL;
}

/* Called with the lock held, once a detached thread is known. */
static int
reaper_ensure(void)
{
   if (reaper_started)
      return 0;
   pthread_t thread;
   pthread_attr_t attributes;
   int result = pthread_attr_init(&attributes);
   if (result != 0)
      return result;
   result = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
   if (result == 0)
      result = __real_pthread_create(&thread, &attributes, reaper, NULL);
   if (result == 0)
      reaper_started = true;
   pthread_attr_destroy(&attributes);
   return result;
}

/* The thread's start routine returned or it called pthread_exit: its record
 * is finished, and a detached one is the reaper's to join. */
static void
thread_finished(void *opaque)
{
   struct thread_record *const record = opaque;
   prospero_trace_checkpoint(PROSPERO_PHASE_HOST_THREAD_FINISH,
                            (uintptr_t)record->start, (uintptr_t)record->stack.base,
                            record->stack.bytes);
   prospero_trace_unregister_stack();
   pthread_mutex_lock(&lock);
   record->finished = true;
   if (record->detached)
      pthread_cond_signal(&reaper_wake);
   pthread_mutex_unlock(&lock);
}

static void
key_create(void)
{
   finish_key_status = pthread_key_create(&finish_key, thread_finished);
}

static uint32_t
mxcsr_read(void)
{
   uint32_t mxcsr;
   __asm__ volatile("stmxcsr %0" : "=m"(mxcsr));
   return mxcsr;
}

static void
mxcsr_write(uint32_t mxcsr)
{
   __asm__ volatile("ldmxcsr %0" : : "m"(mxcsr));
}

/* src/cxa.c: the calling thread's C++ thread_local destructors, now. */
void ps5p_run_thread_destructors(void);

static void *
thread_start(void *opaque)
{
   struct thread_record *const record = opaque;
   mxcsr_write(record->mxcsr);
   prospero_trace_register_stack((uintptr_t)record->stack.base + record->stack.guard_bytes,
                                record->stack.bytes);
   prospero_trace_checkpoint(PROSPERO_PHASE_HOST_THREAD_START, (uintptr_t)record->start,
                            (uintptr_t)record->stack.base + record->stack.guard_bytes,
                            record->stack.bytes);
   if (pthread_setspecific(finish_key, record) != 0) {
      /* Starting user work without its exit record would make detached
       * cleanup unreliable. This failure cannot be returned synchronously
       * through pthread_create once the native thread has started. */
      abort();
   }
   void *const result = record->start(record->argument);
   prospero_trace_checkpoint(PROSPERO_PHASE_HOST_THREAD_RETURN, (uintptr_t)record->start,
                            (uintptr_t)result, 0);
   /* Its thread_local destructors, while its thread-local storage is whole
    * (src/cxa.c). */
   ps5p_run_thread_destructors();
   return result;
}

/* A thread on a stack of its own (or libkernel's): only the creator's MXCSR
 * travels with it. */
struct plain_start {
   void *(*start)(void *);
   void *argument;
   uint32_t mxcsr;
};

static void *
plain_thread_start(void *opaque)
{
   const struct plain_start begin = *(const struct plain_start *)opaque;
   free(opaque);
   mxcsr_write(begin.mxcsr);
   // Native/caller-owned stack bounds are unknown here. Do not guess them.
   prospero_trace_checkpoint(PROSPERO_PHASE_HOST_THREAD_START, (uintptr_t)begin.start, 0, 0);
   void *const result = begin.start(begin.argument);
   prospero_trace_checkpoint(PROSPERO_PHASE_HOST_THREAD_RETURN, (uintptr_t)begin.start,
                            (uintptr_t)result, 0);
   ps5p_run_thread_destructors();
   return result;
}

static int
create_plain(pthread_t *thread, const pthread_attr_t *attributes, void *(*start)(void *), void *argument)
{
   struct plain_start *const begin = malloc(sizeof(*begin));
   if (!begin)
      return EAGAIN;
   begin->start = start;
   begin->argument = argument;
   begin->mxcsr = mxcsr_read();
   const int result = __real_pthread_create(thread, attributes, plain_thread_start, begin);
   if (result != 0)
      free(begin);
   return result;
}

/* -------------------------------------------------------------- the wraps */

/* Clone the attributes without taking ownership of an explicit caller stack.
 * The usable size is distinct from rounded direct-memory backing. Any failed
 * attribute query/copy leaves the original attributes for the native path. */
static bool
stack_attributes(const pthread_attr_t *attributes, pthread_attr_t *copy, bool *detached,
                  size_t *usable_bytes, size_t *bytes, size_t *guard_bytes)
{
   *detached = false;
   *usable_bytes = PS5_THREAD_STACK_BYTES;
   if (attributes) {
      /* pthread_attr_getstackaddr, not getstack: glibc's getstack derives an
       * address even for attributes that place no stack. */
      void *address = NULL;
      size_t size = 0;
      if (pthread_attr_getstackaddr(attributes, &address) != 0 || address != NULL)
         return false;
      if (pthread_attr_getstacksize(attributes, &size) != 0)
         return false;
      if (size > *usable_bytes)
         *usable_bytes = size;
   }
   if (*usable_bytes > SIZE_MAX - (PS5P_DIRECT_UNIT - 1))
      return false;
   *bytes = ps5p_round_up(*usable_bytes, PS5P_DIRECT_UNIT);
   if (pthread_attr_init(copy) != 0)
      return false;
   size_t guard_size;
   if (pthread_attr_getguardsize(attributes ? attributes : copy, &guard_size) != 0 ||
       guard_size > SIZE_MAX - (PS5P_PAGE - 1))
      goto failed;
   *guard_bytes = ps5p_round_up(guard_size > STACK_GUARD ? guard_size : STACK_GUARD, PS5P_PAGE);
   if (*bytes > (size_t)INT64_MAX || *guard_bytes > (size_t)INT64_MAX - *bytes)
      goto failed;
   if (attributes) {
      int value;
      struct sched_param parameter;
      if (pthread_attr_getdetachstate(attributes, &value) != 0)
         goto failed;
      *detached = value == PTHREAD_CREATE_DETACHED;
      if (pthread_attr_getinheritsched(attributes, &value) != 0 ||
          pthread_attr_setinheritsched(copy, value) != 0 ||
          pthread_attr_getschedpolicy(attributes, &value) != 0 ||
          pthread_attr_setschedpolicy(copy, value) != 0 ||
          pthread_attr_getschedparam(attributes, &parameter) != 0 ||
          pthread_attr_setschedparam(copy, &parameter) != 0 ||
          pthread_attr_getscope(attributes, &value) != 0 ||
          pthread_attr_setscope(copy, value) != 0 ||
          pthread_attr_setguardsize(copy, guard_size) != 0)
         goto failed;
   }
   if (pthread_attr_setdetachstate(copy, PTHREAD_CREATE_JOINABLE) != 0)
      goto failed;
   return true;

failed:
   pthread_attr_destroy(copy);
   return false;
}

int
__wrap_pthread_create(pthread_t *thread, const pthread_attr_t *attributes, void *(*start)(void *),
                      void *argument)
{
   pthread_attr_t copy;
   bool detached;
   size_t usable_bytes, bytes, guard_bytes;
   if (!stack_attributes(attributes, &copy, &detached, &usable_bytes, &bytes, &guard_bytes))
      return create_plain(thread, attributes, start, argument);
   int result = pthread_once(&key_once, key_create);
   if (result == 0)
      result = finish_key_status;
   if (result != 0) {
      pthread_attr_destroy(&copy);
      return result;
   }

   struct thread_record *const record = calloc(1, sizeof(*record));
   if (!record) {
      pthread_attr_destroy(&copy);
      return EAGAIN;
   }
   record->start = start;
   record->argument = argument;
   record->mxcsr = mxcsr_read();
   record->detached = detached;

   pthread_mutex_lock(&lock);
   const bool have_stack = stack_take(&record->stack, bytes, guard_bytes);
   pthread_mutex_unlock(&lock);
   if (!have_stack) {
      /* No direct memory: preserve the usable size, including requests larger
       * than the platform default. Never turn a 16 MiB request into 2 MiB. */
      free(record);
      result = pthread_attr_setdetachstate(
          &copy, detached ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE);
      if (result == 0)
         result = pthread_attr_setstacksize(&copy, usable_bytes);
      if (result == 0)
         result = create_plain(thread, &copy, start, argument);
      pthread_attr_destroy(&copy);
      return result;
   }
   result = pthread_attr_setstack(&copy, (char *)record->stack.base + guard_bytes, usable_bytes);

   /* The record is listed before the thread runs, so its end finds it. */
   pthread_mutex_lock(&lock);
   if (result == 0 && detached)
      result = reaper_ensure();
   if (result == 0)
      result = __real_pthread_create(&record->thread, &copy, thread_start, record);
   if (result == 0) {
      record->next = records;
      records = record;
      *thread = record->thread;
   } else {
      stack_give(&record->stack);
   }
   pthread_mutex_unlock(&lock);
   pthread_attr_destroy(&copy);
   if (result != 0)
      free(record);
   return result;
}

int
__wrap_pthread_join(pthread_t thread, void **value)
{
   pthread_mutex_lock(&lock);
   struct thread_record *ours = NULL;
   for (struct thread_record *record = records; record; record = record->next) {
      if (pthread_equal(record->thread, thread)) {
         /* Internally joinable for the reaper, externally still detached. */
         if (record->detached) {
            pthread_mutex_unlock(&lock);
            return EINVAL;
         }
         ours = record;
         break;
      }
   }
   pthread_mutex_unlock(&lock);
   const int result = __real_pthread_join(thread, value);
   if (result == 0 && ours) {
      pthread_mutex_lock(&lock);
      /* pthread_t may be recycled by a new native creation after join has
       * returned. Only the original record belongs to this join. */
      struct thread_record *const record = record_take(ours);
      if (record)
         stack_give(&record->stack);
      pthread_mutex_unlock(&lock);
      free(record);
   }
   return result;
}

int
__wrap_pthread_detach(pthread_t thread)
{
   pthread_mutex_lock(&lock);
   for (struct thread_record *record = records; record; record = record->next) {
      if (pthread_equal(record->thread, thread)) {
         const int result = record->detached ? EINVAL : reaper_ensure();
         if (result == 0) {
            record->detached = true;
            if (record->finished)
               pthread_cond_signal(&reaper_wake);
         }
         pthread_mutex_unlock(&lock);
         return result;
      }
   }
   pthread_mutex_unlock(&lock);
   return __real_pthread_detach(thread);
}

void
ps5_thread_stacks(unsigned *live, unsigned *cached_stacks)
{
   pthread_mutex_lock(&lock);
   *live = live_stacks;
   *cached_stacks = cached;
   pthread_mutex_unlock(&lock);
}

void
ps5_pthread_exit(void *value)
{
   /* The thread_local destructors first, as its start routine returning
    * would run them (src/cxa.c). */
   ps5p_run_thread_destructors();
   pthread_exit(value);
}
