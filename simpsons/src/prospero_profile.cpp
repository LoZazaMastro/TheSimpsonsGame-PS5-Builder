#include "prospero_profile.h"

#include <array>
#include <cerrno>
#include <fcntl.h>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <rex/cvar.h>
#include <rex/platform.h>
#include <toml++/toml.hpp>

#if REX_PLATFORM_PROSPERO
#include <rex/filesystem.h>
#include <rex/rex_app.h>
#include <rex/ui/prospero_paths.h>
#include <rex/ui/startup_prospero.h>
#include "prospero_profile_embedded.h"
#endif

// SaveConfig writes only non-default values. Default 0 makes the completed
// version survive settings-overlay saves, without resetting choices next boot.
REXCVAR_DEFINE_INT32(prospero_profile_version, 0, "Platform/PS5",
                     "Completed PS5 profile migration version")
    .range(0, std::numeric_limits<int32_t>::max())
    .lifecycle(rex::cvar::Lifecycle::kInitOnly)
    .debug_only();

namespace simpsons::prospero {
namespace {
namespace fs = std::filesystem;
constexpr size_t kMaxConfigBytes = 1024 * 1024;

// Language, paths, input, audio and save-related options are not migrated.
// The PC launcher's separate native-resource experiments are not enabled here.
constexpr std::array<std::string_view, 18> kMigratedKeys = {
    "prospero_profile_version", "fullscreen", "resolution", "gpu",
    "render_target_path_vulkan", "resolution_scale",
    "native_rt_original_resolution_targets", "present_effect", "swap_post_effect",
    "eye_shading", "anisotropic_override", "ink_outlines", "ink_outline_color",
    "frame_rate", "menu_frame_rate", "vsync",
    "vulkan_allow_present_mode_immediate", "vulkan_allow_present_mode_fifo_relaxed",
};

[[noreturn]] void FileError(const char* operation, const fs::path& path, int error) {
  throw std::system_error(error, std::generic_category(),
                          std::string(operation) + " '" + path.string() + "'");
}

class Descriptor {
 public:
  explicit Descriptor(int fd) : fd_(fd) {}
  Descriptor(const Descriptor&) = delete;
  Descriptor& operator=(const Descriptor&) = delete;
  ~Descriptor() { if (fd_ >= 0) close(fd_); }
  int get() const { return fd_; }
  void Close(const fs::path& path) {
    const int fd = fd_;
    fd_ = -1;
    // Never retry close: the descriptor may already have been released.
    if (close(fd) != 0) FileError("Cannot close configuration file", path, errno);
  }
 private:
  int fd_;
};

struct ExistingFile {
  bool exists = false;
  std::string bytes;
  mode_t mode = 0666;
};

ExistingFile ReadFile(const fs::path& path) {
  // PS5 may reject lstat. Inspect the opened descriptor directly, and refuse
  // a final symlink instead of resolving it.
  const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW);
  if (fd < 0) {
    if (errno == ENOENT) return {};
    if (errno == ELOOP || errno == EMLINK) {
      FileError("Configuration symbolic links are not supported on PS5", path, errno);
    }
    FileError("Cannot read configuration", path, errno);
  }
  Descriptor file(fd);
  struct stat info {};
  if (fstat(fd, &info) != 0) FileError("Cannot inspect configuration", path, errno);
  if (!S_ISREG(info.st_mode)) FileError("Configuration is not a regular file", path, EINVAL);
  if (info.st_size < 0 || uint64_t(info.st_size) > kMaxConfigBytes) {
    FileError("Configuration exceeds the 1 MiB limit", path, EFBIG);
  }
  ExistingFile result{true, {}, mode_t(info.st_mode & 0777)};
  std::array<char, 4096> buffer;
  for (;;) {
    const ssize_t count = read(fd, buffer.data(), buffer.size());
    if (count < 0 && errno == EINTR) continue;
    if (count < 0) FileError("Cannot read configuration", path, errno);
    if (!count) break;
    if (size_t(count) > kMaxConfigBytes - result.bytes.size()) {
      FileError("Configuration exceeds the 1 MiB limit", path, EFBIG);
    }
    result.bytes.append(buffer.data(), size_t(count));
  }
  file.Close(path);
  return result;
}

void WriteFile(int fd, const fs::path& path, std::string_view bytes, mode_t mode) {
  Descriptor file(fd);
  // Only newly created files reach this function. Keep the previous mode
  // despite mkstemp's 0600 and umask; never chmod the existing configuration.
  if (fchmod(fd, mode) != 0) FileError("Cannot set new configuration file mode", path, errno);
  while (!bytes.empty()) {
    const ssize_t count = write(fd, bytes.data(), bytes.size());
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) FileError("Cannot write configuration file", path, count < 0 ? errno : EIO);
    bytes.remove_prefix(size_t(count));
  }
  if (fsync(fd) != 0) FileError("Cannot flush configuration file", path, errno);
  file.Close(path);
}

