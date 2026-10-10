#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace simpsons::prospero {

inline constexpr int32_t kProfileVersion = 34;
enum class ProfileAction { kCreated, kMigrated, kUnchanged, kSeededFromTitle };
struct ProfileResult {
  ProfileAction action;
  int32_t version;
  std::filesystem::path backup_path;
};

// Called before the normal TOML loader. Back up previous bytes, then migrate
// only selected graphics/timing values. Current or newer profiles are kept.
// Failures throw; no success/version is published before the file is committed.
// The final path must be a regular file, not a symlink. Inspect via open/fstat;
// do not require PS5 lstat, canonical or readlink support for a regular config.
ProfileResult PrepareProsperoProfile(const std::filesystem::path& config_path,
                                    std::string_view defaults);

// A first launch from a read-only title image may have a builder-selected TOML
// inside /app0 while writable state lives below /data/homebrew/PPSA74207.
// Seed missing writable state from that validated title config, byte for byte.
// An existing writable profile always takes precedence and the title is never
// changed. Missing title seeds fall back to the embedded default profile.
ProfileResult PrepareProsperoProfileFromTitle(const std::filesystem::path& config_path,
                                             std::string_view defaults,
                                             const std::filesystem::path& title_config_path);

}  // namespace simpsons::prospero
