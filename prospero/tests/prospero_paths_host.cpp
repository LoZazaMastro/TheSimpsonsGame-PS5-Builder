// Host filesystem regression checks for the production portable-path helpers.
// These exercise real POSIX I/O; they do not claim PS5 mount/loader validation.
#include <rex/ui/prospero_paths.h>

#include <csignal>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace rex::ui::prospero;

#if defined(PROSPERO_HOST_CHDIR_EPERM)
// Test-process boundary only: reproduce the console's rejected chdir with
// the host standard library. This implementation is never linked into a title
// and deliberately reports failure, rather than pretending the CWD changed.
unsigned chdir_attempts = 0;
extern "C" int chdir(const char*) noexcept {
  ++chdir_attempts;
  errno = EPERM;
  return -1;
}
#endif

namespace {

void Check(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

std::string Read(const fs::path& path) {
  std::ifstream input(path);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::string ExpectFailure(const std::function<void()>& operation) {
  try {
    operation();
  } catch (const std::runtime_error& error) {
    return error.what();
  }
  throw std::runtime_error("operation unexpectedly succeeded");
}

bool InChild(const std::function<void()>& operation, bool drop_privileges) {
  std::cout.flush();
  pid_t child = fork();
  Check(child >= 0, "fork failed");
  if (!child) {
    if (drop_privileges && geteuid() == 0) {
      if (setgid(65534) || setuid(65534)) _exit(100);
    }
    try {
      operation();
      _exit(0);
    } catch (const std::exception& error) {
      std::cerr << "child: " << error.what() << '\n';
      _exit(101);
    }
  }
  int status = 0;
  Check(waitpid(child, &status, 0) == child, "waitpid failed");
  if (drop_privileges && WIFEXITED(status) && WEXITSTATUS(status) == 100) {
    // Some managed host runners expose uid 0 but disallow setuid/setgid. Do
    // not bypass that restriction or claim their root process models a PS5
    // title's access rights. Other tests still exercise actual filesystem I/O.
    return false;
  }
  Check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child filesystem check failed");
  return true;
}

}  // namespace

int main() {
  char temporary[] = "/tmp/simpsons-prospero-paths-XXXXXX";
  const char* created = mkdtemp(temporary);
  Check(created != nullptr, "mkdtemp failed");
  const fs::path root = created;
  try {
    const auto initial_cwd = fs::current_path();
    Check(initial_cwd != root, "host CWD must differ from the simulated title root");
#if defined(PROSPERO_HOST_CHDIR_EPERM)
    bool rejected = false;
    try {
      // This is the exact standard-library call formerly used by the PS5
      // SetupEnvironment path; verify its real EPERM exception semantics.
      fs::current_path(root);
    } catch (const fs::filesystem_error& error) {
      rejected = error.code() == std::errc::operation_not_permitted;
    }
    Check(rejected && chdir_attempts == 1, "chdir EPERM injection did not reproduce startup failure");
    Check(fs::current_path() == initial_cwd, "rejected chdir changed the host working directory");
    std::cout << "PASS former startup call throws EPERM at the real host filesystem boundary\n";
#endif
    Check(ResolveAppPath(root, "cache") == root / "cache", "relative path lost title root");
    Check(ResolveAppPath(root, {}).empty(), "unset optional path became nonempty");
    Check(ResolveAppPath(root, "/explicit/cache") == "/explicit/cache",
          "explicit absolute override was changed");
    std::cout << "PASS relative and absolute paths\n";

    const auto direct_paths = DefaultStoragePaths(root, root);
    Check(direct_paths.game_data == root / "gamedata" &&
              direct_paths.user_data == root / "userdata" &&
              direct_paths.cache == root / "cache" && direct_paths.logs == root / "logs",
          "direct title-folder defaults changed");
    RequireWritableDirectory(direct_paths.user_data);
    RequireWritableDirectory(direct_paths.cache);
    RequireWritableDirectory(direct_paths.logs);
    fs::create_directory(direct_paths.game_data);
    { std::ofstream file(direct_paths.game_data / "asset.bin"); file << "title data"; }
    Check(Read(ResolveAppPath(root, "gamedata/asset.bin")) == "title data",
          "game data access depended on the process working directory");
    Check(fs::is_empty(root / "cache"), "write probe left a file behind");
    const auto config = direct_paths.config;
    EnsureDefaultProfile(config, "frame_rate = 60\n");
    Check(Read(config) == "frame_rate = 60\n", "initial profile contents differ");
    {
      std::ofstream modified(config);
      modified << "frame_rate = 30\n# existing preferences\n";
    }
    EnsureDefaultProfile(config, "frame_rate = 60\n");
    RequireWritableFile(config);
    Check(Read(config) == "frame_rate = 30\n# existing preferences\n",
          "existing profile was modified");
    std::cout << "PASS writable storage and preservation of existing preferences\n";

    auto collision = root / "file-instead-of-directory";
    { std::ofstream file(collision); file << "untouched"; }
    auto error = ExpectFailure([&] { RequireWritableDirectory(collision); });
    Check(error.find(collision.string()) != std::string::npos, "failure omitted actual path");
    Check(error.find("No additional storage directory") != std::string::npos,
          "failure omitted storage policy");
    Check(Read(collision) == "untouched", "directory check modified an existing file");
    error = ExpectFailure([&] { EnsureDefaultProfile(root / "userdata", "bad"); });
    Check(error.find("regular file") != std::string::npos, "directory accepted as config");
    std::cout << "PASS path collisions produce a concrete error without modifying data\n";

    fs::create_directories(root / "real" / "child");
    fs::create_symlink(root / "real" / "child", root / "alias");
    const auto symlink_parent = ResolveAppPath(root, "alias/../cache");
    RequireWritableDirectory(symlink_parent);
    Check(fs::is_directory(root / "real" / "cache"), "symlink/.. semantics were collapsed");
    const auto explicit_path = root / "explicit-other-directory";
    RequireWritableDirectory(ResolveAppPath(root / "title", explicit_path));
    std::cout << "PASS explicit directories and symlink semantics remain supported\n";

    // Model an FTP-owned title root with writable pre-staged subdirectories.
    // A different runtime UID can create its config/cache/logs/saves without
    // permission to create or rename anything in the title root itself.
    const auto title = root / "ftp-title";
    fs::create_directories(title);
    Check(chmod(root.c_str(), 0755) == 0 && chmod(title.c_str(), 0555) == 0,
          "cannot set title permissions");
    // Prepare these as the packaging process before dropping runtime rights.
    Check(chmod(title.c_str(), 0755) == 0, "cannot prepare title");
    for (const char* name : {"userdata", "cache", "logs"}) {
      fs::create_directory(title / name);
      Check(chmod((title / name).c_str(), 0777) == 0, "cannot set data directory permissions");
    }
    Check(chmod(title.c_str(), 0555) == 0, "cannot restrict title root");
    bool runtime_uid_test = InChild([&] {
      for (const char* name : {"userdata", "cache", "logs"}) {
        RequireWritableDirectory(title / name);
      }
      EnsureDefaultProfile(title / "userdata" / "simpsons.toml", "user_language = 6\n");
      auto denied = ExpectFailure([&] { RequireWritableDirectory(title); });
      Check(denied.find("cannot write this path") != std::string::npos,
            "permission diagnostic did not explain runtime-user access");
    }, true);
    if (runtime_uid_test) {
      Check(Read(title / "userdata" / "simpsons.toml") == "user_language = 6\n",
            "runtime user failed to initialize title-local config");
      std::cout << "PASS a nonwritable title root with writable packaged data subdirectories\n";
    } else {
      std::cout << "SKIP different runtime UID: host runner disallows setuid/setgid\n";
    }

    const auto read_only_config = root / "read-only.toml";
    { std::ofstream file(read_only_config); file << "preserve me\n"; }
    Check(chmod(read_only_config.c_str(), 0444) == 0, "cannot make config read-only");
    bool file_access_test = InChild([&] {
      auto denied = ExpectFailure([&] { EnsureDefaultProfile(read_only_config, "replacement\n"); });
      Check(denied.find(read_only_config.string()) != std::string::npos,
            "unwritable config diagnostic omitted its path");
    }, true);
    Check(Read(read_only_config) == "preserve me\n", "read-only preferences were modified");
    if (file_access_test) {
      std::cout << "PASS unwritable existing config fails without replacement\n";
    } else {
      std::cout << "SKIP unwritable-file UID check: host runner disallows setuid/setgid\n";
    }

    InChild([&] {
      std::signal(SIGXFSZ, SIG_IGN);
      const rlimit limit{1024, 1024};
      Check(setrlimit(RLIMIT_FSIZE, &limit) == 0, "cannot constrain test file size");
      auto interrupted = root / "partial-profile.toml";
      auto failure = ExpectFailure([&] { EnsureDefaultProfile(interrupted, std::string(8192, 'x')); });
      Check(failure.find(interrupted.string()) != std::string::npos, "write error omitted path");
      Check(!fs::exists(interrupted), "failed first write left a partial default config");
    }, false);
    std::cout << "PASS failed initial write removes its incomplete profile\n";

    struct statvfs mount_info{};
    if (statvfs("/sys", &mount_info) == 0 && (mount_info.f_flag & ST_RDONLY)) {
      const auto image_paths = DefaultStoragePaths("/sys", root / "image-state");
      Check(image_paths.game_data == "/sys/gamedata",
            "read-only image's game data were redirected to writable storage");
      RequireWritableDirectory(image_paths.user_data);
      RequireWritableDirectory(image_paths.cache);
      RequireWritableDirectory(image_paths.logs);
      EnsureDefaultProfile(image_paths.config, "user_language = 6\n");
      EnsureDefaultProfile(image_paths.config, "user_language = 1\n");
      Check(Read(image_paths.config) == "user_language = 6\n",
            "read-only image state configuration was replaced");
      Check(ResolveAppPath("/sys", "custom/cache") == "/sys/custom/cache",
            "relative override anchor followed the writable-state fallback");
      std::cout << "PASS read-only app root with separate writable state and preserved profile\n";
      auto read_only = ExpectFailure([] { RequireWritableDirectory("/sys"); });
      Check(read_only.find("mounted read-only") != std::string::npos,
            "actual read-only filesystem did not produce the mount diagnostic");
      std::cout << "PASS actual read-only mount reports why portable storage cannot start\n";
    } else {
      std::cout << "SKIP actual read-only mount: host /sys is not a read-only filesystem\n";
    }

    chmod(title.c_str(), 0755);
    fs::remove_all(root);
    Check(fs::current_path() == initial_cwd,
          "explicit path preparation changed the process working directory");
#if defined(PROSPERO_HOST_CHDIR_EPERM)
    Check(chdir_attempts == 1, "production path helpers tried changing the working directory");
    std::cout << "PASS explicit game/config/cache/log/state paths work with unrelated CWD and chdir denied\n";
#endif
    std::cout << "Portable path host checks complete; PS5 hardware remains untested.\n";
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    std::error_code ignored;
    fs::permissions(root / "ftp-title", fs::perms::owner_all, fs::perm_options::add, ignored);
    fs::remove_all(root, ignored);
    return EXIT_FAILURE;
  }
}
