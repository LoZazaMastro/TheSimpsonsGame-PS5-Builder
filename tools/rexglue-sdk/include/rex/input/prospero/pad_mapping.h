// BSD 3-Clause License. See the project LICENSE.
#pragma once

#include <rex/input/input.h>
#include <rex/input/prospero/pad_abi.h>

namespace rex::input::prospero {

inline int16_t MapStickAxis(uint8_t value) {
  // Both central codes (127 and 128) represent zero; preserve full travel
  // and let the game apply its existing stick dead zones.
  return value < 128 ? int16_t((int(value) - 127) * 32768 / 127)
                     : int16_t((int(value) - 128) * 32767 / 127);
}

inline X_INPUT_GAMEPAD MapPadState(const PadData& pad) {
  X_INPUT_GAMEPAD gamepad{};
  uint16_t buttons = 0;
  const auto map = [&](PadButton native, X_INPUT_GAMEPAD_BUTTON guest) {
    if (pad.buttons & native) {
      buttons |= guest;
    }
  };
  map(kPadUp, X_INPUT_GAMEPAD_DPAD_UP);
  map(kPadDown, X_INPUT_GAMEPAD_DPAD_DOWN);
  map(kPadLeft, X_INPUT_GAMEPAD_DPAD_LEFT);
  map(kPadRight, X_INPUT_GAMEPAD_DPAD_RIGHT);
  map(kPadOptions, X_INPUT_GAMEPAD_START);
  map(kPadTouchpad, X_INPUT_GAMEPAD_BACK);
  map(kPadL3, X_INPUT_GAMEPAD_LEFT_THUMB);
  map(kPadR3, X_INPUT_GAMEPAD_RIGHT_THUMB);
  map(kPadL1, X_INPUT_GAMEPAD_LEFT_SHOULDER);
  map(kPadR1, X_INPUT_GAMEPAD_RIGHT_SHOULDER);
  map(kPadCross, X_INPUT_GAMEPAD_A);
  map(kPadCircle, X_INPUT_GAMEPAD_B);
  map(kPadSquare, X_INPUT_GAMEPAD_X);
  map(kPadTriangle, X_INPUT_GAMEPAD_Y);
  gamepad.buttons = buttons;
  gamepad.left_trigger = pad.analog_buttons.l2;
  gamepad.right_trigger = pad.analog_buttons.r2;
  gamepad.thumb_lx = MapStickAxis(pad.left_stick.x);
  gamepad.thumb_ly = MapStickAxis(uint8_t(255 - pad.left_stick.y));
  gamepad.thumb_rx = MapStickAxis(pad.right_stick.x);
  gamepad.thumb_ry = MapStickAxis(uint8_t(255 - pad.right_stick.y));
  return gamepad;
}

inline bool IsSettingsChord(const X_INPUT_GAMEPAD& gamepad) {
  constexpr uint16_t chord = X_INPUT_GAMEPAD_LEFT_SHOULDER |
                             X_INPUT_GAMEPAD_RIGHT_SHOULDER | X_INPUT_GAMEPAD_START;
  return (uint16_t(gamepad.buttons) & chord) == chord;
}

inline uint64_t PadKeyField(const X_INPUT_GAMEPAD& gamepad) {
  // Match XInput's virtual-key directions, including a single diagonal key
  // rather than simultaneous cardinal keys when a stick is diagonal.
  uint64_t field = uint16_t(gamepad.buttons) & 0xf3ff;
  field |= uint64_t(gamepad.left_trigger > 31) << 16;
  field |= uint64_t(gamepad.right_trigger > 31) << 17;
  for (size_t stick = 0; stick < 2; ++stick) {
    const int16_t x = stick ? int16_t(gamepad.thumb_rx) : int16_t(gamepad.thumb_lx);
    const int16_t y = stick ? int16_t(gamepad.thumb_ry) : int16_t(gamepad.thumb_ly);
    const bool up = y > 0x4e00, down = y < -0x4e01;
    const bool right = x > 0x4e00, left = x < -0x4e01;
    const size_t base = 18 + stick * 8;
    if (up && left) field |= uint64_t(1) << (base + 4);
    else if (up && right) field |= uint64_t(1) << (base + 5);
    else if (down && right) field |= uint64_t(1) << (base + 6);
    else if (down && left) field |= uint64_t(1) << (base + 7);
    else if (up) field |= uint64_t(1) << base;
    else if (down) field |= uint64_t(1) << (base + 1);
    else if (right) field |= uint64_t(1) << (base + 2);
    else if (left) field |= uint64_t(1) << (base + 3);
  }
  return field;
}

}  // namespace rex::input::prospero
