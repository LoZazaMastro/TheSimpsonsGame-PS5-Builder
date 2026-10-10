/* Host checks of the real mspace allocator and matched SDK heap wrappers.
 * Only kernel calls and libc-failure boundaries are modelled/intercepted.
 * This is not linked into the title. */
#define _GNU_SOURCE 1
#include "ps5platform/heap.h"
#include "ps5platform/kernel.h"
#include "host_kernel.h"
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

#ifdef HEAP_TRACE_TEST
#include <rex/platform/trace_prospero.h>
#endif

#ifdef HEAP_LOCAL_LOCK_TEST
#include "allocator_lock.h"
#endif

typedef void *mspace;
mspace create_mspace(size_t, int);
size_t destroy_mspace(mspace);
void *mspace_malloc(mspace, size_t);
void *mspace_realloc(mspace, void *, size_t);
void mspace_free(mspace, void *);
size_t mspace_usable_size(const void *);
void *__wrap_malloc(size_t);
void *__wrap_calloc(size_t, size_t);
void *__wrap_realloc(void *, size_t);
void *__wrap_reallocf(void *, size_t);
void __wrap_free(void *);
int __wrap_posix_memalign(void **, size_t, size_t);
void *__wrap_aligned_alloc(size_t, size_t);
void *__wrap_memalign(size_t, size_t);
void *__wrap_reallocarray(void *, size_t, size_t);
size_t __wrap_malloc_usable_size(const void *);
ssize_t __wrap_getline(char **, size_t *, FILE *);
ssize_t __wrap_getdelim(char **, size_t *, int, FILE *);

#define CHECK(condition) do { if (!(condition)) { \
  fprintf(stderr, "FAIL %s:%d: %s (errno=%d)\n", \
          __func__, __LINE__, #condition, errno); _Exit(1); \
} } while (0)

static void check_bytes(const void *pointer, size_t bytes, unsigned char value) {
  const unsigned char *p = pointer;
  for (size_t i = 0; i < bytes; ++i) CHECK(p[i] == value);
}

/* --wrap applies to these boundaries, never to all host malloc/free calls. */
static atomic_int fail_libc_malloc, fail_libc_realloc;
static atomic_uint real_malloc_calls, real_realloc_calls, real_free_calls;
void *__real___real_malloc(size_t);
void *__real___real_realloc(void *, size_t);
void __real___real_free(void *);
void *__wrap___real_malloc(size_t bytes) {
  atomic_fetch_add(&real_malloc_calls, 1);
  if (atomic_load(&fail_libc_malloc)) { errno = ENOMEM; return NULL; }
  return __real___real_malloc(bytes);
}
void *__wrap___real_realloc(void *p, size_t bytes) {
  atomic_fetch_add(&real_realloc_calls, 1);
  if (atomic_load(&fail_libc_realloc)) { errno = ENOMEM; return NULL; }
  return __real___real_realloc(p, bytes);
}
void __wrap___real_free(void *p) {
  atomic_fetch_add(&real_free_calls, 1);
  __real___real_free(p);
}

struct mapping_record { int64_t physical; size_t bytes; };
static struct mapping_record mappings[1024];
static atomic_uint mapping_count;
int32_t __real_sceKernelAllocateDirectMemory(int64_t, int64_t, size_t,
                                            size_t, int, int64_t *);
int32_t __wrap_sceKernelAllocateDirectMemory(int64_t start, int64_t end,
    size_t bytes, size_t alignment, int kind, int64_t *physical) {
  const int32_t result = __real_sceKernelAllocateDirectMemory(
      start, end, bytes, alignment, kind, physical);
  if (result == 0) {
    const unsigned n = atomic_fetch_add(&mapping_count, 1);
    CHECK(n < sizeof(mappings) / sizeof(mappings[0]));
    mappings[n] = (struct mapping_record){*physical, bytes};
  }
  return result;
}

