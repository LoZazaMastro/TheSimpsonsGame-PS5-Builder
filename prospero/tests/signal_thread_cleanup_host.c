// Keep the cancellation registration in C, compiled without exceptions.
// glibc's C++ pthread_cleanup_push is only a destructor/landing-pad helper;
// a signal interrupting a noexcept raise call can bypass it. The PS5 SDK uses
// explicit native cleanup registration. This harness likewise requests the
// host pthread's native record instead of assuming Linux's C++ macro ABI.
#include <pthread.h>

struct signal_test_cleanup_start {
  void (*run)(void*);
  void (*cleanup)(void*);
  void* context;
};

void* signal_test_with_native_cleanup(void* opaque) {
  const struct signal_test_cleanup_start* start = opaque;
  pthread_cleanup_push(start->cleanup, start->context);
  start->run(start->context);
  pthread_cleanup_pop(1);
  return 0;
}
