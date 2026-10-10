// BSD 3-Clause License. See the project LICENSE.
#include <rex/input/prospero/prospero_input_driver.h>

#include <algorithm>
#include <bit>
#include <cstring>

#include <rex/input/prospero/pad_mapping.h>
#include <rex/logging.h>
#include <rex/platform.h>
#include <rex/platform/framegen_prospero.h>
#include <rex/platform/trace_game_prospero.h>
#include <rex/ui/overlay_input.h>
#include <rex/ui/virtual_key.h>

#if !REX_PLATFORM_PROSPERO
#error The Prospero input backend requires a native PS5 build.
#endif

namespace rex::input::prospero {
namespace {

using Clock = std::chrono::steady_clock;
using rex::prospero::diagnostics::PathHash;
using rex::prospero::diagnostics::RateLimit;
RateLimit input_changes{64};
RateLimit input_errors{8};
constexpr uint16_t kFrameGenerationChord = X_INPUT_GAMEPAD_LEFT_SHOULDER |
    X_INPUT_GAMEPAD_RIGHT_SHOULDER | X_INPUT_GAMEPAD_LEFT_THUMB | X_INPUT_GAMEPAD_RIGHT_THUMB;

class PadManager {
 public:
  ~PadManager() {
    for (auto& slot : slots_) {
      CloseSlot(slot);
    }
  }

  bool Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    return InitializeLocked();
  }

