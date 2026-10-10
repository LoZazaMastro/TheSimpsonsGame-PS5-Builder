// Native PS5 terminal-fault diagnostics. The writer is also host-testable.
#pragma once

#include <cstddef>
#include <cstdint>
#include <signal.h>

namespace rex::platform {

enum class ProsperoFaultAccess : uint8_t { kUnhandled, kRead, kWrite };

// page_fault_trap comes from the target's T_PAGEFLT, not an assumed x86
// vector. SIGBUS alignment/device faults and instruction fetches are excluded.
ProsperoFaultAccess ClassifyProsperoMemoryFault(int signal_number, uint64_t trap_number,
                                             uint64_t error, uint64_t page_fault_trap) noexcept;

struct ProsperoFaultRecord {
  int32_t signal_number = 0;
  int32_t signal_code = 0;
  uint64_t fault_address = 0;
  uint64_t rip = 0;
  uint64_t rsp = 0;
  uint64_t rbp = 0;
  uint64_t flags = 0;
  uint64_t trap_number = 0;
  uint64_t page_fault_error = 0;
  // Instruction-encoding order: RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
  // R8, R9, R10, R11, R12, R13, R14, R15.
  uint64_t registers[16] = {};
  uint64_t image_base = 0;
  uint64_t reporter_address = 0;
  bool image_base_known = false;
  bool context_present = false;
};

// Normal startup only. Opens a regular append-only destination and retains
// its descriptor for the process lifetime, including faults during shutdown.
// Failure returns false with errno set; no directory or permission changes.
bool OpenProsperoFaultLog(const char* path) noexcept;
bool ProsperoFaultLogAvailable() noexcept;

// Pure bounded formatting, with no libc formatting, heap or locks. Returns
// bytes written (no terminating NUL is required); never exceeds capacity.
size_t FormatProsperoFaultRecord(const ProsperoFaultRecord& record,
                               char* destination, size_t capacity) noexcept;

// Signal context: records only the first terminal fault in this process,
// freezes/dumps the fixed trace ring, and attempts a separately guarded raw
// stack dump only inside registered live current-thread stack bounds.
// Uses fixed buffers, lock-free flags and bounded write attempts to the
// descriptor opened above. Failed fault-record output is mirrored to fd 2.
// No open, allocation, mutex, stdio, fsync, path lookup or stack unwinding.
void WriteProsperoFaultRecord(const ProsperoFaultRecord& record) noexcept;

// The link wrapper calls this before the real abort. It records the return
// address of the first explicit abort call in linked code, independently of
// the following signal record. Dumps the fixed event ring and bounded known
// current stack before native abort replaces the machine context.
// Native-module internal calls may bypass it.
// No TLS, allocation, locks or unwinding; the same retained descriptor is used.
void WriteProsperoAbortCaller(uintptr_t return_address) noexcept;

// Allocator-detected failures, recorded before abort changes the machine
// context. The supplied labels are static strings; pointers are formatted
// without dereferencing potentially corrupt heap memory. Uses the same
// retained descriptor and bounded, allocation-free writer as fault records.
void WriteProsperoHeapFailure(const char* kind, const char* operation, unsigned line,
                             uintptr_t arena, uintptr_t pointer) noexcept;

// PS5 context adapter and normal initialization, implemented alongside the
// architecture exception handler. Log-open failure is diagnosed normally and
// leaves fd 2 as a best-effort fallback; it does not block the game.
void InitializeProsperoFaultDiagnostics();
void ReportProsperoFatalSignal(int signal_number, const siginfo_t* info,
                              const void* context) noexcept;

}  // namespace rex::platform
