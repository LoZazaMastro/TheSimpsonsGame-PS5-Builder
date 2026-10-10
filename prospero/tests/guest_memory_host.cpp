// Exercise the actual game's pointer/endian helpers on a sparse Linux host
// mapping. This checks address arithmetic, not PS5 kernel mapping behavior.
#include "guest_memory.h"

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include <sys/mman.h>
#include <unistd.h>

namespace {

void Require(bool value, const char* message, uint32_t guest) {
  if (!value) {
    std::fprintf(stderr, "FAIL: %s at guest %08X\n", message, guest);
    std::exit(1);
  }
}

struct AddressCase {
  uint32_t guest;
  size_t prospero_host_offset;
};

}  // namespace

int main() {
  static_assert(sizeof(void*) == 8);
  // Include an unaligned physical access and the top of uint32_t: the latter
  // catches addition of the host padding in 32 bits before adding the base.
  constexpr std::array cases{
      AddressCase{0x00010000u, 0x00010000ull},
      AddressCase{0x82000000u, 0x82000000ull},
      AddressCase{0xDFFFFFFCu, 0xDFFFFFFCull},
      AddressCase{0xE0000000u, 0xE0001000ull},
      AddressCase{0xE0000001u, 0xE0001001ull},
      AddressCase{0xFFCFFFFCu, 0xFFD00FFCull},
      AddressCase{0xFFFFFFFCu, 0x100000FFCull},
  };
  constexpr size_t arena_size = 0x100002000ull;
  auto* base = static_cast<uint8_t*>(
      mmap(nullptr, arena_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  if (base == MAP_FAILED) {
    std::perror("mmap sparse host arena");
    return 1;
  }
  const long host_page_size = sysconf(_SC_PAGESIZE);
  Require(host_page_size > 0, "host page size", 0);
  const size_t page_size = static_cast<size_t>(host_page_size);
  for (const auto& test : cases) {
#if REX_PLATFORM_PROSPERO
    const size_t offset = test.prospero_host_offset;
#else
    const size_t offset = test.guest;
#endif
    auto* expected = base + offset;
    Require(simpsons::GuestToHost(base, test.guest) == expected,
            "mutable address translation", test.guest);
    Require(simpsons::GuestToHost(static_cast<const uint8_t*>(base), test.guest) == expected,
            "const address translation", test.guest);
    if (mprotect(base + (offset / page_size) * page_size, page_size,
                 PROT_READ | PROT_WRITE) != 0) {
      std::perror("mprotect host test page");
      return 1;
    }
    expected[0] = 0x12;
    expected[1] = 0x34;
    expected[2] = 0x56;
    expected[3] = 0x78;
    Require(simpsons::LoadGuestU32(base, test.guest) == 0x12345678u,
            "read big-endian guest word", test.guest);
    simpsons::StoreGuestU32(base, test.guest, 0xFEDCBA98u);
    Require(expected[0] == 0xFE && expected[1] == 0xDC && expected[2] == 0xBA &&
                expected[3] == 0x98,
            "write big-endian guest word", test.guest);
    simpsons::StoreGuestFloat(base, test.guest, -13.5f);
    Require(expected[0] == 0xC1 && expected[1] == 0x58 && expected[2] == 0 &&
                expected[3] == 0,
            "write big-endian guest float", test.guest);
    Require(simpsons::LoadGuestFloat(base, test.guest) == -13.5f,
            "read big-endian guest float", test.guest);
  }
  Require(munmap(base, arena_size) == 0, "release sparse host arena", 0);
  std::printf("PASS: %zu guest address/endian cases (%s branch, Linux host only)\n",
              cases.size(), REX_PLATFORM_PROSPERO ? "PS5" : "desktop");
}
