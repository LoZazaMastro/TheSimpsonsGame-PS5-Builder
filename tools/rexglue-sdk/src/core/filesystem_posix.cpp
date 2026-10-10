/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <iostream>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <rex/assert.h>
#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/string.h>
#if REX_PLATFORM_PROSPERO
#include <rex/platform/trace_game_prospero.h>
#endif

#include <dirent.h>
#include <ftw.h>
#include <libgen.h>
#include <pwd.h>
#if REX_PLATFORM_PROSPERO
#define fseeko64 fseeko
#define ftello64 ftello
#define ftruncate64 ftruncate
#define off64_t off_t
#endif

namespace rex {

std::string path_to_utf8(const std::filesystem::path& path) {
  return path.string();
}

std::u16string path_to_utf16(const std::filesystem::path& path) {
  return rex::string::to_utf16(path.string());
}

std::filesystem::path to_path(const std::string_view source) {
  return source;
}

std::filesystem::path to_path(const std::u16string_view source) {
  return rex::string::to_utf8(source);
}

namespace filesystem {

#if REX_PLATFORM_PROSPERO
namespace {
using rex::prospero::diagnostics::Line;
using rex::prospero::diagnostics::PathHash;
using rex::prospero::diagnostics::RateLimit;
RateLimit file_open_logs{32}, file_error_logs{16}, file_close_logs{8}, file_short_logs{8};

void TraceFileError(uintptr_t phase, const std::filesystem::path& path, int fd,
                    const char* operation, size_t offset, size_t requested, int error) {
  if (!file_error_logs.Allow(phase)) return;
  rex::prospero::diagnostics::PreserveErrno preserve;
  const auto hash = PathHash(path.native());
  if (phase == PROSPERO_GAME_FILE_OTHER_ERROR) {
    prospero_trace_checkpoint(phase, hash, PathHash(operation), uint32_t(error));
  } else {
    prospero_trace_checkpoint(phase, hash, offset,
                              (uint64_t(uint32_t(error)) << 32) | uint32_t(requested));
  }
  Line line("file_error");
  line.Number("phase", phase); line.Number("path_hash", hash);
  line.Number("fd_u32", uint32_t(fd)); line.Number("errno", uint32_t(error));
  line.Number("offset", offset); line.Number("requested", requested);
  line.Number("suppressed_total", file_error_logs.total_suppressed());
  line.Path("operation", operation); line.Path("path_pct", path.native()); line.Write();
}
}  // namespace
#endif

std::filesystem::path GetExecutablePath() {
#if REX_PLATFORM_PROSPERO
  return "/app0/eboot.bin";
#else
  char buff[FILENAME_MAX] = "";
  readlink("/proc/self/exe", buff, FILENAME_MAX);
  std::string s(buff);
  return s;
#endif
}

std::filesystem::path GetExecutableFolder() {
  return GetExecutablePath().parent_path();
}

std::filesystem::path GetUserFolder() {
#if REX_PLATFORM_PROSPERO
  return GetExecutableFolder() / "userdata";
#else
  // get preferred data home
  char* home = std::getenv("XDG_DATA_HOME");
  if (home) {
    return std::string(home);
  }

  // if XDG_DATA_HOME not set, fallback to HOME directory
  home = std::getenv("HOME");

  // if HOME not set, fall back to this
  if (home == NULL) {
    struct passwd pw1;
    struct passwd* pw;
    char buf[4096];  // could potentionally lower this
    getpwuid_r(getuid(), &pw1, buf, sizeof(buf), &pw);
    assert(&pw1 == pw);  // sanity check
    home = pw->pw_dir;
  }

  return std::filesystem::path(home) / ".local" / "share";
#endif
}

std::filesystem::path GetLegacyUserFolder() {
  // The POSIX location has always been the XDG data home - nothing to migrate.
  return {};
}

FILE* OpenFile(const std::filesystem::path& path, const std::string_view mode) {
  return fopen(path.c_str(), std::string(mode).c_str());
}

bool Seek(FILE* file, int64_t offset, int origin) {
  return fseeko64(file, off64_t(offset), origin) == 0;
}

int64_t Tell(FILE* file) {
  return int64_t(ftello64(file));
}

bool TruncateStdioFile(FILE* file, uint64_t length) {
  if (fflush(file)) {
    return false;
  }
  int64_t position = Tell(file);
  if (position < 0) {
    return false;
  }
  if (ftruncate64(fileno(file), off64_t(length))) {
    return false;
  }
  if (uint64_t(position) > length) {
    if (!Seek(file, 0, SEEK_END)) {
      return false;
    }
  }
  return true;
}

static int removeCallback(const char* fpath, const struct stat* sb, int typeflag,
                          struct FTW* ftwbuf) {
  int rv = remove(fpath);
  return rv;
}

static uint64_t convertUnixtimeToWinFiletime(time_t unixtime) {
  // Linux uses number of seconds since 1/1/1970, and Windows uses
  // number of nanoseconds since 1/1/1601
  // so we convert linux time to nanoseconds and then add the number of
  // nanoseconds from 1601 to 1970
  // see https://msdn.microsoft.com/en-us/library/ms724228
  uint64_t filetime = filetime = (unixtime * 10000000) + 116444736000000000;
  return filetime;
}

bool CreateEmptyFile(const std::filesystem::path& path) {
  int file = creat(path.c_str(), 0774);
  if (file >= 0) {
    close(file);
    return true;
  }
  return false;
}

class PosixFileHandle : public FileHandle {
 public:
  PosixFileHandle(std::filesystem::path path, int handle, bool append_only)
      : FileHandle(std::move(path)), handle_(handle), append_only_(append_only) {}
  ~PosixFileHandle() override {
    [[maybe_unused]] const int result = close(handle_);
#if REX_PLATFORM_PROSPERO
    if (result < 0) TraceFileError(PROSPERO_GAME_FILE_OTHER_ERROR, path(), handle_,
                                  "close", 0, 0, errno);
    if (file_close_logs.Allow(PROSPERO_GAME_FILE_CLOSE)) {
      const uint64_t read = read_bytes_.load(std::memory_order_relaxed);
      const uint64_t written = written_bytes_.load(std::memory_order_relaxed);
      const auto hash = PathHash(path().native());
      prospero_trace_checkpoint(PROSPERO_GAME_FILE_CLOSE, hash, read, written);
      Line line("file_close"); line.Number("path_hash", hash);
      line.Number("fd_u32", uint32_t(handle_)); line.Number("result_u32", uint32_t(result));
      line.Number("reads", reads_.load(std::memory_order_relaxed)); line.Number("read_bytes", read);
      line.Number("writes", writes_.load(std::memory_order_relaxed)); line.Number("written_bytes", written);
      line.Number("short_reads", short_reads_.load(std::memory_order_relaxed));
      line.Number("short_writes", short_writes_.load(std::memory_order_relaxed));
      line.Number("suppressed_total", file_close_logs.total_suppressed());
      line.Path("path_pct", path().native()); line.Write();
    }
#endif
    handle_ = -1;
  }
  bool Read(size_t file_offset, void* buffer, size_t buffer_length,
            size_t* out_bytes_read) override {
    *out_bytes_read = 0;
#if REX_PLATFORM_PROSPERO
    reads_.fetch_add(1, std::memory_order_relaxed);
#endif
    ssize_t out;
    do {
      out = pread(handle_, buffer, buffer_length, file_offset);
    } while (out < 0 && errno == EINTR);
    if (out < 0) {
#if REX_PLATFORM_PROSPERO
      TraceFileError(PROSPERO_GAME_FILE_READ_ERROR, path(), handle_, "pread",
                      file_offset, buffer_length, errno);
#endif
      return false;
    }
    *out_bytes_read = static_cast<size_t>(out);
#if REX_PLATFORM_PROSPERO
    read_bytes_.fetch_add(size_t(out), std::memory_order_relaxed);
    if (size_t(out) < buffer_length && short_reads_.fetch_add(1, std::memory_order_relaxed) == 0 &&
        file_short_logs.Allow(PROSPERO_GAME_FILE_SHORT)) {
      TraceShort("read", file_offset, buffer_length, size_t(out));
    }
#endif
    return true;
  }
  bool Write(size_t file_offset, const void* buffer, size_t buffer_length,
             size_t* out_bytes_written) override {
    *out_bytes_written = 0;
#if REX_PLATFORM_PROSPERO
    writes_.fetch_add(1, std::memory_order_relaxed);
#endif
    ssize_t out;
    do {
      // A pure FILE_APPEND_DATA handle writes atomically at EOF. Using write
      // with O_APPEND is portable; pwrite's handling of O_APPEND differs
      // between Linux and BSD. FILE_WRITE_DATA handles retain explicit offsets.
      out = append_only_ ? write(handle_, buffer, buffer_length)
                         : pwrite(handle_, buffer, buffer_length, file_offset);
    } while (out < 0 && errno == EINTR);
    if (out < 0) {
#if REX_PLATFORM_PROSPERO
      TraceFileError(PROSPERO_GAME_FILE_WRITE_ERROR, path(), handle_, append_only_ ? "write" : "pwrite",
                      file_offset, buffer_length, errno);
#endif
      return false;
    }
    *out_bytes_written = static_cast<size_t>(out);
#if REX_PLATFORM_PROSPERO
    written_bytes_.fetch_add(size_t(out), std::memory_order_relaxed);
    if (size_t(out) < buffer_length) {
      short_writes_.fetch_add(1, std::memory_order_relaxed);
      if (file_short_logs.Allow(PROSPERO_GAME_FILE_SHORT))
        TraceShort("write", file_offset, buffer_length, size_t(out));
    }
#endif
    return true;
  }
  bool SetLength(size_t length) override {
    const int result = ftruncate(handle_, length);
#if REX_PLATFORM_PROSPERO
    if (result < 0) TraceFileError(PROSPERO_GAME_FILE_OTHER_ERROR, path(), handle_,
                                  "ftruncate", length, 0, errno);
#endif
    return result >= 0;
  }
  void Flush() override {
    [[maybe_unused]] const int result = fsync(handle_);
#if REX_PLATFORM_PROSPERO
    if (result < 0) TraceFileError(PROSPERO_GAME_FILE_OTHER_ERROR, path(), handle_,
                                  "fsync", 0, 0, errno);
#endif
  }

