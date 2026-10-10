// Host-runnable checks for the native audio boundary, no PS5 device required.
// g++ -std=c++23 -msse4.1 -I tools/rexglue-sdk/include this_file.cpp -o test
#include <rex/audio/prospero/pcm_conversion.h>

#include <cassert>
#include <cstring>
#include <limits>

using namespace rex::audio::prospero;

int main() {
  std::array<float, kSamplesPerFrame * kGuestChannels> guest{};
  const auto set_sample = [&](size_t channel, size_t sample, float value) {
    const uint32_t bits = std::bit_cast<uint32_t>(value);
    const uint8_t bytes[]{uint8_t(bits >> 24), uint8_t(bits >> 16),
                          uint8_t(bits >> 8), uint8_t(bits)};
    std::memcpy(&guest[channel * kSamplesPerFrame + sample], bytes, sizeof(bytes));
  };
  const auto silent = ConvertGuestFrame(guest.data());
  for (int16_t sample : silent) assert(sample == 0);

  // Isolated channel impulses also detect swapped planes, wrong endianness,
  // interleaving errors, and accidental LFE-as-center mapping.
  constexpr int16_t expected_left[]{6553, 0, 3277, 0, 6553, 0};
  constexpr int16_t expected_right[]{0, 6553, 3277, 0, 0, 6553};
  for (size_t channel = 0; channel < kGuestChannels; ++channel) {
    guest.fill(0);
    set_sample(channel, 7, 0.5f);
    const auto pcm = ConvertGuestFrame(guest.data());
    for (size_t sample = 0; sample < kSamplesPerFrame; ++sample) {
      assert(pcm[2 * sample] == (sample == 7 ? expected_left[channel] : 0));
      assert(pcm[2 * sample + 1] == (sample == 7 ? expected_right[channel] : 0));
    }
  }
  guest.fill(0);
  for (size_t channel = 0; channel < kGuestChannels; ++channel) {
    set_sample(channel, 3, -0.5f);
    set_sample(channel, 4, 3.0f);
  }
  auto pcm = ConvertGuestFrame(guest.data());
  assert(pcm[6] == -16384 && pcm[7] == -16384);
  assert(pcm[8] == 32767 && pcm[9] == 32767);

  assert(FloatToSigned16(0.0f) == 0);
  assert(FloatToSigned16(1.0f) == 32767);
  assert(FloatToSigned16(-1.0f) == -32768);
  assert(FloatToSigned16(100.0f) == 32767);
  assert(FloatToSigned16(-100.0f) == -32768);
  assert(FloatToSigned16(std::numeric_limits<float>::infinity()) == 0);
  assert(FloatToSigned16(-std::numeric_limits<float>::infinity()) == 0);
  assert(FloatToSigned16(std::numeric_limits<float>::quiet_NaN()) == 0);
}
