// Atomic PS5 settings persistence, isolated from rendering and audio threads.
// BSD 3-Clause License. See the project LICENSE.
#pragma once

#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <variant>

namespace rex::platform::prospero {

using ConfigValue = std::variant<bool, int64_t, double, std::string>;
using ConfigValues = std::map<std::string, ConfigValue>;

// Reads scalar values using the same section_key flattening as ReXGlue.
// Parse errors are surfaced, never replaced with an empty configuration.
ConfigValues ReadSettingsValues(const std::filesystem::path& path);

// Writes a complete, validated TOML table to a sibling temporary file, flushes
// it, then renames it over the original. Unknown keys/tables and mode survive;
// alternative nested spellings of changed keys are removed before insertion.
struct SettingsSaveResult {
  // A nonempty warning means the new file is installed, but directory-entry
  // durability could not be confirmed. This is distinct from a failed save.
  std::string durability_warning;
};
SettingsSaveResult SaveSettingsValues(const std::filesystem::path& path, const ConfigValues& changes);

class SettingsConfigStore {
 public:
  struct Status {
    uint64_t requested = 0;
    uint64_t persisted = 0;
    uint64_t failed = 0;
    std::string error;
    std::string durability_warning;
  };
  using Observer = std::function<void(const Status&)>;
  explicit SettingsConfigStore(std::filesystem::path path, Observer observer = {},
                               std::function<void()> idle_observer = {});
  ~SettingsConfigStore();
  SettingsConfigStore(const SettingsConfigStore&) = delete;
  SettingsConfigStore& operator=(const SettingsConfigStore&) = delete;

  // Non-blocking with respect to disk I/O. Later edits coalesce while the
  // dedicated writer works; the last accepted request is drained on shutdown.
  uint64_t Queue(ConfigValues values);
  Status GetStatus() const;

 private:
  void Run();
  std::filesystem::path path_;
  Observer observer_;
  std::function<void()> idle_observer_;
  mutable std::mutex mutex_;
  std::condition_variable wake_;
  ConfigValues pending_;
  Status status_;
  bool pending_ready_ = false;
  bool stopping_ = false;
  std::thread worker_;
};
}  // namespace rex::platform::prospero
