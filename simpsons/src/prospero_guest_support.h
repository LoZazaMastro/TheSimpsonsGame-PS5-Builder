#pragma once

#include <cstdint>

namespace rex::memory {
class Memory;
}

namespace simpsons {

enum class ProsperoBootAllocation : uint8_t { kCursor, kHashtable, kResource };

// Called after an image is loaded, before any of its guest threads run. The
// allocations belong to that Memory arena and are reclaimed with the arena.
void InitializeProsperoGuestSupport(rex::memory::Memory* memory);

// Backing for existing upstream boot-race fallbacks, allocated through the
// runtime instead of borrowing addresses from the GPU's physical-memory views.
// A failed allocation is fatal and diagnosed; no arbitrary address is returned.
uint32_t GetProsperoBootAllocation(uint8_t* base, uint32_t owner,
                                  ProsperoBootAllocation kind);

}  // namespace simpsons
