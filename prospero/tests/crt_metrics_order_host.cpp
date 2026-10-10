// Drive the real native CRT entry point with controlled kernel/import seams.
// The host exit adapter executes the CRT's own atexit registrations in reverse.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <rex/platform/trace_prospero.h>
#include <rex/platform/fault_prospero.h>

namespace {
using Callback = void (*)();
Callback exits[16];
unsigned exit_count = 0, stop_calls = 0;
bool metrics_running = false, late_reporter = false, cpp_destroyed = false;
bool fini_called = false, loader_called = false;
int main_status = 0;

void CppDestructor() {
  late_reporter = metrics_running;
  cpp_destroyed = true;
  std::printf("CXX_DESTRUCTOR metrics_running=%u\n", unsigned(metrics_running));
}
void FiniArray() {
  if (!cpp_destroyed) std::abort();
  fini_called = true;
}
void LoaderTeardown() {
  if (!fini_called) std::abort();
  loader_called = true;
}
}  // namespace

extern "C" int host_atexit(Callback callback) {
  if (exit_count == 16) std::abort();
  exits[exit_count++] = callback;
  return 0;
}
extern "C" [[noreturn]] void host_exit(int status) {
  while (exit_count) exits[--exit_count]();
  const bool okay = !late_reporter && cpp_destroyed && fini_called && loader_called &&
                    !metrics_running && status == main_status;
  std::printf("RESULT order=%s stop_calls=%u status=%d\n", okay ? "PASS" : "FAIL", stop_calls, status);
  std::fflush(stdout);
  std::_Exit(okay ? status : 42);
}
extern "C" void FixtureConstructor() { host_atexit(CppDestructor); }
extern "C" {
Callback fixture_preinit_start[] = {nullptr};
Callback fixture_init_start[] = {FixtureConstructor};
Callback fixture_fini_start[] = {FiniArray};
}
__asm__(".globl fixture_preinit_end\n.set fixture_preinit_end,fixture_preinit_start\n"
        ".globl fixture_init_end\n.set fixture_init_end,fixture_init_start+8\n"
        ".globl fixture_fini_end\n.set fixture_fini_end,fixture_fini_start+8\n"
        ".globl __prospero_linked_entry\n.set __prospero_linked_entry,0x1234\n");

extern "C" int fixture_application_main(int, char**, char**) {
  metrics_running = true;
  return main_status;
}
extern "C" void prospero_metrics_stop() {
  ++stop_calls;
  metrics_running = false;
}
extern "C" void ps5_fp_ieee() {}
extern "C" void _init_env(void*) {}
extern "C" int sceKernelOpen(const char*, int, mode_t) { return 17; }
extern "C" int sceKernelMkdir(const char*, mode_t) { return 0; }
extern "C" ssize_t sceKernelWrite(int, const void*, std::size_t count) { return count; }
extern "C" int sceKernelFsync(int) { return 0; }
extern "C" int sceKernelClose(int) { return 0; }
extern "C" int sceKernelDebugOutText(int, const char*) { return 0; }
extern "C" void prospero_trace_set_image_anchors(uintptr_t, uintptr_t) noexcept {}
extern "C" void prospero_trace_event(uint32_t, uintptr_t, uint64_t, uint64_t, uint64_t, uint64_t) noexcept {}
extern "C" void prospero_trace_checkpoint(uint64_t, uint64_t, uint64_t, uint64_t) noexcept {}
namespace rex::platform {
bool OpenProsperoFaultLog(const char*) noexcept { return true; }
}
extern "C" [[noreturn]] void fixture_run_start(void*, Callback);

int main(int argc, char** argv) {
  if (argc != 2) return 3;
  main_status = std::atoi(argv[1]);
  struct { uint64_t argc; char* argv; } parameters{0, nullptr};
  fixture_run_start(&parameters, LoaderTeardown);
}
