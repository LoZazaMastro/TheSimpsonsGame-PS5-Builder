/* Host checks for the actual PS5 thread wrapper and the pinned SDK kernel model.
 * SPDX-License-Identifier: GPL-3.0-or-later
 * These are Linux pthreads on real mapped stacks, not a PS5 ABI/hardware test.
 */
#define _GNU_SOURCE 1
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <ps5platform/kernel.h>
#include <ps5platform/libc.h>
#include "host_kernel.h"

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define MIB ((size_t)1 << 20)
#define PAGE ((size_t)0x4000)
#define UNIT ((size_t)0x10000)
#define CHECK(value) do { ++checks; if (!(value)) { \
  fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #value); exit(1); \
} } while (0)

static unsigned checks;
static pthread_mutex_t observed_lock = PTHREAD_MUTEX_INITIALIZER;
struct allocation { int64_t offset; size_t bytes; void *address; bool live; };
static struct allocation allocations[128];
static unsigned allocation_calls, release_calls;
static size_t live_bytes;
static int bad_release;
static atomic_int create_calls, fail_create, fail_join, fail_key, fail_setstack, fail_specific;
static atomic_int inject_reused_id, reused_id_active, freed_reused_record;
static bool capture_reused_record;
static void *reused_record;
static pthread_t previous_id, replacement_id;
static size_t last_native_size, last_native_guard;
static int last_native_detach, last_native_inherit, last_native_policy, last_native_scope;
static bool last_native_explicit;

int __real_pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
int __real_pthread_join(pthread_t, void **);
int __real_pthread_detach(pthread_t);
static void inject_replacement_after_join(pthread_t joined);

int observed_pthread_equal(pthread_t first, pthread_t second) {
  if (atomic_load(&reused_id_active) &&
      ((pthread_equal(first, previous_id) && pthread_equal(second, replacement_id)) ||
       (pthread_equal(first, replacement_id) && pthread_equal(second, previous_id)))) return 1;
  return pthread_equal(first, second);
}

void observed_free(void *pointer) {
  if (atomic_load(&reused_id_active) && pointer == reused_record)
    atomic_store(&freed_reused_record, 1);
  free(pointer);
}

int observed_pthread_create(pthread_t *thread, const pthread_attr_t *attributes,
                            void *(*start)(void *), void *argument) {
  atomic_fetch_add(&create_calls, 1);
  if (capture_reused_record) reused_record = argument;
  if (attributes) {
    void *address = NULL;
    pthread_attr_getstackaddr(attributes, &address);
    pthread_attr_getstacksize(attributes, &last_native_size);
    pthread_attr_getguardsize(attributes, &last_native_guard);
    pthread_attr_getdetachstate(attributes, &last_native_detach);
    pthread_attr_getinheritsched(attributes, &last_native_inherit);
    pthread_attr_getschedpolicy(attributes, &last_native_policy);
    pthread_attr_getscope(attributes, &last_native_scope);
    last_native_explicit = address != NULL;
  }
  if (atomic_exchange(&fail_create, 0)) return EAGAIN;
  return __real_pthread_create(thread, attributes, start, argument);
}

int observed_pthread_join(pthread_t thread, void **value) {
  if (atomic_exchange(&fail_join, 0)) return EINVAL;
  const int result = __real_pthread_join(thread, value);
  if (!result && atomic_exchange(&inject_reused_id, 0)) inject_replacement_after_join(thread);
  return result;
}

int observed_pthread_detach(pthread_t thread) { return __real_pthread_detach(thread); }

int observed_key_create(pthread_key_t *key, void (*destructor)(void *)) {
  if (atomic_exchange(&fail_key, 0)) return EAGAIN;
  return pthread_key_create(key, destructor);
}

int observed_attr_setstack(pthread_attr_t *attr, void *address, size_t size) {
  if (atomic_exchange(&fail_setstack, 0)) return EINVAL;
  return pthread_attr_setstack(attr, address, size);
}

