// Real native CRT hook versus its original generated PowerPC function, using
// shared host mappings for the guest aliases. No graphics or console execution.
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

#include NATIVE_CRT_SOURCE

namespace {
constexpr size_t kBackingSize = 0x10000;
constexpr size_t kReserveSize = 0x100002000ull;
void Check(bool value, const char* message) {
  if (!value) { std::perror(message); std::exit(3); }
}

struct Mapping {
  uint8_t* base;
  int physical;
  int image;
  Mapping() {
    base = static_cast<uint8_t*>(mmap(nullptr, kReserveSize, PROT_NONE,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    Check(base != MAP_FAILED, "reserve guest addresses");
    physical = memfd_create("guest-physical", 0);
    image = memfd_create("guest-image", 0);
    Check(physical >= 0 && image >= 0, "memfd_create");
    Check(!ftruncate(physical, kBackingSize) && !ftruncate(image, kBackingSize), "ftruncate");
    for (uint32_t address : {0x7F000000u, 0xA0000000u, 0xC0000000u, 0xE0000000u})
      Check(mmap(base + address, kBackingSize, PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_FIXED, physical, 0) == base + address, "map physical alias");
    for (uint32_t address : {0x80000000u, 0x90000000u})
      Check(mmap(base + address, kBackingSize, PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_FIXED, image, 0) == base + address, "map image alias");
  }
  ~Mapping() { munmap(base, kReserveSize); close(physical); close(image); }
};

struct View { const char* name; uint32_t base; uint32_t offset; };

void Seed(uint8_t* backing) {
  for (size_t i = 0; i < kBackingSize; ++i)
    backing[i] = uint8_t((i * 13 + (i / 17) * 31) & 255);
}

int CompareGroup(Mapping& mapping, const std::vector<View>& views, uint8_t* backing,
                 int& cases) {
  int failures = 0;
  for (const auto& source : views) for (const auto& destination : views) {
    for (const auto delta : {-97, -16, -1, 0, 1, 16, 97, 256}) {
      for (const auto size : {0u, 1u, 3u, 17u, 64u, 129u}) {
        const auto source_address = source.base + 0x3000 - source.offset;
        const auto destination_address = destination.base + 0x3000 + delta - destination.offset;
        Seed(backing);
        PPCContext expected_context{};
        expected_context.r3.u64 = destination_address;
        expected_context.r4.u64 = source_address;
        expected_context.r5.u64 = size;
        __imp__sub_82A3D1D8(expected_context, mapping.base);
        std::array<uint8_t, kBackingSize> expected;
        std::memcpy(expected.data(), backing, kBackingSize);
        Seed(backing);
        PPCContext actual_context{};
        actual_context.r3.u64 = destination_address;
        actual_context.r4.u64 = source_address;
        actual_context.r5.u64 = size;
        sub_82A3D1D8(actual_context, mapping.base);
        const bool passed = !std::memcmp(expected.data(), backing, kBackingSize) &&
                            actual_context.r3.u32 == destination_address;
        ++cases;
        if (!passed) {
          if (failures < 12)
            std::printf("FAIL %s -> %s delta=%d size=%u\n", source.name,
                        destination.name, delta, size);
          ++failures;
        }
      }
    }
  }
  return failures;
}
}  // namespace

int main() {
  Mapping mapping;
  int cases = 0;
  int failures = CompareGroup(mapping,
      {{"GPU", 0x7F000000u, 0}, {"A0", 0xA0000000u, 0},
       {"C0", 0xC0000000u, 0}, {"E0", 0xE0000000u, 0x1000}},
      mapping.base + 0xA0000000u, cases);
  failures += CompareGroup(mapping,
      {{"XEX64K", 0x80000000u, 0}, {"XEX4K", 0x90000000u, 0}},
      mapping.base + 0x80000000u, cases);
  std::printf("Native/generated copy equivalence: %d cases, %d failures\n", cases, failures);
#ifndef EXPECT_BASELINE
  for (uint64_t end : {0x7F000000ull, 0x80000000ull, 0x90000000ull,
                       0xA0000000ull, 0xC0000000ull, 0xE0000000ull, 0x100000000ull}) {
    Check(OneHostRange(uint32_t(end - 8), 8), "range ending at view edge");
    Check(!OneHostRange(uint32_t(end - 8), 9), "range crossing view edge");
    ++cases;
  }
  Check(GuestBacking(0xE0000000).backing_address == 0x100001000ull, "E0 physical offset");
  Check(!ForwardCopyIsPlain(0xA0001001, 0xE0000000, 64), "E0/A0 actual overlap");
  Check(ForwardCopyIsPlain(0xA0000000, 0xE0000000, 64), "E0/A0 different physical bytes");
  Check(!ForwardCopyIsPlain(0x90001010, 0x80001000, 64), "XEX alias overlap");
  std::printf("PASS mapping boundaries, wraparound, physical offset and non-overlap\n");
#endif
  return failures ? 2 : 0;
}
