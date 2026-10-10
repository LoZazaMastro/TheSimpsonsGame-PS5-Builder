// Host-runnable PS5-to-XInput boundary checks, no controller required.
// g++ -std=c++23 -msse4.1 -I tools/rexglue-sdk/include this_file.cpp -o test
#include <rex/input/prospero/pad_mapping.h>

#include <cassert>
#include <cstring>

using namespace rex::input;
using namespace rex::input::prospero;

int main() {
  PadData pad{};
  pad.left_stick = {127, 128};
  pad.right_stick = {128, 127};
  auto state = MapPadState(pad);
  const X_INPUT_GAMEPAD neutral{};
  assert(std::memcmp(&state, &neutral, sizeof(state)) == 0);

  struct ButtonCase { uint32_t pad; uint16_t guest; };
  constexpr ButtonCase cases[] = {
      {kPadCross, X_INPUT_GAMEPAD_A}, {kPadCircle, X_INPUT_GAMEPAD_B},
      {kPadSquare, X_INPUT_GAMEPAD_X}, {kPadTriangle, X_INPUT_GAMEPAD_Y},
      {kPadL1, X_INPUT_GAMEPAD_LEFT_SHOULDER}, {kPadR1, X_INPUT_GAMEPAD_RIGHT_SHOULDER},
      {kPadL3, X_INPUT_GAMEPAD_LEFT_THUMB}, {kPadR3, X_INPUT_GAMEPAD_RIGHT_THUMB},
      {kPadOptions, X_INPUT_GAMEPAD_START}, {kPadTouchpad, X_INPUT_GAMEPAD_BACK},
      {kPadUp, X_INPUT_GAMEPAD_DPAD_UP}, {kPadDown, X_INPUT_GAMEPAD_DPAD_DOWN},
      {kPadLeft, X_INPUT_GAMEPAD_DPAD_LEFT}, {kPadRight, X_INPUT_GAMEPAD_DPAD_RIGHT},
  };
  for (const auto& test : cases) {
    pad.buttons = test.pad;
    state = MapPadState(pad);
    assert(uint16_t(state.buttons) == test.guest);
  }
  pad.buttons = kPadL2 | kPadR2;
  pad.analog_buttons = {31, 255};
  state = MapPadState(pad);
  assert(uint16_t(state.buttons) == 0);
  assert(state.left_trigger == 31 && state.right_trigger == 255);
  assert(PadKeyField(state) == (uint64_t(1) << 17));

  assert(MapStickAxis(0) == -32768);
  assert(MapStickAxis(127) == 0 && MapStickAxis(128) == 0);
  assert(MapStickAxis(255) == 32767);
  for (unsigned value = 1; value < 256; ++value) {
    assert(MapStickAxis(uint8_t(value)) >= MapStickAxis(uint8_t(value - 1)));
  }
  pad = {};
  pad.left_stick = {0, 0};
  pad.right_stick = {255, 255};
  state = MapPadState(pad);
  assert(int16_t(state.thumb_lx) == -32768 && int16_t(state.thumb_ly) == 32767);
  assert(int16_t(state.thumb_rx) == 32767 && int16_t(state.thumb_ry) == -32768);
  assert(PadKeyField(state) == ((uint64_t(1) << 22) | (uint64_t(1) << 32)));

  pad.buttons = kPadOptions | kPadL1;
  assert(!IsSettingsChord(MapPadState(pad)));
  pad.buttons |= kPadR1;
  assert(IsSettingsChord(MapPadState(pad)));

  // Guest memory must receive big-endian button and signed-axis words.
  state = {};
  state.buttons = X_INPUT_GAMEPAD_A;
  state.thumb_lx = int16_t(-32768);
  const auto* bytes = reinterpret_cast<const uint8_t*>(&state);
  assert(bytes[0] == 0x10 && bytes[1] == 0x00);
  assert(bytes[4] == 0x80 && bytes[5] == 0x00);
}