int observed_setspecific(pthread_key_t key, const void *value) {
  if (atomic_exchange(&fail_specific, 0)) return ENOMEM;
  return pthread_setspecific(key, value);
}

int32_t observed_allocate(int64_t first, int64_t last, size_t bytes, size_t alignment,
                          int type, int64_t *offset) {
  int32_t result = sceKernelAllocateDirectMemory(first, last, bytes, alignment, type, offset);
  if (result) return result;
  pthread_mutex_lock(&observed_lock);
  ++allocation_calls;
  for (size_t i = 0; i < 128; ++i) {
    if (!allocations[i].live) {
      allocations[i] = (struct allocation){*offset, bytes, NULL, true};
      live_bytes += bytes;
      pthread_mutex_unlock(&observed_lock);
      return 0;
    }
  }
  abort();
}

int32_t observed_map(void **address, size_t bytes, int protection, int flags,
                     int64_t offset, size_t alignment) {
  int32_t result = sceKernelMapDirectMemory(address, bytes, protection, flags, offset, alignment);
  if (result) return result;
  pthread_mutex_lock(&observed_lock);
  for (size_t i = 0; i < 128; ++i)
    if (allocations[i].live && allocations[i].offset == offset)
      allocations[i].address = *address;
  pthread_mutex_unlock(&observed_lock);
  return 0;
}

int32_t observed_release(int64_t offset, size_t bytes) {
  pthread_mutex_lock(&observed_lock);
  for (size_t i = 0; i < 128; ++i) {
    if (allocations[i].live && allocations[i].offset == offset) {
      if (allocations[i].bytes != bytes) bad_release = 1;
      const int32_t result = sceKernelReleaseDirectMemory(offset, bytes);
      if (!result) {
        ++release_calls;
        live_bytes -= allocations[i].bytes;
        allocations[i].live = false;
      }
      pthread_mutex_unlock(&observed_lock);
      return result;
    }
  }
  bad_release = 1;
  pthread_mutex_unlock(&observed_lock);
  return EINVAL;
}

static size_t direct_bytes(void) {
  pthread_mutex_lock(&observed_lock);
  const size_t result = live_bytes;
  pthread_mutex_unlock(&observed_lock);
  return result;
}

static bool is_direct(void *address) {
  bool result = false;
  pthread_mutex_lock(&observed_lock);
  for (size_t i = 0; i < 128; ++i)
    if (allocations[i].live && allocations[i].address == address) result = true;
  pthread_mutex_unlock(&observed_lock);
  return result;
}

static uint32_t mxcsr_read(void) { uint32_t result; __asm__("stmxcsr %0" : "=m"(result)); return result; }
static void mxcsr_write(uint32_t value) { __asm__("ldmxcsr %0" : : "m"(value)); }

static double now_seconds(void) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now.tv_sec + now.tv_nsec / 1e9;
}

static void wait_value(atomic_int *value, int expected) {
  const double deadline = now_seconds() + 5;
  while (atomic_load_explicit(value, memory_order_acquire) != expected) {
    CHECK(now_seconds() < deadline);
    usleep(1000);
  }
}

static void wait_live(unsigned expected) {
  const double deadline = now_seconds() + 5;
  for (;;) {
    unsigned live, cached;
    ps5_thread_stacks(&live, &cached);
    if (live == expected) return;
    CHECK(now_seconds() < deadline);
    usleep(1000);
  }
}

struct job {
  atomic_int ready, release, destructor_count;
  size_t stack_size;
  void *stack_base;
  bool direct, deep, exit_explicit, fault_guard, destructors;
  uint32_t mxcsr;
  int destructor_order[2];
};

static void destructor_one(void *opaque) {
  struct job *job = opaque;
  int index = atomic_fetch_add(&job->destructor_count, 1);
  if (index < 2) job->destructor_order[index] = 1;
}

static void destructor_two(void *opaque) {
  struct job *job = opaque;
  int index = atomic_fetch_add(&job->destructor_count, 1);
  if (index < 2) job->destructor_order[index] = 2;
}

