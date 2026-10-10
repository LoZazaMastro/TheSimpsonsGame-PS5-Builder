/**
 * @file        perf/frame_index.h
 * @brief       Guest-observed frame counter
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */
#pragma once

#include <cstdint>

namespace rex::perf {

// Monotonic count of guest frames presented, incremented in VdSwap on the
// guest thread itself.
//
// Deliberately NOT behind REXGLUE_ENABLE_PERF_COUNTERS, and deliberately not
// the command processor's own frame counter: guest-side instrumentation needs
// a frame boundary observed at the moment the guest declares one, with no
// command-queue skew, and it has to work in any build configuration.
//
// This lives in its own header rather than in perf/counter.h because
// counter.h is pulled in by the generated recompilation unit header, so every
// one of the ~82k translated functions depends on it -- touching it forces a
// multi-hour rebuild of the whole game for what is otherwise a two-line
// addition.
uint64_t GuestFrameIndex();
void AdvanceGuestFrameIndex();

}  // namespace rex::perf
