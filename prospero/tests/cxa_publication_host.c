/* Exercise the real SDK registry. Thread creation, pthread_once and the TLS
 * keys are native host APIs; no PS5 destructor ordering is simulated here. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int ps5___cxa_thread_atexit_impl(void (*destructor)(void *), void *object, void *dso);
void ps5p_run_thread_destructors(void);

static pthread_barrier_t start_gate;
static atomic_uint reader_calls;
static atomic_uint destroyed;
static atomic_bool registered;
static atomic_uint errors;
static unsigned expected_at_exit;

static void check(int value, const char *message) {
  if (!value) {
    fprintf(stderr, "FAIL %s\n", message);
    atomic_fetch_add_explicit(&errors, 1, memory_order_relaxed);
  }
}

static void count_destructor(void *object) {
  check(object == &destroyed, "destructor receives its registered object");
  atomic_fetch_add_explicit(&destroyed, 1, memory_order_relaxed);
}

static void nested_destructor(void *object) {
  count_destructor(object);
  check(ps5___cxa_thread_atexit_impl(count_destructor, object, NULL) == 0,
        "destructor may register another destructor");
}

static void *first_registration(void *ignored) {
  (void)ignored;
  pthread_barrier_wait(&start_gate);
  /* Relaxed polling deliberately creates no publication edge for the SDK's
   * plain bool. The reader has never entered its pthread_once routine. */
  while (atomic_load_explicit(&reader_calls, memory_order_relaxed) < 1000) {}
  check(ps5___cxa_thread_atexit_impl(nested_destructor, &destroyed, NULL) == 0,
        "first registration succeeds");
  ps5p_run_thread_destructors();
  atomic_store_explicit(&registered, 1, memory_order_relaxed);
  return NULL;
}

static void *unregistered_thread_exit(void *ignored) {
  (void)ignored;
  pthread_barrier_wait(&start_gate);
  do {
    ps5p_run_thread_destructors();
    atomic_fetch_add_explicit(&reader_calls, 1, memory_order_relaxed);
  } while (!atomic_load_explicit(&registered, memory_order_relaxed));
  return NULL;
}

static void *normal_registration(void *mode) {
  for (unsigned i = 0; i != 16; ++i) {
    check(ps5___cxa_thread_atexit_impl(nested_destructor, &destroyed, NULL) == 0,
          "parallel registration succeeds");
  }
  if ((uintptr_t)mode & 1) {
    ps5p_run_thread_destructors();
    ps5p_run_thread_destructors();
  }
  return NULL;
}

static void check_exit_order(void) {
  check(atomic_load_explicit(&destroyed, memory_order_relaxed) == expected_at_exit,
        "atexit drains the main thread registration exactly once");
  if (atomic_load_explicit(&errors, memory_order_relaxed)) _Exit(2);
  puts("PASS first-use publication, parallel keys, nested destructors, repeat drain and atexit");
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  /* Registered before the SDK's atexit hook, so it observes completed SDK
   * cleanup when normal process exit executes hooks in reverse order. */
  check(atexit(check_exit_order) == 0, "install exit-order observer");
  check(pthread_barrier_init(&start_gate, NULL, 3) == 0, "create start barrier");
  pthread_t creator, reader;
  check(pthread_create(&reader, NULL, unregistered_thread_exit, NULL) == 0, "create non-registering thread");
  check(pthread_create(&creator, NULL, first_registration, NULL) == 0, "create first-registering thread");
  pthread_barrier_wait(&start_gate);
  pthread_join(creator, NULL);
  pthread_join(reader, NULL);
  pthread_barrier_destroy(&start_gate);
  check(atomic_load_explicit(&destroyed, memory_order_relaxed) == 2,
        "first thread drains its nested destructors once");

  pthread_t peers[16];
  for (uintptr_t i = 0; i != 16; ++i)
    check(pthread_create(&peers[i], NULL, normal_registration, (void *)i) == 0,
          "create parallel registry thread");
  for (unsigned i = 0; i != 16; ++i) pthread_join(peers[i], NULL);
  expected_at_exit = 2 + 16 * 16 * 2 + 1;
  check(atomic_load_explicit(&destroyed, memory_order_relaxed) == expected_at_exit - 1,
        "parallel threads drain via explicit call or native key exit");
  check(ps5___cxa_thread_atexit_impl(count_destructor, &destroyed, NULL) == 0,
        "register main-thread destructor for process exit");
  printf("reader_calls=%u destructors_before_exit=%u\n",
         atomic_load_explicit(&reader_calls, memory_order_relaxed),
         atomic_load_explicit(&destroyed, memory_order_relaxed));
  return atomic_load_explicit(&errors, memory_order_relaxed) ? 2 : 0;
}
