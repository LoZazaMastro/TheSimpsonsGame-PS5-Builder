// Bounded, allocation-free frame interval statistics.
// BSD 3-Clause License. See the project LICENSE.
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace rex::platform::prospero {
struct FrameWindow {
  static constexpr size_t kCapacity = 1024;
  uint64_t samples[kCapacity]{};
  size_t stored = 0;
  uint64_t count = 0, sum_ns = 0;
  uint64_t min_ns = UINT64_MAX, max_ns = 0;
  uint64_t over_33ms = 0, over_50ms = 0, over_100ms = 0;
  uint64_t rejected = 0, distribution_dropped = 0;

  void Add(uint64_t ns) noexcept {
    if (!ns || ns > UINT64_MAX - sum_ns || count == UINT64_MAX) { ++rejected; return; }
    ++count; sum_ns += ns;
    min_ns = std::min(min_ns, ns); max_ns = std::max(max_ns, ns);
    over_33ms += ns > 33333333; over_50ms += ns > 50000000; over_100ms += ns > 100000000;
    if (stored < kCapacity) samples[stored++] = ns;
    else ++distribution_dropped;
  }

  static uint64_t RateMilli(uint64_t count, uint64_t ns) noexcept {
    return ns ? uint64_t((__uint128_t(count) * 1000000000000ULL + ns / 2) / ns) : 0;
  }
  uint64_t AverageFpsMilli() const noexcept { return RateMilli(count, sum_ns); }
  void Sort() noexcept { std::sort(samples, samples + stored); }
  uint64_t Percentile(unsigned percent) const noexcept {
    if (!stored || !percent || percent > 100) return 0;
    return samples[(stored * percent + 99) / 100 - 1];
  }
  uint64_t OnePercentLowMilli() const noexcept {
    if (!stored) return 0;
    const size_t count = (stored + 99) / 100;
    uint64_t sum = 0;
    for (size_t i = stored - count; i < stored; ++i) sum += samples[i];
    return RateMilli(count, sum);
  }
};
}  // namespace rex::platform::prospero
