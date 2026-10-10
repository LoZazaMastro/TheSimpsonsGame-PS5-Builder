/*
 * Local allocator mutex for The Simpsons Game Recompiled, 2026.
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Four-byte atomic ownership, as in Sonic's pinned dlmalloc configuration.
 * Contended waiters sleep after a bounded spin so a preempted owner at a
 * lower real-time priority can run. No allocator, pthread or emulated-TLS
 * calls are made here. Only the slow path accesses native errno.
 */
#ifndef SIMPSONS_PROSPERO_ALLOCATOR_LOCK_H
#define SIMPSONS_PROSPERO_ALLOCATOR_LOCK_H

#include <errno.h>
#include <stdint.h>
#include <time.h>

typedef uint32_t prospero_allocator_mutex;

_Static_assert(sizeof(prospero_allocator_mutex) == 4,
               "The allocator mutex must remain four bytes");
_Static_assert(__atomic_always_lock_free(sizeof(prospero_allocator_mutex), 0),
               "The allocator mutex must not call an out-of-line atomic runtime");

static inline int
prospero_allocator_lock_init(prospero_allocator_mutex *lock)
{
   __atomic_store_n(lock, 0, __ATOMIC_RELAXED);
   return 0;
}

static inline int
prospero_allocator_lock_destroy(prospero_allocator_mutex *lock)
{
   (void)lock;
   return 0;
}

/* dlmalloc expects TRY_LOCK to return one on acquisition, zero otherwise. */
static inline int
prospero_allocator_lock_try(prospero_allocator_mutex *lock)
{
   uint32_t expected = 0;
   return __atomic_compare_exchange_n(lock, &expected, 1, 0,
                                      __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

/* dlmalloc expects ACQUIRE_LOCK to return zero only with ownership held. */
static inline int
prospero_allocator_lock_acquire(prospero_allocator_mutex *lock)
{
   unsigned failures = 0;
   for (;;) {
      if (__atomic_load_n(lock, __ATOMIC_RELAXED) == 0 &&
          prospero_allocator_lock_try(lock))
         return 0;
      if (++failures == 64) {
         const struct timespec pause = {0, 50000};
         const int saved_errno = errno;
         /* EINTR and other sleep failures still lead back to acquisition.
          * Neither a failed sleep nor a wakeup grants lock ownership. */
         (void)nanosleep(&pause, NULL);
         errno = saved_errno;
         failures = 0;
      } else {
#if defined(__i386__) || defined(__x86_64__)
         __asm__ __volatile__("pause");
#endif
      }
   }
}

static inline void
prospero_allocator_lock_release(prospero_allocator_mutex *lock)
{
   __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
}

#define MLOCK_T prospero_allocator_mutex
#define INITIAL_LOCK(lock) prospero_allocator_lock_init(lock)
#define DESTROY_LOCK(lock) prospero_allocator_lock_destroy(lock)
#define ACQUIRE_LOCK(lock) prospero_allocator_lock_acquire(lock)
#define RELEASE_LOCK(lock) prospero_allocator_lock_release(lock)
#define TRY_LOCK(lock) prospero_allocator_lock_try(lock)

#endif