class PendingFile {
 public:
  explicit PendingFile(fs::path path) : path_(std::move(path)) {}
  ~PendingFile() { if (!path_.empty()) unlink(path_.c_str()); }
  void Keep() { path_.clear(); }
 private:
  fs::path path_;
};

bool IsMigratedKey(std::string_view key) {
  for (const auto selected : kMigratedKeys) if (key == selected) return true;
  return false;
}

// ReXGlue maps [section] key to section_key. Remove equivalent nested forms
// too, otherwise the old value could win when the loader later visits them.
void RemoveMigratedKeys(toml::table& table, const std::string& prefix = {}) {
  std::vector<std::string> remove;
  for (auto&& [key, value] : table) {
    const std::string name = prefix.empty() ? std::string(key.str())
                                           : prefix + "_" + std::string(key.str());
    if (value.is_table()) RemoveMigratedKeys(*value.as_table(), name);
    else if (IsMigratedKey(name)) remove.emplace_back(key.str());
  }
  for (const auto& key : remove) table.erase(key);
}

int32_t ProfileVersion(const toml::table& table) {
  const auto* value = table.get("prospero_profile_version");
  if (!value) return 0;
  const auto version = value->value<int64_t>();
  if (!version || *version < 0 || *version > std::numeric_limits<int32_t>::max()) {
    throw std::runtime_error("Invalid prospero_profile_version; configuration was not modified");
  }
  return int32_t(*version);
}

fs::path BackupOriginal(const fs::path& path, const ExistingFile& original) {
  for (unsigned int attempt = 0; attempt < 1000; ++attempt) {
    fs::path backup = path;
    backup += ".before-v3.4";
    if (attempt) backup += "." + std::to_string(attempt);
    backup += ".bak";
    const int fd = open(backup.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, original.mode);
    if (fd < 0) {
      if (errno == EEXIST) continue;
      FileError("Cannot create profile backup", backup, errno);
    }
    PendingFile pending(backup);
    WriteFile(fd, backup, original.bytes, original.mode);
    pending.Keep();
    return backup;
  }
  FileError("All profile backup names are already in use", path, EEXIST);
}
}  // namespace

ProfileResult PrepareProsperoProfile(const fs::path& config_path, std::string_view defaults) {
  return PrepareProsperoProfileFromTitle(config_path, defaults, {});
}

