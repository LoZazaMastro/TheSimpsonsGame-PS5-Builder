/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/audio/audio_driver.h>
#include <rex/audio/flags.h>

REXCVAR_DEFINE_BOOL(audio_mute, false, "Audio", "Mute audio output");
// Guest frames are planar 6 x 256 big-endian floats; the companion .ts file
// contains their monotonic submission timestamps as uint64 nanoseconds.
REXCVAR_DEFINE_STRING(
    audio_dump_file, "", "Audio",
    "Write every submitted audio frame (raw 6 x 256 big-endian floats) to this file, "
    "and uint64 nanosecond timestamps to <file>.ts; empty = off")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_BOOL(audio_log_underruns, false, "Audio",
                    "Log how many played frames were silence because no frame was queued, and how "
                    "many frames the game submitted were entirely silent (diagnostic)");

namespace rex::audio {

AudioDriver::AudioDriver(memory::Memory* memory) : memory_(memory) {}

AudioDriver::~AudioDriver() = default;

}  // namespace rex::audio
