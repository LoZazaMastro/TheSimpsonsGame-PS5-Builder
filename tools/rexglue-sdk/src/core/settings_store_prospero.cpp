// BSD 3-Clause License. See the project LICENSE.
#include <rex/platform/settings_store_prospero.h>
#include <toml++/toml.hpp>

#include <cerrno>
#include <chrono>
#include <cmath>
#include <fcntl.h>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace rex::platform::prospero {
namespace {
constexpr size_t kMaxConfigBytes = 1024 * 1024;
struct File {
  int fd = -1;
  ~File() { if (fd >= 0) close(fd); }
  void Close() {
    const int value = fd;
    fd = -1;
    if (value >= 0 && close(value))
      throw std::system_error(errno, std::generic_category(), "Close settings file");
  }
};
struct Existing {
  std::string bytes;
  mode_t mode = 0666;
  bool exists = false;
};
[[noreturn]] void Error(const char* operation) {
  throw std::system_error(errno, std::generic_category(), operation);
}
Existing Read(const std::filesystem::path& path) {
  File input{open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW)};
  if (input.fd < 0) {
    if (errno == ENOENT) return {};
    Error("Read settings file");
  }
  struct stat info{};
  if (fstat(input.fd, &info)) Error("Inspect settings file");
  if (!S_ISREG(info.st_mode) || info.st_size < 0 || uint64_t(info.st_size) > kMaxConfigBytes)
    throw std::runtime_error("Settings must be a regular file smaller than 1 MiB");
  Existing result{{}, mode_t(info.st_mode & 0777), true};
  char buffer[4096];
  for (;;) {
    const ssize_t count = read(input.fd, buffer, sizeof(buffer));
    if (count < 0 && errno == EINTR) continue;
    if (count < 0) Error("Read settings file");
    if (!count) break;
    if (size_t(count) > kMaxConfigBytes - result.bytes.size())
      throw std::runtime_error("Settings exceed the 1 MiB limit");
    result.bytes.append(buffer, size_t(count));
  }
  input.Close();
  return result;
}
void Collect(const toml::table& table, ConfigValues& output, const std::string& prefix = {}) {
  for (const auto& [key, value] : table) {
    const auto name = prefix.empty() ? std::string(key.str()) : prefix + "_" + std::string(key.str());
    if (const auto* child = value.as_table()) Collect(*child, output, name);
    else if (const auto* item = value.as_boolean()) output[name] = item->get();
    else if (const auto* item = value.as_integer()) output[name] = item->get();
    else if (const auto* item = value.as_floating_point()) output[name] = item->get();
    else if (const auto* item = value.as_string()) output[name] = item->get();
  }
}
void RemoveChanged(toml::table& table, const ConfigValues& changes,
                   const std::string& prefix = {}) {
  std::vector<std::string> remove;
  for (auto& [key, value] : table) {
    const auto name = prefix.empty() ? std::string(key.str()) : prefix + "_" + std::string(key.str());
    if (auto* child = value.as_table()) RemoveChanged(*child, changes, name);
    else if (changes.contains(name)) remove.emplace_back(key.str());
  }
  for (const auto& key : remove) table.erase(key);
}
}  // namespace

ConfigValues ReadSettingsValues(const std::filesystem::path& path) {
  const auto file = Read(path);
  const auto table = toml::parse(file.bytes);
  ConfigValues result;
  Collect(table, result);
  return result;
}

