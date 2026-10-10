#pragma once

// Implemented by the native app CRT. Safe before the C/C++ runtime, allocation,
// cvars or ReXGlue logging are initialized. Each call appends and flushes a
// marker to <prospero_state_root()>/logs/startup.log and mirrors it to kernel
// debug. The first native open fixes storage before constructors/threads run.
extern "C" void prospero_startup_log(const char* message);

// /app0 by default. Only native EROFS on the first log open, or EROFS on its
// mkdir paired with ENOENT on that open, selects /data/homebrew/PPSA74207.
// Runtime state follows this immutable choice;
// game data and the anchor for explicit relative paths remain /app0.
extern "C" const char* prospero_state_root();
