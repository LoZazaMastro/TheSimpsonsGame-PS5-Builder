// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// ReX/PS5 adaptation: native timestamps, actual 60/120-Hz ceiling, Auto 2x/3x/4x only.

#include <algorithm>
#include <cmath>
#include <utility>

#include <limits>
#include "frame_gen_pacer.h"

namespace rex::ui::vulkan::framegen {

namespace {

using Clock = std::chrono::steady_clock;

constexpr f32 INTERVAL_SMOOTHING = 0.25f;
constexpr f32 MINIMUM_BASE_RATE = 10.0f;
constexpr f32 BURST_CADENCE_RATIO = 3.0f;
constexpr f32 BURST_TARGET_RATIO = 2.0f;
constexpr f32 PROBE_THROUGHPUT_TOLERANCE = 0.95f;
constexpr f32 PROBE_BASE_COLLAPSE_RATIO = 0.70f;
constexpr f32 PROBE_MARGINAL_GAIN = 1.15f;
constexpr f32 TARGET_SATISFIED_RATIO = 0.95f;
constexpr f32 UNLOADED_BASE_RETENTION = 0.75f;
constexpr f32 CREDIT_EPSILON = 1.0e-4f;
constexpr u32 MAX_PROBE_FAILURES = 4;

constexpr auto STABILIZATION_DURATION = std::chrono::seconds(1);
constexpr auto PROBE_DURATION = std::chrono::seconds(1);
constexpr auto DEFICIT_DURATION = std::chrono::seconds(1);
constexpr auto PROBE_STEP_DELAY = std::chrono::milliseconds(250);

[[nodiscard]] Clock::duration ProbeBackoff(u32 failures) {
    switch (failures) {
    case 1:
        return std::chrono::seconds(5);
    case 2:
        return std::chrono::seconds(15);
    case 3:
        return std::chrono::seconds(30);
    default:
        return std::chrono::seconds(60);
    }
}

} // Anonymous namespace

FrameGenPlan FrameGenPacer::Plan(size_t capacity, u64 native_time_ns,
                                 u32 actual_output_millihertz) {
    if (!options.enabled) return {.reason = "off"};
    if (!actual_output_millihertz) return {.reason = "output_unknown"};
    const size_t requested_limit = options.target_rate == 0
        ? std::clamp<size_t>(options.multiplier, 2, 4) - 1 : 3;
    const size_t ceiling = std::min(capacity, requested_limit);
    if (ceiling == 0) {
        Reset();
        return {.reason = "no_capacity"};
    }
    if (native_time_ns > u64(std::numeric_limits<int64_t>::max())) {
        Reset();
        return {.reason = "invalid_native_timestamp"};
    }
    const Clock::time_point now{std::chrono::duration_cast<Clock::duration>(
        std::chrono::nanoseconds(native_time_ns))};
    const size_t previous_generations = std::exchange(issued_generations, 0);
    if (!last_frame) {
        last_frame = now;
        return {.ceiling = ceiling, .reason = "first_native_frame"};
    }
    const Clock::duration interval = now - *last_frame;
    const f32 interval_seconds = std::chrono::duration<f32>(interval).count();
    last_frame = now;
    if (interval_seconds <= 0.0f) {
        Stabilize(now);
        return {.ceiling = ceiling, .reason = "non_monotonic_native_time"};
    }
    const f32 output_rate = static_cast<f32>(std::min(actual_output_millihertz, 120000u)) / 1000.0f;
    const f32 target_rate = options.target_rate == 0 ? output_rate
        : std::min(output_rate, static_cast<f32>(options.target_rate));
    if (smoothed_interval > 0.0f) {
        const f32 burst_threshold = std::max(BURST_CADENCE_RATIO / smoothed_interval,
                                             target_rate * BURST_TARGET_RATIO);
        if (1.0f / interval_seconds > burst_threshold) {
            DeferEvaluations(interval);
            output_credit = 0.0f;
            return {.output_target = target_rate, .ceiling = ceiling, .reason = "native_burst"};
        }
    }
    if (interval_seconds > 1.0f / MINIMUM_BASE_RATE) {
        Stabilize(now);
        return {.output_target = target_rate, .ceiling = ceiling, .reason = "native_gap"};
    }
    smoothed_interval = smoothed_interval > 0.0f
        ? smoothed_interval + (interval_seconds - smoothed_interval) * INTERVAL_SMOOTHING
        : interval_seconds;
    const f32 base_rate = 1.0f / smoothed_interval;
    if (previous_generations == 0) {
        unloaded_base_rate = unloaded_base_rate > 0.0f
            ? unloaded_base_rate + (base_rate - unloaded_base_rate) * INTERVAL_SMOOTHING
            : base_rate;
    }
    if (stable_until) {
        if (now < *stable_until) {
            return {.native_rate = base_rate, .output_target = target_rate, .ceiling = ceiling,
                    .reason = "stabilizing"};
        }
        stable_until.reset();
    }
    // Avoid paying for the whole optical-flow chain merely to bridge the last
    // few percent of a 60-Hz output with an already 60-FPS native source.
    if (base_rate >= target_rate * TARGET_SATISFIED_RATIO) {
        output_credit = 0.0f;
        return {.native_rate = base_rate, .output_target = target_rate, .ceiling = ceiling,
                .reason = "fills_output"};
    }
    if (options.target_rate == 0) {
        limit = ceiling;
    } else {
        UpdateLimit(now, base_rate, target_rate, ceiling);
    }
    const size_t allowed = std::min(limit, ceiling);
    const f32 desired_outputs = std::min(smoothed_interval * target_rate,
                                         static_cast<f32>(ceiling + 1));
    if (allowed == 0 || desired_outputs <= 1.0f) {
        output_credit = 0.0f;
        return {.native_rate = base_rate, .output_target = target_rate, .ceiling = ceiling,
                .reason = "adaptive_probe_wait"};
    }
    output_credit += desired_outputs;
    const size_t outputs = std::max<size_t>(1,
        static_cast<size_t>(std::floor(output_credit + CREDIT_EPSILON)));
    const size_t generations = std::min(outputs - 1, allowed);
    output_credit -= static_cast<f32>(generations + 1);
    if (output_credit < 0.0f) output_credit = 0.0f;
    else if (generations == allowed && output_credit >= 1.0f)
        output_credit = std::fmod(output_credit, 1.0f);
    issued_generations = generations;
    return {.generations = generations, .warm = true, .native_rate = base_rate,
            .output_target = target_rate, .ceiling = ceiling,
            .reason = generations > 0 ? "active" : "output_credit"};
}

void FrameGenPacer::UpdateLimit(Clock::time_point now, f32 base_rate, f32 target_rate,
                                size_t ceiling) {
    limit = std::min(limit, ceiling);

    if (probe_until) {
        if (now < *probe_until) {
            return;
        }
        probe_until.reset();
        output_credit = 0.0f;

        const f32 previous_output =
            std::min(target_rate, probe_base_rate * static_cast<f32>(probe_previous_limit + 1));
        const f32 current_output =
            std::min(target_rate, base_rate * static_cast<f32>(limit + 1));

        const bool throughput_regressed =
            current_output < previous_output * PROBE_THROUGHPUT_TOLERANCE;
        const bool collapsed_for_marginal_gain =
            base_rate < probe_base_rate * PROBE_BASE_COLLAPSE_RATIO &&
            current_output < previous_output * PROBE_MARGINAL_GAIN;
        const bool emulation_slowed = unloaded_base_rate > 0.0f &&
                                      base_rate < unloaded_base_rate * UNLOADED_BASE_RETENTION;

        if (throughput_regressed || collapsed_for_marginal_gain || emulation_slowed) {
            limit = probe_previous_limit;
            probe_failures = std::min(probe_failures + 1, MAX_PROBE_FAILURES);
            next_probe = now + ProbeBackoff(probe_failures);
            deficit_since.reset();
            return;
        }

        probe_failures = 0;
        next_probe = now + PROBE_STEP_DELAY;
    }

    if (base_rate * static_cast<f32>(limit + 1) >= target_rate * TARGET_SATISFIED_RATIO ||
        limit >= ceiling) {
        deficit_since.reset();
        return;
    }

    if (!deficit_since) {
        deficit_since = now;
        return;
    }
    if (now - *deficit_since < DEFICIT_DURATION) {
        return;
    }
    if (next_probe && now < *next_probe) {
        return;
    }

    probe_previous_limit = limit;
    probe_base_rate = base_rate;
    ++limit;
    probe_until = now + PROBE_DURATION;
    deficit_since.reset();
    output_credit = 0.0f;
}

void FrameGenPacer::DeferEvaluations(Clock::duration amount) {
    const auto defer = [amount](std::optional<Clock::time_point>& deadline) {
        if (deadline) {
            *deadline += amount;
        }
    };
    defer(stable_until);
    defer(probe_until);
    defer(next_probe);
    deficit_since.reset();
}

void FrameGenPacer::Stabilize(Clock::time_point now) {
    stable_until = now + STABILIZATION_DURATION;
    probe_until.reset();
    deficit_since.reset();
    smoothed_interval = 0.0f;
    output_credit = 0.0f;
}

void FrameGenPacer::Reset() {
    last_frame.reset();
    stable_until.reset();
    probe_until.reset();
    next_probe.reset();
    deficit_since.reset();
    smoothed_interval = 0.0f;
    output_credit = 0.0f;
    probe_base_rate = 0.0f;
    unloaded_base_rate = 0.0f;
    issued_generations = 0;
    probe_previous_limit = 0;
    limit = 0;
    probe_failures = 0;
}

} // namespace rex::ui::vulkan::framegen
