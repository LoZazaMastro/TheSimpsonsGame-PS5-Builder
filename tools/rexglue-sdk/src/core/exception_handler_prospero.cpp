/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/exception_handler.h>

#if REX_PLATFORM_PROSPERO

#include <signal.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <stdexcept>
#include <string>
#include <system_error>

#include <rex/assert.h>
#include <rex/logging.h>
#include <rex/math.h>
#include <rex/platform.h>
#include <rex/platform/fault_prospero.h>
#include <rex/platform/trace_phases_prospero.h>
#include <rex/platform/metrics_prospero.h>
#include <rex/ui/startup_prospero.h>

#include <ucontext.h>
#include <ps5platform/context.h>
#include <machine/fpu.h>
#include <machine/trap.h>

namespace rex::platform {
namespace {
bool fault_diagnostics_initialized = false;
uintptr_t fault_image_base = 0;
bool fault_image_base_known = false;
}  // namespace

void InitializeProsperoFaultDiagnostics() {
  if (fault_diagnostics_initialized) return;
  const std::string path = std::string(prospero_state_root()) + "/logs/fault.log";
  if (!OpenProsperoFaultLog(path.c_str())) {
    const int failure = errno;
    prospero_startup_log(("diagnostics: terminal fault report unavailable at " + path +
                          "; errno=" + std::to_string(failure) + "; fallback=fd2").c_str());
  } else {
    prospero_startup_log(("diagnostics: terminal fault report opened at " + path).c_str());
  }
  // Resolve image metadata outside signal context. A base of zero is valid
  // for this native title, so store the lookup result separately from it.
  Dl_info info{};
  fault_image_base_known = dladdr(reinterpret_cast<const void*>(&ReportProsperoFatalSignal),
                                 &info) != 0;
  if (fault_image_base_known) fault_image_base = uintptr_t(info.dli_fbase);
  prospero_trace_checkpoint(PROSPERO_PHASE_DIAGNOSTICS_READY,
                           uintptr_t(&ReportProsperoFatalSignal), fault_image_base,
                           fault_image_base_known ? 1 : 0);
  fault_diagnostics_initialized = true;
  prospero_metrics_start();
}

void ReportProsperoFatalSignal(int signal_number, const siginfo_t* info,
                              const void* context) noexcept {
  ProsperoFaultRecord record{};
  record.signal_number = signal_number;
  record.signal_code = info ? info->si_code : 0;
  record.fault_address = info ? uintptr_t(info->si_addr) : 0;
  record.context_present = context != nullptr;
  record.image_base = fault_image_base;
  record.image_base_known = fault_image_base_known;
  record.reporter_address = uintptr_t(&ReportProsperoFatalSignal);
  if (context) {
    const auto& machine = static_cast<const ucontext_t*>(context)->uc_mcontext;
    record.rip = uint64_t(machine.mc_rip);
    record.rsp = uint64_t(machine.mc_rsp);
    record.rbp = uint64_t(machine.mc_rbp);
    record.flags = uint64_t(machine.mc_rflags);
    record.trap_number = uint64_t(machine.mc_trapno);
    record.page_fault_error = uint64_t(machine.mc_err);
    const uint64_t registers[] = {
        uint64_t(machine.mc_rax), uint64_t(machine.mc_rcx), uint64_t(machine.mc_rdx),
        uint64_t(machine.mc_rbx), uint64_t(machine.mc_rsp), uint64_t(machine.mc_rbp),
        uint64_t(machine.mc_rsi), uint64_t(machine.mc_rdi), uint64_t(machine.mc_r8),
        uint64_t(machine.mc_r9), uint64_t(machine.mc_r10), uint64_t(machine.mc_r11),
        uint64_t(machine.mc_r12), uint64_t(machine.mc_r13), uint64_t(machine.mc_r14),
        uint64_t(machine.mc_r15)};
    for (size_t index = 0; index < 16; ++index) record.registers[index] = registers[index];
  }
  WriteProsperoFaultRecord(record);
}
}  // namespace rex::platform