SettingsSaveResult SaveSettingsValues(const std::filesystem::path& path, const ConfigValues& changes) {
  const auto existing = Read(path);
  auto table = toml::parse(existing.bytes);
  ConfigValues existing_values;
  Collect(table, existing_values);
  RemoveChanged(table, changes);
  for (const auto& [key, value] : changes) {
    if (key.empty() || key.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_") != std::string::npos)
      throw std::runtime_error("Invalid settings key");
    if (const auto* number = std::get_if<double>(&value); number && !std::isfinite(*number))
      throw std::runtime_error("Invalid non-finite settings value");
    auto replacement = value;
    if (key == "prospero_profile_version") {
      const auto found = existing_values.find(key);
      const auto* requested_version = std::get_if<int64_t>(&value);
      const auto* current_version = found == existing_values.end() ? nullptr : std::get_if<int64_t>(&found->second);
      if (requested_version && current_version && *current_version > *requested_version)
        replacement = *current_version;
    }
    std::visit([&](const auto& typed) { table.insert_or_assign(key, typed); }, replacement);
  }
  std::ostringstream stream;
  stream << "# The Simpsons Game - saved by the PS5 settings menu.\n" << table << '\n';
  const std::string output = stream.str();
  if (output.size() > kMaxConfigBytes) throw std::runtime_error("Settings exceed the 1 MiB limit");

  std::string temporary = path.string() + ".settings-XXXXXX";
  std::vector<char> name(temporary.begin(), temporary.end());
  name.push_back(0);
  File staged{mkstemp(name.data())};
  if (staged.fd < 0) Error("Create temporary settings file");
  struct Cleanup {
    const char* path;
    ~Cleanup() { if (path) unlink(path); }
  } cleanup{name.data()};
  if (fchmod(staged.fd, existing.mode)) Error("Preserve settings file permissions");
  std::string_view remaining(output);
  while (!remaining.empty()) {
    const ssize_t count = write(staged.fd, remaining.data(), remaining.size());
    if (count < 0 && errno == EINTR) continue;
    if (count < 0) Error("Write settings file");
    if (!count) throw std::runtime_error("Settings write made no progress");
    remaining.remove_prefix(size_t(count));
  }
  if (fsync(staged.fd)) Error("Flush settings file");
  staged.Close();
  // Optimistic conflict detection, matching the startup profile transaction.
  // An external edit observed during staging wins; Retry merges its unknown
  // fields afresh. No advisory lock can exclude arbitrary external writers.
  const auto current = Read(path);
  if (current.exists != existing.exists || current.mode != existing.mode || current.bytes != existing.bytes)
    throw std::runtime_error("The settings file changed externally during saving; retry to merge the latest file");
  if (rename(name.data(), path.c_str())) Error("Replace settings file");
  cleanup.path = nullptr;
  // Flush the directory entry as well as the file contents. If the filesystem
  // rejects this, the file is already installed: report a durability warning,
  // never the false claim that the settings were not saved.
  File directory{open(path.parent_path().c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY)};
  if (directory.fd < 0) return {std::system_error(errno, std::generic_category(), "Open settings directory for flush").what()};
  if (fsync(directory.fd)) return {std::system_error(errno, std::generic_category(), "Flush settings directory").what()};
  try { directory.Close(); }
  catch (const std::exception& failure) { return {failure.what()}; }
  return {};
}

SettingsConfigStore::SettingsConfigStore(std::filesystem::path path, Observer observer,
                                         std::function<void()> idle_observer)
    : path_(std::move(path)), observer_(std::move(observer)),
      idle_observer_(std::move(idle_observer)), worker_([this] { Run(); }) {}
SettingsConfigStore::~SettingsConfigStore() {
  {
    std::lock_guard lock(mutex_);
    stopping_ = true;
  }
  wake_.notify_one();
  if (worker_.joinable()) worker_.join();
}
uint64_t SettingsConfigStore::Queue(ConfigValues values) {
  uint64_t generation;
  {
    std::lock_guard lock(mutex_);
    pending_ = std::move(values);
    pending_ready_ = true;
    generation = ++status_.requested;
    status_.error.clear();
    status_.durability_warning.clear();
  }
  wake_.notify_one();
  return generation;
}
SettingsConfigStore::Status SettingsConfigStore::GetStatus() const {
  std::lock_guard lock(mutex_);
  return status_;
}
void SettingsConfigStore::Run() {
  for (;;) {
    if (idle_observer_) {
      try { idle_observer_(); } catch (...) {}
    }
    ConfigValues values;
    uint64_t generation;
    {
      std::unique_lock lock(mutex_);
      if (!wake_.wait_for(lock, std::chrono::milliseconds(250),
                          [this] { return stopping_ || pending_ready_; })) continue;
      if (!pending_ready_) return;
      // Coalesce slider/gamepad repeats without doing I/O on the caller.
      if (!stopping_) wake_.wait_for(lock, std::chrono::milliseconds(250), [this] { return stopping_; });
      values = std::move(pending_);
      pending_ready_ = false;
      generation = status_.requested;
    }
    std::string error, warning;
    try { warning = SaveSettingsValues(path_, values).durability_warning; }
    catch (const std::exception& failure) { error = failure.what(); }
    Status result;
    {
      std::lock_guard lock(mutex_);
      if (error.empty()) status_.persisted = generation;
      else status_.failed = generation;
      if (generation == status_.requested) {
        status_.error = error;
        status_.durability_warning = warning;
      }
      result = status_;
      // Observer describes this completed attempt, even if a newer request is
      // already queued. GetStatus still reports the latest requested version.
      result.requested = generation;
      result.error = std::move(error);
      result.durability_warning = std::move(warning);
    }
    if (observer_) {
      // Diagnostic observers cannot terminate the persistence worker.
      try { observer_(result); } catch (...) {}
    }
  }
}
}  // namespace rex::platform::prospero
