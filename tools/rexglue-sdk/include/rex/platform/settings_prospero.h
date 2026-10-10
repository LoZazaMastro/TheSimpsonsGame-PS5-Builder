// PS5 live settings. The UI publishes requests; each owning consumer acknowledges
// the exact version it used. No live setting writes the non-atomic CVar storage.
// BSD 3-Clause License. See the project LICENSE.
#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace rex::platform::prospero {

enum class LiveSetting : uint8_t {
  kFrameRates,
  kAnisotropy,
  kSwapPostEffect,
  kPresentEffect,
  kFsrSharpnessReduction,
  kLetterbox,
  kMute,
  kSubtitles,
  kVSync,
  kColorSaturation,
  kColorVibrance,
  kColorContrast,
  kColorBrightness,
  kCount,
};

// The high word is a monotonically increasing request version. The low word is
// a signed value, or two uint16 frame rates. A reader always sees the value and
// its version together, including when the UI publishes another request.
struct SettingRequest {
  uint64_t packed = 0;
  uint32_t version() const { return uint32_t(packed >> 32); }
  int32_t value() const { return int32_t(uint32_t(packed)); }
};

class LiveSettings {
 public:
  // Initialize before starting any renderer, audio or guest thread.
  void Initialize(LiveSetting key, int32_t value) {
    if (key == LiveSetting::kFrameRates) {
      const int32_t game = IsSupportedFrameRate(GameFrameRate(value)) ? GameFrameRate(value) : 60;
      const int32_t menu = IsSupportedFrameRate(MenuFrameRate(value)) ? MenuFrameRate(value) : game;
      value = PackFrameRates(game, menu);
    }
    auto& slot = slots_[size_t(key)];
    const uint64_t initial = (uint64_t(1) << 32) | uint32_t(value);
    slot.requested.store(initial, std::memory_order_relaxed);
    slot.applied.store(uint32_t(value), std::memory_order_relaxed);
    slot.rejected.store(0, std::memory_order_relaxed);
  }

  // UI thread only. Version zero is reserved for an uninitialized slot.
  SettingRequest Request(LiveSetting key, int32_t value) {
    auto& slot = slots_[size_t(key)];
    const uint64_t old = slot.requested.load(std::memory_order_relaxed);
    if (key == LiveSetting::kFrameRates &&
        (!IsSupportedFrameRate(GameFrameRate(value)) || !IsSupportedFrameRate(MenuFrameRate(value))))
      return {old};
    if (int32_t(uint32_t(old)) == value) return {old};
    const uint32_t version = uint32_t(old >> 32) + 1;
    // Four billion UI edits within one launch are outside the supported range.
    if (!version) return {old};
    const uint64_t next = (uint64_t(version) << 32) | uint32_t(value);
    slot.requested.store(next, std::memory_order_release);
    return {next};
  }

  SettingRequest Requested(LiveSetting key) const {
    return {slots_[size_t(key)].requested.load(std::memory_order_acquire)};
  }
  SettingRequest Applied(LiveSetting key) const {
    return {slots_[size_t(key)].applied.load(std::memory_order_acquire)};
  }
  SettingRequest Rejected(LiveSetting key) const {
    return {slots_[size_t(key)].rejected.load(std::memory_order_acquire)};
  }
  // A pipeline required by this request could not be created. The previous
  // usable policy remains active; the UI must not report this request applied.
  void Reject(LiveSetting key, SettingRequest request) {
    slots_[size_t(key)].rejected.store(request.packed, std::memory_order_release);
  }
  // actual_effect comes from the pass that was really recorded, after optional
  // pipeline/image fallback. Successful submission alone does not imply FXAA.
  void CompleteEffect(LiveSetting key, SettingRequest request, int32_t actual_effect,
                       bool submitted) {
    if (!submitted) return;
    if (actual_effect == request.value()) Acknowledge(key, request);
    else Reject(key, request);
  }

