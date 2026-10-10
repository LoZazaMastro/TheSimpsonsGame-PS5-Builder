// The recompiled guest changes MXCSR through this real runtime register.
// Build with the actual guest compiler policy; do not enable FENV_ACCESS here,
// since that would hide a missing -ffp-model=strict on the game target.
#include <rex/ppc/context.h>

#include <bit>
#include <cstdint>
#include <cstdio>

[[gnu::noinline]] uint32_t GuestAdd(uint32_t mode, bool negative) {
  rex::ppc::FPSCRRegister fpscr{};
  const uint32_t previous = rex::platform::FPSCRPlatform::getcsr();
  fpscr.csr = previous;
  fpscr.storeFromGuest(mode);
  // Exactly halfway between adjacent floats. These constants are representable
  // and the correct result depends on the guest's active rounding mode.
  const float value = negative ? (-1.0f + -0x1p-24f) : (1.0f + 0x1p-24f);
  const auto bits = std::bit_cast<uint32_t>(value);
  rex::platform::FPSCRPlatform::setcsr(previous);
  return bits;
}

[[gnu::noinline]] uint32_t GuestRoundToSingle(uint32_t mode) {
  rex::ppc::FPSCRRegister fpscr{};
  const uint32_t previous = rex::platform::FPSCRPlatform::getcsr();
  fpscr.csr = previous;
  fpscr.storeFromGuest(mode);
  const double source = 1.0 + 0x1p-24;
  const float value = static_cast<float>(source);
  const auto bits = std::bit_cast<uint32_t>(value);
  rex::platform::FPSCRPlatform::setcsr(previous);
  return bits;
}

int main() {
  int failures = 0;
  for (uint32_t mode = 0; mode < 4; ++mode) {
    const auto positive = GuestAdd(mode, false);
    const auto negative = GuestAdd(mode, true);
    const auto converted = GuestRoundToSingle(mode);
    const uint32_t positive_expected = 0x3f800000 + (mode == rex::ppc::kRoundUp);
    const uint32_t negative_expected = 0xbf800000 + (mode == rex::ppc::kRoundDown);
    const bool passed = positive == positive_expected && negative == negative_expected &&
                        converted == positive_expected;
    failures += !passed;
    std::printf("%s guest_mode=%u add=0x%08x negative=0x%08x frsp=0x%08x\n",
                passed ? "PASS" : "FAIL", mode, positive, negative, converted);
  }
  std::printf("%d of four guest rounding modes failed\n", failures);
  return failures ? 2 : 0;
}
