// BSD 3-Clause License. See the project LICENSE.
#pragma once

#include <array>
#include <chrono>
#include <mutex>

#include <rex/input/input_driver.h>
#include <rex/input/prospero/pad_abi.h>

namespace rex::input::prospero {

// Shared native handles serve both guest XInput and the full-screen PS5 UI.
// This function returns controller state for UI navigation, consuming the
// player-one L1+R1+L3+R3 session shortcut before any UI or guest consumer sees it.
// GetState additionally applies focus, overlay capture and settings-chord rules.
X_RESULT PollPadState(uint32_t user_index, X_INPUT_STATE* out_state);

class ProsperoInputDriver final : public InputDriver {
 public:
  explicit ProsperoInputDriver(rex::ui::Window* window, size_t window_z_order);
  ~ProsperoInputDriver() override;

  X_STATUS Setup() override;
  X_RESULT GetCapabilities(uint32_t user_index, uint32_t flags,
                            X_INPUT_CAPABILITIES* out_caps) override;
  X_RESULT GetState(uint32_t user_index, X_INPUT_STATE* out_state) override;
  X_RESULT SetState(uint32_t user_index, X_INPUT_VIBRATION* vibration) override;
  X_RESULT GetKeystroke(uint32_t user_index, uint32_t flags,
                         X_INPUT_KEYSTROKE* out_keystroke) override;

 private:
  struct GuestState {
    X_INPUT_GAMEPAD gamepad{};
    uint32_t packet = 0;
    bool connected = false;
    bool captured = false;
    bool capture_known = false;
    uint64_t keys = 0;
    uint64_t repeat_key = 0;
    std::chrono::steady_clock::time_point repeat_at{};
  };
  std::mutex states_mutex_;
  std::array<GuestState, kPadUserCount> states_{};
};

}  // namespace rex::input::prospero