__attribute__((noinline)) static void use_deep_stack(void) {
  volatile unsigned char frame[3 * MIB];
  for (size_t i = 0; i < sizeof(frame); i += 4096) frame[i] = (unsigned char)(i / 4096);
  unsigned sum = 0;
  for (size_t i = 0; i < sizeof(frame); i += 4096) sum += frame[i];
  if (sum != 3 * (255 * 256 / 2)) abort();
}

static void *worker(void *opaque) {
  struct job *job = opaque;
  job->mxcsr = mxcsr_read();
  pthread_attr_t running;
  if (pthread_getattr_np(pthread_self(), &running) != 0) abort();
  if (pthread_attr_getstack(&running, &job->stack_base, &job->stack_size) != 0) abort();
  pthread_attr_destroy(&running);
  job->direct = is_direct(job->stack_base);
  if (job->fault_guard) *(volatile char *)((char *)job->stack_base - 1) = 1;
  if (job->deep) use_deep_stack();
  if (job->destructors &&
      (ps5___cxa_thread_atexit_impl(destructor_one, job, NULL) != 0 ||
       ps5___cxa_thread_atexit_impl(destructor_two, job, NULL) != 0)) abort();
  atomic_store_explicit(&job->ready, 1, memory_order_release);
  while (!atomic_load_explicit(&job->release, memory_order_acquire)) usleep(1000);
  if (job->exit_explicit) ps5_pthread_exit((void *)(uintptr_t)0x1234);
  return (void *)(uintptr_t)0x1234;
}

static pthread_t start_job(struct job *job, size_t bytes, bool detached) {
  pthread_attr_t attrs;
  CHECK(pthread_attr_init(&attrs) == 0);
  CHECK(pthread_attr_setstacksize(&attrs, bytes) == 0);
  CHECK(pthread_attr_setdetachstate(&attrs, detached ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE) == 0);
  pthread_t thread;
  CHECK(pthread_create(&thread, &attrs, worker, job) == 0);
  pthread_attr_destroy(&attrs);
  wait_value(&job->ready, 1);
  return thread;
}

static void finish_job(pthread_t thread, struct job *job) {
  atomic_store_explicit(&job->release, 1, memory_order_release);
  void *result = NULL;
  CHECK(pthread_join(thread, &result) == 0);
  CHECK(result == (void *)(uintptr_t)0x1234);
}

static struct job replacement_job;

static void inject_replacement_after_join(pthread_t joined) {
  previous_id = joined;
  capture_reused_record = true;
  /* A 64 KiB request exercises direct records in the unmodified SDK too. */
  replacement_id = start_job(&replacement_job, 65536, false);
  capture_reused_record = false;
  atomic_store(&reused_id_active, 1);
}

static void test_reused_id(void) {
  struct job first = {0};
  pthread_t thread = start_job(&first, 65536, false);
  atomic_store(&inject_reused_id, 1);
  finish_job(thread, &first);
  CHECK(replacement_job.direct && reused_record != NULL);
  CHECK(!atomic_load(&freed_reused_record));
  atomic_store(&reused_id_active, 0);
  finish_job(replacement_id, &replacement_job);
  wait_live(0);
  CHECK(!bad_release);
}

static void test_sizes(bool baseline) {
  const size_t sizes[] = {2 * MIB, 4 * MIB, 16 * MIB};
  mxcsr_write(0x9fc0);
  for (unsigned i = 0; i < 3; ++i) {
    struct job job = {.deep = sizes[i] > 2 * MIB, .destructors = true, .exit_explicit = i == 2};
    pthread_t thread = start_job(&job, sizes[i], false);
    CHECK(job.stack_size == sizes[i]);
    CHECK(job.direct == !baseline);
    CHECK((job.mxcsr & ~0x3fu) == 0x9fc0); /* Polling may set FP exception flags. */
    if (!baseline) CHECK(direct_bytes() >= sizes[i]);
    finish_job(thread, &job);
    CHECK(atomic_load(&job.destructor_count) == 2);
    CHECK(job.destructor_order[0] == 2 && job.destructor_order[1] == 1);
    wait_live(0);
  }
  CHECK(!bad_release);
  if (baseline) CHECK(allocation_calls == 0 && host_direct_allocations() == 0);
}

