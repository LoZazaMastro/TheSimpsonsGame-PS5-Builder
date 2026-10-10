#include "prospero_guest_support.h"

#include "guest_memory.h"

#include <map>
#include <mutex>
#include <stdexcept>
#include <utility>

#include <rex/logging.h>
#include <rex/system/xmemory.h>

namespace simpsons {
namespace {

struct BootAllocations {
  std::mutex mutex;
  rex::memory::Memory* memory = nullptr;
  std::map<std::pair<ProsperoBootAllocation, uint32_t>, uint32_t> owners;
};

BootAllocations& Allocations() {
  // Destruction only releases host bookkeeping. Guest storage is owned by the
  // runtime's arena, which may already be gone when static objects are destroyed.
  static BootAllocations allocations;
  return allocations;
}

const char* AllocationName(ProsperoBootAllocation kind) {
  switch (kind) {
    case ProsperoBootAllocation::kCursor:
      return "scratch cursor";
    case ProsperoBootAllocation::kHashtable:
      return "hash-table container";
    case ProsperoBootAllocation::kResource:
      return "resource container";
  }
  return "unknown boot fallback";
}

[[noreturn]] void AllocationFailure(ProsperoBootAllocation kind, uint32_t owner,
                                  const char* reason) {
  REXLOG_ERROR("PS5 {} fallback failed for owner {:08X}: {}", AllocationName(kind), owner,
               reason);
  throw std::runtime_error("PS5 could not allocate a valid guest boot fallback; see log");
}

}  // namespace

void InitializeProsperoGuestSupport(rex::memory::Memory* memory) {
  if (!memory || !memory->virtual_membase()) {
    REXLOG_ERROR("PS5 guest support initialized without a live memory arena");
    throw std::runtime_error("PS5 guest support requires a live memory arena");
  }
  auto& allocations = Allocations();
  std::lock_guard lock(allocations.mutex);
  // A new load may reuse the previous Memory object's host address. Reset by
  // lifecycle event, not pointer equality, so an old guest address is never
  // returned in a new arena. Prior allocations are owned by their old arena.
  allocations.owners.clear();
  allocations.memory = memory;
}

uint32_t GetProsperoBootAllocation(uint8_t* base, uint32_t owner,
                                  ProsperoBootAllocation kind) {
  auto& allocations = Allocations();
  std::lock_guard lock(allocations.mutex);
  auto* memory = allocations.memory;
  if (!memory || memory->virtual_membase() != base) {
    AllocationFailure(kind, owner, "no matching initialized memory arena");
  }
  const auto key = std::make_pair(kind, owner);
  if (const auto it = allocations.owners.find(key); it != allocations.owners.end()) {
    return it->second;
  }

  const uint32_t size = kind == ProsperoBootAllocation::kCursor ? 0x400000u : 0x1000u;
  // This uses host-side heap bookkeeping and Memory::Zero. It cannot recurse
  // through the game's memcpy or allocator replacement hooks.
  const uint32_t address = memory->SystemHeapAlloc(size, 0x1000u,
                                                 rex::memory::kSystemHeapVirtual);
  if (!address) {
    AllocationFailure(kind, owner, "virtual SystemHeapAlloc returned no allocation");
  }
  try {
    switch (kind) {
      case ProsperoBootAllocation::kCursor:
        break;
      case ProsperoBootAllocation::kHashtable:
        StoreGuestU32(base, address, address + 64u);
        StoreGuestU32(base, address + 8u, 64u);
        break;
      case ProsperoBootAllocation::kResource:
        StoreGuestU32(base, address + 8u, 1u);
        break;
      default:
        AllocationFailure(kind, owner, "unknown allocation kind");
    }
    // Publish only after every container field is initialized; two racing
    // callers for the same owner cannot see a partially initialized object.
    allocations.owners.emplace(key, address);
  } catch (...) {
    memory->SystemHeapFree(address);
    throw;
  }
  REXLOG_WARN("PS5 {} fallback: owner {:08X}, allocated {:08X} bytes at virtual {:08X}",
               AllocationName(kind), owner, size, address);
  return address;
}

}  // namespace simpsons
