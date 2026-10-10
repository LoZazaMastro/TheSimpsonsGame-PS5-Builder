// Native PS5 AudioOut ABI used by ps5-payload-dev/SDL's PS5 audio backend.
// Public reference: https://github.com/ps5-payload-dev/SDL/blob/
// ee4c47dc0d617b3bc8f35108f9956baf228a1322/src/audio/ps5/SDL_ps5audio.h
// These are ABI declarations, not a dependency on SDL.
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

#include <cstdint>

extern "C" {
int32_t sceAudioOutInit(void);
int32_t sceAudioOutOpen(int32_t user_id, int32_t type, int32_t index,
                        uint32_t samples, uint32_t sample_rate, uint32_t format);
int32_t sceAudioOutOutput(int32_t handle, const void* samples);
int32_t sceAudioOutClose(int32_t handle);
}

namespace rex::audio::prospero {
inline constexpr int32_t kAudioSystemUser = 0xff;
inline constexpr int32_t kAudioPortMain = 0;
inline constexpr uint32_t kAudioFormatS16Stereo = 1;
inline constexpr uint32_t kAudioAlreadyInitialized = 0x8026000e;
}  // namespace rex::audio::prospero