namespace rex::arch {

bool signal_handlers_installed_ = false;
struct sigaction original_sigill_handler_;
struct sigaction original_sigsegv_handler_;
struct sigaction original_sigbus_handler_;

// This can be as large as needed, but isn't often needed.
// As we will be sometimes firing many exceptions we want to avoid having to
// scan the table too much or invoke many custom handlers.
constexpr size_t kMaxHandlerCount = 8;

// All custom handlers, left-aligned and null terminated.
// Executed in order.
std::pair<ExceptionHandler::Handler, void*> handlers_[kMaxHandlerCount];

static void ForwardUnhandledException(int signal_number, siginfo_t* signal_info,
                                     void* signal_context) {
  const struct sigaction& previous = signal_number == SIGILL ? original_sigill_handler_
      : signal_number == SIGBUS ? original_sigbus_handler_ : original_sigsegv_handler_;
  if (previous.sa_handler == SIG_IGN) return;
  if (previous.sa_handler != SIG_DFL && previous.sa_handler != nullptr) {
    if (previous.sa_flags & SA_SIGINFO)
      previous.sa_sigaction(signal_number, signal_info, signal_context);
    else
      previous.sa_handler(signal_number);
    return;
  }
  // The previous handler is normally the SEH adapter. Only a direct default
  // disposition is terminal here; recovered SEH/write-watch faults stay quiet.
  platform::ReportProsperoFatalSignal(signal_number, signal_info, signal_context);
  sigaction(signal_number, &previous, nullptr);
  raise(signal_number);
}

static void ExceptionHandlerCallback(int signal_number, siginfo_t* signal_info,
                                     void* signal_context) {
  mcontext_t& mcontext = reinterpret_cast<ucontext_t*>(signal_context)->uc_mcontext;

  // FreeBSD can use SIGBUS for faults other than page protection (for example
  // alignment or device errors). Exclude these and instruction fetches before
  // the MMIO stale-protection recheck can mistake them for accessible data.
  const auto memory_access = platform::ClassifyProsperoMemoryFault(
      signal_number, uint64_t(mcontext.mc_trapno), uint64_t(mcontext.mc_err), T_PAGEFLT);
  if ((signal_number == SIGSEGV || signal_number == SIGBUS) &&
      memory_access == platform::ProsperoFaultAccess::kUnhandled) {
    ForwardUnhandledException(signal_number, signal_info, signal_context);
    return;
  }

  HostThreadContext thread_context;

#if REX_ARCH_AMD64
  thread_context.rip = uint64_t(mcontext.mc_rip);
  thread_context.eflags = uint32_t(mcontext.mc_rflags);
  // The REG_ order may be different than the register indices in the
  // instruction encoding.
  thread_context.rax = uint64_t(mcontext.mc_rax);
  thread_context.rcx = uint64_t(mcontext.mc_rcx);
  thread_context.rdx = uint64_t(mcontext.mc_rdx);
  thread_context.rbx = uint64_t(mcontext.mc_rbx);
  thread_context.rsp = uint64_t(mcontext.mc_rsp);
  thread_context.rbp = uint64_t(mcontext.mc_rbp);
  thread_context.rsi = uint64_t(mcontext.mc_rsi);
  thread_context.rdi = uint64_t(mcontext.mc_rdi);
  thread_context.r8 = uint64_t(mcontext.mc_r8);
  thread_context.r9 = uint64_t(mcontext.mc_r9);
  thread_context.r10 = uint64_t(mcontext.mc_r10);
  thread_context.r11 = uint64_t(mcontext.mc_r11);
  thread_context.r12 = uint64_t(mcontext.mc_r12);
  thread_context.r13 = uint64_t(mcontext.mc_r13);
  thread_context.r14 = uint64_t(mcontext.mc_r14);
  thread_context.r15 = uint64_t(mcontext.mc_r15);
  std::memcpy(thread_context.xmm_registers, reinterpret_cast<struct savefpu*>(mcontext.mc_fpstate)->sv_xmm,
              sizeof(thread_context.xmm_registers));
#elif REX_ARCH_ARM64
  std::memcpy(thread_context.x, mcontext.regs, sizeof(thread_context.x));
  thread_context.sp = mcontext.sp;
  thread_context.pc = mcontext.pc;
  thread_context.pstate = mcontext.pstate;
  struct fpsimd_context* mcontext_fpsimd = nullptr;
  struct esr_context* mcontext_esr = nullptr;
  for (struct _aarch64_ctx* mcontext_extension =
           reinterpret_cast<struct _aarch64_ctx*>(mcontext.__reserved);
       mcontext_extension->magic;
       mcontext_extension = reinterpret_cast<struct _aarch64_ctx*>(
           reinterpret_cast<uint8_t*>(mcontext_extension) + mcontext_extension->size)) {
    switch (mcontext_extension->magic) {
      case FPSIMD_MAGIC:
        mcontext_fpsimd = reinterpret_cast<struct fpsimd_context*>(mcontext_extension);
        break;
      case ESR_MAGIC:
        mcontext_esr = reinterpret_cast<struct esr_context*>(mcontext_extension);
        break;
      default:
        break;
    }
  }
  assert_not_null(mcontext_fpsimd);
  if (mcontext_fpsimd) {
    thread_context.fpsr = mcontext_fpsimd->fpsr;
    thread_context.fpcr = mcontext_fpsimd->fpcr;
    std::memcpy(thread_context.v, mcontext_fpsimd->vregs, sizeof(thread_context.v));
  }
#endif  // REX_ARCH

  Exception ex;
  switch (signal_number) {
    case SIGILL:
      ex.InitializeIllegalInstruction(&thread_context);
      break;
    case SIGSEGV:
    case SIGBUS: {
      Exception::AccessViolationOperation access_violation_operation;
#if REX_ARCH_AMD64
      access_violation_operation =
          memory_access == platform::ProsperoFaultAccess::kWrite
              ? Exception::AccessViolationOperation::kWrite
              : Exception::AccessViolationOperation::kRead;
#elif REX_ARCH_ARM64
      // For a Data Abort (EC - ESR_EL1 bits 31:26 - 0b100100 from a lower
      // Exception Level, 0b100101 without a change in the Exception Level),
      // bit 6 is 0 for reading from a memory location, 1 for writing to a
      // memory location.
      if (mcontext_esr && ((mcontext_esr->esr >> 26) & 0b111110) == 0b100100) {
        access_violation_operation = (mcontext_esr->esr & (UINT64_C(1) << 6))
                                         ? Exception::AccessViolationOperation::kWrite
                                         : Exception::AccessViolationOperation::kRead;
      } else {
        // Determine the memory access direction based on which instruction has
        // requested it.
        // esr_context may be unavailable on certain hosts (for instance, on
        // Android, it was added only in NDK r16 - which is the first NDK
        // version to support the Android API level 27, while NDK r15 doesn't
        // have esr_context in its API 26 sigcontext.h).
        // On AArch64 (unlike on AArch32), the program counter is the address of
        // the currently executing instruction.
        bool instruction_is_store;
        if (IsArm64LoadPrefetchStore(*reinterpret_cast<const uint32_t*>(mcontext.pc),
                                     instruction_is_store)) {
          access_violation_operation = instruction_is_store
                                           ? Exception::AccessViolationOperation::kWrite
                                           : Exception::AccessViolationOperation::kRead;
        } else {
          assert_always(
              "No ESR in the exception thread context, or it's not a Data "
              "Abort, and the faulting instruction is not a known load, "
              "prefetch or store instruction");
          access_violation_operation = Exception::AccessViolationOperation::kUnknown;
        }
      }
#else
      access_violation_operation = Exception::AccessViolationOperation::kUnknown;
#endif  // REX_ARCH
      ex.InitializeAccessViolation(&thread_context,
                                   reinterpret_cast<uint64_t>(signal_info->si_addr),
                                   access_violation_operation);
    } break;
    default:
      assert_unhandled_case(signal_number);
  }

  for (size_t i = 0; i < rex::countof(handlers_) && handlers_[i].first; ++i) {
    if (handlers_[i].first(&ex, handlers_[i].second)) {
      // Exception handled.
#if REX_ARCH_AMD64
      mcontext.mc_rip = static_cast<__register_t>(thread_context.rip);
      mcontext.mc_rflags = static_cast<__register_t>(thread_context.eflags);
      uint32_t modified_register_index;
      // The order must match the order in X64Register.
      __register_t* const kIntRegisterMap[] = {
          &mcontext.mc_rax, &mcontext.mc_rcx, &mcontext.mc_rdx, &mcontext.mc_rbx,
          &mcontext.mc_rsp, &mcontext.mc_rbp, &mcontext.mc_rsi, &mcontext.mc_rdi,
          &mcontext.mc_r8, &mcontext.mc_r9, &mcontext.mc_r10, &mcontext.mc_r11,
          &mcontext.mc_r12, &mcontext.mc_r13, &mcontext.mc_r14, &mcontext.mc_r15,
      };
      uint16_t modified_int_registers_remaining = ex.modified_int_registers();
      while (rex::bit_scan_forward(modified_int_registers_remaining, &modified_register_index)) {
        modified_int_registers_remaining &= ~(UINT16_C(1) << modified_register_index);
        *kIntRegisterMap[modified_register_index] =
            thread_context.int_registers[modified_register_index];
      }
      uint16_t modified_xmm_registers_remaining = ex.modified_xmm_registers();
      while (rex::bit_scan_forward(modified_xmm_registers_remaining, &modified_register_index)) {
        modified_xmm_registers_remaining &= ~(UINT16_C(1) << modified_register_index);
        std::memcpy(&reinterpret_cast<struct savefpu*>(mcontext.mc_fpstate)->sv_xmm[modified_register_index],
                    &thread_context.xmm_registers[modified_register_index], sizeof(vec128_t));
      }
#elif REX_ARCH_ARM64
      uint32_t modified_register_index;
      uint32_t modified_x_registers_remaining = ex.modified_x_registers();
      while (rex::bit_scan_forward(modified_x_registers_remaining, &modified_register_index)) {
        modified_x_registers_remaining &= ~(UINT32_C(1) << modified_register_index);
        mcontext.regs[modified_register_index] = thread_context.x[modified_register_index];
      }
      mcontext.sp = thread_context.sp;
      mcontext.pc = thread_context.pc;
      mcontext.pstate = thread_context.pstate;
      if (mcontext_fpsimd) {
        mcontext_fpsimd->fpsr = thread_context.fpsr;
        mcontext_fpsimd->fpcr = thread_context.fpcr;
        uint32_t modified_v_registers_remaining = ex.modified_v_registers();
        while (rex::bit_scan_forward(modified_v_registers_remaining, &modified_register_index)) {
          modified_v_registers_remaining &= ~(UINT32_C(1) << modified_register_index);
          std::memcpy(&mcontext_fpsimd->vregs[modified_register_index],
                      &thread_context.v[modified_register_index], sizeof(vec128_t));
          mcontext.regs[modified_register_index] = thread_context.x[modified_register_index];
        }
      }
#endif  // REX_ARCH
      return;
    }
  }
  ForwardUnhandledException(signal_number, signal_info, signal_context);
}

void ExceptionHandler::Install(Handler fn, void* data) {
  if (!fn) throw std::invalid_argument("Cannot install a null PS5 exception handler");
  size_t slot = 0;
  while (slot < kMaxHandlerCount && handlers_[slot].first) ++slot;
  if (slot == kMaxHandlerCount) {
    throw std::runtime_error("Too many PS5 exception handlers installed");
  }
  const bool first_installation = !signal_handlers_installed_;
  if (!signal_handlers_installed_) {
    platform::InitializeProsperoFaultDiagnostics();
    struct sigaction signal_handler;

    std::memset(&signal_handler, 0, sizeof(signal_handler));
    signal_handler.sa_sigaction = ExceptionHandlerCallback;
    signal_handler.sa_flags = SA_SIGINFO;

    sigemptyset(&signal_handler.sa_mask);
    const int signals[] = {SIGILL, SIGSEGV, SIGBUS};
    struct sigaction* originals[] = {&original_sigill_handler_, &original_sigsegv_handler_,
                                     &original_sigbus_handler_};
    for (size_t index = 0; index < 3; ++index) {
      if (sigaction(signals[index], &signal_handler, originals[index]) != 0) {
        const int failure = errno;
        while (index) {
          --index;
          sigaction(signals[index], originals[index], nullptr);
        }
        throw std::system_error(failure, std::generic_category(),
                                "Cannot install PS5 memory exception handlers");
      }
    }
    signal_handlers_installed_ = true;
  }
  // Publish the callback after its context is initialized and after every
  // required sigaction succeeded; Release builds reject a full table too.
  handlers_[slot].second = data;
  handlers_[slot].first = fn;
  if (first_installation) {
    const std::string path = std::string(prospero_state_root()) + "/logs/fault.log";
    const std::string report = platform::ProsperoFaultLogAvailable()
                                   ? path : "unavailable (" + path + "; fallback=fd2)";
    prospero_startup_log(("runtime: PS5 fault handlers ready (v3.4); report=" + report).c_str());
  }
}

void ExceptionHandler::Uninstall(Handler fn, void* data) {
  for (size_t i = 0; i < rex::countof(handlers_); ++i) {
    if (handlers_[i].first == fn && handlers_[i].second == data) {
      for (; i < rex::countof(handlers_) - 1; ++i) {
        handlers_[i] = handlers_[i + 1];
      }
      handlers_[i].first = nullptr;
      handlers_[i].second = nullptr;
      break;
    }
  }

  bool has_any = false;
  for (size_t i = 0; i < rex::countof(handlers_); ++i) {
    if (handlers_[i].first) {
      has_any = true;
      break;
    }
  }
  if (!has_any) {
    if (signal_handlers_installed_) {
      if (sigaction(SIGILL, &original_sigill_handler_, NULL) != 0) {
        assert_always("Failed to restore original SIGILL handler");
      }
      if (sigaction(SIGSEGV, &original_sigsegv_handler_, NULL) != 0) {
        assert_always("Failed to restore original SIGSEGV handler");
      }
      if (sigaction(SIGBUS, &original_sigbus_handler_, NULL) != 0) {
        assert_always("Failed to restore original SIGBUS handler");
      }
      signal_handlers_installed_ = false;
    }
  }
}

}  // namespace rex::arch

#endif  // REX_PLATFORM_PROSPERO