static void test_reuse(void) {
  const size_t sizes[] = {2 * MIB, 4 * MIB, 16 * MIB, 4 * MIB, 2 * MIB, 16 * MIB};
  void *bases[3] = {0};
  for (unsigned i = 0; i < 6; ++i) {
    struct job job = {0};
    pthread_t thread = start_job(&job, sizes[i], false);
    CHECK(job.direct && job.stack_size == sizes[i]);
    unsigned slot = sizes[i] == 2 * MIB ? 0 : sizes[i] == 4 * MIB ? 1 : 2;
    if (bases[slot]) CHECK(job.stack_base == bases[slot]);
    bases[slot] = job.stack_base;
    finish_job(thread, &job);
  }
  CHECK(allocation_calls == 3 && direct_bytes() == 22 * MIB);
}

static void test_budget(bool count_limit) {
  const unsigned count = count_limit ? 40 : 6;
  const size_t size = count_limit ? 2 * MIB : 16 * MIB;
  struct job jobs[40] = {0};
  pthread_t threads[40];
  for (unsigned i = 0; i < count; ++i) threads[i] = start_job(&jobs[i], size, false);
  CHECK(direct_bytes() == count * size);
  unsigned live, cached;
  ps5_thread_stacks(&live, &cached);
  CHECK(live == count);
  for (unsigned i = 0; i < count; ++i) atomic_store(&jobs[i].release, 1);
  for (unsigned i = 0; i < count; ++i) finish_job(threads[i], &jobs[i]);
  ps5_thread_stacks(&live, &cached);
  CHECK(live == 0 && cached <= 32);
  CHECK(direct_bytes() == 64 * MIB);
  CHECK(cached == (count_limit ? 32 : 4));
  CHECK(release_calls == count - cached && !bad_release);
}

static void test_detached(void) {
  struct job first = {.destructors = true, .exit_explicit = true};
  pthread_t thread = start_job(&first, 16 * MIB, true);
  CHECK(first.direct);
  CHECK(pthread_join(thread, NULL) == EINVAL);
  CHECK(pthread_detach(thread) == EINVAL);
  atomic_store(&first.release, 1);
  wait_live(0);
  CHECK(atomic_load(&first.destructor_count) == 2);
  struct job second = {0};
  thread = start_job(&second, 4 * MIB, false);
  CHECK(pthread_detach(thread) == 0);
  atomic_store(&second.release, 1);
  wait_live(0);
  CHECK(!bad_release);
}

static void test_explicit_stack(void) {
  const size_t size = 4 * MIB, guard = 2 * PAGE;
  void *area = mmap(NULL, size + guard, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  CHECK(area != MAP_FAILED);
  void *stack = (char *)area + guard;
  CHECK(mprotect(stack, size, PROT_READ | PROT_WRITE) == 0);
  pthread_attr_t attr;
  CHECK(pthread_attr_init(&attr) == 0);
  CHECK(pthread_attr_setstack(&attr, stack, size) == 0);
  CHECK(pthread_attr_setguardsize(&attr, guard) == 0);
  CHECK(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0);
  CHECK(pthread_attr_setschedpolicy(&attr, SCHED_OTHER) == 0);
  CHECK(pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM) == 0);
  struct job job = {.deep = true, .destructors = true};
  pthread_t thread;
  CHECK(pthread_create(&thread, &attr, worker, &job) == 0);
  pthread_attr_destroy(&attr);
  wait_value(&job.ready, 1);
  CHECK(job.stack_base == stack && job.stack_size == size && !job.direct);
  CHECK(last_native_explicit && last_native_size == size && last_native_guard == guard);
  CHECK(last_native_inherit == PTHREAD_EXPLICIT_SCHED && last_native_policy == SCHED_OTHER);
  CHECK(last_native_scope == PTHREAD_SCOPE_SYSTEM);
  finish_job(thread, &job);
  CHECK(atomic_load(&job.destructor_count) == 2 && allocation_calls == 0);
  CHECK(munmap(area, size + guard) == 0);
}

