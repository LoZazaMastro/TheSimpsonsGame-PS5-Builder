// The actual ReXGlue backend against the payload SDK's host kernel model.
// This exercises mapping semantics; it does not execute a PS5 binary.
#include <rex/memory/utils.h>
#include <ps5platform/kernel.h>
#include <ps5platform/shm.h>
extern "C" {
#include "host_kernel.h"
}

#include <array>
#include <cassert>
#include <cstdio>
#include <limits>
#include <sys/mman.h>

using namespace rex::memory;

// The SDK's host model deliberately has no query implementation. Supply the
// Linux equivalent here only; the production wrapper calls the console kernel
// directly and does not read /proc or use this implementation.
static bool fail_query;
extern "C" int32_t sceKernelQueryMemoryProtection(
    void* address, void** start, void** end, uint32_t* protection) {
  if (fail_query) return -1;
  FILE* maps = std::fopen("/proc/self/maps", "r");
  if (!maps) return -1;
  char line[1024];
  int result = -1;
  const auto at = reinterpret_cast<uintptr_t>(address);
  while (std::fgets(line, sizeof(line), maps)) {
    unsigned long long low, high;
    char permissions[5]{};
    if (std::sscanf(line, "%llx-%llx %4s", &low, &high, permissions) != 3) continue;
    if (at < low || at >= high) continue;
    *start = reinterpret_cast<void*>(low);
    *end = reinterpret_cast<void*>(high);
    *protection = (permissions[0] == 'r' ? PS5_KERNEL_PROT_CPU_READ : 0) |
                  (permissions[1] == 'w' ? PS5_KERNEL_PROT_CPU_WRITE : 0) |
                  (permissions[2] == 'x' ? PS5_KERNEL_PROT_CPU_EXEC : 0);
    result = 0;
    break;
  }
  std::fclose(maps);
  return result;
}

static PageAccess Access(void* address) {
  size_t length = 0;
  PageAccess result;
  assert(QueryProtect(address, length, result));
  assert(length >= 0x4000);
  return result;
}

static void CheckNoAllocations() {
  ps5_shm_stats stats{};
  ps5_shm_live(&stats);
  assert(stats.objects == 0 && stats.views == 0 && stats.ranges == 0);
  assert(stats.object_bytes == 0 && stats.view_bytes == 0 && stats.range_bytes == 0);
  assert(stats.committed_bytes == 0 && host_direct_allocations() == 0);
}