  X_RESULT Poll(uint32_t user_index, X_INPUT_STATE* out_state) {
    if (!out_state || user_index >= kPadUserCount) {
      return X_ERROR_INVALID_PARAMETER;
    }
    *out_state = {};
    std::unique_lock<std::mutex> lock(mutex_);
    if (!InitializeLocked()) {
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }
    RefreshUsersLocked();
    auto& slot = slots_[user_index];
    if (slot.user_id < 0) {
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }
    if (slot.handle < 0 && Clock::now() >= slot.next_open) {
      slot.next_open = Clock::now() + std::chrono::milliseconds(500);
      slot.handle = scePadOpen(slot.user_id, kPadPortStandard, 0, nullptr);
      slot.owned = slot.handle >= 0;
      if (uint32_t(slot.handle) == kPadAlreadyOpened) {
        slot.handle = scePadGetHandle(slot.user_id, kPadPortStandard, 0);
      }
      if (slot.handle < 0) {
        LogError(slot, "scePadOpen", slot.handle);
        return X_ERROR_DEVICE_NOT_CONNECTED;
      }
      slot.last_error = 0;
    }
    if (slot.handle < 0) {
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }
    PadData native{};
    const int result = scePadReadState(slot.handle, &native);
    if (result != 0) {
      LogError(slot, "scePadReadState", result);
      CloseSlot(slot);
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }
    if (!native.connected) {
      // Reconnecting with the shortcut held must not look like a new press.
      slot.frame_generation_chord_armed = false;
      slot.frame_generation_chord_consumed = false;
      if (slot.connected) {
        prospero_trace_checkpoint(PROSPERO_GAME_INPUT_CONNECTION, uint32_t(slot.user_id),
                                  uint32_t(slot.handle), 0);
        ++slot.packet;
        slot.gamepad = {};
        slot.connected = false;
        slot.rumble = false;
        slot.vibration_known = false;
        slot.last_vibration = {};
      }
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }
    const auto gamepad = MapPadState(native);
    if (!slot.connected) {
      prospero_trace_checkpoint(PROSPERO_GAME_INPUT_CONNECTION, uint32_t(slot.user_id),
                                uint32_t(slot.handle), 1);
      // Compatible mode exposes the two ordinary motors without creating a
      // second AudioOut vibration stream or changing the game's rumble model.
      const int mode = scePadSetVibrationMode(slot.handle, 2);
      slot.rumble = mode == 0;
      slot.vibration_known = false;
      if (mode != 0) {
        LogError(slot, "scePadSetVibrationMode", mode);
      }
    }
    if (uint16_t(slot.gamepad.buttons) != uint16_t(gamepad.buttons) &&
        input_changes.Allow(PROSPERO_GAME_INPUT_BUTTONS)) {
      prospero_trace_checkpoint(PROSPERO_GAME_INPUT_BUTTONS, user_index,
                                uint16_t(slot.gamepad.buttons), uint16_t(gamepad.buttons));
    }
    const bool analog_changed = slot.gamepad.left_trigger != gamepad.left_trigger ||
        slot.gamepad.right_trigger != gamepad.right_trigger ||
        int16_t(slot.gamepad.thumb_lx) != int16_t(gamepad.thumb_lx) ||
        int16_t(slot.gamepad.thumb_ly) != int16_t(gamepad.thumb_ly) ||
        int16_t(slot.gamepad.thumb_rx) != int16_t(gamepad.thumb_rx) ||
        int16_t(slot.gamepad.thumb_ry) != int16_t(gamepad.thumb_ry);
    const auto now = Clock::now();
    if (analog_changed || slot.analog_pending) {
      if (now >= slot.next_analog_trace) {
        const uint64_t axes = uint64_t(uint16_t(int16_t(gamepad.thumb_lx))) |
            (uint64_t(uint16_t(int16_t(gamepad.thumb_ly))) << 16) |
            (uint64_t(uint16_t(int16_t(gamepad.thumb_rx))) << 32) |
            (uint64_t(uint16_t(int16_t(gamepad.thumb_ry))) << 48);
        prospero_trace_checkpoint(PROSPERO_GAME_INPUT_ANALOG,
            (uint64_t(slot.analog_coalesced) << 32) | user_index, axes,
            uint32_t(gamepad.left_trigger) | (uint32_t(gamepad.right_trigger) << 8));
        slot.analog_coalesced = 0;
        slot.analog_pending = false;
        slot.next_analog_trace = now + std::chrono::milliseconds(250);
      } else if (analog_changed) {
        ++slot.analog_coalesced;
        slot.analog_pending = true;
      }
    }
    if (!slot.connected || std::memcmp(&slot.gamepad, &gamepad, sizeof(gamepad)) != 0) {
      ++slot.packet;
      slot.gamepad = gamepad;
    }
    slot.connected = true;
    out_state->packet_number = slot.packet;
    out_state->gamepad = slot.gamepad;
    bool toggle_frame_generation = false;
    if (user_index == 0) {
      const uint16_t buttons = uint16_t(out_state->gamepad.buttons);
      const uint16_t chord_buttons = buttons & kFrameGenerationChord;
      if (!chord_buttons) {
        slot.frame_generation_chord_armed = true;
        slot.frame_generation_chord_consumed = false;
      } else if (chord_buttons == kFrameGenerationChord) {
        toggle_frame_generation = slot.frame_generation_chord_armed;
        slot.frame_generation_chord_armed = false;
        slot.frame_generation_chord_consumed = true;
      }
      if (slot.frame_generation_chord_consumed) {
        // Keep the entire shortcut suppressed until all four buttons are up.
        // In particular, releasing L1/R1 first must not expose L3+R3 to freecam.
        // UI polls, guest GetState and GetKeystroke share this single latch.
        out_state->gamepad.buttons = uint16_t(buttons & ~kFrameGenerationChord);
      }
    }
    lock.unlock();
    if (toggle_frame_generation) {
      // This is only an atomic session request. Core diagnostics and eventual
      // renderer changes happen outside the pad lock; no GPU work runs here.
      platform::prospero::ToggleFrameGenerationForSession();
    }
    return X_ERROR_SUCCESS;
  }

  bool HasRumble(uint32_t user_index) {
    std::lock_guard<std::mutex> lock(mutex_);
    return user_index < kPadUserCount && slots_[user_index].connected && slots_[user_index].rumble;
  }

