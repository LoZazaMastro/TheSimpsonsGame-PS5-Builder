#include <rex/ui/prospero_paths.h>

#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace rex::ui::prospero {
namespace {

[[noreturn]] void FailPath(const char* operation, const std::filesystem::path& path,
                           const std::error_code& error) {
  std::string message = std::string(operation) + " '" + path.string() + "': " +
                        error.message();
  if (error == std::errc::read_only_file_system) {
    message += ". The selected storage is mounted read-only. Configuration, logs, cache "
               "and saves require a writable directory.";
  } else if (error == std::errc::permission_denied ||
             error == std::errc::operation_not_permitted) {
    message += ". The title's runtime user cannot write this path. Check the mount and "
               "the permissions of userdata, cache and logs.";
  }
  message += " No additional storage directory was selected.";
  throw std::runtime_error(message);
}

[[noreturn]] void FailPath(const char* operation, const std::filesystem::path& path,
                           int error) {
  FailPath(operation, path, std::error_code(error, std::generic_category()));
}

// Preserve the first error while completing cleanup. EINTR is retried for
// writes; close is never retried because the descriptor may already be closed.
int WriteAndClose(int fd, std::string_view contents) {
  int error = 0;
  while (!contents.empty()) {
    ssize_t written = write(fd, contents.data(), contents.size());
    if (written < 0 && errno == EINTR) continue;
    if (written <= 0) {
      error = written < 0 ? errno : EIO;
      break;
    }
    contents.remove_prefix(static_cast<size_t>(written));
  }
  if (!error && fsync(fd) != 0) error = errno;
  if (close(fd) != 0 && !error) error = errno;
  return error;
}

}  // namespace

StoragePaths DefaultStoragePaths(const std::filesystem::path& app_root,
                                 const std::filesystem::path& state_root) {
  return {app_root / "gamedata", state_root / "userdata", state_root / "cache",
          state_root / "userdata" / "simpsons.toml", state_root / "logs"};
}

std::filesystem::path ResolveAppPath(const std::filesystem::path& app_root,
                                     const std::filesystem::path& path) {
  if (path.empty()) return {};
  // Do not collapse '..': an explicit path may cross a symlink, whose parent
  // must retain normal filesystem semantics.
  return path.is_absolute() ? path : app_root / path;
}

void RequireWritableDirectory(const std::filesystem::path& path) {
  std::error_code error;
  std::filesystem::create_directories(path, error);
  if (error) FailPath("Cannot create writable directory", path, error);
  if (!std::filesystem::is_directory(path, error)) {
    if (!error) error = std::make_error_code(std::errc::not_a_directory);
    FailPath("Expected a writable directory", path, error);
  }

  auto name = (path / ".startup-write-check-XXXXXX").string();
  std::vector<char> temporary(name.begin(), name.end());
  temporary.push_back('\0');
  int fd = mkstemp(temporary.data());
  if (fd < 0) FailPath("Cannot write to directory", path, errno);

  int io_error = WriteAndClose(fd, "write-check\n");
  if (unlink(temporary.data()) != 0 && !io_error) io_error = errno;
  if (io_error) FailPath("Cannot complete a write in directory", path, io_error);
}

void RequireWritableFile(const std::filesystem::path& path) {
  std::error_code error;
  if (!std::filesystem::is_regular_file(path, error)) {
    if (!error) error = std::make_error_code(std::errc::invalid_argument);
    FailPath("Expected a writable regular file", path, error);
  }
  // Opening without O_TRUNC verifies write access while preserving the user's
  // existing preferences byte-for-byte. It also detects read-only mounts.
  int fd = open(path.c_str(), O_WRONLY | O_APPEND);
  if (fd < 0) FailPath("Cannot open file for writing", path, errno);
  if (close(fd) != 0) FailPath("Cannot close writable file", path, errno);
}

void EnsureDefaultProfile(const std::filesystem::path& path, std::string_view contents) {
  int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    if (errno == EEXIST) {
      RequireWritableFile(path);
      return;
    }
    FailPath("Cannot create initial configuration", path, errno);
  }
  int error = WriteAndClose(fd, contents);
  if (error) {
    unlink(path.c_str());
    FailPath("Cannot write initial configuration", path, error);
  }
}

}  // namespace rex::ui::prospero