static void test_attributes(void) {
  pthread_attr_t attr;
  CHECK(pthread_attr_init(&attr) == 0);
  const size_t size = 3 * MIB + 0x1230;
  CHECK(pthread_attr_setstacksize(&attr, size) == 0);
  CHECK(pthread_attr_setguardsize(&attr, 3 * PAGE) == 0);
  CHECK(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0);
  CHECK(pthread_attr_setschedpolicy(&attr, SCHED_OTHER) == 0);
  struct sched_param parameter = {.sched_priority = 0};
  CHECK(pthread_attr_setschedparam(&attr, &parameter) == 0);
  CHECK(pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM) == 0);
  struct job job = {0};
  pthread_t thread;
  CHECK(pthread_create(&thread, &attr, worker, &job) == 0);
  wait_value(&job.ready, 1);
  CHECK(job.direct && job.stack_size == size);
  CHECK(last_native_size == size && last_native_guard == 3 * PAGE);
  CHECK(last_native_inherit == PTHREAD_EXPLICIT_SCHED && last_native_policy == SCHED_OTHER);
  CHECK(last_native_scope == PTHREAD_SCOPE_SYSTEM);
  CHECK(direct_bytes() == ((size + UNIT - 1) / UNIT) * UNIT);
  void *untouched_address = (void *)1;
  size_t untouched_size;
  CHECK(pthread_attr_getstackaddr(&attr, &untouched_address) == 0 && !untouched_address);
  CHECK(pthread_attr_getstacksize(&attr, &untouched_size) == 0 && untouched_size == size);
  finish_job(thread, &job);
  pthread_attr_destroy(&attr);
}

static void test_failure(const char *mode) {
  pthread_attr_t attr;
  CHECK(pthread_attr_init(&attr) == 0);
  CHECK(pthread_attr_setstacksize(&attr, 16 * MIB) == 0);
  struct job job = {.release = 1};
  pthread_t thread;
  bool fallback = false;
  int expected = 0;
  if (!strcmp(mode, "map-fail")) { host_fail(HOST_CALL_MAP, 1); fallback = true; }
  if (!strcmp(mode, "alloc-fail")) { host_fail(HOST_CALL_ALLOCATE, 1); fallback = true; }
  if (!strcmp(mode, "reserve-fail")) { host_fail(HOST_CALL_RESERVE, -1); fallback = true; }
  if (!strcmp(mode, "create-fail")) { atomic_store(&fail_create, 1); expected = EAGAIN; }
  if (!strcmp(mode, "attr-fail")) { atomic_store(&fail_setstack, 1); expected = EINVAL; }
  if (!strcmp(mode, "key-fail")) { atomic_store(&fail_key, 1); expected = EAGAIN; }
  if (!strcmp(mode, "fallback-fail")) {
    host_fail(HOST_CALL_ALLOCATE, 1); atomic_store(&fail_create, 1); expected = EAGAIN;
  }
  const int result = pthread_create(&thread, &attr, worker, &job);
  CHECK(result == expected);
  if (fallback) {
    CHECK(pthread_join(thread, NULL) == 0);
    CHECK(!job.direct && job.stack_size == 16 * MIB);
    CHECK(last_native_size == 16 * MIB && !last_native_explicit);
    CHECK(direct_bytes() == 0 && host_direct_allocations() == 0);
  }
  if (expected) {
    CHECK(!atomic_load(&job.ready));
    wait_live(0);
    if (!strcmp(mode, "key-fail") || !strcmp(mode, "attr-fail"))
      CHECK(atomic_load(&create_calls) == 0);
  }
  if (!strcmp(mode, "fallback-fail")) CHECK(last_native_size == 16 * MIB);
  CHECK(!bad_release);
  pthread_attr_destroy(&attr);
}