  X_RESULT SetRumble(uint32_t user_index, uint16_t large, uint16_t small) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (user_index >= kPadUserCount) {
      return X_ERROR_INVALID_PARAMETER;
    }
    auto& slot = slots_[user_index];
    if (slot.handle < 0 || !slot.connected) {
      return X_ERROR_DEVICE_NOT_CONNECTED;
    }
    if (!slot.rumble) {
      return X_ERROR_FUNCTION_FAILED;
    }
    PadVibration vibration{uint8_t(large >> 8), uint8_t(small >> 8)};
    if (slot.vibration_known && vibration.large_motor == slot.last_vibration.large_motor &&
        vibration.small_motor == slot.last_vibration.small_motor) {
      return X_ERROR_SUCCESS;
    }
    const int result = scePadSetVibration(slot.handle, &vibration);
    if (input_changes.Allow(PROSPERO_GAME_INPUT_RUMBLE)) {
      prospero_trace_checkpoint(PROSPERO_GAME_INPUT_RUMBLE, uint32_t(slot.user_id),
          uint32_t(vibration.large_motor) | (uint32_t(vibration.small_motor) << 8),
          uint32_t(result));
    }
    if (result != 0) {
      LogError(slot, "scePadSetVibration", result);
      return X_ERROR_FUNCTION_FAILED;
    }
    slot.last_vibration = vibration;
    slot.vibration_known = true;
    return X_ERROR_SUCCESS;
  }

  void StopRumble() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& slot : slots_) {
      if (slot.handle >= 0 && slot.rumble) {
        const PadVibration zero{};
        const int result = scePadSetVibration(slot.handle, &zero);
        if (result != 0) {
          LogError(slot, "scePadSetVibration (stop)", result);
        } else {
          slot.last_vibration = {};
          slot.vibration_known = true;
        }
      }
    }
  }

 private:
  struct Slot {
    int user_id = -1;
    int handle = -1;
    bool owned = false;
    bool connected = false;
    bool rumble = false;
    bool vibration_known = false;
    uint32_t packet = 0;
    uint32_t last_error = 0;
    X_INPUT_GAMEPAD gamepad{};
    PadVibration last_vibration{};
    Clock::time_point next_open{};
    Clock::time_point next_analog_trace{};
    uint32_t analog_coalesced = 0;
    bool analog_pending = false;
    // Start disarmed; require a sampled release after startup or disconnect.
    bool frame_generation_chord_armed = false;
    bool frame_generation_chord_consumed = false;
  };

  static void LogError(Slot& slot, const char* operation, int code) {
    if (slot.last_error != uint32_t(code) && input_errors.Allow(PROSPERO_GAME_INPUT_ERROR)) {
      rex::prospero::diagnostics::PreserveErrno preserve;
      prospero_trace_checkpoint(PROSPERO_GAME_INPUT_ERROR, uint32_t(slot.user_id),
                                PathHash(operation), uint32_t(code));
      REXLOG_WARN("PS5 {} (user {}) failed: 0x{:08X}", operation, slot.user_id, uint32_t(code));
      slot.last_error = uint32_t(code);
    }
  }

  static void CloseSlot(Slot& slot) {
    if (slot.connected) {
      prospero_trace_checkpoint(PROSPERO_GAME_INPUT_CONNECTION, uint32_t(slot.user_id),
                                uint32_t(slot.handle), 0);
    }
    if (slot.handle >= 0) {
      if (slot.rumble) {
        const PadVibration zero{};
        scePadSetVibration(slot.handle, &zero);
      }
      if (slot.owned) {
        const int result = scePadClose(slot.handle);
        if (result != 0) {
          LogError(slot, "scePadClose", result);
        }
      }
    }
    slot.handle = -1;
    slot.owned = false;
    if (slot.connected) {
      ++slot.packet;
    }
    slot.connected = false;
    slot.rumble = false;
    slot.vibration_known = false;
    slot.gamepad = {};
    slot.last_vibration = {};
    slot.analog_pending = false;
    slot.analog_coalesced = 0;
    slot.frame_generation_chord_armed = false;
    slot.frame_generation_chord_consumed = false;
    slot.next_analog_trace = {};
    slot.next_open = Clock::now() + std::chrono::milliseconds(500);
  }

  bool InitializeLocked() {
    if (initialization_attempted_) {
      return initialized_;
    }
    initialization_attempted_ = true;
    const int user_result = sceUserServiceInitialize(nullptr);
    prospero_trace_checkpoint(PROSPERO_GAME_INPUT_INIT, 0, uint32_t(user_result), 0);
    if (user_result != 0 && uint32_t(user_result) != kUserServiceAlreadyInitialized) {
      REXLOG_ERROR("PS5 sceUserServiceInitialize failed: 0x{:08X}", uint32_t(user_result));
      return false;
    }
    const int pad_result = scePadInit();
    prospero_trace_checkpoint(PROSPERO_GAME_INPUT_INIT, 1, uint32_t(pad_result), 0);
    if (pad_result != 0) {
      REXLOG_ERROR("PS5 scePadInit failed: 0x{:08X}", uint32_t(pad_result));
      return false;
    }
    initialized_ = true;
    RefreshUsersLocked();
    return true;
  }

  void RefreshUsersLocked() {
    const auto now = Clock::now();
    if (now < next_user_refresh_) {
      return;
    }
    next_user_refresh_ = now + std::chrono::milliseconds(500);
    std::array<int, kPadUserCount> users{-1, -1, -1, -1};
    const int result = sceUserServiceGetLoginUserIdList(users.data());
    if (result != 0) {
      if (last_user_error_ != uint32_t(result)) {
        REXLOG_WARN("PS5 sceUserServiceGetLoginUserIdList failed: 0x{:08X}", uint32_t(result));
        last_user_error_ = uint32_t(result);
      }
      for (auto& slot : slots_) {
        CloseSlot(slot);
        slot.user_id = -1;
      }
      return;
    }
    last_user_error_ = 0;
    int initial_user = -1;
    if (sceUserServiceGetInitialUser(&initial_user) == 0) {
      const auto initial = std::find(users.begin(), users.end(), initial_user);
      if (initial != users.end()) {
        std::iter_swap(users.begin(), initial);
      }
    }
    // Keep existing assignments stable across login-list reordering. New
    // logged-in users take the first free slot, with the initial user first
    // at startup. The system audio user 0xff is never a fabricated player.
    for (auto& slot : slots_) {
      if (slot.user_id >= 0 &&
          std::find(users.begin(), users.end(), slot.user_id) == users.end()) {
        prospero_trace_checkpoint(PROSPERO_GAME_INPUT_USER, &slot - slots_.data(),
                                  uint32_t(slot.user_id), 0);
        CloseSlot(slot);
        slot.user_id = -1;
      }
    }
    for (int user : users) {
      if (user < 0 || user == kPadSystemUser) {
        continue;
      }
      const auto existing = std::find_if(slots_.begin(), slots_.end(),
                                         [user](const Slot& slot) { return slot.user_id == user; });
      if (existing != slots_.end()) {
        continue;
      }
      const auto free = std::find_if(slots_.begin(), slots_.end(),
                                     [](const Slot& slot) { return slot.user_id < 0; });
      if (free != slots_.end()) {
        free->user_id = user;
        free->next_open = now;
        free->last_error = 0;
        prospero_trace_checkpoint(PROSPERO_GAME_INPUT_USER, free - slots_.begin(),
                                  uint32_t(user), 1);
      }
    }
  }

  std::mutex mutex_;
  bool initialization_attempted_ = false;
  bool initialized_ = false;
  uint32_t last_user_error_ = 0;
  Clock::time_point next_user_refresh_{};
  std::array<Slot, kPadUserCount> slots_{};
};