static void TestAliasesAndReservation() {
  constexpr size_t arena_size = 0x120000000ull;
  auto* arena = static_cast<uint8_t*>(AllocFixed(
      nullptr, arena_size, AllocationType::kReserve, PageAccess::kNoAccess));
  assert(arena && reinterpret_cast<uintptr_t>(arena) % 0x10000 == 0);
  const auto at = reinterpret_cast<uintptr_t>(arena);
  assert(at >= 0x300000000ull || at + arena_size <= 0x200000000ull);
  auto object = CreateFileMappingHandle("test", 0x20000, PageAccess::kReadWrite, false);
  assert(object != kFileMappingHandleInvalid);
  // Reserve the real 4.5 GiB span, but back small slices at its physical alias
  // locations so the test needs KiB of RAM, not 4.5 GiB of game memory.
  const std::array<size_t, 4> offsets{0xA0000000, 0xC0000000, 0xE0000000, 0x100000000};
  for (size_t offset : offsets) {
    assert(MapFileView(object, arena + offset, 0x20000, PageAccess::kReadWrite, 0) == arena + offset);
  }
  auto* physical = arena + 0x100000000;
  physical[0x1000] = 0x73;
  for (size_t offset : offsets) assert(arena[offset + 0x1000] == 0x73);
  assert(AllocFixed(arena + 0xE0001000, 0x1000, AllocationType::kCommit,
                    PageAccess::kReadWrite) == arena + 0xE0001000);
  assert(physical[0x1000] == 0x73);  // Commit must preserve shared backing/data.
  assert(!DeallocFixed(arena, 0, DeallocationType::kRelease));  // Live aliases.
  assert(!MapFileView(object, arena + 0xA0000000, 0x20000, PageAccess::kReadWrite, 0));
  // A new anonymous commit must not coexist with shared views in this arena.
  assert(!AllocFixed(arena, 0x4000, AllocationType::kCommit, PageAccess::kReadWrite));

  // Never overwrite an unrelated mapping or the GPU VM window.
  void* unrelated = mmap(nullptr, 0x20000, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(unrelated != MAP_FAILED);
  *static_cast<uint8_t*>(unrelated) = 0x42;
  assert(!MapFileView(object, unrelated, 0x4000, PageAccess::kReadWrite, 0));
  assert(*static_cast<uint8_t*>(unrelated) == 0x42);
  assert(!MapFileView(object, reinterpret_cast<void*>(0x200000000ull), 0x4000,
                       PageAccess::kReadWrite, 0));
  munmap(unrelated, 0x20000);

  // Close can precede unmap, but cannot allow a new view from the closed handle.
  CloseFileMappingHandle(object, "test");
  assert(physical[0x1000] == 0x73);
  assert(!MapFileView(object, nullptr, 0x4000, PageAccess::kReadWrite, 0));
  for (size_t offset : offsets) assert(UnmapFileView(object, arena + offset, 0x20000));
  assert(Access(arena + 0xE0000000) == PageAccess::kNoAccess);
  assert(DeallocFixed(arena, 0, DeallocationType::kRelease));
  CheckNoAllocations();
}

static void TestProtectionAndAnonymousLifecycle() {
  auto* block = static_cast<uint8_t*>(AllocFixed(
      nullptr, 0x10000, AllocationType::kReserveCommit, PageAccess::kReadWrite));
  assert(block);
  assert(block[0] == 0 && block[0xFFFF] == 0);
  block[0] = 1;
  block[0x4000] = 2;
  block[0x8000] = 3;
  PageAccess old;
  assert(Protect(block + 0x3FFF, 2, PageAccess::kReadOnly, &old));
  assert(old == PageAccess::kReadWrite);
  assert(Access(block) == PageAccess::kReadOnly);
  assert(Access(block + 0x4000) == PageAccess::kReadOnly);
  assert(Access(block + 0x8000) == PageAccess::kReadWrite);
  assert(AllocFixed(block, 0x8000, AllocationType::kCommit, PageAccess::kReadWrite) == block);
  assert(block[0] == 1 && block[0x4000] == 2);

  assert(DeallocFixed(block + 0x4000, 0x4000, DeallocationType::kDecommit));
  assert(Access(block + 0x4000) == PageAccess::kNoAccess);
  assert(Access(block) == PageAccess::kReadWrite);
  assert(AllocFixed(block + 0x4000, 0x4000, AllocationType::kCommit,
                    PageAccess::kReadWrite) == block + 0x4000);
  assert(block[0x4000] == 0 && block[0] == 1 && block[0x8000] == 3);

  auto object = CreateFileMappingHandle("test", 0x10000, PageAccess::kReadWrite, false);
  assert(object != kFileMappingHandleInvalid);
  assert(!MapFileView(object, block, 0x4000, PageAccess::kReadWrite, 0));
  assert(block[0] == 1);  // Never replace an existing anonymous unit with a view.
  CloseFileMappingHandle(object, "test");

  fail_query = true;
  size_t query_length = 111;
  old = PageAccess::kReadWrite;
  assert(!QueryProtect(block, query_length, old));
  assert(query_length == 0 && old == PageAccess::kNoAccess);
  assert(!Protect(block, 0x4000, PageAccess::kReadOnly, &old));
  fail_query = false;
  assert(Access(block) == PageAccess::kReadWrite);
  host_fail(HOST_CALL_PROTECT, 1);
  assert(!Protect(block, 0x4000, PageAccess::kReadOnly));
  assert(Access(block) == PageAccess::kReadWrite);

  assert(!DeallocFixed(block + 0x4000, 0, DeallocationType::kRelease));
  assert(!DeallocFixed(block, 0x10000, DeallocationType::kRelease));
  assert(DeallocFixed(block, 0, DeallocationType::kRelease));
  CheckNoAllocations();
}

static void TestFailureCleanupAndLimits() {
  assert(!AllocFixed(nullptr, 0, AllocationType::kReserve, PageAccess::kNoAccess));
  assert(!AllocFixed(nullptr, std::numeric_limits<size_t>::max(),
                    AllocationType::kReserve, PageAccess::kNoAccess));
  assert(!Protect(reinterpret_cast<void*>(std::numeric_limits<uintptr_t>::max() - 8),
                  16, PageAccess::kReadOnly));
  host_fail(HOST_CALL_ALLOCATE, 1);
  assert(CreateFileMappingHandle("test", 0x10000, PageAccess::kReadWrite, false) ==
         kFileMappingHandleInvalid);
  CheckNoAllocations();
  host_fail(HOST_CALL_ALLOCATE, 1);
  assert(!AllocFixed(nullptr, 0x20000, AllocationType::kReserveCommit, PageAccess::kReadWrite));
  CheckNoAllocations();
  host_fail(HOST_CALL_ALLOCATE, 2);  // Commit backs one unit, then fails.
  assert(!AllocFixed(nullptr, 0x20000, AllocationType::kReserveCommit, PageAccess::kReadWrite));
  CheckNoAllocations();

  auto* arena = static_cast<uint8_t*>(AllocFixed(
      nullptr, 0x20000, AllocationType::kReserve, PageAccess::kNoAccess));
  auto object = CreateFileMappingHandle("test", 0x10000, PageAccess::kReadWrite, false);
  assert(arena && object != kFileMappingHandleInvalid);
  assert(!MapFileView(object, arena, 0x10000, PageAccess::kReadWrite, 0x1000));
  assert(!MapFileView(object, arena, 0x20000, PageAccess::kReadWrite, 0));
  host_fail(HOST_CALL_MAP, 1);
  assert(!MapFileView(object, arena, 0x10000, PageAccess::kReadWrite, 0));
  assert(MapFileView(object, arena, 0x10000, PageAccess::kReadWrite, 0) == arena);
  assert(!UnmapFileView(object, arena, 0x4000));
  assert(UnmapFileView(object, arena, 0x10000));
  CloseFileMappingHandle(object, "test");
  assert(DeallocFixed(arena, 0, DeallocationType::kRelease));
  CheckNoAllocations();
}

int main() {
  assert(page_size() == 0x4000 && allocation_granularity() == 0x10000);
  TestAliasesAndReservation();
  TestProtectionAndAnonymousLifecycle();
  TestFailureCleanupAndLimits();
  std::puts("PS5 memory backend: host-model alias, protection, lifecycle and failure tests passed.");
}
