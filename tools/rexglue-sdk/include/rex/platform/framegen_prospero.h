// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 The Simpsons Game PS5 contributors
#pragma once

#include <cstdint>
#include <filesystem>

namespace rex::platform::prospero {

struct FrameGenerationOptions {
  bool enabled = false;
  // Zero selects Auto. Explicit targets are 60 and 120 presented frames/s.
  uint32_t target_rate = 0;
  uint32_t multiplier = 2;
};

struct FrameGenerationSessionRequest {
  bool enabled = false;
  // Monotonic request revision lets the renderer notice an OFF/ON pair even
  // when both requests arrive while one frame is being prepared.
  uint64_t revision = 0;
};

// Called after the application's paths have been resolved, before Vulkan starts.
// No DLL is loaded or executed: it is a container of native SPIR-V resources.
void ConfigureFrameGenerationPaths(const std::filesystem::path& user_data_root,
                                   const std::filesystem::path& title_root);

// Prefer the writable user data directory. The title copy is read-only fallback.
// An empty result means neither file is currently present. This function only
// checks file existence/type, never PE metadata, a version or a hash whitelist.
std::filesystem::path ResolveFrameGenerationDllPath();
bool IsFrameGenerationDllPresent();

// Saved startup options. The session shortcut does not change these CVars or
// write the user's TOML. Target and multiplier remain fixed for this launch.
FrameGenerationOptions ReadFrameGenerationOptions();

// Call once after LoadConfig and before creating input / renderer objects.
void InitializeFrameGenerationSession();
FrameGenerationSessionRequest ReadFrameGenerationSessionRequest();
bool IsFrameGenerationRequestedForSession();

// The quick menu uses an explicit state, sharing the controller's atomic
// request path. A repeated request for the same state does not reset history.
// This publishes intent only; the caller separately saves its startup choice.
FrameGenerationSessionRequest SetFrameGenerationForSession(bool enabled);

// Safe for the controller's polling threads. Publishes and logs intent without
// calling Vulkan, changing the configuration or managing renderer resources.
// Returns the newly requested state. The presenter applies it at a safe point.
bool ToggleFrameGenerationForSession();

}  // namespace rex::platform::prospero
