// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 The Simpsons Game PS5 contributors

#include <rex/platform/framegen_prospero.h>

#include <algorithm>
#include <atomic>
#include <fcntl.h>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>

#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(frame_gen, false, "Frame generation",
                    "Experimental frame generation; requires a local Lossless.dll")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_INT32(frame_gen_target_rate, 0, "Frame generation",
                     "Presented FPS target: 0 (Auto), 60 or 120")
    .validator([](std::string_view value) {
      return value == "0" || value == "60" || value == "120";
    })
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);
REXCVAR_DEFINE_INT32(frame_gen_multiplier, 2, "Frame generation",
                     "Maximum Auto multiplier: 2x, 3x or 4x")
    .range(2, 4)
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

namespace rex::platform::prospero {
namespace {
std::mutex paths_mutex;
std::filesystem::path user_dll_path;
std::filesystem::path title_dll_path;
// Low bit is the requested state; the remaining bits are its revision.
// Startup initializes this before input and Vulkan objects can observe it.
std::atomic<uint64_t> session_request{0};

bool IsReadableRegularFile(const std::filesystem::path& path) {
  if (path.empty()) return false;
  // Match the PS5 profile reader: path-based stat/lstat may be denied even
  // while ordinary open + fstat is permitted. No contents or version checks.
  const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW);
  if (fd < 0) return false;
  struct stat info {};
  const bool present = fstat(fd, &info) == 0 && S_ISREG(info.st_mode);
  close(fd);  // Never retry a close after the descriptor may have been released.
  return present;
}
}  // namespace

void ConfigureFrameGenerationPaths(const std::filesystem::path& user_data_root,
                                   const std::filesystem::path& title_root) {
  std::lock_guard<std::mutex> lock(paths_mutex);
  user_dll_path = user_data_root.empty()
                      ? std::filesystem::path{}
                      : user_data_root / "lossless" / "Lossless.dll";
  title_dll_path = title_root.empty()
                       ? std::filesystem::path{}
                       : title_root / "userdata" / "lossless" / "Lossless.dll";
}

std::filesystem::path ResolveFrameGenerationDllPath() {
  std::filesystem::path primary;
  std::filesystem::path fallback;
  {
    std::lock_guard<std::mutex> lock(paths_mutex);
    primary = user_dll_path;
    fallback = title_dll_path;
  }
  if (IsReadableRegularFile(primary)) {
    return primary;
  }
  if (IsReadableRegularFile(fallback)) {
    return fallback;
  }
  return {};
}

bool IsFrameGenerationDllPresent() {
  return !ResolveFrameGenerationDllPath().empty();
}

FrameGenerationOptions ReadFrameGenerationOptions() {
  FrameGenerationOptions options;
  options.enabled = REXCVAR_GET(frame_gen);
  const int32_t target = REXCVAR_GET(frame_gen_target_rate);
  options.target_rate = target == 60 || target == 120 ? uint32_t(target) : 0;
  options.multiplier = uint32_t(std::clamp(REXCVAR_GET(frame_gen_multiplier), 2, 4));
  return options;
}

void InitializeFrameGenerationSession() {
  session_request.store(uint64_t(2) | uint64_t(REXCVAR_GET(frame_gen)),
                        std::memory_order_release);
}

FrameGenerationSessionRequest ReadFrameGenerationSessionRequest() {
  const uint64_t packed = session_request.load(std::memory_order_acquire);
  return {bool(packed & 1), packed >> 1};
}

bool IsFrameGenerationRequestedForSession() {
  return ReadFrameGenerationSessionRequest().enabled;
}

FrameGenerationSessionRequest SetFrameGenerationForSession(bool enabled) {
  uint64_t previous = session_request.load(std::memory_order_acquire);
  for (;;) {
    if (bool(previous & 1) == enabled) return {enabled, previous >> 1};
    const uint64_t desired = ((previous + uint64_t(2)) & ~uint64_t(1)) | uint64_t(enabled);
    if (session_request.compare_exchange_weak(
            previous, desired, std::memory_order_acq_rel, std::memory_order_acquire)) {
      REXLOG_INFO("framegen: session request={} revision={} source=quick-menu; "
                    "startup CVars unchanged, menu persists its selected startup value separately",
                    enabled ? "on" : "off", desired >> 1);
      return {enabled, desired >> 1};
    }
  }
}

bool ToggleFrameGenerationForSession() {
  uint64_t previous = session_request.load(std::memory_order_acquire);
  uint64_t desired;
  do {
    desired = (previous + uint64_t(2)) ^ uint64_t(1);
  } while (!session_request.compare_exchange_weak(
      previous, desired, std::memory_order_acq_rel, std::memory_order_acquire));
  const bool enabled = bool(desired & 1);
  REXLOG_INFO("framegen: session request={} revision={} source=controller "
               "chord=L1+R1+L3+R3; startup configuration unchanged",
               enabled ? "on" : "off", desired >> 1);
  return enabled;
}

}  // namespace rex::platform::prospero