PadManager& Pads() {
  static PadManager pads;
  return pads;
}

constexpr std::array<ui::VirtualKey, 34> kKeyLookup = {
    ui::VirtualKey::kXInputPadDpadUp, ui::VirtualKey::kXInputPadDpadDown,
    ui::VirtualKey::kXInputPadDpadLeft, ui::VirtualKey::kXInputPadDpadRight,
    ui::VirtualKey::kXInputPadStart, ui::VirtualKey::kXInputPadBack,
    ui::VirtualKey::kXInputPadLThumbPress, ui::VirtualKey::kXInputPadRThumbPress,
    ui::VirtualKey::kXInputPadLShoulder, ui::VirtualKey::kXInputPadRShoulder,
    ui::VirtualKey::kNone, ui::VirtualKey::kNone,
    ui::VirtualKey::kXInputPadA, ui::VirtualKey::kXInputPadB,
    ui::VirtualKey::kXInputPadX, ui::VirtualKey::kXInputPadY,
    ui::VirtualKey::kXInputPadLTrigger, ui::VirtualKey::kXInputPadRTrigger,
    ui::VirtualKey::kXInputPadLThumbUp, ui::VirtualKey::kXInputPadLThumbDown,
    ui::VirtualKey::kXInputPadLThumbRight, ui::VirtualKey::kXInputPadLThumbLeft,
    ui::VirtualKey::kXInputPadLThumbUpLeft, ui::VirtualKey::kXInputPadLThumbUpRight,
    ui::VirtualKey::kXInputPadLThumbDownRight, ui::VirtualKey::kXInputPadLThumbDownLeft,
    ui::VirtualKey::kXInputPadRThumbUp, ui::VirtualKey::kXInputPadRThumbDown,
    ui::VirtualKey::kXInputPadRThumbRight, ui::VirtualKey::kXInputPadRThumbLeft,
    ui::VirtualKey::kXInputPadRThumbUpLeft, ui::VirtualKey::kXInputPadRThumbUpRight,
    ui::VirtualKey::kXInputPadRThumbDownRight, ui::VirtualKey::kXInputPadRThumbDownLeft,
};

}  // namespace

