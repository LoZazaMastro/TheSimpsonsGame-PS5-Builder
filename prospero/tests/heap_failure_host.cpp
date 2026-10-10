// Exercise the production allocator-failure reporter without allocating.
#include <rex/platform/fault_prospero.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

extern "C" [[noreturn]] void prospero_heap_failure(
    const char*, const char*, unsigned, void*, void*);
extern "C" void* __real_malloc(size_t);
extern "C" void* __real_calloc(size_t, size_t);
extern "C" void* __real_realloc(void*, size_t);
static volatile sig_atomic_t refuse_allocations;

extern "C" void* __wrap_malloc(size_t size) {
  if (refuse_allocations) _exit(97);
  return __real_malloc(size);
}
extern "C" void* __wrap_calloc(size_t count, size_t size) {
  if (refuse_allocations) _exit(97);
  return __real_calloc(count, size);
}
extern "C" void* __wrap_realloc(void* pointer, size_t size) {
  if (refuse_allocations) _exit(97);
  return __real_realloc(pointer, size);
}

static void Check(bool value, const char* message) {
  if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static std::string Read(const std::string& path) {
  std::ifstream stream(path);
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
static void SignalHandler(int number, siginfo_t*, void*) {
  rex::platform::ProsperoFaultRecord record{};
  record.signal_number = number;
  rex::platform::WriteProsperoFaultRecord(record);
  signal(number, SIG_DFL);
  raise(number);
}

int main() {
  const rlimit no_core{0, 0};
  Check(setrlimit(RLIMIT_CORE, &no_core) == 0, "disable core files");
  char directory[] = "/tmp/simpsons-heap-report-XXXXXX";
  Check(mkdtemp(directory), "create fixture directory");
  const std::string root(directory), direct = root + "/direct.txt", fatal = root + "/fatal.txt";
  pid_t child = fork();
  Check(child >= 0, "fork writer check");
  if (child == 0) {
    if (!rex::platform::OpenProsperoFaultLog(direct.c_str())) _exit(90);
    refuse_allocations = 1;
    errno = EDOM;
    rex::platform::WriteProsperoHeapFailure("corruption", "tmalloc_small", 4544,
        0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
    rex::platform::WriteProsperoHeapFailure("second", "must_not_be_written", 2, 0, 0);
    _exit(errno == EDOM ? 0 : 91);
  }
  int status = 0;
  Check(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
        "writer preserves errno and does not allocate or dereference pointers");
  const auto captured = Read(direct);
  Check(captured.find("heap failure: kind=corruption operation=tmalloc_small line=4544") != std::string::npos,
        "exact allocator detection site retained");
  Check(captured.find("arena=0x123456789abcdef0 pointer=0xfedcba9876543210") != std::string::npos,
        "full-width invalid addresses are formatted without dereference");
  Check(captured.find("must_not_be_written") == std::string::npos,
        "only first heap failure retained");
  Check(captured.find("terminal fault:") == std::string::npos,
        "heap record does not invent signal context");
  std::puts("PASS heap reporter: detection site, full-width addresses, once-only, errno, no allocation or dereference");

  { std::ofstream old(fatal); old << "previous session\n"; }
  child = fork();
  Check(child >= 0, "fork fatal helper check");
  if (child == 0) {
    if (!rex::platform::OpenProsperoFaultLog(fatal.c_str())) _exit(92);
    struct sigaction action{};
    action.sa_sigaction = SignalHandler; action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGABRT, &action, nullptr) != 0) _exit(93);
    refuse_allocations = 1;
    prospero_heap_failure("usage", "mspace_realloc", 5762,
        reinterpret_cast<void*>(0x2000000020ULL), reinterpret_cast<void*>(0x2000001000ULL));
  }
  Check(waitpid(child, &status, 0) == child && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
        "production failure helper always terminates through real abort");
  const auto terminal = Read(fatal);
  const auto heap_at = terminal.find("heap failure: kind=usage operation=mspace_realloc line=5762");
  const auto abort_at = terminal.find("abort caller=");
  const auto signal_at = terminal.find("terminal fault: signal=6");
  Check(terminal.starts_with("previous session\n") && heap_at != std::string::npos &&
            abort_at != std::string::npos && signal_at != std::string::npos &&
            heap_at < abort_at && abort_at < signal_at,
        "old log preserved; heap detection then abort caller then real signal");
  Check(terminal.find("TRACE BEGIN") != std::string::npos &&
            terminal.find("kind=heap_detail(10)") != std::string::npos &&
            terminal.find("RAW STACK unavailable") != std::string::npos,
        "failure helper dumps retained events without reading an unregistered stack");
  std::puts("PASS real heap-failure helper: append preservation, diagnostic ordering, native SIGABRT");
  unlink(direct.c_str()); unlink(fatal.c_str()); rmdir(root.c_str());
}
