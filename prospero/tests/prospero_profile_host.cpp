#include "prospero_profile.h"

#include <cerrno>
#include <cstdarg>
#include <dlfcn.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include <rex/cvar.h>
#include <toml++/toml.hpp>

REXCVAR_DEFINE_STRING(present_effect, "bilinear", "UI/Presenter", "test runtime flag");
REXCVAR_DEFINE_UINT32(user_language, 1, "Kernel", "test runtime flag");
REXCVAR_DEFINE_INT32(menu_frame_rate, 30, "GPU", "test runtime flag");

namespace fs = std::filesystem;
using simpsons::prospero::PrepareProsperoProfile;
using simpsons::prospero::PrepareProsperoProfileFromTitle;
using simpsons::prospero::ProfileAction;

enum class Failure { kNone, kReadOpenPerm, kReadOpenAccess, kBackupOpen, kBackupWrite,
                     kStageCreate, kStageWrite, kStageFlush, kCommit, kNewWrite,
                     kExternalEdit, kExternalSymlink };
enum class FileRole { kBackup, kStage, kNew };
static Failure failure = Failure::kNone;
static std::map<int, FileRole> roles;
static bool stage_partial_written = false;
static fs::path external_edit_path;
static fs::path external_symlink_destination;

extern "C" int __real_open(const char*, int, ...);
extern "C" int __real_mkstemp(char*);
extern "C" ssize_t __real_write(int, const void*, size_t);
extern "C" int __real_fsync(int);
extern "C" int __real_rename(const char*, const char*);
extern "C" int __real_close(int);

extern "C" int __wrap_open(const char* path, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list args; va_start(args, flags); mode = mode_t(va_arg(args, int)); va_end(args);
  }
  const bool backup = std::string(path).find(".before-v3.4") != std::string::npos;
  if (!(flags & O_CREAT) && (flags & O_ACCMODE) == O_RDONLY) {
    if (failure == Failure::kReadOpenPerm) { errno = EPERM; return -1; }
    if (failure == Failure::kReadOpenAccess) { errno = EACCES; return -1; }
  }
  if (backup && failure == Failure::kBackupOpen) { errno = EACCES; return -1; }
  const int fd = (flags & O_CREAT) ? __real_open(path, flags, mode) : __real_open(path, flags);
  if (fd >= 0 && (flags & O_CREAT)) roles[fd] = backup ? FileRole::kBackup : FileRole::kNew;
  return fd;
}
extern "C" int __wrap_mkstemp(char* path) {
  if (failure == Failure::kStageCreate) { errno = ENOSPC; return -1; }
  const int fd = __real_mkstemp(path);
  if (fd >= 0) roles[fd] = FileRole::kStage;
  return fd;
}
extern "C" ssize_t __wrap_write(int fd, const void* data, size_t count) {
  const auto role = roles.find(fd);
  if (role != roles.end()) {
    if ((role->second == FileRole::kBackup && failure == Failure::kBackupWrite) ||
        (role->second == FileRole::kNew && failure == Failure::kNewWrite)) {
      errno = EIO; return -1;
    }
    if (role->second == FileRole::kStage && failure == Failure::kStageWrite) {
      if (stage_partial_written) { errno = ENOSPC; return -1; }
      stage_partial_written = true;
      return __real_write(fd, data, std::min(size_t(11), count));
    }
  }
  return __real_write(fd, data, count);
}
extern "C" int __wrap_fsync(int fd) {
  const auto role = roles.find(fd);
  if (role != roles.end() && role->second == FileRole::kStage) {
    if (failure == Failure::kStageFlush) { errno = EIO; return -1; }
    if (failure == Failure::kExternalEdit) {
      std::ofstream(external_edit_path) << "# changed externally\nuser_language = 5\n";
    }
    if (failure == Failure::kExternalSymlink) {
      if (unlink(external_edit_path.c_str()) != 0 ||
          symlink(external_symlink_destination.c_str(), external_edit_path.c_str()) != 0) {
        errno = EIO; return -1;
      }
    }
  }
  return __real_fsync(fd);
}
extern "C" int __wrap_rename(const char* from, const char* to) {
  if (failure == Failure::kCommit) { errno = EACCES; return -1; }
  return __real_rename(from, to);
}
extern "C" int __wrap_close(int fd) { roles.erase(fd); return __real_close(fd); }