static atomic_int track_sleep;
static atomic_uint sleep_calls, sleep_interruptions, sleep_successes;
int __real_nanosleep(const struct timespec *, struct timespec *);
int __wrap_nanosleep(const struct timespec *request, struct timespec *remaining) {
  if (!atomic_load(&track_sleep)) return __real_nanosleep(request, remaining);
  const unsigned n = atomic_fetch_add(&sleep_calls, 1);
  CHECK(request->tv_sec == 0 && request->tv_nsec == 50000);
  if (n == 0) {
    atomic_fetch_add(&sleep_interruptions, 1);
    if (remaining) *remaining = *request;
    errno = EINTR;
    return -1;
  }
  const int result = __real_nanosleep(request, remaining);
  if (result == 0) atomic_fetch_add(&sleep_successes, 1);
  return result;
}

__attribute__((noreturn)) void prospero_heap_failure(
    const char *kind, const char *operation, unsigned line,
    void *space, void *pointer) {
  char message[384];
  const int n = snprintf(message, sizeof(message),
      "OBSERVER prospero_heap_failure kind=%s operation=%s line=%u mspace=%p pointer=%p\n",
      kind, operation, line, space, pointer);
  if (n > 0) {
    const ssize_t written = write(STDERR_FILENO, message,
        (size_t)n < sizeof(message) ? (size_t)n : sizeof(message) - 1);
    (void)written;
  }
#ifdef HEAP_TRACE_TEST
  (void)prospero_trace_dump_fd(STDERR_FILENO);
#endif
  abort();
}

enum { THREADS = 8, SLOTS = 32, ITERATIONS = 6000, HANDOFF_SLOTS = 64,
       HANDOFF_ROUNDS = 40 };
static mspace shared_space;
static pthread_barrier_t barrier;
struct slot { void *pointer; size_t bytes; unsigned char value; };
static struct slot handoff[THREADS][HANDOFF_SLOTS];

static void *shared_worker(void *arg) {
  const unsigned id = (unsigned)(uintptr_t)arg;
  uint32_t random = 0x9137421u + id;
  struct slot slots[SLOTS] = {{0}};
  (void)pthread_barrier_wait(&barrier);
  for (unsigned i = 0; i < ITERATIONS; ++i) {
    random = random * 1664525u + 1013904223u;
    struct slot *s = &slots[(random >> 8) % SLOTS];
    const size_t bytes = 1 + ((random >> 16) % 4096);
    const unsigned char value = (unsigned char)(id * 23 + i);
    if (s->pointer) {
      check_bytes(s->pointer, s->bytes, s->value);
      if ((random & 7) == 0) {
        mspace_free(shared_space, s->pointer);
        *s = (struct slot){0};
        continue;
      }
      void *p = mspace_realloc(shared_space, s->pointer, bytes);
      CHECK(p != NULL);
      check_bytes(p, s->bytes < bytes ? s->bytes : bytes, s->value);
      s->pointer = p;
    } else {
      s->pointer = mspace_malloc(shared_space, bytes);
      CHECK(s->pointer != NULL);
    }
    CHECK(((uintptr_t)s->pointer & 31) == 0);
    CHECK(mspace_usable_size(s->pointer) >= bytes);
    memset(s->pointer, value, bytes);
    s->bytes = bytes;
    s->value = value;
  }
  for (unsigned i = 0; i < SLOTS; ++i) {
    if (slots[i].pointer) {
      check_bytes(slots[i].pointer, slots[i].bytes, slots[i].value);
      mspace_free(shared_space, slots[i].pointer);
    }
  }
  return NULL;
}

static void *handoff_worker(void *arg) {
  const unsigned id = (unsigned)(uintptr_t)arg;
  for (unsigned round = 0; round < HANDOFF_ROUNDS; ++round) {
    for (unsigned i = 0; i < HANDOFF_SLOTS; ++i) {
      struct slot *s = &handoff[id][i];
      s->bytes = 1 + (round * 37 + i * 43 + id * 11) % 2048;
      s->value = (unsigned char)(round + i + id * 17);
      s->pointer = __wrap_malloc(s->bytes);
      CHECK(s->pointer && ps5_heap_owns(s->pointer));
      CHECK(((uintptr_t)s->pointer & 31) == 0);
      memset(s->pointer, s->value, s->bytes);
    }
    (void)pthread_barrier_wait(&barrier);
    for (unsigned i = 0; i < HANDOFF_SLOTS; ++i) {
      struct slot *s = &handoff[(id + THREADS - 1) % THREADS][i];
      check_bytes(s->pointer, s->bytes, s->value);
      void *p = __wrap_realloc(s->pointer, s->bytes + 511);
      CHECK(p && ps5_heap_owns(p));
      CHECK(((uintptr_t)p & 31) == 0);
      check_bytes(p, s->bytes, s->value);
      memset(p, s->value, s->bytes + 511);
      check_bytes(p, s->bytes + 511, s->value);
      __wrap_free(p);
    }
    (void)pthread_barrier_wait(&barrier);
  }
  return NULL;
}

