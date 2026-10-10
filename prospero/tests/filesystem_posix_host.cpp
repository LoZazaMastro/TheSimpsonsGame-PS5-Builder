// Exercise the production FileHandle/ListFiles backend against real POSIX files.
// Fault injection is confined to syscall results; no filesystem implementation
// or access-mask conversion is duplicated here.
#include <rex/filesystem.h>
#include <rex/platform/trace_prospero.h>

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {
int failures = 0;
int interrupted_reads = 0;
int interrupted_writes = 0;
int read_error = 0, write_error = 0;
bool unknown_directory_types = false;
bool denied_stat = false;

void Check(bool condition, const char* label) {
  std::printf("%s %s\n", condition ? "PASS" : "FAIL", label);
  failures += !condition;
}

bool Seed(const std::filesystem::path& path, const char* contents = "abcdef") {
  FILE* file = std::fopen(path.c_str(), "wb");
  if (!file) return false;
  const bool ok = std::fwrite(contents, 1, std::strlen(contents), file) == std::strlen(contents);
  return std::fclose(file) == 0 && ok;
}

std::string ReadWhole(const std::filesystem::path& path) {
  FILE* file = std::fopen(path.c_str(), "rb");
  if (!file) return "<unavailable>";
  std::array<char, 256> bytes{};
  const size_t count = std::fread(bytes.data(), 1, bytes.size(), file);
  std::fclose(file);
  return {bytes.data(), count};
}

bool Contains(const std::vector<rex::filesystem::FileInfo>& entries, const char* name,
              rex::filesystem::FileInfo::Type type) {
  for (const auto& entry : entries) {
    if (entry.name == name) return entry.type == type;
  }
  return false;
}
}  // namespace

extern "C" ssize_t __real_pread(int, void*, size_t, off_t);
extern "C" ssize_t __real_pwrite(int, const void*, size_t, off_t);
extern "C" ssize_t __real_write(int, const void*, size_t);
extern "C" dirent* __real_readdir(DIR*);
extern "C" int __real_stat(const char*, struct stat*);

extern "C" ssize_t __wrap_pread(int fd, void* data, size_t length, off_t offset) {
  if (read_error) { errno = read_error; return -1; }
  if (interrupted_reads > 0) {
    --interrupted_reads;
    errno = EINTR;
    return -1;
  }
  return __real_pread(fd, data, length, offset);
}
extern "C" ssize_t __wrap_pwrite(int fd, const void* data, size_t length, off_t offset) {
  if (write_error) { errno = write_error; return -1; }
  if (interrupted_writes > 0) {
    --interrupted_writes;
    errno = EINTR;
    return -1;
  }
  return __real_pwrite(fd, data, length, offset);
}
extern "C" ssize_t __wrap_write(int fd, const void* data, size_t length) {
  if (interrupted_writes > 0) {
    --interrupted_writes;
    errno = EINTR;
    return -1;
  }
  return __real_write(fd, data, length);
}
extern "C" dirent* __wrap_readdir(DIR* dir) {
  dirent* entry = __real_readdir(dir);
  if (entry && unknown_directory_types &&
      (!std::strcmp(entry->d_name, "child") || !std::strcmp(entry->d_name, "parent-link"))) {
    entry->d_type = DT_UNKNOWN;
  }
  return entry;
}
extern "C" int __wrap_stat(const char* path, struct stat* data) {
  const char* name = std::strrchr(path, '/');
  if (denied_stat && name && !std::strcmp(name + 1, "denied")) {
    std::memset(data, 0xA5, sizeof(*data));
    errno = EPERM;
    return -1;
  }
  return __real_stat(path, data);
}

