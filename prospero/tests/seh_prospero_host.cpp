// Production seh_posix.cpp, PS5 branch, Release mode. Only sigaction's syscall
// boundary and the PS5 context/report adapters are supplied by this harness.
// It tests rollback and callback flow, NOT unwinding through a PS5 signal frame.
#include <rex/platform/exceptions.h>
#include <rex/platform/fault_prospero.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <system_error>

namespace {
int action_calls = 0;
int fail_action = 0;
int initialization_calls = 0;
int ready_markers = 0;
volatile sig_atomic_t fatal_reports = 0;
volatile sig_atomic_t previous_calls = 0;

void Check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

void PreviousHandler(int, siginfo_t*, void*) {
  previous_calls = previous_calls + 1;
}
}  // namespace

extern "C" int __real_sigaction(int, const struct sigaction*, struct sigaction*) noexcept;
extern "C" int __wrap_sigaction(int signal_number, const struct sigaction* action,
                               struct sigaction* previous) noexcept {
  if (action && ++action_calls == fail_action) {
    errno = EACCES;
    return -1;
  }
  return __real_sigaction(signal_number, action, previous);
}

extern "C" void prospero_startup_log(const char* text) {
  if (std::strstr(text, "signal handlers installed")) ++ready_markers;
}

namespace rex::platform {
void InitializeProsperoFaultDiagnostics() { ++initialization_calls; }
void ReportProsperoFatalSignal(int, const siginfo_t*, const void*) noexcept {
  fatal_reports = fatal_reports + 1;
}
}  // namespace rex::platform

int main() {
  static_assert(REX_PLATFORM_PROSPERO);
#ifndef NDEBUG
#error This boundary test must exercise the Release configuration.
#endif
  constexpr int signals[] = {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT};
  struct sigaction before[5]{};
  struct sigaction previous{};
  previous.sa_sigaction = PreviousHandler;
  previous.sa_flags = SA_SIGINFO;
  sigemptyset(&previous.sa_mask);
  for (size_t index = 0; index < 5; ++index) {
    Check(__real_sigaction(signals[index], &previous, &before[index]) == 0,
          "cannot prepare original host signal handlers");
  }

  fail_action = 2;
  bool failed = false;
  try {
    rex::platform::seh_initialize();
  } catch (const std::system_error& error) {
    failed = error.code().value() == EACCES;
  }
  Check(failed, "second sigaction failure was not explicitly propagated in Release");
  Check(action_calls == 3, "first installed handler was not rolled back");
  Check(!rex::platform::g_seh_initialized.load() && ready_markers == 0,
        "failed initialization published a ready state");
  for (int signal_number : signals) {
    struct sigaction current{};
    Check(__real_sigaction(signal_number, nullptr, &current) == 0,
          "cannot inspect actual host signal disposition");
    Check((current.sa_flags & SA_SIGINFO) && current.sa_sigaction == PreviousHandler,
          "failed initialization changed an original signal disposition");
  }
  puts("PASS Release: second sigaction fails, first rolls back, no false ready state");

  fail_action = 0;
  rex::platform::seh_initialize();
  Check(rex::platform::g_seh_initialized.load() && ready_markers == 1 &&
            initialization_calls == 2,
        "retry did not install the actual host signal handlers");
  struct sigaction floating_point{};
  Check(__real_sigaction(SIGFPE, nullptr, &floating_point) == 0 &&
            floating_point.sa_sigaction != PreviousHandler,
        "actual installed SIGFPE handler was not replaced");
  siginfo_t information{};
  information.si_code = FPE_FLTDIV;
  information.si_addr = reinterpret_cast<void*>(uintptr_t(0x12345000));
  bool recovered = false;
  {
    rex::SehGuard guard;
    try {
      // Call the installed production callback directly so this check does
      // not claim anything about a host/PS5 unwinder's signal-frame ABI.
      floating_point.sa_sigaction(SIGFPE, &information, nullptr);
    } catch (const rex::SehException& error) {
      recovered = error.code() == rex::SehException::FLOAT_DIVIDE_BY_ZERO &&
                  error.address() == uintptr_t(information.si_addr);
    }
  }
  Check(recovered && !rex::platform::seh_active() && fatal_reports == 0,
        "recovered SEH callback produced a terminal fault report");
  struct sigaction bus{};
  Check(__real_sigaction(SIGBUS, nullptr, &bus) == 0, "cannot inspect SIGBUS callback");
  bus.sa_sigaction(SIGBUS, &information, nullptr);
  Check(previous_calls == 1 && fatal_reports == 0,
        "previous custom handler was not preserved, or produced a false fatal report");
  for (size_t index = 0; index < 5; ++index) {
    Check(__real_sigaction(signals[index], &before[index], nullptr) == 0,
          "cannot restore host signal disposition after test");
  }
  puts("PASS actual installation retry; recovered SEH callback and custom chain stay quiet");
}