static void run_threads(void *(*worker)(void *)) {
  pthread_t threads[THREADS];
  CHECK(pthread_barrier_init(&barrier, NULL, THREADS) == 0);
  for (unsigned i = 0; i < THREADS; ++i)
    CHECK(pthread_create(&threads[i], NULL, worker, (void *)(uintptr_t)i) == 0);
  for (unsigned i = 0; i < THREADS; ++i) CHECK(pthread_join(threads[i], NULL) == 0);
  CHECK(pthread_barrier_destroy(&barrier) == 0);
}

static void test_threads(void) {
  shared_space = create_mspace(0, 1);
  CHECK(shared_space != NULL);
  run_threads(shared_worker);
  CHECK(destroy_mspace(shared_space) > 0);
  puts("PASS shared locked mspace: 8 threads, 48000 small alloc/free/realloc operations, payload and alignment32");
  run_threads(handoff_worker);
  struct ps5_heap_stats stats;
  ps5_heap_stats(&stats);
  CHECK(stats.arenas == THREADS);
  puts("PASS SDK wrappers: 8 arenas, 20480 cross-thread realloc/free handoffs, payload and alignment32");
}

#ifdef HEAP_TRACE_TEST
static atomic_int stats_stop;
static atomic_ullong stats_samples;
static void *stats_observer(void *unused) {
  (void)unused;
  size_t previous_peak = 0;
  while (!atomic_load_explicit(&stats_stop, memory_order_relaxed)) {
    struct ps5_heap_stats stats;
    ps5_heap_stats(&stats);
    CHECK(stats.range_base != 0 && stats.range_bytes >= ((size_t)1 << 30));
    CHECK(stats.mapped_bytes <= stats.range_bytes);
    CHECK(stats.peak_bytes >= previous_peak && stats.peak_bytes <= stats.range_bytes);
    CHECK(stats.segments <= 1024 && stats.arenas <= 8);
    /* Field samples are independently atomic, not a transaction. Do not
     * assert mapped <= peak for two reads spanning a concurrent change. */
    previous_peak = stats.peak_bytes;
    atomic_fetch_add_explicit(&stats_samples, 1, memory_order_relaxed);
  }
  return NULL;
}
#endif

static void test_alignment(void) {
  for (size_t alignment = 8; alignment <= 65536; alignment *= 2) {
    void *p = NULL;
    CHECK(__wrap_posix_memalign(&p, alignment, 4117) == 0);
    CHECK(p && ps5_heap_owns(p) && ((uintptr_t)p % alignment) == 0);
    CHECK(__wrap_malloc_usable_size(p) >= 4117);
    memset(p, 0x61, 4117);
    check_bytes(p, 4117, 0x61);
    __wrap_free(p);
    p = __wrap_aligned_alloc(alignment, alignment * 2);
    CHECK(p && ps5_heap_owns(p) && ((uintptr_t)p % alignment) == 0);
    memset(p, 0x9a, alignment * 2);
    check_bytes(p, alignment * 2, 0x9a);
    __wrap_free(p);
  }
  void *sentinel = (void *)(uintptr_t)0x42;
  CHECK(__wrap_posix_memalign(&sentinel, 24, 100) == EINVAL);
  CHECK(sentinel == (void *)(uintptr_t)0x42);
  errno = 0;
  CHECK(__wrap_reallocarray(NULL, SIZE_MAX, 2) == NULL && errno == ENOMEM);
  unsigned char *p = __wrap_malloc(8);
  CHECK(p != NULL);
  p[0] = 0x87;
  p = __wrap_realloc(p, 0);
  CHECK(p != NULL && p[0] == 0x87);
  __wrap_free(p);
  puts("PASS alignment8..65536; invalid alignment, overflow, and zero-size realloc contracts");
}

