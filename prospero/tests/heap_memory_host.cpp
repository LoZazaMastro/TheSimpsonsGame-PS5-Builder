// Real guest heaps + PS5 backend + SDK host kernel. No console/GPU execution.
#include <rex/system/xmemory.h>
#include <rex/logging.h>
#include <rex/chrono/clock.h>
#include <ps5platform/kernel.h>
#include <ps5platform/shm.h>
extern "C" {
#include "host_kernel.h"
}
#include <array>
#include <cassert>
#include <cstdio>
#include <chrono>
using namespace rex::memory;

// Only unrelated services use adapters; production heap/cvar/watch/backend
// code is compiled unchanged into this executable.
namespace rex {
LogCategoryId RegisterLogCategory(const char*) { return LogCategoryId(0); }
spdlog::logger* GetLoggerRaw(LogCategoryId) { return spdlog::default_logger_raw(); }
}
namespace rex::chrono {
uint64_t Clock::QueryHostTickCount() { return std::chrono::steady_clock::now().time_since_epoch().count(); }
uint64_t Clock::QueryHostTickFrequency() { return 1000000000; }
}
namespace rex::runtime {
// Memory's MMIO handler remains null in this harness.
MMIOHandler::~MMIOHandler() = default;
thread_local uintptr_t g_access_violation_pc;
}
static bool fail_query;
extern "C" int32_t sceKernelQueryMemoryProtection(void* address, void** start, void** end,
                                                   uint32_t* protection) {
  if (fail_query) return -1;
  FILE* maps = std::fopen("/proc/self/maps", "r");
  if (!maps) return -1;
  char line[1024];
  const auto at = reinterpret_cast<uintptr_t>(address);
  int result = -1;
  while (std::fgets(line, sizeof(line), maps)) {
    unsigned long long low, high;
    char flags[5]{};
    if (std::sscanf(line, "%llx-%llx %4s", &low, &high, flags) != 3 || at < low || at >= high) continue;
    *start = reinterpret_cast<void*>(low);
    *end = reinterpret_cast<void*>(high);
    *protection = (flags[0] == 'r' ? PS5_KERNEL_PROT_CPU_READ : 0) |
                  (flags[1] == 'w' ? PS5_KERNEL_PROT_CPU_WRITE : 0) |
                  (flags[2] == 'x' ? PS5_KERNEL_PROT_CPU_EXEC : 0);
    result = 0;
    break;
  }
  std::fclose(maps);
  return result;
}
#if !defined(REX_PROSPERO_HEAP_BASELINE)
namespace rex::memory {
class ProsperoHeapHostTest {
 public:
  static void Initialize(Memory& memory, uint8_t* base) {
    memory.virtual_membase_ = base;
    memory.physical_membase_ = base + 0x100000000ull;
    memory.heaps_.v00000000.Initialize(&memory, base, HeapType::kGuestVirtual, 0, 0x40000, 0x1000);
    memory.heaps_.physical.Initialize(&memory, memory.physical_membase_, HeapType::kGuestPhysical,
                                      0, 0x20000000, 0x1000);
    memory.heaps_.vA0000000.Initialize(&memory, base, HeapType::kGuestPhysical,
                                      0xA0000000, 0x20000000, 0x10000, &memory.heaps_.physical);
    memory.heaps_.vC0000000.Initialize(&memory, base, HeapType::kGuestPhysical,
                                      0xC0000000, 0x20000000, 0x1000000, &memory.heaps_.physical);
    memory.heaps_.vE0000000.Initialize(&memory, base, HeapType::kGuestPhysical,
                                      0xE0000000, 0x1FD00000, 0x1000, &memory.heaps_.physical);
  }
  static bool Fault(Memory& memory, void* address, bool write) {
    return memory.AccessViolationCallback(rex::thread::global_critical_region::AcquireDirect(), address, write);
  }
};
}
#endif
static PageAccess HostAccess(void* address) {
  size_t length = 0;
  PageAccess access;
  assert(QueryProtect(address, length, access));
  return access;
}
static uint32_t GuestAccess(BaseHeap* heap, uint32_t address) {
  uint32_t access = UINT32_MAX;
  assert(heap->QueryProtect(address, &access));
  return access;
}
constexpr uint32_t kRW = kMemoryProtectRead | kMemoryProtectWrite;
constexpr uint32_t kRO = kMemoryProtectRead;
constexpr uint32_t kCommit = kMemoryAllocationReserve | kMemoryAllocationCommit;
#if !defined(REX_PROSPERO_HEAP_BASELINE)
struct WatchEvents { unsigned calls = 0; uint32_t begin = 0; uint32_t length = 0; };
static std::pair<uint32_t, uint32_t> OnWatch(void* context, uint32_t begin, uint32_t length, bool) {
  auto& events = *static_cast<WatchEvents*>(context);
  ++events.calls;
  events.begin = begin;
  events.length = length;
  return {begin, length};
}
static void TestVirtual(Memory& memory, uint8_t* base) {
  auto* heap = memory.LookupHeap(0x10000);
  assert(heap->AllocFixed(0x10000, 0x8000, 0x1000, kCommit, kRW));
  base[0x10000] = 1;
  base[0x11000] = 2;
  uint32_t old = UINT32_MAX;
  assert(heap->Protect(0x10000, 0x1000, kRO, &old));
  assert(old == kRW && GuestAccess(heap, 0x10000) == kRO);
  assert(GuestAccess(heap, 0x11000) == kRW && HostAccess(base + 0x10000) == PageAccess::kReadWrite);
  base[0x11000] = 3;
  assert(heap->Protect(0x11000, 0x3000, kRO));
  assert(HostAccess(base + 0x10000) == PageAccess::kReadOnly);
  assert(heap->Protect(0x10000, 0x1000, kRO, &old) && old == kRO);
  assert(!ProsperoHeapHostTest::Fault(memory, base + 0x10000, true));
  assert(HostAccess(base + 0x10000) == PageAccess::kReadOnly);
  assert(heap->Protect(0x11000, 0x1000, kRW, &old) && old == kRO);
  assert(HostAccess(base + 0x10000) == PageAccess::kReadWrite);
  assert(heap->Decommit(0x11000, 0x1000));
  assert(HostAccess(base + 0x10000) == PageAccess::kReadOnly);
  assert(heap->AllocFixed(0x11000, 0x1000, 0x1000, kMemoryAllocationCommit, kRW));
  assert(base[0x10000] == 1 && base[0x11000] == 3);
  assert(HostAccess(base + 0x10000) == PageAccess::kReadWrite);
  assert(heap->Protect(0x13FFF, 2, kRO));
  assert(GuestAccess(heap, 0x13000) == kRO && GuestAccess(heap, 0x14000) == kRO);
  assert(GuestAccess(heap, 0x15000) == kRW);
  assert(heap->AllocFixed(0x20000, 0x1000, 0x1000, kCommit, kRW));
  assert(heap->AllocFixed(0x21000, 0x1000, 0x1000, kCommit, kRO));
  assert(GuestAccess(heap, 0x20000) == kRW && HostAccess(base + 0x20000) == PageAccess::kReadWrite);
  base[0x20000] = 0xA9;
  old = 0xAB;
  fail_query = true;
  assert(!heap->Protect(0x10000, 0x8000, kRO, &old));
  fail_query = false;
  assert(old == 0xAB && GuestAccess(heap, 0x11000) == kRW);
  host_fail(HOST_CALL_PROTECT, 1);
  assert(!heap->Protect(0x10000, 0x8000, kRO, &old));
  assert(old == 0xAB && GuestAccess(heap, 0x11000) == kRW);
  assert(HostAccess(base + 0x10000) == PageAccess::kReadWrite);
  assert(!heap->Protect(0x3FFFF, 2, kRW));
  assert(!heap->AllocFixed(0x40000, 0x1000, 0x1000, kCommit, kRW));
  assert(!heap->QueryProtect(0x40000, &old));
  assert(!heap->Decommit(0x40000, 0x1000));
  assert(!heap->Release(0x40000));
  assert(rex::cvar::SetFlagByName("protect_on_release", "true"));
  uint32_t released = 0xCD;
  host_fail(HOST_CALL_PROTECT, 1);
  assert(!heap->Release(0x10000, &released) && released == 0xCD);
  assert(GuestAccess(heap, 0x11000) == kRW);
  assert(heap->Release(0x10000, &released) && released == 0x8000);
  assert(HostAccess(base + 0x10000) == PageAccess::kNoAccess);
  assert(!heap->Release(0x10000));
  assert(rex::cvar::SetFlagByName("protect_on_release", "false"));
  std::puts("PASS guest4KiB/host16KiB protection, neighbours, oldProtect, recommit/release, bounds, current-RO recovery");
}
static void TestPhysical(Memory& memory, uint8_t* base) {
  auto* heap = static_cast<PhysicalHeap*>(memory.LookupHeap(0xE0000000));
  auto* parent = memory.GetPhysicalHeap();
  assert(heap->host_address_offset() == 0x1000);
  assert(heap->AllocFixed(0xE0000000, 0x1000, 0x1000, kCommit, kRO));
  assert(heap->AllocFixed(0xE0001000, 0x1000, 0x1000, kCommit, kRW));
  base[0xE0002000] = 0x5A;
  assert(base[0xA0002000] == 0x5A && base[0xC0002000] == 0x5A && base[0x100002000ull] == 0x5A);
  assert(GuestAccess(parent, 0x2000) == kRW);
  uint32_t old = UINT32_MAX;
  assert(heap->Protect(0xE0001000, 0x1000, kRO, &old) && old == kRW);
  assert(GuestAccess(parent, 0x2000) == kRO && HostAccess(base + 0xE0000000) == PageAccess::kReadOnly);
  *memory.TranslatePhysical(0x2000) = 0x6D;  // Real GPU-readback-style host write.
  assert(base[0xE0002000] == 0x6D && HostAccess(base + 0xE0000000) == PageAccess::kReadOnly);
  assert(HostAccess(memory.TranslatePhysical(0x2000)) == PageAccess::kReadWrite);
  assert(heap->Protect(0xE0001000, 0x1000, kRW));
  WatchEvents events;
  auto* watch = memory.RegisterPhysicalMemoryInvalidationCallback(OnWatch, &events);
  assert(heap->EnableAccessCallbacks(0x2000, 0x1000, true, false));
  assert(HostAccess(base + 0xE0000000) == PageAccess::kReadOnly);
  assert(!ProsperoHeapHostTest::Fault(memory, base + 0xE0001000, true));
  assert(events.calls == 0);
  assert(heap->AllocFixed(0xE0002000, 0x1000, 0x1000, kCommit, kRW));
  assert(HostAccess(base + 0xE0000000) == PageAccess::kReadOnly);
  assert(ProsperoHeapHostTest::Fault(memory, base + 0xE0003000, true));
  assert(events.calls == 1 && events.begin == 0x1000 && events.length == 0x3000);
  assert(HostAccess(base + 0xE0000000) == PageAccess::kReadWrite);
  base[0xE0003000] = 0x6B;
  assert(base[0x100003000ull] == 0x6B);
  assert(heap->AllocFixed(0xE0003000, 0x1000, 0x1000, kCommit, kRW));
  assert(heap->EnableAccessCallbacks(0x4000, 0x1000, true, false));
  assert(HostAccess(base + 0xE0000000) == PageAccess::kReadWrite);
  assert(HostAccess(base + 0xE0004000) == PageAccess::kReadOnly);
  assert(ProsperoHeapHostTest::Fault(memory, base + 0xE0004000, true));
  assert(events.begin == 0x4000 && events.length == 0x4000);
  assert(heap->AllocFixed(0xE0007000, 0xC000, 0x1000, kCommit, kRW));
  assert(heap->Protect(0xE000B000, 0x4000, kRO));
  old = 0xBC;
  host_fail(HOST_CALL_PROTECT, 2);
  assert(!heap->Protect(0xE0007000, 0xC000, kRO, &old));
  assert(old == 0xBC && GuestAccess(heap, 0xE0007000) == kRW && GuestAccess(parent, 0x8000) == kRW);
  assert(HostAccess(base + 0xE0008000) == PageAccess::kReadWrite);
  assert(HostAccess(base + 0x100008000ull) == PageAccess::kReadWrite);
  assert(HostAccess(base + 0xE0010000) == PageAccess::kReadWrite);
  assert(HostAccess(base + 0xE000C000) == PageAccess::kReadOnly);
  assert(GuestAccess(heap, 0xE000B000) == kRO && GuestAccess(parent, 0xC000) == kRO);
  host_fail(HOST_CALL_PROTECT, 1);
  assert(!heap->EnableAccessCallbacks(0x8000, 0x4000, true, false));
  assert(!heap->TriggerCallbacks(rex::thread::global_critical_region::AcquireDirect(),
                                 0xE0007000, 1, true, false));
  assert(HostAccess(base + 0xE0008000) == PageAccess::kReadWrite);
  assert(heap->EnableAccessCallbacks(0x8000, 0x4000, true, false));
  host_fail(HOST_CALL_PROTECT, 1);
  bool succeeded = true;
  assert(!heap->TriggerCallbacks(rex::thread::global_critical_region::AcquireDirect(),
                                 0xE0007000, 1, true, false, true, &succeeded));
  assert(!succeeded && HostAccess(base + 0xE0008000) == PageAccess::kReadOnly);
  assert(heap->TriggerCallbacks(rex::thread::global_critical_region::AcquireDirect(),
                                0xE0007000, 1, true, false));
  assert(HostAccess(base + 0xE0008000) == PageAccess::kReadWrite);
  // Failure while allocating must publish neither the parent nor alias pages.
  const uint32_t free_before = parent->unreserved_page_count();
  host_fail(HOST_CALL_PROTECT, 1);
  assert(!heap->AllocFixed(0xE001F000, 0x1000, 0x1000, kCommit, kRO));
  assert(parent->unreserved_page_count() == free_before && GuestAccess(parent, 0x20000) == 0);
  assert(GuestAccess(heap, 0xE001F000) == 0);
  assert(heap->AllocFixed(0xE001F000, 0x1000, 0x1000, kCommit, kRO));
  // A 64KiB guest page must update all sixteen parent4KiB pages together.
  auto* a = static_cast<PhysicalHeap*>(memory.LookupHeap(0xA0100000));
  uint32_t allocation = 0;
  assert(a->AllocRange(0xA0100000, 0xA010FFFF, 0x10000, 0, kCommit, kRW, false, &allocation));
  assert(allocation == 0xA0100000);
  assert(a->Protect(allocation + 0x3000, 0x1000, kRO, &old) && old == kRW);
  assert(GuestAccess(parent, 0x100000) == kRO && GuestAccess(parent, 0x10F000) == kRO);
  assert(HostAccess(base + allocation) == PageAccess::kReadOnly);
  assert(a->Protect(allocation, 0x10000, kRW));
  memory.EnablePhysicalMemoryAccessCallbacks(0x100000, 0x1000, true, false);
  assert(HostAccess(base + 0xA0100000) == PageAccess::kReadOnly);
  assert(HostAccess(base + 0xC0100000) == PageAccess::kReadOnly);
  assert(HostAccess(base + 0xE0100000) == PageAccess::kReadOnly);
  assert(ProsperoHeapHostTest::Fault(memory, base + 0xE0100000, true));
  assert(events.begin == 0x100000 && events.length == 0x4000);
  assert(HostAccess(base + 0xA0100000) == PageAccess::kReadOnly);
  assert(ProsperoHeapHostTest::Fault(memory, base + 0xA0100000, true));
  assert(ProsperoHeapHostTest::Fault(memory, base + 0xC0100000, true));
  // The final E host16KiB contains only ONE valid guest4KiB page.
  assert(heap->AllocFixed(0xFFCFF000, 0x1000, 0x1000, kCommit, kRW));
  assert(heap->EnableAccessCallbacks(0x1FD00000, 0x1000, true, false));
  assert(ProsperoHeapHostTest::Fault(memory, base + 0xFFD00000, true));
  assert(events.begin == 0x1FD00000 && events.length == 0x1000);
  assert(!heap->QueryProtect(0xFFD00000, &old));
  assert(!heap->Protect(0xFFCFFFFF, 2, kRO));
  assert(!heap->AllocFixed(0xFFD00000, 0x1000, 0x1000, kCommit, kRW));
  assert(rex::cvar::SetFlagByName("protect_on_release", "true"));
  assert(heap->Release(0xFFCFF000));
  assert(HostAccess(base + 0xFFD00000) == PageAccess::kNoAccess);
  assert(HostAccess(base + 0x11FD00000ull) == PageAccess::kReadWrite);
  assert(rex::cvar::SetFlagByName("protect_on_release", "false"));
  memory.UnregisterPhysicalMemoryInvalidationCallback(watch);
  std::puts("PASS canonical/A/C/E aliases, raw readback, E first/last page, mixed rights, watches, allocation rollback");
}
#endif
int main() {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  auto* arena = static_cast<uint8_t*>(AllocFixed(nullptr, 0x120000000ull,
                        AllocationType::kReserve, PageAccess::kNoAccess));
  assert(arena);
  constexpr size_t bytes = 0x1000000;
  const auto object = CreateFileMappingHandle("heap-host", bytes, PageAccess::kReadWrite, false);
  assert(object != kFileMappingHandleInvalid);
  constexpr std::array<size_t, 5> offsets{0, 0xA0000000, 0xC0000000, 0xE0000000, 0x100000000ull};
  for (size_t offset : offsets)
    assert(MapFileView(object, arena + offset, bytes, PageAccess::kReadWrite, 0));
  const auto tail = CreateFileMappingHandle("heap-tail", 0x4000, PageAccess::kReadWrite, false);
  assert(tail != kFileMappingHandleInvalid);
  assert(MapFileView(tail, arena + 0xFFD00000, 0x4000, PageAccess::kReadWrite, 0));
  assert(MapFileView(tail, arena + 0x11FD00000ull, 0x4000, PageAccess::kReadWrite, 0));
#if defined(REX_PROSPERO_HEAP_BASELINE)
  {
    VirtualHeap heap;
    heap.Initialize(nullptr, arena, HeapType::kGuestVirtual, 0, 0x40000, 0x1000);
    assert(heap.AllocFixed(0x10000, 0x8000, 0x1000, kCommit, kRW));
    uint32_t old = UINT32_MAX;
    const bool accepted = heap.Protect(0x10000, 0x1000, kRO, &old);
    assert(!accepted && old == UINT32_MAX && GuestAccess(&heap, 0x10000) == kRW);
    std::puts("BASELINE DEFECT REPRODUCED: valid guest4KiB Protect rejected on host16KiB");
    assert(heap.AllocFixed(0x20000, 0x1000, 0x1000, kCommit, kRW));
    assert(heap.AllocFixed(0x21000, 0x1000, 0x1000, kCommit, kRO));
    assert(GuestAccess(&heap, 0x20000) == kRW && HostAccess(arena + 0x20000) == PageAccess::kReadOnly);
    std::puts("BASELINE DEFECT REPRODUCED: committing adjacent guestRO revokes host write access for guestRW neighbour");
    heap.Dispose();
  }
#else
  {
    Memory memory;
    ProsperoHeapHostTest::Initialize(memory, arena);
    TestVirtual(memory, arena);
    TestPhysical(memory, arena);
  }
#endif
  assert(UnmapFileView(tail, arena + 0xFFD00000, 0x4000));
  assert(UnmapFileView(tail, arena + 0x11FD00000ull, 0x4000));
  CloseFileMappingHandle(tail, "heap-tail");
  for (size_t offset : offsets) assert(UnmapFileView(object, arena + offset, bytes));
  CloseFileMappingHandle(object, "heap-host");
  assert(DeallocFixed(arena, 0, DeallocationType::kRelease));
  ps5_shm_stats stats{};
  ps5_shm_live(&stats);
  assert(!stats.objects && !stats.views && !stats.ranges && !host_direct_allocations());
  std::puts("Heap host checks complete. 16MiB shared backing; no PS5 execution claim.");
}
