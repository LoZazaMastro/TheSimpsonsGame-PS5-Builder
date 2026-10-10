// Coherent publication of the guest callback and its guest argument.
// BSD 3-Clause License. See the project LICENSE.
#pragma once
#include <atomic>
#include <cstdint>

namespace rex::graphics {
class InterruptCallbackState {
 public:
  struct Snapshot {
    uint32_t callback;
    uint32_t user_data;
  };

  void Publish(uint32_t callback, uint32_t user_data) noexcept {
    value_.store((uint64_t(callback) << 32) | user_data, std::memory_order_release);
  }

  Snapshot Read() const noexcept {
    const uint64_t value = value_.load(std::memory_order_acquire);
    return {uint32_t(value >> 32), uint32_t(value)};
  }

 private:
  std::atomic<uint64_t> value_{0};
};
}  // namespace rex::graphics
