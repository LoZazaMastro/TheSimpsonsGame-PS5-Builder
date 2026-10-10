// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// ReX/PS5 adaptation: native timestamps, actual 60/120-Hz ceiling, Auto 2x/3x/4x only.

#pragma once

#include <rex/platform/framegen_prospero.h>

#include <chrono>
#include <optional>

#include "types.h"

namespace rex::ui::vulkan::framegen {

struct FrameGenPlan {
    size_t generations{};
    bool warm{};
    f32 native_rate{};
    f32 output_target{};
    size_t ceiling{};
    const char* reason = "warmup";
};

class FrameGenPacer {
public:
    explicit FrameGenPacer(rex::platform::prospero::FrameGenerationOptions options_ = {})
        : options(options_) {}
    [[nodiscard]] FrameGenPlan Plan(size_t capacity, u64 native_time_ns,
                                    u32 actual_output_millihertz);
    void CommitGeneratedCount(size_t count) { issued_generations = count; }

    void Reset();

private:
    using Clock = std::chrono::steady_clock;

    void Stabilize(Clock::time_point now);
    void DeferEvaluations(Clock::duration amount);
    void UpdateLimit(Clock::time_point now, f32 base_rate, f32 target_rate, size_t ceiling);

    rex::platform::prospero::FrameGenerationOptions options;
    std::optional<Clock::time_point> last_frame;
    std::optional<Clock::time_point> stable_until;
    std::optional<Clock::time_point> probe_until;
    std::optional<Clock::time_point> next_probe;
    std::optional<Clock::time_point> deficit_since;
    f32 smoothed_interval{};
    f32 output_credit{};
    f32 probe_base_rate{};
    f32 unloaded_base_rate{};
    size_t issued_generations{};
    size_t probe_previous_limit{};
    size_t limit{};
    u32 probe_failures{};
};

} // namespace rex::ui::vulkan::framegen
