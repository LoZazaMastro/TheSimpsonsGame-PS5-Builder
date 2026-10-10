// Reproduce an actual signal in the production emutls array-growth window.
// realloc itself is real and ASan-instrumented. Only its return boundary is
// intercepted to deliver a deterministic signal after a moving allocation.
// V38_EMUTLS_SIGNAL_HANDLER exercises the old handler's TLS lookup; the fixed
// variant calls the production preallocated signal registry and callback.
#define _GNU_SOURCE 1
#include <rex/platform/signal_thread_prospero.h>

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct control { uintptr_t size, align, index; void* initial; };
extern void* __emutls_get_address(struct control*);
static struct control current_thread = {sizeof(uintptr_t), sizeof(uintptr_t), 0, 0};
static struct control extra[256];
static volatile sig_atomic_t armed;
static volatile sig_atomic_t fired;
static volatile sig_atomic_t invalid;
static const uintptr_t expected = UINT64_C(0x123456789abcdef0);

static void wait_callback(void* context) {
  if (*(uintptr_t*)context != expected) invalid = 1;
  ++fired;
}

static void on_signal(int number) {
  (void)number;
#ifdef V38_EMUTLS_SIGNAL_HANDLER
  // Exactly the unsafe old access pattern: resolving already initialized
  // emulated TLS still consults the array invalidated by the interrupted realloc.
  wait_callback(__emutls_get_address(&current_thread));
#else
  if (prospero_signal_dispatch_suspend() != 1) invalid = 2;
  if (!prospero_signal_seh_active()) invalid = 3;
  if (!prospero_signal_seh_capture(UINT32_C(0xC0000005), UINT64_C(0x12345000))) invalid = 4;
#endif
}

void* signal_test_realloc(void* old, size_t bytes) {
  void* next = realloc(old, bytes);
  if (armed && next && next != old) {
    armed = 0;
    raise(SIGUSR1);
  }
  return next;
}

static void* run(void* inject) {
  uintptr_t* current = __emutls_get_address(&current_thread);
  *current = expected;
  assert(prospero_signal_bind_suspend(current, wait_callback) == 0);
  assert(prospero_signal_seh_set_active(1) == 0);
  armed = inject != NULL;
  if (!armed) raise(SIGUSR1);
  for (unsigned i = 0; i < 256; ++i) {
    extra[i].size = sizeof(uintptr_t);
    extra[i].align = sizeof(uintptr_t);
    *(uintptr_t*)__emutls_get_address(&extra[i]) = i;
  }
  assert(fired == 1 && invalid == 0 && armed == 0);
  assert(*(uintptr_t*)__emutls_get_address(&current_thread) == expected);
  for (unsigned i = 0; i < 256; ++i)
    assert(*(uintptr_t*)__emutls_get_address(&extra[i]) == i);
#ifndef V38_EMUTLS_SIGNAL_HANDLER
  uint32_t code;
  uintptr_t information0, information1;
  assert(prospero_signal_seh_read(&code, &information0, &information1) == 1);
  assert(code == UINT32_C(0xC0000005) && information0 == 0 && information1 == UINT64_C(0x12345000));
#endif
  // Deliberately rely on real native key cleanup at pthread return. The
  // production emutls destructor must clear the registry before freeing TLS.
  return NULL;
}

int main(int argc, char** argv) {
  (void)argv;
  struct sigaction action = {0};
  action.sa_handler = on_signal;
  sigemptyset(&action.sa_mask);
  assert(sigaction(SIGUSR1, &action, 0) == 0);
  pthread_t thread;
  assert(pthread_create(&thread, 0, run, argc > 1 ? &thread : NULL) == 0);
  assert(pthread_join(thread, NULL) == 0);
  puts("PASS real emutls growth, signal observed, all 256 TLS controls preserved, native cleanup");
}