 private:
  int handle_ = -1;
  bool append_only_ = false;
#if REX_PLATFORM_PROSPERO
  void TraceShort(const char* operation, size_t offset, size_t requested, size_t completed) {
    const auto hash = PathHash(path().native());
    prospero_trace_checkpoint(PROSPERO_GAME_FILE_SHORT, hash, requested, completed);
    Line line("file_short"); line.Number("path_hash", hash); line.Number("offset", offset);
    line.Number("requested", requested); line.Number("completed", completed);
    line.Number("suppressed_total", file_short_logs.total_suppressed());
    line.Path("operation", operation); line.Path("path_pct", path().native()); line.Write();
  }
  std::atomic<uint64_t> reads_{0}, read_bytes_{0}, writes_{0}, written_bytes_{0};
  std::atomic<uint64_t> short_reads_{0}, short_writes_{0};
#endif
};

std::unique_ptr<FileHandle> FileHandle::OpenExisting(const std::filesystem::path& path,
                                                     uint32_t desired_access) {
  // These POSIX access modes are mutually exclusive values, not independent
  // permission bits (O_RDONLY is zero). Select one after decoding the Xbox
  // access mask, including the specific bits added by VirtualFileSystem.
  const bool readable = desired_access & (FileAccess::kGenericRead | FileAccess::kGenericExecute |
                                          FileAccess::kGenericAll | FileAccess::kFileReadData);
  const bool write_data = desired_access &
                          (FileAccess::kGenericWrite | FileAccess::kGenericAll |
                           FileAccess::kFileWriteData);
  const bool append_only = !write_data && (desired_access & FileAccess::kFileAppendData);
  const bool writable = write_data || append_only;
  int open_access = writable ? (readable ? O_RDWR : O_WRONLY) : O_RDONLY;
  if (append_only) {
    open_access |= O_APPEND;
  }
  int handle = open(path.c_str(), open_access);
#if REX_PLATFORM_PROSPERO
  const int error = handle < 0 ? errno : 0;
  auto& limit = handle < 0 ? file_error_logs : file_open_logs;
  if (limit.Allow(PROSPERO_GAME_FILE_OPEN)) {
    const auto hash = PathHash(path.native());
    prospero_trace_checkpoint(PROSPERO_GAME_FILE_OPEN, hash, uint32_t(open_access),
                              (uint64_t(uint32_t(error)) << 32) | uint32_t(handle));
    Line line("file_open"); line.Number("path_hash", hash);
    line.Number("fd_u32", uint32_t(handle)); line.Number("errno", uint32_t(error));
    line.Number("flags", uint32_t(open_access)); line.Number("access", desired_access);
    line.Number("suppressed_total", limit.total_suppressed());
    line.Path("path_pct", path.native()); line.Write();
  }
#endif
  if (handle == -1) {
    // TODO(benvanik): pick correct response.
    return nullptr;
  }
  return std::make_unique<PosixFileHandle>(path, handle, append_only);
}

bool GetInfo(const std::filesystem::path& path, FileInfo* out_info) {
  struct stat st;
  if (stat(path.c_str(), &st) == 0) {
    if (S_ISDIR(st.st_mode)) {
      out_info->type = FileInfo::Type::kDirectory;
      out_info->total_size = 0;
    } else {
      out_info->type = FileInfo::Type::kFile;
      out_info->total_size = st.st_size;
    }
    out_info->path = path.parent_path();
    out_info->name = path.filename();
    out_info->create_timestamp = convertUnixtimeToWinFiletime(st.st_ctime);
    out_info->access_timestamp = convertUnixtimeToWinFiletime(st.st_atime);
    out_info->write_timestamp = convertUnixtimeToWinFiletime(st.st_mtime);
    return true;
  }
  return false;
}

std::vector<FileInfo> ListFiles(const std::filesystem::path& path) {
  std::vector<FileInfo> result;

  DIR* dir = opendir(path.c_str());
  if (!dir) {
#if REX_PLATFORM_PROSPERO
    TraceFileError(PROSPERO_GAME_FILE_OTHER_ERROR, path, -1, "opendir", 0, 0, errno);
#endif
    return result;
  }

  while (auto ent = readdir(dir)) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
      continue;
    }

