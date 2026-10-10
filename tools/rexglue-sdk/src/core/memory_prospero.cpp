// PS5 memory backend for the open PS5 Payload SDK platform library.
// SPDX-License-Identifier: BSD-3-Clause
//
// Initial bring-up uses eager direct-memory backing for shared guest memory.
// Its aliases share one object; they must never be replaced by anonymous
// commits. Hardware behavior and the full 4.5 GiB budget remain unvalidated.

#include <rex/memory/utils.h>

#include <ps5platform/kernel.h>
#include <ps5platform/shm.h>

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <new>

namespace rex::memory {
namespace {

constexpr size_t kPage = PS5_KERNEL_PAGE_SIZE;
constexpr size_t kAllocationUnit = PS5_KERNEL_DIRECT_ALIGNMENT;

struct SharedMapping {
  ps5_shm object{};
  size_t view_count = 0;
  bool closed = false;
};

struct View {
  size_t length;
  FileMappingHandle handle;
  bool keep_reserved;
};

struct Reservation {
  size_t length;
  // Shared views and anonymous 64 KiB commits use different ownership in the
  // platform library. Never mix them in one reservation: a partial anonymous
  // commit could otherwise replace an adjacent shared view in the same unit.
  bool anonymous_backing = false;
};

// Only allocation/lifetime operations take this lock. Protect and QueryProtect
// are also called from the exception path and must use only kernel calls and
// stack variables, without this registry, allocation, logging or libc locks.
std::mutex allocation_mutex;
std::map<uintptr_t, Reservation> reservations;
std::map<uintptr_t, View> views;
std::map<FileMappingHandle, SharedMapping> shared_mappings;
FileMappingHandle next_mapping_handle = 1;

bool RoundLength(size_t value, size_t alignment, size_t& rounded) {
  if (!value || value > std::numeric_limits<size_t>::max() - (alignment - 1)) {
    return false;
  }
  rounded = (value + alignment - 1) & ~(alignment - 1);
  return true;
}

bool PageSpan(void* address, size_t length, uintptr_t& begin, size_t& bytes) {
  const uintptr_t at = reinterpret_cast<uintptr_t>(address);
  if (!address || !length || length > std::numeric_limits<uintptr_t>::max() - at) {
    return false;
  }
  const uintptr_t end = at + length;
  if (end > std::numeric_limits<uintptr_t>::max() - (kPage - 1)) {
    return false;
  }
  begin = at & ~(uintptr_t(kPage) - 1);
  bytes = ((end + kPage - 1) & ~(uintptr_t(kPage) - 1)) - begin;
  return true;
}

int Protection(PageAccess access) {
  switch (access) {
    case PageAccess::kNoAccess: return 0;
    case PageAccess::kReadOnly: return PS5_SHM_READ;
    case PageAccess::kReadWrite: return PS5_SHM_READ | PS5_SHM_WRITE;
    case PageAccess::kExecuteReadOnly: return PS5_SHM_READ | PS5_SHM_EXEC;
    case PageAccess::kExecuteReadWrite: return PS5_SHM_READ | PS5_SHM_WRITE | PS5_SHM_EXEC;
    default: return -1;
  }
}

PageAccess FromProtection(uint32_t protection) {
  if (protection & PS5_KERNEL_PROT_CPU_EXEC) {
    return (protection & PS5_KERNEL_PROT_CPU_WRITE) ? PageAccess::kExecuteReadWrite
                                                   : PageAccess::kExecuteReadOnly;
  }
  if (protection & PS5_KERNEL_PROT_CPU_WRITE) return PageAccess::kReadWrite;
  if (protection & PS5_KERNEL_PROT_CPU_READ) return PageAccess::kReadOnly;
  return PageAccess::kNoAccess;
}

auto FindReservation(uintptr_t begin, size_t length) {
  auto found = reservations.upper_bound(begin);
  if (found == reservations.begin()) return reservations.end();
  --found;
  const size_t offset = begin - found->first;
  return offset <= found->second.length && length <= found->second.length - offset
      ? found : reservations.end();
}

bool HasReservation(uintptr_t begin, size_t length) {
  return FindReservation(begin, length) != reservations.end();
}

bool OverlapsView(uintptr_t begin, size_t length) {
  auto found = views.upper_bound(begin);
  if (found != views.begin()) {
    const auto previous = std::prev(found);
    if (begin - previous->first < previous->second.length) return true;
  }
  return found != views.end() && found->first - begin < length;
}

bool CoveredByViews(uintptr_t begin, size_t length) {
  const uintptr_t end = begin + length;
  auto found = views.upper_bound(begin);
  if (found != views.begin()) --found;
  uintptr_t cursor = begin;
  for (; found != views.end() && cursor < end; ++found) {
    const uintptr_t view_end = found->first + found->second.length;
    if (view_end <= cursor) continue;
    if (found->first > cursor) return false;
    cursor = view_end;
  }
  return cursor >= end;
}

void DestroyClosedMapping(FileMappingHandle handle) {
  auto found = shared_mappings.find(handle);
  if (found != shared_mappings.end() && found->second.closed && !found->second.view_count) {
    ps5_shm_destroy(&found->second.object);
    shared_mappings.erase(found);
  }
}

}  // namespace

size_t page_size() { return kPage; }
size_t allocation_granularity() { return kAllocationUnit; }

bool IsWritableExecutableMemorySupported() {
  // Direct memory acquires execute through sceKernelMprotect after mapping,
  // as implemented by ps5_shm_map and ps5_vrange_commit in the payload SDK.
  return true;
}

bool QueryProtect(void* base_address, size_t& length, PageAccess& access_out) {
  length = 0;
  access_out = PageAccess::kNoAccess;
  if (!base_address) return false;
  const uintptr_t page = reinterpret_cast<uintptr_t>(base_address) & ~(uintptr_t(kPage) - 1);
  void* start = nullptr;
  void* end = nullptr;
  uint32_t protection = 0;
  if (sceKernelQueryMemoryProtection(reinterpret_cast<void*>(page), &start, &end, &protection)) {
    return false;
  }
  if (reinterpret_cast<uintptr_t>(start) > page || reinterpret_cast<uintptr_t>(end) <= page) {
    return false;
  }
  length = reinterpret_cast<uintptr_t>(end) - page;
  access_out = FromProtection(protection);
  return true;
}

bool Protect(void* base_address, size_t length, PageAccess access, PageAccess* out_old_access) {
  if (out_old_access) *out_old_access = PageAccess::kNoAccess;
  uintptr_t begin;
  size_t bytes;
  const int protection = Protection(access);
  if (protection < 0 || !PageSpan(base_address, length, begin, bytes)) return false;
  PageAccess old_access = PageAccess::kNoAccess;
  if (out_old_access) {
    size_t old_length = 0;
    if (!QueryProtect(reinterpret_cast<void*>(begin), old_length, old_access)) return false;
  }
  if (sceKernelMprotect(reinterpret_cast<void*>(begin), bytes, protection)) return false;
  if (out_old_access) *out_old_access = old_access;
  return true;
}

void* AllocFixed(void* base_address, size_t length, AllocationType allocation_type,
                 PageAccess access) {
  const int protection = Protection(access);
  size_t reservation_length;
  if (protection < 0 || !RoundLength(length, kAllocationUnit, reservation_length)) return nullptr;
  if (allocation_type != AllocationType::kReserve && allocation_type != AllocationType::kCommit &&
      allocation_type != AllocationType::kReserveCommit) return nullptr;

  std::lock_guard lock(allocation_mutex);
  if (base_address) {
    uintptr_t begin;
    size_t bytes;
    if (!PageSpan(base_address, length, begin, bytes)) return nullptr;
    if (allocation_type != AllocationType::kReserve) {
      if (CoveredByViews(begin, bytes)) {
        // This is already a view of the guest object. Allocating anonymous
        // memory here would destroy its aliases; commit is a protection change.
        return Protect(base_address, length, access) ? base_address : nullptr;
      }
      auto reservation = FindReservation(begin, bytes);
      if (reservation != reservations.end()) {
        if (OverlapsView(reservation->first, reservation->second.length)) return nullptr;
        reservation->second.anonymous_backing = true;
        return ps5_vrange_commit(base_address, length, protection) == 0 ? base_address : nullptr;
      }
    }
    if (HasReservation(begin, bytes) || OverlapsView(begin, bytes)) return nullptr;
    if (reinterpret_cast<uintptr_t>(base_address) % kAllocationUnit) return nullptr;
    if (reservation_length > std::numeric_limits<uintptr_t>::max() - begin) return nullptr;
  }

  void* reserved = base_address;
  const int reserved_result = base_address
      ? ps5_vrange_reserve_at(base_address, reservation_length)
      : ps5_vrange_reserve(reservation_length, nullptr, kAllocationUnit, &reserved);
  if (reserved_result != 0) return nullptr;
  try {
    const auto inserted = reservations.emplace(reinterpret_cast<uintptr_t>(reserved),
        Reservation{reservation_length, allocation_type != AllocationType::kReserve});
    if (!inserted.second) {
      ps5_vrange_release(reserved, reservation_length);
      return nullptr;
    }
  } catch (const std::bad_alloc&) {
    ps5_vrange_release(reserved, reservation_length);
    return nullptr;
  }
  if (allocation_type != AllocationType::kReserve &&
      ps5_vrange_commit(reserved, length, protection) != 0) {
    // Commit may have backed a prefix of the range; the platform release
    // returns those units as well as the reservation.
    if (ps5_vrange_release(reserved, reservation_length) == 0) {
      reservations.erase(reinterpret_cast<uintptr_t>(reserved));
    }
    return nullptr;
  }
  return reserved;
}

bool DeallocFixed(void* base_address, size_t length, DeallocationType deallocation_type) {
  if (!base_address) return false;
  std::lock_guard lock(allocation_mutex);
  const uintptr_t at = reinterpret_cast<uintptr_t>(base_address);
  if (deallocation_type == DeallocationType::kRelease) {
    // Release means the whole original reservation. Reject an interior guest
    // allocation and a live shared view, never tear down their parent arena.
    if (length != 0) return false;
    auto found = reservations.find(at);
    if (found == reservations.end() || OverlapsView(at, found->second.length)) return false;
    if (ps5_vrange_release(base_address, found->second.length) != 0) return false;
    reservations.erase(found);
    return true;
  }
  if (deallocation_type != DeallocationType::kDecommit) return false;
  uintptr_t begin;
  size_t bytes;
  if (!PageSpan(base_address, length, begin, bytes)) return false;
  if (CoveredByViews(begin, bytes)) {
    // Eager shared backing cannot be returned page by page. Preserve every
    // alias, zero the requested span for a later commit, then deny access to
    // this view. Other aliases continue referring to the same underlying data.
    if (!Protect(base_address, length, PageAccess::kReadWrite)) return false;
    std::memset(base_address, 0, length);
    return Protect(base_address, length, PageAccess::kNoAccess);
  }
  if (!HasReservation(begin, bytes) || OverlapsView(begin, bytes)) return false;
  if (ps5_vrange_decommit(base_address, length) != 0) return false;
  // A partially decommitted 64 KiB unit remains physically backed; explicitly
  // remove access to its 16 KiB pages as well.
  return Protect(base_address, length, PageAccess::kNoAccess);
}

FileMappingHandle CreateFileMappingHandle(const std::filesystem::path& path, size_t length,
                                          PageAccess access, bool commit) {
  (void)path;
  (void)commit;
  size_t rounded;
  if (Protection(access) < 0 || !RoundLength(length, kAllocationUnit, rounded)) {
    return kFileMappingHandleInvalid;
  }
  SharedMapping mapping;
  std::fprintf(stderr,
      "[Prospero memory] Eager shared backing: %zu bytes (%zu MiB) of direct memory; "
      "not sparse. PS5 hardware budget is unvalidated.\n", rounded, rounded >> 20);
  const int result = ps5_shm_create(rounded, &mapping.object);
  if (result != 0) {
    std::fprintf(stderr, "[Prospero memory] ps5_shm_create failed: 0x%08x\n", unsigned(result));
    return kFileMappingHandleInvalid;
  }
  std::lock_guard lock(allocation_mutex);
  if (next_mapping_handle == std::numeric_limits<FileMappingHandle>::max()) {
    ps5_shm_destroy(&mapping.object);
    return kFileMappingHandleInvalid;
  }
  const FileMappingHandle handle = next_mapping_handle++;
  try {
    shared_mappings.emplace(handle, mapping);
  } catch (const std::bad_alloc&) {
    ps5_shm_destroy(&mapping.object);
    return kFileMappingHandleInvalid;
  }
  return handle;
}

void CloseFileMappingHandle(FileMappingHandle handle, const std::filesystem::path& path) {
  (void)path;
  std::lock_guard lock(allocation_mutex);
  auto found = shared_mappings.find(handle);
  if (found == shared_mappings.end()) return;
  // A handle can be closed before its final view, just like a desktop file
  // mapping. Defer freeing direct memory until the last view is unmapped.
  found->second.closed = true;
  DestroyClosedMapping(handle);
}

void* MapFileView(FileMappingHandle handle, void* base_address, size_t length, PageAccess access,
                  size_t file_offset) {
  size_t bytes;
  const int protection = Protection(access);
  if (protection < 0 || file_offset % kPage || !RoundLength(length, kPage, bytes)) return nullptr;
  const uintptr_t at = reinterpret_cast<uintptr_t>(base_address);
  if (base_address && (at % kPage || bytes > std::numeric_limits<uintptr_t>::max() - at)) {
    return nullptr;
  }
  std::lock_guard lock(allocation_mutex);
  auto mapping = shared_mappings.find(handle);
  if (mapping == shared_mappings.end() || mapping->second.closed ||
      file_offset > mapping->second.object.bytes ||
      bytes > mapping->second.object.bytes - file_offset) return nullptr;

  // FIXED mappings are permitted only inside a reservation owned by this
  // backend. An arbitrary address must never overwrite the GPU's VM window.
  if (base_address) {
    auto reservation = FindReservation(at, bytes);
    if (reservation == reservations.end() || reservation->second.anonymous_backing ||
        OverlapsView(at, bytes)) return nullptr;
  }
  void* mapped = nullptr;
  const int result = ps5_shm_map(&mapping->second.object, file_offset, bytes, base_address, protection,
                                 base_address ? PS5_SHM_FIXED : 0, &mapped);
  if (result != 0) return nullptr;
  try {
    views.emplace(reinterpret_cast<uintptr_t>(mapped), View{bytes, handle, base_address != nullptr});
  } catch (const std::bad_alloc&) {
    ps5_shm_unmap(mapped, bytes, base_address ? PS5_SHM_KEEP_RESERVED : 0);
    return nullptr;
  }
  ++mapping->second.view_count;
  return mapped;
}

bool UnmapFileView(FileMappingHandle handle, void* base_address, size_t length) {
  size_t bytes;
  if (!base_address || !RoundLength(length, kPage, bytes)) return false;
  std::lock_guard lock(allocation_mutex);
  auto found = views.find(reinterpret_cast<uintptr_t>(base_address));
  if (found == views.end() || found->second.handle != handle || found->second.length != bytes) {
    return false;
  }
  if (ps5_shm_unmap(base_address, bytes, found->second.keep_reserved ? PS5_SHM_KEEP_RESERVED : 0)) {
    return false;
  }
  views.erase(found);
  auto mapping = shared_mappings.find(handle);
  if (mapping != shared_mappings.end()) --mapping->second.view_count;
  DestroyClosedMapping(handle);
  return true;
}

}  // namespace rex::memory