void Check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
std::string Read(const fs::path& path) {
  std::ifstream input(path, std::ios::binary);
  Check(bool(input), "fixture read failed");
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void Write(const fs::path& path, std::string_view text) {
  std::ofstream output(path, std::ios::binary);
  output << text;
  Check(bool(output), "fixture write failed");
}
template<class F> void ExpectFailure(F&& call) {
  bool threw = false;
  try { call(); } catch (const std::exception&) { threw = true; }
  Check(threw, "failure was reported as success");
}
template<class F> void ExpectFileError(int expected, F&& call) {
  bool threw = false;
  try { call(); } catch (const std::system_error& error) {
    Check(error.code().value() == expected, "wrong file error was reported");
    threw = true;
  }
  Check(threw, "file error was ignored or reported as success");
}
void CheckSymlink(const fs::path& path, const fs::path& destination) {
  char text[4096];
  const ssize_t length = readlink(path.c_str(), text, sizeof(text));
  Check(length >= 0 && size_t(length) < sizeof(text), "configuration symlink was replaced");
  Check(std::string(text, size_t(length)) == destination.string(), "symlink destination changed");
}
size_t BackupCount(const fs::path& dir) {
  size_t count = 0;
  for (const auto& item : fs::directory_iterator(dir)) {
    if (item.path().filename().string().find(".before-v3.4") != std::string::npos) ++count;
    Check(item.path().filename().string().find(".upgrade-v34-") == std::string::npos,
          "temporary migration file left behind");
  }
  return count;
}
fs::path Case(const fs::path& root, const std::string& name) {
  fs::create_directory(root / name);
  return root / name / "simpsons.toml";
}

// The supplied file has 13 assignments and one comment. No game assets are used.
constexpr std::string_view kPlayerConfig = R"TOML(# Auto-generated cvar configuration
menu_frame_rate = 60
ink_outlines = "soft"
allow_game_relative_writes = true
gpu = "vulkan"
swap_post_effect = "fxaa"
anisotropic_override = 5
resolution_scale = 3
present_effect = "fsr"
fullscreen = true
resolution = "4k"
vulkan_allow_present_mode_immediate = false
vulkan_allow_present_mode_fifo_relaxed = false
user_language = 6
)TOML";
constexpr std::string_view kOldSeed = R"TOML(fullscreen = true
resolution = "4k"
gpu = "vulkan"
render_target_path_vulkan = "native"
resolution_scale = 3
native_rt_original_resolution_targets = ""
present_effect = "fsr"
present_fsr_sharpness_reduction = 0.2
swap_post_effect = "fxaa"
eye_shading = "clean"
anisotropic_override = 5
ink_outlines = "soft"
ink_outline_strength = 0.60
ink_outline_color = "000000"
frame_rate = 60
menu_frame_rate = 30
vsync = true
vulkan_allow_present_mode_immediate = false
vulkan_allow_present_mode_fifo_relaxed = false
user_language = 6
)TOML";

void CheckCurrent(const fs::path& path) {
  const auto cfg = toml::parse_file(path.string());
  Check(cfg["prospero_profile_version"].value<int>() == 34, "version missing");
  Check(cfg["resolution"].value<std::string>() == "4k", "output changed");
  Check(cfg["resolution_scale"].value<int>() == 3, "render is not 4K");
  Check(cfg["frame_rate"].value<int>() == 60 && cfg["menu_frame_rate"].value<int>() == 60,
        "60 FPS targets not migrated");
  Check(cfg["present_effect"].value<std::string>() == "bilinear", "FSR/CAS still active");
  Check(cfg["swap_post_effect"].value<std::string>() == "none", "FXAA still active");
  Check(cfg["eye_shading"].value<std::string>() == "original" &&
        cfg["ink_outlines"].value<std::string>() == "original", "custom visual patch retained");
  Check(cfg["anisotropic_override"].value<int>() == 5, "anisotropy not 16x");
}