    FileInfo info;

    info.name = ent->d_name;
    struct stat st{};
    const auto full_path = path / info.name;
    bool is_directory = ent->d_type == DT_DIR;
    if (ent->d_type == DT_UNKNOWN) {
      // Filesystems may omit d_type. Do not follow a hidden symlink here:
      // HostPathDevice recursively populates directories, so a link to a
      // parent would otherwise recurse forever. Native PS5 can deny lstat;
      // open/fstat also works there, and O_NONBLOCK avoids blocking on FIFOs.
      const int fd = open(full_path.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
      if (fd < 0) continue;
      const int result = fstat(fd, &st);
      close(fd);
      if (result != 0) continue;
      is_directory = S_ISDIR(st.st_mode);
    } else if (stat(full_path.c_str(), &st) != 0) {
      // Entries may disappear between readdir and stat, or be inaccessible.
      // Never publish uninitialized size/type/timestamps into the guest VFS.
      continue;
    }
    info.create_timestamp = convertUnixtimeToWinFiletime(st.st_ctime);
    info.access_timestamp = convertUnixtimeToWinFiletime(st.st_atime);
    info.write_timestamp = convertUnixtimeToWinFiletime(st.st_mtime);
    info.path = path;
    if (is_directory) {
      info.type = FileInfo::Type::kDirectory;
      info.total_size = 0;
    } else {
      info.type = FileInfo::Type::kFile;
      info.total_size = st.st_size;
    }
    result.push_back(info);
  }
  closedir(dir);
  return result;
}

}  // namespace filesystem
}  // namespace rex