static void test_dirty_calloc(void) {
  const size_t bytes = (size_t)40 << 20;
  const long long allocations = host_direct_allocations();
  unsigned n = atomic_load(&mapping_count);
  unsigned char *dirty = __wrap_malloc(bytes);
  CHECK(dirty && ps5_heap_owns(dirty));
  CHECK(atomic_load(&mapping_count) == n + 1);
  const struct mapping_record backing = mappings[n];
  CHECK(dirty[0] == 0xa5 && dirty[bytes / 2] == 0xa5 && dirty[bytes - 1] == 0xa5);
  memset(dirty, 0x77, bytes);
  __wrap_free(dirty);
  CHECK(host_direct_allocations() == allocations);
  n = atomic_load(&mapping_count);
  unsigned char *cleared = __wrap_calloc(1, bytes);
  CHECK(cleared && ps5_heap_owns(cleared));
  CHECK(atomic_load(&mapping_count) == n + 1);
  CHECK(mappings[n].physical == backing.physical && mappings[n].bytes == backing.bytes);
  check_bytes(cleared, bytes, 0);
  __wrap_free(cleared);
  CHECK(host_direct_allocations() == allocations);
  puts("PASS calloc40MiB clears every byte on reused physical backing dirtied by the SDK kernel model");
}

static void test_failures_and_foreign(void) {
  const size_t large = (size_t)64 << 20;
  unsigned char *owned = __wrap_malloc(128);
  unsigned char *preserved = __wrap_malloc(128);
  CHECK(owned && preserved && ps5_heap_owns(owned) && ps5_heap_owns(preserved));
  memset(owned, 0x53, 128);
  memset(preserved, 0xc2, 128);
  struct ps5_heap_stats before, after;
  ps5_heap_stats(&before);
  const long long allocations = host_direct_allocations();
  host_fail(HOST_CALL_MAP, -1);
  CHECK(ps5_heap_malloc(large) == NULL);
  unsigned calls = atomic_load(&real_malloc_calls);
  void *fallback = __wrap_malloc(large);
  CHECK(fallback && !ps5_heap_owns(fallback));
  CHECK(atomic_load(&real_malloc_calls) == calls + 1);
  __wrap_free(fallback);
  unsigned char *moved = __wrap_realloc(owned, large);
  CHECK(moved && !ps5_heap_owns(moved));
  check_bytes(moved, 128, 0x53);
  __wrap_free(moved);
  atomic_store(&fail_libc_malloc, 1);
  CHECK(__wrap_realloc(preserved, large) == NULL);
  check_bytes(preserved, 128, 0xc2);
  atomic_store(&fail_libc_malloc, 0);
  host_fail(HOST_CALL_NONE, 0);
  CHECK(host_direct_allocations() == allocations);
  ps5_heap_stats(&after);
  CHECK(before.mapped_bytes == after.mapped_bytes && before.segments == after.segments);
  CHECK(after.libc_fallbacks == before.libc_fallbacks + 2);
  __wrap_free(preserved);
  puts("PASS map failures release direct memory; malloc/realloc fallback and failed-realloc preservation");

  unsigned char *foreign = malloc(128);
  CHECK(foreign && !ps5_heap_owns(foreign));
  memset(foreign, 0xab, 128);
  calls = atomic_load(&real_realloc_calls);
  foreign = __wrap_realloc(foreign, 256);
  CHECK(foreign && !ps5_heap_owns(foreign));
  CHECK(atomic_load(&real_realloc_calls) == calls + 1);
  check_bytes(foreign, 128, 0xab);
  CHECK(__wrap_malloc_usable_size(foreign) >= 256);
  calls = atomic_load(&real_free_calls);
  __wrap_free(foreign);
  CHECK(atomic_load(&real_free_calls) == calls + 1);
  foreign = malloc(128);
  CHECK(foreign && !ps5_heap_owns(foreign));
  memset(foreign, 0x38, 128);
  atomic_store(&fail_libc_realloc, 1);
  calls = atomic_load(&real_free_calls);
  moved = __wrap_realloc(foreign, 4096);
  CHECK(moved && ps5_heap_owns(moved));
  CHECK(atomic_load(&real_free_calls) == calls + 1);
  check_bytes(moved, 128, 0x38);
  atomic_store(&fail_libc_realloc, 0);
  __wrap_free(moved);
  puts("PASS foreign libc realloc/free routing and libc-exhaustion move into the title heap");
}

