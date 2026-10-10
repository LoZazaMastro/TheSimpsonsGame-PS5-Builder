// Exercise the production bounded writer and classifier. A real Linux signal
// checks output/termination; its ucontext is NOT a test of the PS5 ABI.
#include <rex/platform/fault_prospero.h>

#include <array>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <unistd.h>

using namespace rex::platform;

static void RealFaultHandler(int signal_number, siginfo_t* info, void* context) {
  ProsperoFaultRecord record{};
  record.signal_number = signal_number;
  record.signal_code = info->si_code;
  record.fault_address = uintptr_t(info->si_addr);
  record.context_present = context != nullptr;
#if defined(__linux__) && defined(__x86_64__)
  const auto& machine = static_cast<ucontext_t*>(context)->uc_mcontext;
  record.rip = uint64_t(machine.gregs[REG_RIP]);
  record.rsp = uint64_t(machine.gregs[REG_RSP]);
  record.rbp = uint64_t(machine.gregs[REG_RBP]);
  record.page_fault_error = uint64_t(machine.gregs[REG_ERR]);
  record.registers[4] = record.rsp;
  record.registers[5] = record.rbp;
#endif
  WriteProsperoFaultRecord(record);
  // A second dispatch must not duplicate a terminal record.
  WriteProsperoFaultRecord(record);
  signal(signal_number, SIG_DFL);
  raise(signal_number);
}

static std::string Read(const std::string& path) {
  std::ifstream input(path);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

int main() {
  const rlimit no_core{0, 0};
  assert(setrlimit(RLIMIT_CORE, &no_core) == 0);
  constexpr uint64_t kSdkPageFault = 12;
  using A = ProsperoFaultAccess;
  assert(ClassifyProsperoMemoryFault(SIGSEGV, kSdkPageFault, 0, kSdkPageFault) == A::kRead);
  assert(ClassifyProsperoMemoryFault(SIGSEGV, kSdkPageFault, 2, kSdkPageFault) == A::kWrite);
  assert(ClassifyProsperoMemoryFault(SIGSEGV, kSdkPageFault, 0x10, kSdkPageFault) == A::kUnhandled);
  assert(ClassifyProsperoMemoryFault(SIGSEGV, kSdkPageFault, 0x12, kSdkPageFault) == A::kUnhandled);
  assert(ClassifyProsperoMemoryFault(SIGBUS, kSdkPageFault, 2, kSdkPageFault) == A::kWrite);
  assert(ClassifyProsperoMemoryFault(SIGBUS, 9, 2, kSdkPageFault) == A::kUnhandled);
  assert(ClassifyProsperoMemoryFault(SIGBUS, 14, 0, kSdkPageFault) == A::kUnhandled);
  assert(ClassifyProsperoMemoryFault(SIGBUS, kSdkPageFault, 0x10, kSdkPageFault) == A::kUnhandled);
  assert(ClassifyProsperoMemoryFault(SIGILL, kSdkPageFault, 0, kSdkPageFault) == A::kUnhandled);
  puts("PASS data, NX and non-page-fault SIGBUS classification");

  ProsperoFaultRecord record{};
  record.signal_number = 11;
  record.signal_code = std::numeric_limits<int32_t>::min();
  record.fault_address = 0xffffffffffffffffULL;
  record.rip = 0x0123456789abcdefULL;
  record.rsp = 0xfedcba9876543210ULL;
  record.image_base_known = true;
  record.context_present = true;
  std::array<char, 1030> buffer;
  buffer.fill('!');
  size_t used = FormatProsperoFaultRecord(record, buffer.data() + 3, 1024);
  assert(used < 1024);
  assert(buffer[0] == '!' && buffer[2] == '!' && buffer[1027] == '!');
  const std::string text(buffer.data() + 3, used);
  assert(text.find("si_code=-2147483648") != std::string::npos);
  assert(text.find("fault=0xffffffffffffffff") != std::string::npos);
  assert(text.find("RIP=0x0123456789abcdef") != std::string::npos);
  assert(text.find("image_base_known=1 image_base=0x0000000000000000") != std::string::npos);
  assert(text.back() == '\n');
  for (size_t capacity : {size_t(0), size_t(1), size_t(31)}) {
    buffer.fill('!');
    used = FormatProsperoFaultRecord(record, buffer.data() + 3, capacity);
    assert(used <= capacity && buffer[2] == '!' && buffer[capacity + 3] == '!');
  }
  puts("PASS bounded formatting, full 64-bit addresses and known-zero image base");

  errno = 0;
  assert(!OpenProsperoFaultLog("relative.log") && errno == EINVAL);
  char directory[] = "/tmp/simpsons-fault-report-XXXXXX";
  assert(mkdtemp(directory));
  const std::string root = directory;
  const std::string path = root + "/fault.log";
  const std::string fifo = root + "/not-a-log";
  assert(mkfifo(fifo.c_str(), 0600) == 0);
  assert(!OpenProsperoFaultLog(fifo.c_str()));
  assert(!OpenProsperoFaultLog(root.c_str()));
  { std::ofstream initial(path); initial << "previous session\n"; }
  const std::string link = root + "/symlink.log";
  assert(symlink(path.c_str(), link.c_str()) == 0);
  assert(!OpenProsperoFaultLog(link.c_str()));
  puts("PASS bad/nonregular destinations rejected without replacing existing data");

  const size_t page = size_t(sysconf(_SC_PAGESIZE));
  void* memory = mmap(nullptr, page, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(memory != MAP_FAILED);
  const pid_t child = fork();
  assert(child >= 0);
  if (child == 0) {
    if (!OpenProsperoFaultLog(path.c_str())) _exit(100);
    // Rename after opening proves the handler uses the pre-opened descriptor,
    // not a pathname opened again from the signal.
    if (rename(path.c_str(), (root + "/captured.log").c_str()) != 0) _exit(101);
    struct sigaction action{};
    action.sa_sigaction = RealFaultHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &action, nullptr) != 0) _exit(102);
    *static_cast<volatile uint8_t*>(memory) = 1;
    _exit(103);
  }
  int status = 0;
  assert(waitpid(child, &status, 0) == child);
  assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV);
  const std::string captured = Read(root + "/captured.log");
  assert(captured.starts_with("previous session\n"));
  const size_t marker = captured.find("[PPSA74207] terminal fault:");
  assert(marker != std::string::npos);
  assert(captured.find("[PPSA74207] terminal fault:", marker + 1) == std::string::npos);
  assert(captured.find("context_present=1") != std::string::npos);
  assert(captured.find("TRACE BEGIN") != std::string::npos);
  assert(captured.find("TRACE BEGIN", captured.find("TRACE BEGIN") + 1) == std::string::npos);
  assert(captured.find("RAW STACK unavailable") != std::string::npos);
  char expected[40];
  std::snprintf(expected, sizeof(expected), "fault=0x%016llx", (unsigned long long)uintptr_t(memory));
  assert(captured.find(expected) != std::string::npos);
#if defined(__linux__) && defined(__x86_64__)
  assert(captured.find("RIP=0x0000000000000000") == std::string::npos);
  assert(captured.find("RSP=0x0000000000000000") == std::string::npos);
#endif
  assert(munmap(memory, page) == 0);
  unlink((root + "/captured.log").c_str());
  unlink(fifo.c_str());
  unlink(link.c_str());
  rmdir(root.c_str());
  puts("PASS real host fault: append via retained FD, once only, original fatal signal preserved");
}