X_RESULT PollPadState(uint32_t user_index, X_INPUT_STATE* out_state) {
  return Pads().Poll(user_index, out_state);
}

ProsperoInputDriver::ProsperoInputDriver(rex::ui::Window* window, size_t window_z_order)
    : InputDriver(window, window_z_order) {}

ProsperoInputDriver::~ProsperoInputDriver() {
  Pads().StopRumble();
}

X_STATUS ProsperoInputDriver::Setup() {
  return Pads().Initialize() ? X_STATUS_SUCCESS : X_STATUS_UNSUCCESSFUL;
}

X_RESULT ProsperoInputDriver::GetCapabilities(uint32_t user_index, uint32_t flags,
                                              X_INPUT_CAPABILITIES* out_caps) {
  // XAM has already resolved ANY_USER to a concrete user index, but forwards
  // its selector bit to the backend in both capabilities entry points.
  constexpr uint32_t kXamAnyUser = uint32_t(1) << 30;
  if (!out_caps || (flags & ~(uint32_t(X_INPUT_FLAG_GAMEPAD) | kXamAnyUser))) {
    return X_ERROR_INVALID_PARAMETER;
  }
  *out_caps = {};
  X_INPUT_STATE state{};
  const X_RESULT result = PollPadState(user_index, &state);
  if (result != X_ERROR_SUCCESS) {
    return result;
  }
  out_caps->type = XINPUT_DEVTYPE_GAMEPAD;
  out_caps->sub_type = 1;
  const bool rumble = Pads().HasRumble(user_index);
  out_caps->flags = uint16_t(rumble ? X_INPUT_CAPS_FFB_SUPPORTED : 0);
  out_caps->gamepad.buttons = 0xf3ff;
  out_caps->gamepad.left_trigger = 0xff;
  out_caps->gamepad.right_trigger = 0xff;
  out_caps->gamepad.thumb_lx = out_caps->gamepad.thumb_ly = int16_t(32767);
  out_caps->gamepad.thumb_rx = out_caps->gamepad.thumb_ry = int16_t(32767);
  out_caps->vibration.left_motor_speed = uint16_t(rumble ? 0xffff : 0);
  out_caps->vibration.right_motor_speed = uint16_t(rumble ? 0xffff : 0);
  return X_ERROR_SUCCESS;
}

