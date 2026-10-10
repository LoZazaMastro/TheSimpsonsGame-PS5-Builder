// Exercise the real MMIO exception callback in the PS5 compile branch.
// Native exception delivery and memory queries are injected here; instruction
// decoding, callback routing and permission checks are production code.
#include <rex/exception_handler.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/system/mmio_handler.h>

#include <cstdio>
#include <cstdlib>

namespace {
using rex::arch::Exception;
using rex::memory::PageAccess;
rex::arch::ExceptionHandler::Handler installed_handler;
void* installed_context;
PageAccess queried_access;
bool query_succeeds;
bool recovery_succeeds;
unsigned queries, callbacks, translations, checks;
constexpr uintptr_t kBase = 0x100000000ull;
constexpr uintptr_t kPhysical = 0x200000000ull;
constexpr uintptr_t kEnd = kPhysical + 0x1FFFFFFFull;  // Caller uses inclusive end.

void Check(bool value, const char* message) {
  if (!value) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
  }
  ++checks;
}
uint32_t Translate(const void*, const void* address) {
  ++translations;
  return uint32_t(reinterpret_cast<uintptr_t>(address) - kBase);
}
bool Recover(std::unique_lock<std::recursive_mutex> lock, void* context,
             void*, bool) {
  Check(lock.owns_lock(), "recovery receives the held critical-region lock");
  Check(context == &recovery_succeeds, "recovery context preserved");
  ++callbacks;
  return recovery_succeeds;
}
bool Fault(uintptr_t address, Exception::AccessViolationOperation operation) {
  rex::arch::HostThreadContext context{};
  context.rip = 0x12345678;
  Exception exception;
  exception.InitializeAccessViolation(&context, address, operation);
  return installed_handler(&exception, installed_context);
}
void Reset(PageAccess access, bool success = true, bool recover = false) {
  queried_access = access;
  query_succeeds = success;
  recovery_succeeds = recover;
  queries = callbacks = translations = 0;
}
}  // namespace

namespace rex {
spdlog::logger* GetLoggerRaw(LogCategoryId) { return nullptr; }
LogCategoryId RegisterLogCategory(const char*) { return LogCategoryId{0}; }
namespace memory {
size_t page_size() { return 0x4000; }
bool QueryProtect(void*, size_t& length, PageAccess& access) {
  ++queries;
  // Deliberately retain a writable output even on failure in one test: the
  // public bool result, not an incidental backend output, must govern retry.
  access = queried_access;
  length = query_succeeds ? 0x4000 : 0;
  return query_succeeds;
}
}  // namespace memory
namespace arch {
void ExceptionHandler::Install(Handler callback, void* context) {
  Check(!installed_handler, "one installed handler");
  installed_handler = callback;
  installed_context = context;
}
void ExceptionHandler::Uninstall(Handler callback, void* context) {
  Check(callback == installed_handler && context == installed_context,
        "matching uninstall");
  installed_handler = nullptr;
  installed_context = nullptr;
}
}  // namespace arch
}  // namespace rex

int main() {
  auto handler = rex::runtime::MMIOHandler::Install(
      reinterpret_cast<uint8_t*>(kBase), reinterpret_cast<uint8_t*>(kPhysical),
      reinterpret_cast<uint8_t*>(kEnd), Translate, nullptr, Recover,
      &recovery_succeeds);
  Check(handler && installed_handler, "production handler installed");
  using Operation = Exception::AccessViolationOperation;

  for (PageAccess access : {PageAccess::kReadOnly, PageAccess::kExecuteReadOnly}) {
    Reset(access);
    Check(Fault(kBase + 0x1000, Operation::kRead), "readable page retries read");
    Check(queries == 1 && callbacks == 0, "read retry needs successful query");
    Reset(access);
    Check(!Fault(kBase + 0x1000, Operation::kWrite), "read-only page cannot retry write");
    Check(callbacks == 1, "read-only write reaches recovery callback");
  }
  for (PageAccess access : {PageAccess::kReadWrite, PageAccess::kExecuteReadWrite}) {
    Reset(access);
    Check(Fault(kBase + 0x1000, Operation::kWrite), "writable page retries write");
    Check(queries == 1 && callbacks == 0, "write retry avoids redundant callback");
  }
  for (PageAccess output : {PageAccess::kNoAccess, PageAccess::kReadWrite}) {
    Reset(output, false);
    Check(!Fault(kBase + 0x1000, Operation::kWrite), "failed query never proves retry safe");
    Check(callbacks == 1, "failed query reaches callback");
  }
  for (Operation operation : {Operation::kRead, Operation::kWrite}) {
    Reset(PageAccess::kNoAccess);
    Check(!Fault(kBase + 0x1000, operation), "no-access page remains unhandled");
    Check(callbacks == 1, "no-access callback invoked");
    Reset(PageAccess::kNoAccess, true, true);
    Check(Fault(kBase + 0x1000, operation), "actual callback recovery accepted");
  }
  for (uintptr_t address : {kBase - 1, kEnd + 1}) {
    Reset(PageAccess::kReadWrite);
    Check(!Fault(address, Operation::kWrite), "outside mapping rejected");
    Check(queries == 0 && callbacks == 0 && translations == 0, "bounds checked first");
  }
  for (uintptr_t address : {kBase, kEnd}) {
    Reset(PageAccess::kReadWrite);
    Check(Fault(address, Operation::kWrite), "inclusive first/last bytes accepted");
    Check(queries == 1, "boundary query performed");
  }
  Reset(PageAccess::kReadWrite);
  Check(!Fault(kBase, Operation::kUnknown), "non-data fault cannot enter recovery");
  Check(queries == 0 && callbacks == 0, "unknown operation bypasses page query");
  rex::arch::HostThreadContext context{};
  Exception illegal;
  illegal.InitializeIllegalInstruction(&context);
  Check(!installed_handler(&illegal, installed_context), "illegal instruction not treated as memory write");
  handler.reset();
  Check(!installed_handler, "handler removed");
  std::printf("PASS: real MMIO fault callback, %u checks; permissions, failed queries, recovery and mapping bounds\n", checks);
}
