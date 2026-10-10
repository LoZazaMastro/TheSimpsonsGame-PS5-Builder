// Exercise the real link wrapper and retained-descriptor writer with a real
// abort signal. This checks call-site capture, not the PS5 signal ABI.
#include <rex/platform/fault_prospero.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

static void Check(bool value, const char* message) {
  if (!value) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
  }
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

[[noreturn]] __attribute__((noinline, optnone)) static void AbortFromTest() {
  // optnone keeps the return address within this function for a symbol-local
  // assertion, rather than allowing a tail call into a different test frame.
  std::abort();
}

int main() {
  const rlimit no_core{0, 0};
  Check(setrlimit(RLIMIT_CORE, &no_core) == 0, "disable test core files");
  char directory[] = "/tmp/simpsons-abort-report-XXXXXX";
  Check(mkdtemp(directory) != nullptr, "create test directory");
  const std::string root(directory);
  const std::string direct = root + "/direct.log";
  pid_t child = fork();
  Check(child >= 0, "fork caller-writer test");
  if (child == 0) {
    if (!rex::platform::OpenProsperoFaultLog(direct.c_str())) _exit(90);
    errno = EDOM;
    rex::platform::WriteProsperoAbortCaller(0x123456789abcdef0ULL);
    rex::platform::WriteProsperoAbortCaller(0xfedcba9876543210ULL);
    _exit(errno == EDOM ? 0 : 91);
  }
  int status = 0;
  Check(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
        "abort caller writer preserves errno");
  const auto text = Read(direct);
  Check(text.find("abort caller=0x123456789abcdef0") != std::string::npos,
        "full-width caller address retained");
  Check(text.find("fedcba9876543210") == std::string::npos,
        "only first explicit caller retained");
  Check(text.find("terminal fault:") == std::string::npos,
        "caller marker does not invent signal context");
  Check(text.find("TRACE BEGIN") != std::string::npos &&
            text.find("RAW STACK unavailable") != std::string::npos,
        "abort captures event history without reading an unregistered stack");
  std::puts("PASS abort caller writer: full address, once only, errno and separate signal context");

  const std::string native = root + "/native.log";
  { std::ofstream previous(native); previous << "previous session\n"; }
  child = fork();
  Check(child >= 0, "fork actual-abort test");
  if (child == 0) {
    if (!rex::platform::OpenProsperoFaultLog(native.c_str())) _exit(92);
    struct sigaction action{};
    action.sa_sigaction = SignalHandler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGABRT, &action, nullptr) != 0) _exit(93);
    AbortFromTest();
  }
  Check(waitpid(child, &status, 0) == child && WIFSIGNALED(status) &&
            WTERMSIG(status) == SIGABRT,
        "native abort termination preserved");
  const auto captured = Read(native);
  const std::string marker = "[PPSA74207] abort caller=";
  const auto position = captured.find(marker);
  Check(position != std::string::npos, "abort caller recorded before native termination");
  const auto address = std::strtoull(captured.c_str() + position + marker.size(), nullptr, 16);
  const auto function = reinterpret_cast<uintptr_t>(&AbortFromTest);
  Check(address > function && address < function + 256,
        "recorded address belongs to actual abort caller");
  Check(captured.starts_with("previous session\n"), "old diagnostic content preserved");
  Check(captured.find("terminal fault: signal=6") > position &&
            captured.find("terminal fault: signal=6") != std::string::npos,
        "caller marker followed by separate real signal record");
  const auto trace = captured.find("TRACE BEGIN");
  Check(trace != std::string::npos && captured.find("TRACE BEGIN", trace + 1) == std::string::npos,
        "abort and signal share exactly one event-ring dump");
  unlink(direct.c_str());
  unlink(native.c_str());
  rmdir(root.c_str());
  std::puts("PASS real abort: linked call site captured before SIGABRT, original termination preserved");
}