X_RESULT ProsperoInputDriver::GetState(uint32_t user_index, X_INPUT_STATE* out_state) {
  if (!out_state || user_index >= kPadUserCount) {
    return X_ERROR_INVALID_PARAMETER;
  }
  X_INPUT_STATE raw{};
  const X_RESULT result = PollPadState(user_index, &raw);
  const bool captured = !is_active() || ui::IsOverlayInputActive() || IsSettingsChord(raw.gamepad);
  const X_INPUT_GAMEPAD effective = captured ? X_INPUT_GAMEPAD{} : raw.gamepad;
  if (captured && result == X_ERROR_SUCCESS && Pads().HasRumble(user_index)) {
    Pads().SetRumble(user_index, 0, 0);
  }
  std::lock_guard<std::mutex> lock(states_mutex_);
  auto& state = states_[user_index];
  const bool connected = result == X_ERROR_SUCCESS;
  if (!state.capture_known || state.captured != captured) {
    state.capture_known = true;
    state.captured = captured;
    if (input_changes.Allow(PROSPERO_GAME_INPUT_CAPTURE)) {
      prospero_trace_checkpoint(PROSPERO_GAME_INPUT_CAPTURE, user_index, captured, connected);
    }
  }
  if (connected != state.connected || std::memcmp(&effective, &state.gamepad, sizeof(effective))) {
    state.gamepad = effective;
    state.connected = connected;
    ++state.packet;
  }
  *out_state = {};
  out_state->packet_number = state.packet;
  out_state->gamepad = state.gamepad;
  return result;
}

X_RESULT ProsperoInputDriver::SetState(uint32_t user_index, X_INPUT_VIBRATION* vibration) {
  if (!vibration) {
    return X_ERROR_INVALID_PARAMETER;
  }
  X_INPUT_STATE state{};
  const X_RESULT result = PollPadState(user_index, &state);
  if (result != X_ERROR_SUCCESS) {
    return result;
  }
  const bool captured = !is_active() || ui::IsOverlayInputActive() || IsSettingsChord(state.gamepad);
  return Pads().SetRumble(user_index, captured ? 0 : uint16_t(vibration->left_motor_speed),
                          captured ? 0 : uint16_t(vibration->right_motor_speed));
}

X_RESULT ProsperoInputDriver::GetKeystroke(uint32_t user_index, uint32_t,
                                           X_INPUT_KEYSTROKE* out_keystroke) {
  if (!out_keystroke || (user_index >= kPadUserCount && user_index != 0xff)) {
    return X_ERROR_INVALID_PARAMETER;
  }
  *out_keystroke = {};
  const uint32_t first = user_index == 0xff ? 0 : user_index;
  const uint32_t end = user_index == 0xff ? uint32_t(kPadUserCount) : user_index + 1;
  bool any_connected = false;
  for (uint32_t user = first; user < end; ++user) {
    X_INPUT_STATE current{};
    const X_RESULT result = GetState(user, &current);
    std::lock_guard<std::mutex> lock(states_mutex_);
    auto& state = states_[user];
    if (result != X_ERROR_SUCCESS) {
      state.keys = state.repeat_key = 0;
      continue;
    }
    any_connected = true;
    const uint64_t keys = PadKeyField(current.gamepad);
    const uint64_t released = state.keys & ~keys;
    const uint64_t pressed = keys & ~state.keys;
    uint64_t event_key = 0;
    uint16_t event_flags = 0;
    const auto now = Clock::now();
    // Drain transitions before repeats so a released key can never repeat.
    if (released) {
      event_key = uint64_t(1) << std::countr_zero(released);
      state.keys &= ~event_key;
      if (state.repeat_key == event_key) {
        state.repeat_key = 0;
      }
      event_flags = X_INPUT_KEYSTROKE_KEYUP;
    } else if (pressed) {
      event_key = uint64_t(1) << std::countr_zero(pressed);
      state.keys |= event_key;
      state.repeat_key = event_key;
      state.repeat_at = now + std::chrono::milliseconds(400);
      event_flags = X_INPUT_KEYSTROKE_KEYDOWN;
    } else if ((keys & state.repeat_key) && now >= state.repeat_at) {
      event_key = state.repeat_key;
      state.repeat_at = now + std::chrono::milliseconds(100);
      event_flags = X_INPUT_KEYSTROKE_KEYDOWN | X_INPUT_KEYSTROKE_REPEAT;
    }
    if (event_key) {
      out_keystroke->virtual_key = uint16_t(kKeyLookup[std::countr_zero(event_key)]);
      out_keystroke->flags = event_flags;
      out_keystroke->user_index = uint8_t(user);
      return X_ERROR_SUCCESS;
    }
  }
  return any_connected ? X_ERROR_EMPTY : X_ERROR_DEVICE_NOT_CONNECTED;
}

}  // namespace rex::input::prospero
