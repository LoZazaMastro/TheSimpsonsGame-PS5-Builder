// Native ABI adapted from ps5-payload-dev/SDL, revision
// ee4c47dc0d617b3bc8f35108f9956baf228a1322, src/joystick/ps5/SDL_ps5joystick.h.
// This version names the ABI types locally and verifies their layout.
/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2015 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#pragma once

#include <cstddef>
#include <cstdint>

namespace rex::input::prospero {

inline constexpr size_t kPadUserCount = 4;
inline constexpr int32_t kPadPortStandard = 0;
inline constexpr int32_t kPadSystemUser = 0xff;
inline constexpr uint32_t kPadAlreadyOpened = 0x80920004;
inline constexpr uint32_t kUserServiceAlreadyInitialized = 0x80960003;

enum PadButton : uint32_t {
  kPadL3 = 0x0002,
  kPadR3 = 0x0004,
  kPadOptions = 0x0008,
  kPadUp = 0x0010,
  kPadRight = 0x0020,
  kPadDown = 0x0040,
  kPadLeft = 0x0080,
  kPadL2 = 0x0100,
  kPadR2 = 0x0200,
  kPadL1 = 0x0400,
  kPadR1 = 0x0800,
  kPadTriangle = 0x1000,
  kPadCircle = 0x2000,
  kPadCross = 0x4000,
  kPadSquare = 0x8000,
  kPadTouchpad = 0x100000,
};

struct PadTouch {
  uint16_t x, y;
  uint8_t finger;
  uint8_t padding[3];
};

struct PadTouchData {
  uint8_t fingers;
  uint8_t padding[3];
  uint32_t reserved;
  PadTouch touch[2];
};

struct PadVibration {
  uint8_t large_motor, small_motor;
};

struct PadData {
  uint32_t buttons;
  struct { uint8_t x, y; } left_stick, right_stick;
  struct { uint8_t l2, r2; } analog_buttons;
  uint16_t padding;
  float orientation[4];
  float angular_velocity[3];
  float acceleration[3];
  PadTouchData touch;
  uint8_t connected;
  uint64_t timestamp;
  uint8_t extension[16];
  uint8_t count;
  uint8_t unknown[15];
};

static_assert(sizeof(PadTouch) == 8);
static_assert(sizeof(PadTouchData) == 24);
static_assert(offsetof(PadData, connected) == 76);
static_assert(offsetof(PadData, timestamp) == 80);
static_assert(sizeof(PadData) == 120);
static_assert(sizeof(PadVibration) == 2);

}  // namespace rex::input::prospero

extern "C" {
int scePadInit(void);
int scePadOpen(int user_id, int type, int index, void* parameter);
int scePadGetHandle(int user_id, int type, int index);
int scePadReadState(int handle, rex::input::prospero::PadData* state);
int scePadSetVibrationMode(int handle, int mode);
int scePadSetVibration(int handle, const rex::input::prospero::PadVibration* vibration);
int scePadClose(int handle);
int sceUserServiceInitialize(void* parameter);
int sceUserServiceGetLoginUserIdList(int user_ids[4]);
int sceUserServiceGetInitialUser(int* user_id);
}