static void test_reaper_failure(bool create_detached) {
  struct job job = {0};
  pthread_attr_t attr;
  CHECK(pthread_attr_init(&attr) == 0);
  CHECK(pthread_attr_setstacksize(&attr, 4 * MIB) == 0);
  pthread_t thread;
  if (create_detached) {
    CHECK(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);
    atomic_store(&fail_create, 1);
    CHECK(pthread_create(&thread, &attr, worker, &job) == EAGAIN);
    CHECK(!atomic_load(&job.ready));
    wait_live(0);
    CHECK(atomic_load(&create_calls) == 1);
  } else {
    CHECK(pthread_create(&thread, &attr, worker, &job) == 0);
    wait_value(&job.ready, 1);
    atomic_store(&fail_create, 1);
    CHECK(pthread_detach(thread) == EAGAIN);
    finish_job(thread, &job); /* Failed detach kept the thread joinable. */
    wait_live(0);
  }
  pthread_attr_destroy(&attr);
}

static void test_reaper_join_failure(void) {
  struct job job = {0};
  pthread_t thread = start_job(&job, 4 * MIB, true);
  (void)thread;
  atomic_store(&fail_join, 1);
  atomic_store(&job.release, 1);
  wait_value(&fail_join, 0);
  unsigned live, cached;
  ps5_thread_stacks(&live, &cached);
  CHECK(live == 1 && cached == 0 && direct_bytes() == 4 * MIB);
  CHECK(release_calls == 0); /* A failed join never unmaps the stack. */
}

static void test_overflow(void) {
  pthread_attr_t attr;
  CHECK(pthread_attr_init(&attr) == 0);
  CHECK(pthread_attr_setstacksize(&attr, SIZE_MAX) == 0);
  struct job job = {0};
  pthread_t thread;
  atomic_store(&fail_create, 1);
  CHECK(pthread_create(&thread, &attr, worker, &job) == EAGAIN);
  CHECK(last_native_size == SIZE_MAX && allocation_calls == 0);
  pthread_attr_destroy(&attr);
}

int main(int argc, char **argv) {
  if (argc != 2) return 2;
  const char *mode = argv[1];
  if (!strcmp(mode, "baseline")) test_sizes(true);
  else if (!strcmp(mode, "sizes")) test_sizes(false);
  else if (!strcmp(mode, "reuse")) test_reuse();
  else if (!strcmp(mode, "cache-count")) test_budget(true);
  else if (!strcmp(mode, "cache-bytes")) test_budget(false);
  else if (!strcmp(mode, "detached")) test_detached();
  else if (!strcmp(mode, "explicit-stack")) test_explicit_stack();
  else if (!strcmp(mode, "attributes")) test_attributes();
  else if (!strcmp(mode, "reaper-fail")) test_reaper_failure(true);
  else if (!strcmp(mode, "detach-fail")) test_reaper_failure(false);
  else if (!strcmp(mode, "reaper-join-fail")) test_reaper_join_failure();
  else if (!strcmp(mode, "overflow")) test_overflow();
  else if (!strcmp(mode, "id-reuse")) test_reused_id();
  else if (!strcmp(mode, "guard-fault") || !strcmp(mode, "specific-fail")) {
    struct job job = {.fault_guard = !strcmp(mode, "guard-fault")};
    if (!strcmp(mode, "specific-fail")) atomic_store(&fail_specific, 1);
    start_job(&job, 4 * MIB, false);
    return 3; /* The process must have terminated with the expected signal. */
  } else if (strstr(mode, "-fail")) test_failure(mode);
  else return 2;
  CHECK(!bad_release);
  printf("PASS %-19s %u checks (Linux pthreads + pinned SDK host kernel)\n", mode, checks);
  return 0;
}
