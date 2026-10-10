/**
 * @file        core/seh_posix.cpp
 * @brief       POSIX platform SEH implementations
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 * @license     BSD 3-Clause License
 */

#include <rex/platform.h>
#include <rex/platform/seh.h>

static_assert(REX_PLATFORM_LINUX || REX_PLATFORM_MAC || REX_PLATFORM_PROSPERO, "This file is POSIX-only");

#include <signal.h>
#include <cstdlib>  // std::abort()

#include <rex/platform/exceptions.h>
#if REX_PLATFORM_PROSPERO
#include <cerrno>
#include <system_error>
#include <rex/platform/fault_prospero.h>
#include <rex/platform/signal_thread_prospero.h>
#include <rex/ui/startup_prospero.h>
#endif

namespace rex::platform {

static thread_local SehThreadState tls_seh_state;
#if !REX_PLATFORM_PROSPERO
static thread_local bool tls_seh_active = false;
#endif

#if REX_PLATFORM_PROSPERO
static constexpr int kSehSignals[] = {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT};
static struct sigaction original_seh_handlers[5]{};

static void ForwardTerminalSignal(int sig, siginfo_t* info, void* context) {
  const struct sigaction* previous = nullptr;
  for (size_t index = 0; index < 5; ++index) {
    if (kSehSignals[index] == sig) previous = &original_seh_handlers[index];
  }
  if (previous && previous->sa_handler == SIG_IGN) return;
  if (previous && previous->sa_handler != SIG_DFL && previous->sa_handler != nullptr) {
    if (previous->sa_flags & SA_SIGINFO)
      previous->sa_sigaction(sig, info, context);
    else
      previous->sa_handler(sig);
    return;
  }
  ReportProsperoFatalSignal(sig, info, context);
  // Keep the original fatal disposition. Do not change access rights, skip
  // an instruction, throw, or return a fabricated successful recovery.
  signal(sig, SIG_DFL);
  raise(sig);
}
#endif

SehThreadState& seh_thread_state() {
#if REX_PLATFORM_PROSPERO
  // Ordinary caller context only. The signal handler records numeric fields
  // in the static registry, without resolving or allocating this TLS object.
  prospero_signal_seh_read(&tls_seh_state.code, &tls_seh_state.info[0],
                          &tls_seh_state.info[1]);
#endif
  return tls_seh_state;
}

int seh_filter(uint32_t /*code*/, void* /*ep*/) {
  // Not used on POSIX - signal handlers throw directly
  return 0;
}

/// Signal handler for SIGSEGV/SIGBUS/SIGFPE/SIGILL
static void signal_handler(int sig, siginfo_t* info, void* context) {
#if REX_PLATFORM_PROSPERO
  // abort/terminate is always terminal, even inside an emulated SEH scope.
  if (sig == SIGABRT) {
    ForwardTerminalSignal(sig, info, context);
    return;
  }
#else
  (void)context;
#endif
  // Only handle if we're in SEH-protected code
#if REX_PLATFORM_PROSPERO
  if (!prospero_signal_seh_active()) {
#else
  if (!tls_seh_active) {
#endif
#if REX_PLATFORM_PROSPERO
    ForwardTerminalSignal(sig, info, context);
#else
    // Not in SEH region - restore default handler and re-raise
    signal(sig, SIG_DFL);
    raise(sig);
#endif
    return;
  }

  // Determine exception code based on signal
  SehException::Code code;
  switch (sig) {
    case SIGSEGV:
      code = SehException::ACCESS_VIOLATION;
      break;
    case SIGBUS:
      code = SehException::IN_PAGE_ERROR;
      break;
    case SIGFPE:
      code = SehException::FLOAT_DIVIDE_BY_ZERO;
      break;
    case SIGILL:
      code = SehException::ILLEGAL_INSTRUCTION;
      break;
    default:
      code = SehException::UNKNOWN;
      break;
  }

  // Get fault address
  uintptr_t address = info ? reinterpret_cast<uintptr_t>(info->si_addr) : 0;

  // Store in thread state for potential rethrow
#if REX_PLATFORM_PROSPERO
  if (!prospero_signal_seh_capture(static_cast<uint32_t>(code), address)) {
    ForwardTerminalSignal(sig, info, context);
    return;
  }
#else
  tls_seh_state.code = static_cast<uint32_t>(code);
  tls_seh_state.info[0] = 0;
  tls_seh_state.info[1] = address;
#endif

  // Preserve the existing SEH exception path. Crossing a native signal frame
  // requires unwind metadata/support from that platform; the host callback
  // test alone does not establish it for PS5's baremetal libunwind build.
  throw SehException(code, address);
}

[[noreturn]] void seh_rethrow() {
  // Map stored code back to signal and re-raise
  uint32_t code = seh_thread_state().code;
  int sig;
  switch (code) {
    case SehException::ACCESS_VIOLATION:
      sig = SIGSEGV;
      break;
    case SehException::IN_PAGE_ERROR:
      sig = SIGBUS;
      break;
    case SehException::FLOAT_DIVIDE_BY_ZERO:
    case SehException::INTEGER_DIVIDE_BY_ZERO:
      sig = SIGFPE;
      break;
    case SehException::ILLEGAL_INSTRUCTION:
      sig = SIGILL;
      break;
    default:
      abort();
  }
  // Restore default handler and re-raise
  signal(sig, SIG_DFL);
  raise(sig);
  abort();  // Should not reach here
}

void seh_initialize() {
#if REX_PLATFORM_PROSPERO
  if (g_seh_initialized.load()) return;
  InitializeProsperoFaultDiagnostics();
  struct sigaction sa{};
  sa.sa_sigaction = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO | SA_NODEFER;
  for (size_t index = 0; index < 5; ++index) {
    if (sigaction(kSehSignals[index], &sa, &original_seh_handlers[index]) != 0) {
      const int failure = errno;
      while (index) {
        --index;
        sigaction(kSehSignals[index], &original_seh_handlers[index], nullptr);
      }
      throw std::system_error(failure, std::generic_category(),
                              "Cannot install PS5 terminal/SEH signal handlers");
    }
  }
  g_seh_initialized.store(true);
  prospero_startup_log("diagnostics: terminal/SEH signal handlers installed (SEGV, BUS, FPE, ILL, ABRT)");
#else
  if (g_seh_initialized.exchange(true)) {
    return;  // Already initialized
  }

  struct sigaction sa;
  sa.sa_sigaction = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO | SA_NODEFER;  // SA_NODEFER allows re-entry for nested exceptions

  sigaction(SIGSEGV, &sa, nullptr);
  sigaction(SIGBUS, &sa, nullptr);
  sigaction(SIGFPE, &sa, nullptr);
  sigaction(SIGILL, &sa, nullptr);
#endif
}

#if REX_PLATFORM_PROSPERO
bool seh_active() {
  return prospero_signal_seh_active() != 0;
}

void seh_set_active(bool active) {
  const int error = prospero_signal_seh_set_active(active ? 1 : 0);
  if (error) {
    throw std::system_error(error, std::generic_category(),
                            "Cannot prepare PS5 per-thread signal state");
  }
}
#else
bool& seh_active() {
  return tls_seh_active;
}
#endif

}  // namespace rex::platform
