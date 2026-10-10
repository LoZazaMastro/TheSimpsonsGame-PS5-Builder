#include <rex/thread/fiber.h>
#include <cerrno>
#include <cstdio>
#include <ucontext.h>

extern "C" int __wrap_swapcontext(ucontext_t*, const ucontext_t*) {
  errno = EPERM;
  return -1;
}
static bool entered = false;
static void Entry(void*) { entered = true; }
int main() {
  auto* from = rex::thread::Fiber::ConvertCurrentThread();
  auto* target = rex::thread::Fiber::Create(65536, Entry, nullptr);
  if (!from || !target) return 2;
  rex::thread::Fiber::SwitchTo(target);
  const bool kept_actual_current = rex::thread::Fiber::Current() == from;
  std::printf("native swap refused EPERM; target entered=%d; Current remains source=%d\n", entered, kept_actual_current);
  // No further destruction: the bug leaves Current dangling if target is freed.
  // Releasing the root would change Current and obscure the observed mismatch.
  return !entered && kept_actual_current ? 0 : 1;
}