  // Called only after the owner installs the policy. Multiple read-only
  // consumers may observe a policy; an older reader cannot regress its ack.
  void Acknowledge(LiveSetting key, SettingRequest used) {
    auto& slot = slots_[size_t(key)];
    auto& applied = slot.applied;
    uint64_t old = applied.load(std::memory_order_relaxed);
    while (uint32_t(old >> 32) < used.version() &&
           !applied.compare_exchange_weak(old, used.packed,
                                          std::memory_order_release,
                                          std::memory_order_relaxed)) {}
    // An optional pass may recover after a temporary fallback while keeping
    // the same request. Clear that stale rejection, but a late completion
    // must never erase the rejection of a newer UI edit.
    old = slot.rejected.load(std::memory_order_relaxed);
    while (uint32_t(old >> 32) && uint32_t(old >> 32) <= used.version() &&
           !slot.rejected.compare_exchange_weak(old, 0,
                                                std::memory_order_release,
                                                std::memory_order_relaxed)) {}
  }

  int32_t Consume(LiveSetting key) {
    auto request = Requested(key);
    Acknowledge(key, request);
    return request.value();
  }

  static int32_t PackFrameRates(int32_t game, int32_t menu) {
    return int32_t(uint32_t(uint16_t(game)) | (uint32_t(uint16_t(menu)) << 16));
  }
  static int32_t GameFrameRate(int32_t packed) { return uint16_t(uint32_t(packed)); }
  static int32_t MenuFrameRate(int32_t packed) { return uint16_t(uint32_t(packed) >> 16); }
  static bool IsSupportedFrameRate(int32_t rate) { return rate == 30 || rate == 60; }

 private:
  struct Slot {
    std::atomic<uint64_t> requested{0};
    std::atomic<uint64_t> applied{0};
    std::atomic<uint64_t> rejected{0};
  };
  std::array<Slot, size_t(LiveSetting::kCount)> slots_{};
};
static_assert(std::atomic<uint64_t>::is_always_lock_free);

// A swap keeps one snapshot of the four colour requests until submission.
// Values use millesimals like FSR sharpness; no renderer reads mutable CVars.
// The order is also the vec4 push-constant layout: saturation, vibrance,
// contrast, brightness. An unavailable request retains the last usable value.
struct SwapColorSettingsRequest {
  static constexpr std::array<LiveSetting, 4> kKeys = {
      LiveSetting::kColorSaturation, LiveSetting::kColorVibrance,
      LiveSetting::kColorContrast, LiveSetting::kColorBrightness};
  static constexpr std::array<int32_t, 4> kNeutral = {1000, 0, 1000, 0};
  std::array<SettingRequest, 4> requests{};
  std::array<int32_t, 4> values = kNeutral;

  static SwapColorSettingsRequest Read(const LiveSettings& settings) {
    SwapColorSettingsRequest result;
    for (size_t i = 0; i < kKeys.size(); ++i) {
      result.requests[i] = settings.Requested(kKeys[i]);
      const auto rejected = settings.Rejected(kKeys[i]);
      result.values[i] = rejected.version() && rejected.packed == result.requests[i].packed
                            ? settings.Applied(kKeys[i]).value()
                            : result.requests[i].value();
    }
    return result;
  }
  bool NeedsPass() const { return values != kNeutral; }
  void WriteConstants(float (&destination)[4]) const {
    for (size_t i = 0; i < values.size(); ++i)
      destination[i] = float(values[i]) * 0.001f;
  }
  void Complete(LiveSettings& settings, bool grade_pass_recorded, bool submitted) const {
    for (size_t i = 0; i < kKeys.size(); ++i)
      settings.CompleteEffect(kKeys[i], requests[i],
          grade_pass_recorded ? values[i] : kNeutral[i], submitted);
  }
};

LiveSettings& GetLiveSettings();
// The only bridge from raw CVars into live storage; called once after config
// loading and before presentation/runtime setup. Subsequent changes use Request.
void InitializeLiveSettingsFromCVars();

}  // namespace rex::platform::prospero