int main(int argc, char** argv) {
  try {
    Check(argc == 3, "usage: profile_host TEMP_DIR PROFILE_TOML");
    const fs::path root = fs::absolute(argv[1]);
    const auto preset = Read(argv[2]);
    // Verify the real filesystem API has the same split as the console:
    // symlink_status -> lstat returns EPERM, while open/fstat/read/write work.
    const auto calls = reinterpret_cast<unsigned long (*)()>(
        dlsym(RTLD_DEFAULT, "profile_lstat_eperm_calls"));
    Check(calls != nullptr, "lstat boundary injector was not loaded");
    const auto denied_status = Case(root, "lstat-eperm-existing");
    Write(denied_status, kPlayerConfig);
    std::error_code status_error;
    (void)fs::symlink_status(denied_status, status_error);
    Check(status_error == std::errc::operation_not_permitted && calls() == 1,
          "real symlink_status did not reach the denied lstat boundary");
    const int readable = open(denied_status.c_str(), O_RDWR | O_NOFOLLOW);
    struct stat opened_info{};
    Check(readable >= 0 && fstat(readable, &opened_info) == 0 && S_ISREG(opened_info.st_mode),
          "fixture does not allow open/fstat for the regular configuration");
    Check(close(readable) == 0, "fixture descriptor close failed");
    std::cout << "PASS boundary: symlink_status returns EPERM while regular-file I/O is allowed\n";
    const auto recovered = PrepareProsperoProfile(denied_status, preset);
    Check(recovered.action == ProfileAction::kMigrated &&
          Read(recovered.backup_path) == kPlayerConfig, "lstat EPERM blocked regular-file migration");
    CheckCurrent(denied_status);
    Check(calls() == 1, "migration still attempted the denied lstat API");
    std::cout << "PASS migration without lstat under EPERM; original backup and version34 preserved\n";
    Check(toml::parse(kPlayerConfig).size() == 13, "player fixture changed");
    Check(toml::parse(kOldSeed).size() == 20, "old seed fixture changed");
    for (const auto& [name, previous] :
         {std::pair{"player", kPlayerConfig}, std::pair{"old-seed", kOldSeed}}) {
      const auto path = Case(root, name);
      Write(path, previous);
      chmod(path.c_str(), 0664);
      const auto result = PrepareProsperoProfile(path, preset);
      Check(result.action == ProfileAction::kMigrated, "not migrated");
      Check(Read(result.backup_path) == previous, "backup is not byte-identical");
      CheckCurrent(path);
      struct stat info{}; stat(path.c_str(), &info);
      Check((info.st_mode & 0777) == 0664, "original file mode changed");
      const auto before = Read(path);
      Check(PrepareProsperoProfile(path, preset).action == ProfileAction::kUnchanged,
            "second boot repeated migration");
      Check(Read(path) == before && BackupCount(path.parent_path()) == 1,
            "second boot changed bytes or made another backup");
    }
    std::cout << "PASS supplied 13-key TOML and old 20-key seed; exact backup, modes and second boot\n";

    const auto fresh = Case(root, "fresh");
    const mode_t old_umask = umask(0077);
    Check(PrepareProsperoProfile(fresh, preset).action == ProfileAction::kCreated, "fresh seed failed");
    umask(old_umask);
    Check(Read(fresh) == preset && BackupCount(fresh.parent_path()) == 0, "fresh seed not exact");
    struct stat fresh_info{}; stat(fresh.c_str(), &fresh_info);
    Check((fresh_info.st_mode & 0777) == 0666, "new file inherited restrictive umask");
#if !defined(PROSPERO_PROFILE_BASELINE)
    const auto image_seed = Case(root, "builder-read-only-image");
    auto builder_table = toml::parse(preset);
    builder_table.insert_or_assign("resolution_scale", 2);
    builder_table.insert_or_assign("frame_rate", 30);
    builder_table.insert_or_assign("present_effect", "fsr");
    builder_table.insert_or_assign("user_language", 5);
    std::ostringstream builder_stream;
    builder_stream << "# Builder choices, preserved byte for byte.\r\n"
                   << toml::toml_formatter(builder_table) << '\n';
    const auto builder_bytes = builder_stream.str();
    Write(image_seed, builder_bytes);
    Check(chmod(image_seed.c_str(), 0444) == 0, "cannot model read-only seed");
    const auto image_state = Case(root, "builder-writable-state");
    const auto image_result = PrepareProsperoProfileFromTitle(image_state, preset, image_seed);
    Check(image_result.action == ProfileAction::kSeededFromTitle &&
          Read(image_state) == builder_bytes && Read(image_seed) == builder_bytes,
          "read-only title lost builder settings or changed its seed");
    struct stat seeded_info{}; stat(image_state.c_str(), &seeded_info);
    Check((seeded_info.st_mode & 0777) == 0666, "writable state inherited read-only seed mode");
    Write(image_state, "prospero_profile_version=34\nframe_rate=60\nuser_language=6\n");
    const auto changed_in_game = Read(image_state);
    Check(PrepareProsperoProfileFromTitle(image_state, preset, image_seed).action == ProfileAction::kUnchanged &&
          Read(image_state) == changed_in_game && Read(image_seed) == builder_bytes,
          "next launch overwrote settings changed during the game");
    const auto no_seed = Case(root, "builder-seed-not-provided");
    Check(PrepareProsperoProfileFromTitle(no_seed, preset, root / "missing-title-config.toml").action == ProfileAction::kCreated &&
          Read(no_seed) == preset, "missing title seed did not preserve default startup");
    const auto direct_builder = Case(root, "builder-direct-folder");
    Write(direct_builder, builder_bytes);
    Check(PrepareProsperoProfileFromTitle(direct_builder, preset, direct_builder).action == ProfileAction::kUnchanged &&
          Read(direct_builder) == builder_bytes, "direct folder builder config was replaced");
    for (const auto& [name, bytes] : {
          std::pair{"builder-bad-seed", std::string("gpu = [broken\n")},
          std::pair{"builder-old-seed", std::string("prospero_profile_version=33\n")},
          std::pair{"builder-incomplete-seed", std::string("prospero_profile_version=34\n")}}) {
      const auto bad_seed = Case(root, name); Write(bad_seed, bytes);
      const auto bad_state = Case(root, std::string(name) + "-state");
      ExpectFailure([&] { PrepareProsperoProfileFromTitle(bad_state, preset, bad_seed); });
      Check(!fs::exists(bad_state) && Read(bad_seed) == bytes,
            "invalid title seed created partial state or changed game data");
      Check(PrepareProsperoProfileFromTitle(image_state, preset, bad_seed).action == ProfileAction::kUnchanged &&
            Read(image_state) == changed_in_game, "an unused title seed blocked existing valid state");
    }
    const auto failed_seed_state = Case(root, "builder-seed-write-failure");
    failure = Failure::kNewWrite;
    ExpectFailure([&] { PrepareProsperoProfileFromTitle(failed_seed_state, preset, image_seed); });
    failure = Failure::kNone;
    Check(!fs::exists(failed_seed_state) && Read(image_seed) == builder_bytes && roles.empty(),
          "failed seed write damaged the image or left partial state");
    Check(PrepareProsperoProfileFromTitle(failed_seed_state, preset, image_seed).action == ProfileAction::kSeededFromTitle,
          "seed retry failed after partial write cleanup");
    std::cout << "PASS builder title seed: read-only image, direct folder, byte identity, writable mode, "
                 "second boot, invalid/missing seeds and failed-write retry\n";
#endif
    const auto custom = Case(root, "custom");
    const std::string extras = std::string(kPlayerConfig) +
        "audio_mute = true\naudio_maxqframes = 48\nkeybind_a = 'Space'\n"
        "game_data_root = '/data/custom game'\nuser_data_root = '/data/custom saves'\n"
        "cache_root = '/data/custom cache'\ncustom_unknown = 'keep me'\n";
    Write(custom, extras);
    const auto result = PrepareProsperoProfile(custom, preset);
    const auto cfg = toml::parse_file(custom.string());
    for (const auto key : {"user_language", "allow_game_relative_writes", "audio_mute",
         "audio_maxqframes", "keybind_a", "game_data_root", "user_data_root", "cache_root",
         "custom_unknown"}) {
      const auto previous = toml::parse(extras);
      Check(cfg[key].value<std::string>() == previous[key].value<std::string>() &&
            cfg[key].value<int64_t>() == previous[key].value<int64_t>() &&
            cfg[key].value<bool>() == previous[key].value<bool>(),
            "unrelated preference changed");
    }
    Check(Read(result.backup_path) == extras, "custom backup changed");
    std::cout << "PASS fresh defaults, language/input/audio/path/unknown preferences preserved\n";

    const auto nested = Case(root, "nested");
    Write(nested, "user_language=5\n[present]\neffect='fsr'\nletterbox=false\n"
                  "[menu]\nframe_rate=30\n[ink]\noutlines='soft'\n");
    PrepareProsperoProfile(nested, preset);
    const auto nested_cfg = toml::parse_file(nested.string());
    Check(!nested_cfg["present"]["effect"] && !nested_cfg["menu"]["frame_rate"],
          "nested old setting still overrides the migration");
    Check(nested_cfg["present"]["letterbox"].value<bool>() == false &&
          nested_cfg["user_language"].value<int>() == 5, "nested unrelated settings changed");
    const auto linked = Case(root, "symlink");
    const auto destination = linked.parent_path() / "actual.toml";
    Write(destination, kPlayerConfig);
    fs::create_symlink(destination.filename(), linked);
    ExpectFileError(ELOOP, [&] { PrepareProsperoProfile(linked, preset); });
    CheckSymlink(linked, destination.filename());
    Check(Read(destination) == kPlayerConfig && BackupCount(linked.parent_path()) == 0,
          "rejected symlink changed its destination or created a backup");
    const auto dangling = Case(root, "dangling-symlink");
    fs::create_symlink("missing.toml", dangling);
    ExpectFileError(ELOOP, [&] { PrepareProsperoProfile(dangling, preset); });
    CheckSymlink(dangling, "missing.toml");
    Check(BackupCount(dangling.parent_path()) == 0, "dangling symlink created a backup");
    std::cout << "PASS nested equivalent keys; final symlinks rejected without changing either path\n";

    const auto collision = Case(root, "backup-collision");
    Write(collision, kPlayerConfig);
    const auto occupied = fs::path(collision.string() + ".before-v3.4.bak");
    Write(occupied, "existing backup\n");
    const auto collision_result = PrepareProsperoProfile(collision, preset);
    Check(Read(occupied) == "existing backup\n" && Read(collision_result.backup_path) == kPlayerConfig,
          "previous backup overwritten");
    Check(collision_result.backup_path != occupied, "backup collision not handled");
    for (const auto [kind, name] : {
         std::pair{Failure::kReadOpenPerm, "read-open-eperm"},
         std::pair{Failure::kReadOpenAccess, "read-open-eacces"},
         std::pair{Failure::kBackupOpen, "backup-open"},
         std::pair{Failure::kBackupWrite, "backup-write"},
         std::pair{Failure::kStageCreate, "stage-create"},
         std::pair{Failure::kStageWrite, "stage-write"},
         std::pair{Failure::kStageFlush, "stage-flush"},
         std::pair{Failure::kCommit, "commit"}}) {
      const auto path = Case(root, name);
      Write(path, kPlayerConfig);
      failure = kind; stage_partial_written = false;
      if (kind == Failure::kReadOpenPerm || kind == Failure::kReadOpenAccess) {
        ExpectFileError(kind == Failure::kReadOpenPerm ? EPERM : EACCES,
                        [&] { PrepareProsperoProfile(path, preset); });
      } else {
        ExpectFailure([&] { PrepareProsperoProfile(path, preset); });
      }
      failure = Failure::kNone;
      Check(Read(path) == kPlayerConfig, "failure destroyed the original configuration");
      const auto backups = BackupCount(path.parent_path());
      if (kind == Failure::kReadOpenPerm || kind == Failure::kReadOpenAccess) {
        Check(backups == 0, "read denial was treated as missing configuration");
      }
      Check(roles.empty(), "failure leaked a file descriptor");
      Check(PrepareProsperoProfile(path, preset).action == ProfileAction::kMigrated,
            "retry after failure did not migrate");
    }
    const auto new_failure = Case(root, "new-write-failure");
    failure = Failure::kNewWrite;
    ExpectFailure([&] { PrepareProsperoProfile(new_failure, preset); });
    failure = Failure::kNone;
    Check(!fs::exists(new_failure) && roles.empty(), "partial new config left behind");
    std::cout << "PASS real open denials, backup collisions and I/O failure rollback and retry\n";

    const auto edited = Case(root, "external-edit");
    Write(edited, kPlayerConfig); external_edit_path = edited;
    failure = Failure::kExternalEdit;
    ExpectFailure([&] { PrepareProsperoProfile(edited, preset); });
    failure = Failure::kNone;
    Check(Read(edited) == "# changed externally\nuser_language = 5\n", "external edit lost");
    Check(BackupCount(edited.parent_path()) == 1, "original backup lost after external edit");
    const auto swapped_link = Case(root, "external-symlink");
    Write(swapped_link, kPlayerConfig);
    external_edit_path = swapped_link;
    external_symlink_destination = swapped_link.parent_path() / "external.toml";
    Write(external_symlink_destination, "user_language=5\n");
    failure = Failure::kExternalSymlink;
    ExpectFileError(ELOOP, [&] { PrepareProsperoProfile(swapped_link, preset); });
    failure = Failure::kNone;
    CheckSymlink(swapped_link, external_symlink_destination);
    Check(Read(external_symlink_destination) == "user_language=5\n" &&
          BackupCount(swapped_link.parent_path()) == 1,
          "configuration swapped to a symlink before recheck was overwritten");
    for (const auto text : {"gpu = [broken\n", "prospero_profile_version='bad'\n"}) {
      const auto path = Case(root, text[0] == 'g' ? "invalid-toml" : "invalid-version");
      Write(path, text);
      ExpectFailure([&] { PrepareProsperoProfile(path, preset); });
      Check(Read(path) == text && BackupCount(path.parent_path()) == 0, "invalid input modified");
    }
    const auto future = Case(root, "future");
    Write(future, "prospero_profile_version=35\npresent_effect='cas'\n");
    const auto future_bytes = Read(future);
    Check(PrepareProsperoProfile(future, preset).action == ProfileAction::kUnchanged &&
          Read(future) == future_bytes, "future version downgraded");
    std::cout << "PASS malformed TOML/version, external edits and future version are never overwritten\n";

    // Exercise the production registry/loader/serializer, not a model of it.
    rex::cvar::LoadConfig(custom);
    Check(rex::cvar::Query<int32_t>("prospero_profile_version") == 34, "version CVar not registered");
    REXCVAR_SET(present_effect, std::string("cas"));
    rex::cvar::SaveConfig(custom);
    const auto saved = Read(custom);
    Check(toml::parse(saved)["prospero_profile_version"].value<int>() == 34,
          "real SaveConfig discarded migration marker");
    Check(PrepareProsperoProfile(custom, preset).action == ProfileAction::kUnchanged &&
          Read(custom) == saved, "overlay save caused preferences to reset next boot");
    Check(toml::parse(saved)["present_effect"].value<std::string>() == "cas",
          "post-migration choice reset");
    std::cout << "PASS real ReXGlue LoadConfig/SaveConfig retains version34 and later player choices\n";
    Check(calls() == 1, "production migration attempted lstat in one of the tested paths");
  } catch (const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
