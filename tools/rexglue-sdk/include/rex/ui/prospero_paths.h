#pragma once

#include <filesystem>
#include <string_view>

namespace rex::ui::prospero {

struct StoragePaths {
  std::filesystem::path game_data;
  std::filesystem::path user_data;
  std::filesystem::path cache;
  std::filesystem::path config;
  std::filesystem::path logs;
};

// The CRT selects state_root once. A read-only app image still supplies the
// original game data; only writable defaults use the selected state folder.
StoragePaths DefaultStoragePaths(const std::filesystem::path& app_root,
                                 const std::filesystem::path& state_root);

// Relative application paths are anchored to the mounted title, independent of
// the loader's working directory. Explicit absolute paths remain supported.
std::filesystem::path ResolveAppPath(const std::filesystem::path& app_root,
                                     const std::filesystem::path& path);

// Verify real filesystem operations, not just mode bits: a nullfs mount may be
// writable even when the title root is not owned by the current login user.
// Failures name the actual path and do not select another storage directory.
void RequireWritableDirectory(const std::filesystem::path& path);
void RequireWritableFile(const std::filesystem::path& path);

// Create the initial configuration once. An existing file is never truncated
// or replaced; failed initial writes remove only the file created by this call.
void EnsureDefaultProfile(const std::filesystem::path& path, std::string_view contents);

}  // namespace rex::ui::prospero