int main(int argc, char** argv) {
  const int log = argc > 1 ? open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600) : -1;
  prospero_trace_set_output_fd(log);
  namespace fs = std::filesystem;
  using rex::filesystem::FileAccess;
  using rex::filesystem::FileHandle;
  using Type = rex::filesystem::FileInfo::Type;
  char directory[] = "/tmp/simpsons-file-io-XXXXXX";
  if (!mkdtemp(directory)) return 2;
  const fs::path root(directory), file = root / "save.dat";

  {
    const auto diagnostic_file = root / "space % file";
    Seed(diagnostic_file);
    auto handle = FileHandle::OpenExisting(diagnostic_file, FileAccess::kGenericAll);
    char bytes[16]{}; size_t transferred = 99;
    read_error = EIO;
    Check(handle && !handle->Read(7, bytes, sizeof(bytes), &transferred) &&
          transferred == 0 && errno == EIO, "read diagnostics preserve native EIO and zero result");
    read_error = 0; write_error = EIO;
    Check(handle && !handle->Write(9, "x", 1, &transferred) &&
          transferred == 0 && errno == EIO, "write diagnostics preserve native EIO and zero result");
    write_error = 0;
    Check(handle && handle->Read(0, bytes, sizeof(bytes), &transferred) && transferred == 6,
          "short EOF read remains successful and reports its actual length");
    auto missing = FileHandle::OpenExisting(root / "missing asset", FileAccess::kGenericRead);
    Check(!missing && errno == ENOENT, "failed open diagnostics preserve ENOENT");
  }

  const std::array<std::pair<uint32_t, const char*>, 4> masks{{
      {FileAccess::kGenericRead | FileAccess::kGenericWrite, "generic read/write round trip"},
      {FileAccess::kFileReadData | FileAccess::kFileWriteData, "specific read/write round trip"},
      {FileAccess::kGenericAll, "generic-all read/write round trip"},
      {FileAccess::kGenericAll | FileAccess::kFileReadData | FileAccess::kFileWriteData,
       "VFS-expanded generic-all read/write round trip"},
  }};
  for (const auto& [mask, label] : masks) {
    if (!Seed(file)) return 2;
    auto handle = FileHandle::OpenExisting(file, mask);
    std::array<char, 7> data{};
    size_t written = 99, read = 99;
    const bool ok = handle && handle->Write(2, "XY", 2, &written) && written == 2 &&
                    handle->Read(0, data.data(), 6, &read) && read == 6 &&
                    std::string(data.data(), read) == "abXYef";
    Check(ok, label);
  }

  {
    Seed(file);
    auto handle = FileHandle::OpenExisting(file, FileAccess::kGenericRead);
    size_t transferred = 99;
    Check(handle && !handle->Write(0, "x", 1, &transferred) && transferred == 0,
          "read-only write failure reports zero bytes");
    Check(handle && !handle->SetLength(1) && ReadWhole(file) == "abcdef",
          "read-only truncate preserves the save");
  }
  {
    auto handle = FileHandle::OpenExisting(file, FileAccess::kGenericWrite);
    char byte = 0;
    size_t transferred = 99;
    Check(handle && !handle->Read(0, &byte, 1, &transferred) && transferred == 0,
          "write-only read failure reports zero bytes");
  }
  {
    Seed(file);
    auto handle = FileHandle::OpenExisting(file, FileAccess::kFileAppendData);
    size_t written = 99;
    Check(handle && handle->Write(0, "+", 1, &written) && written == 1 &&
              ReadWhole(file) == "abcdef+", "append-only opens for writing and appends");
  }
  {
    Seed(file);
    auto handle = FileHandle::OpenExisting(
        file, FileAccess::kGenericRead | FileAccess::kGenericWrite | FileAccess::kFileAppendData);
    size_t written = 99;
    Check(handle && handle->Write(2, "XY", 2, &written) && written == 2 &&
              ReadWhole(file) == "abXYef", "write-data plus append access respects explicit offset");
    Check(handle && handle->SetLength(3) && ReadWhole(file) == "abX",
          "read/write save can be truncated");
  }
  {
    Seed(file);
    auto handle = FileHandle::OpenExisting(file, FileAccess::kGenericAll);
    std::array<char, 7> data{};
    size_t read = 99, written = 99;
    interrupted_reads = 1;
    Check(handle && handle->Read(0, data.data(), 6, &read) && read == 6 &&
              std::string(data.data(), read) == "abcdef" && interrupted_reads == 0,
          "interrupted positional read retries without false EOF");
    interrupted_writes = 1;
    Check(handle && handle->Write(2, "XY", 2, &written) && written == 2 &&
              ReadWhole(file) == "abXYef" && interrupted_writes == 0,
          "interrupted positional write retries without losing save data");
    constexpr size_t offset = size_t(5) * 1024 * 1024 * 1024;
    char byte = 0;
    Check(handle && handle->Write(offset, "Z", 1, &written) && written == 1 &&
              handle->Read(offset, &byte, 1, &read) && read == 1 && byte == 'Z' &&
              fs::file_size(file) == offset + 1,
          "positional I/O retains offsets above 4 GiB on a sparse file");
  }

  fs::create_directory(root / "child");
  Seed(root / "child" / "asset", "asset");
  Seed(root / "denied", "hidden");
  fs::create_symlink("missing", root / "broken");
  fs::create_directory_symlink(".", root / "parent-link");
  Check(Contains(rex::filesystem::ListFiles(root), "child", Type::kDirectory),
        "normal directory enumeration finds nested assets");
  unknown_directory_types = true;
  Check(Contains(rex::filesystem::ListFiles(root), "child", Type::kDirectory),
        "DT_UNKNOWN directory uses stat metadata");
  Check(!Contains(rex::filesystem::ListFiles(root), "parent-link", Type::kDirectory),
        "DT_UNKNOWN symlink to parent cannot trigger recursive VFS traversal");
  unknown_directory_types = false;
  denied_stat = true;
  const auto files = rex::filesystem::ListFiles(root);
  Check(!Contains(files, "denied", Type::kFile) && !Contains(files, "denied", Type::kDirectory),
        "failed stat does not publish uninitialized file metadata");
  Check(!Contains(files, "broken", Type::kFile) && !Contains(files, "broken", Type::kDirectory),
        "broken link is skipped after metadata failure");
  denied_stat = false;
  fs::remove_all(root);
  prospero_trace_dump_fd(log);
  prospero_trace_set_output_fd(-1);
  if (log >= 0) close(log);
  std::printf("%d failed checks\n", failures);
  return failures ? 1 : 0;
}