ProfileResult PrepareProsperoProfileFromTitle(const fs::path& config_path,
                                             std::string_view defaults,
                                             const fs::path& title_config_path) {
  if (!config_path.is_absolute()) {
    throw std::invalid_argument("The PS5 configuration path must be absolute");
  }
  if (!title_config_path.empty() && !title_config_path.is_absolute()) {
    throw std::invalid_argument("The title configuration seed path must be absolute");
  }
  const fs::path target = config_path;
  const auto original = ReadFile(target);
  auto preset = toml::parse(defaults);
  if (ProfileVersion(preset) != kProfileVersion) {
    throw std::runtime_error("Embedded PS5 profile has an unexpected version");
  }
  for (const auto key : kMigratedKeys) {
    if (!preset.contains(key) || !preset.get(key)->is_value()) {
      throw std::runtime_error("Embedded PS5 profile is missing a migration value: " +
                               std::string(key));
    }
  }
  if (!original.exists) {
    ExistingFile title_seed;
    int32_t created_version = kProfileVersion;
    if (!title_config_path.empty() && title_config_path != target) {
      title_seed = ReadFile(title_config_path);
      if (title_seed.exists) {
        const auto selected = toml::parse(title_seed.bytes);
        created_version = ProfileVersion(selected);
        if (created_version < kProfileVersion) {
          throw std::runtime_error("The title configuration seed predates this PS5 profile; "
                                   "writable state was not created");
        }
        for (const auto key : kMigratedKeys) {
          if (!selected.contains(key) || !selected.get(key)->is_value()) {
            throw std::runtime_error("The title configuration seed is missing a setting: " +
                                     std::string(key));
          }
        }
      }
    }
    const int fd = open(target.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0666);
    if (fd < 0) FileError("Cannot create initial PS5 profile", target, errno);
    PendingFile pending(target);
    WriteFile(fd, target, title_seed.exists ? std::string_view(title_seed.bytes) : defaults, 0666);
    pending.Keep();
    return {title_seed.exists ? ProfileAction::kSeededFromTitle : ProfileAction::kCreated,
            created_version, {}};
  }
  auto config = toml::parse(original.bytes);
  const int32_t version = ProfileVersion(config);
  if (version >= kProfileVersion) return {ProfileAction::kUnchanged, version, {}};
  RemoveMigratedKeys(config);
  for (const auto key : kMigratedKeys) config.insert_or_assign(key, *preset.get(key));
  std::ostringstream output;
  output << "# PS5 profile v3.4: previous bytes preserved in the adjacent .bak file.\n"
         << toml::toml_formatter(config) << '\n';
  const std::string migrated = output.str();
  if (ProfileVersion(toml::parse(migrated)) != kProfileVersion) {
    throw std::runtime_error("Cannot validate the migrated PS5 profile");
  }
  const auto backup = BackupOriginal(target, original);
  std::string pattern = target.string() + ".upgrade-v34-XXXXXX";
  std::vector<char> temporary(pattern.begin(), pattern.end());
  temporary.push_back('\0');
  const int fd = mkstemp(temporary.data());
  if (fd < 0) FileError("Cannot create profile staging file", target, errno);
  const fs::path stage(temporary.data());
  PendingFile pending(stage);
  WriteFile(fd, stage, migrated, original.mode);
  // Do not replace an external edit made while backup/staging were written.
  const auto current = ReadFile(target);
  if (!current.exists || current.bytes != original.bytes) {
    throw std::runtime_error("PS5 configuration changed during migration; it was not replaced");
  }
  if (rename(stage.c_str(), target.c_str()) != 0) {
    FileError("Cannot commit migrated PS5 profile", target, errno);
  }
  pending.Keep();
  return {ProfileAction::kMigrated, kProfileVersion, backup};
}
}  // namespace simpsons::prospero

#if REX_PLATFORM_PROSPERO
// Called before the TOML loader. The CRT has already selected writable state;
// game-relative path overrides remain anchored to /app0.
void ConfigureProsperoPaths(rex::PathConfig& paths) {
  prospero_startup_log("paths: preparing PS5 profile 34 with builder title seed support");
  const auto root = rex::filesystem::GetExecutableFolder();
  const auto defaults = rex::ui::prospero::DefaultStoragePaths(root, prospero_state_root());
  if (paths.game_data_root.empty()) paths.game_data_root = defaults.game_data;
  if (paths.user_data_root.empty()) paths.user_data_root = defaults.user_data;
  if (paths.cache_root.empty()) paths.cache_root = defaults.cache;
  paths.config_path = defaults.config;
  prospero_startup_log(("paths: state root = " + std::string(prospero_state_root())).c_str());
  prospero_startup_log(("paths: configuration = " + paths.config_path.string()).c_str());
  rex::ui::prospero::RequireWritableDirectory(paths.config_path.parent_path());
  try {
    const auto title_config = root / "userdata" / "simpsons.toml";
    const auto result = simpsons::prospero::PrepareProsperoProfileFromTitle(
        paths.config_path, kProsperoDefaultProfile, title_config);
    const char* action = result.action == simpsons::prospero::ProfileAction::kCreated ? "created" :
                         result.action == simpsons::prospero::ProfileAction::kSeededFromTitle ? "seeded from title settings" :
                         result.action == simpsons::prospero::ProfileAction::kMigrated ? "migrated" :
                         "already current";
    std::string message = "profile: PS5 version " + std::to_string(result.version) + "; " + action;
    if (!result.backup_path.empty()) message += "; backup=" + result.backup_path.string();
    if (result.action == simpsons::prospero::ProfileAction::kSeededFromTitle) {
      message += "; source=" + title_config.string();
    }
    prospero_startup_log(message.c_str());
  } catch (const std::exception& failure) {
    prospero_startup_log(("profile: PS5 migration failed: " + std::string(failure.what())).c_str());
    throw;
  }
  rex::ui::prospero::RequireWritableFile(paths.config_path);
  prospero_startup_log("paths: title configuration is writable");
}
#endif
