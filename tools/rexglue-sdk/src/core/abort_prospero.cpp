// BSD 3-Clause License. See the project LICENSE.
// Keep the program's abort call site before libkernel replaces RIP with its
// own signal/abort context. The native abort still determines termination.
#include <rex/platform/fault_prospero.h>

extern "C" [[noreturn]] void __real_abort();

extern "C" [[noreturn]] __attribute__((noinline)) void __wrap_abort() {
  const auto caller = reinterpret_cast<uintptr_t>(
      __builtin_extract_return_addr(__builtin_return_address(0)));
  rex::platform::WriteProsperoAbortCaller(caller);
  __real_abort();
}

// The local dlmalloc unit calls this only at its existing corruption/usage
// checks. Recording a failure never permits allocation to continue.
extern "C" [[noreturn]] __attribute__((noinline)) void prospero_heap_failure(
    const char* kind, const char* operation, unsigned line, void* arena, void* pointer) {
  rex::platform::WriteProsperoHeapFailure(kind, operation, line,
      reinterpret_cast<uintptr_t>(arena), reinterpret_cast<uintptr_t>(pointer));
  __wrap_abort();
}