static void test_remaining_wrapper_apis(void) {
  __wrap_free(NULL);
  CHECK(__wrap_malloc_usable_size(NULL) == 0);
  void *aligned = __wrap_memalign(4096, 777);
  CHECK(aligned && (uintptr_t)aligned % 4096 == 0);
  memset(aligned, 0x81, 777);
  check_bytes(aligned, 777, 0x81);
  __wrap_free(aligned);

  unsigned char *kept = __wrap_malloc(128);
  CHECK(kept != NULL);
  memset(kept, 0x73, 128);
  errno = 0;
  CHECK(__wrap_reallocarray(kept, SIZE_MAX, 2) == NULL && errno == ENOMEM);
  check_bytes(kept, 128, 0x73);
  __wrap_free(kept);

  // reallocf must release the original on failure, unlike realloc.
  void *foreign = malloc(128);
  CHECK(foreign && !ps5_heap_owns(foreign));
  const unsigned frees = atomic_load(&real_free_calls);
  atomic_store(&fail_libc_realloc, 1);
  host_fail(HOST_CALL_MAP, -1);
  CHECK(__wrap_reallocf(foreign, (size_t)64 << 20) == NULL);
  CHECK(atomic_load(&real_free_calls) == frees + 1);
  host_fail(HOST_CALL_NONE, 0);
  atomic_store(&fail_libc_realloc, 0);

  FILE *stream = tmpfile();
  CHECK(stream != NULL);
  for (unsigned i = 0; i < 1000; ++i) CHECK(fputc('x', stream) == 'x');
  CHECK(fputs("\nend|", stream) >= 0);
  rewind(stream);
  char *line = __wrap_malloc(16);
  size_t capacity = 16;
  CHECK(line != NULL);
  CHECK(__wrap_getline(&line, &capacity, stream) == 1001);
  CHECK(ps5_heap_owns(line) && capacity >= 1002 && line[1000] == '\n' && line[1001] == 0);
  check_bytes(line, 1000, 'x');
  CHECK(__wrap_getdelim(&line, &capacity, '|', stream) == 4);
  CHECK(memcmp(line, "end|", 5) == 0);
  CHECK(__wrap_getline(&line, &capacity, stream) == -1);
  errno = 0;
  CHECK(__wrap_getdelim(NULL, &capacity, '|', stream) == -1 && errno == EINVAL);
  __wrap_free(line);
  CHECK(fclose(stream) == 0);
  puts("PASS all12 wrapper APIs: memalign, preserved overflow source, reallocf failure-free, title-buffer getline/getdelim growth and EOF/error");
}

#ifdef HEAP_LOCAL_LOCK_TEST
static prospero_allocator_mutex held_lock;
static atomic_int lock_acquired;
static void *lock_waiter(void *unused) {
  (void)unused;
  errno = E2BIG;
  CHECK(prospero_allocator_lock_acquire(&held_lock) == 0);
  CHECK(errno == E2BIG);
  atomic_store(&lock_acquired, 1);
  prospero_allocator_lock_release(&held_lock);
  return NULL;
}
static void test_lock(void) {
  CHECK(prospero_allocator_lock_init(&held_lock) == 0);
  CHECK(prospero_allocator_lock_try(&held_lock) == 1);
  CHECK(prospero_allocator_lock_try(&held_lock) == 0);
  atomic_store(&track_sleep, 1);
  pthread_t waiter;
  CHECK(pthread_create(&waiter, NULL, lock_waiter, NULL) == 0);
  const struct timespec pause = {0, 100000};
  for (unsigned i = 0; i < 20000 && atomic_load(&sleep_successes) == 0; ++i)
    (void)__real_nanosleep(&pause, NULL);
  CHECK(atomic_load(&sleep_interruptions) == 1);
  CHECK(atomic_load(&sleep_calls) >= 2 && atomic_load(&sleep_successes) >= 1);
  CHECK(atomic_load(&lock_acquired) == 0);
  prospero_allocator_lock_release(&held_lock);
  CHECK(pthread_join(waiter, NULL) == 0);
  CHECK(atomic_load(&lock_acquired) == 1);
  atomic_store(&track_sleep, 0);
  CHECK(prospero_allocator_lock_destroy(&held_lock) == 0);
  printf("PASS exact production lock: held-owner backoff50us, EINTR=1, sleeps=%u, errno preserved, acquisition only after release\n",
         atomic_load(&sleep_calls));
}
#endif

static void corrupt_child(void) {
  mspace space = create_mspace(0, 1);
  CHECK(space != NULL);
  void *a = mspace_malloc(space, 64);
  void *b = mspace_malloc(space, 64);
  void *c = mspace_malloc(space, 64);
  CHECK(a && b && c);
  mspace_free(space, a);
  /* Deliberate test-only corruption of freed smallbin fd. F points into
   * valid mapped memory, but F->bk is not the chunk being unlinked. */
  ((uintptr_t *)a)[0] = (uintptr_t)a;
  ((uintptr_t *)a)[3] = 0;
  puts("CORRUPT fixture armed: invalid smallbin fd/bk relationship");
  (void)mspace_malloc(space, 64);
  CHECK(!"corruption unexpectedly returned");
}

#ifdef HEAP_TRACE_TEST
/* Out-of-line sites let the runner resolve recorded caller PCs in the actual
 * host executable. The empty compiler barrier prevents a tail-call return. */
__attribute__((noinline)) static void *trace_fixture_allocate(size_t bytes) {
  void *const pointer = __wrap_malloc(bytes);
  __asm__ __volatile__("" ::: "memory");
  return pointer;
}
__attribute__((noinline)) static void trace_fixture_free(void *pointer) {
  __wrap_free(pointer);
  __asm__ __volatile__("" ::: "memory");
}
static void corrupt_trace_child(int bad_address) {
  void *a = trace_fixture_allocate(64);
  void *b = trace_fixture_allocate(64);
  void *c = trace_fixture_allocate(64);
  CHECK(a && b && c);
  trace_fixture_free(a);
  // This is deliberate fixture corruption, never production behavior.
  ((uintptr_t *)a)[0] = bad_address ? 1 : (uintptr_t)a;
  if (!bad_address) ((uintptr_t *)a)[3] = 0;
  printf("TRACE fixture freed=%p bad_address=%d request=64 expected_index=12\n", a, bad_address);
  (void)trace_fixture_allocate(64);
  CHECK(!"trace corruption unexpectedly returned");
}
#endif

int main(int argc, char **argv) {
  const struct rlimit no_core = {0, 0};
  CHECK(setrlimit(RLIMIT_CORE, &no_core) == 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  CHECK(argc == 2);
  void *bootstrap = __wrap_malloc(64);
  CHECK(bootstrap && ps5_heap_owns(bootstrap));
  __wrap_free(bootstrap);
#ifdef HEAP_TRACE_TEST
  if (strcmp(argv[1], "trace-corrupt-backlink") == 0) { corrupt_trace_child(0); return 1; }
  if (strcmp(argv[1], "trace-corrupt-address") == 0) { corrupt_trace_child(1); return 1; }
#endif
  if (strcmp(argv[1], "corrupt") == 0) { corrupt_child(); return 1; }
  CHECK(strcmp(argv[1], "suite") == 0);
#ifdef HEAP_TRACE_TEST
  pthread_t observer;
  CHECK(pthread_create(&observer, NULL, stats_observer, NULL) == 0);
#endif
  test_threads();
  test_alignment();
  test_dirty_calloc();
  test_failures_and_foreign();
  test_remaining_wrapper_apis();
#ifdef HEAP_TRACE_TEST
  atomic_store_explicit(&stats_stop, 1, memory_order_relaxed);
  CHECK(pthread_join(observer, NULL) == 0);
  CHECK(atomic_load_explicit(&stats_samples, memory_order_relaxed) > 0);
  printf("PASS production heap concurrent atomic stats: %llu snapshots during live allocator work\n",
         atomic_load_explicit(&stats_samples, memory_order_relaxed));
#endif
#ifdef HEAP_LOCAL_LOCK_TEST
  test_lock();
#else
  puts("INFO SDK baseline uses its real pthread mutex; atomic-lock-specific test is not applicable");
#endif
  puts("PASS complete healthy allocator suite");
  return 0;
}
