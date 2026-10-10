// Host-only isolation of the production GPU-independent cache control flow.
// The runner includes exact production blocks; Vulkan operations are excluded.
#include <rex/assert.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef NDEBUG
#error This regression must exercise the Release assertion behavior.
#endif

namespace {
unsigned checks = 0;

void Check(bool condition, const char* message) {
  ++checks;
  if (!condition) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
  }
}

struct ThreadCounters {
  int attempts = 0;
  int fail_at = -1;
  int named = 0;
  int joined = 0;
  int destroyed = 0;
} thread_counters;

std::vector<std::string> markers;
template <typename... Args>
void LogMessage(const char*, Args&&...) {}
}  // namespace

#define REXGPU_WARN(...) LogMessage(__VA_ARGS__)
#define REXGPU_ERROR(...) LogMessage(__VA_ARGS__)

extern "C" void prospero_startup_log(const char* message) {
  markers.emplace_back(message);
}
#include "markers.inc"

namespace rex::thread {
class Thread {
 public:
  struct CreationParameters {};
  static std::unique_ptr<Thread> Create(CreationParameters, std::function<void()> work) {
    if (thread_counters.attempts++ == thread_counters.fail_at) {
      return nullptr;
    }
    return std::unique_ptr<Thread>(new Thread(std::move(work)));
  }

  // Virtual, as in the real ReX Thread: invoking this through null in Release
  // is diagnosed by UBSan in the unmodified baseline factory blocks.
  virtual void set_name(std::string name) {
    name_ = std::move(name);
    ++thread_counters.named;
  }

  virtual ~Thread() {
    Check(!thread_.joinable(), "production destroys a worker before waiting for it");
    ++thread_counters.destroyed;
  }

  void Join() {
    Check(thread_.joinable(), "each successfully created thread is waited exactly once");
    thread_.join();
    ++thread_counters.joined;
  }

 private:
  explicit Thread(std::function<void()> work) : thread_(std::move(work)) {}
  std::thread thread_;
  std::string name_;
};

void Wait(Thread* thread, bool alertable) {
  Check(thread && !alertable, "only a real non-alertable thread handle is waited");
  thread->Join();
}
}  // namespace rex::thread

namespace rex::filesystem {
int truncate_calls = 0;
int fail_truncate_at = -1;
bool bypass_actual_truncate = false;
#include "fileops.inc"

bool TruncateStdioFile(FILE* file, uint64_t length) {
  if (truncate_calls++ == fail_truncate_at) {
    errno = EIO;
    return false;
  }
  return bypass_actual_truncate || ActualTruncateStdioFile(file, length);
}
}  // namespace rex::filesystem

namespace {
struct PoolFixture {
  std::vector<std::unique_ptr<rex::thread::Thread>> creation_threads_;
  std::atomic<unsigned> completed{0};

  void CreationThread(size_t) { ++completed; }
  void Start(size_t creation_thread_count) {
#include "pool.inc"
  }
  void Join() {
    for (const auto& worker : creation_threads_) {
      rex::thread::Wait(worker.get(), false);
    }
    creation_threads_.clear();
  }
};

void CheckPool(int fail_at) {
  constexpr size_t count = 4;
  thread_counters.fail_at = fail_at;
  PoolFixture pool;
  pool.Start(count);
  size_t expected = fail_at < 0 ? count : size_t(fail_at);
  Check(pool.creation_threads_.size() == expected, "only started persistent workers are retained");
  Check(thread_counters.attempts == int(expected + (fail_at >= 0)),
        "persistent pool stops trying after its first creation failure");
  Check(thread_counters.named == int(expected), "persistent pool never names a null thread");
  Check((pool.creation_threads_.empty()) == (fail_at == 0),
        "existing synchronous pipeline path is selected when the pool is empty");
  pool.Join();
  Check(pool.completed.load() == expected, "every retained persistent worker runs");
  Check(thread_counters.destroyed == int(expected), "persistent worker ownership is released");
}

void CheckPreload(size_t creation_thread_count, int fail_at) {
  constexpr size_t jobs = 97;
  thread_counters.fail_at = fail_at;
  std::array<std::atomic<unsigned>, jobs> visits{};
  std::atomic<size_t> next_creation_index{0};
  std::atomic<unsigned> completions{0};
  unsigned caller_invocations = 0;
  auto caller = std::this_thread::get_id();
  auto creation_worker = [&]() {
    if (std::this_thread::get_id() == caller) {
      ++caller_invocations;
    }
    while (true) {
      size_t index = next_creation_index.fetch_add(1);
      if (index >= jobs) {
        break;
      }
      ++visits[index];
      std::this_thread::yield();
      ++completions;
    }
  };
#include "preload.inc"
  for (auto& count : visits) {
    Check(count.load() == 1, "each preload job runs exactly once, including partial-pool failure");
  }
  Check(completions.load() == jobs, "preload finishes before captured job state is destroyed");
  int expected = creation_thread_count <= 1 ? 0 :
      (fail_at >= 0 ? fail_at : int(creation_thread_count));
  Check(thread_counters.joined == expected, "all started preload workers are joined");
  Check(thread_counters.destroyed == expected, "all preload handles are released after join");
  Check(caller_invocations == unsigned(creation_thread_count <= 1 || fail_at >= 0),
        "the caller drains jobs when no worker can be started or the pool is partial");
  Check(thread_counters.attempts == (creation_thread_count <= 1 ? 0 : expected + (fail_at >= 0)),
        "preload stops trying after creation failure and respects serial mode");
}

struct ShaderStorageFixture {
  std::unique_ptr<rex::thread::Thread> storage_write_thread_;
  std::mutex storage_write_request_lock_;
  std::condition_variable storage_write_request_cond_;
  bool storage_write_thread_shutdown_ = false;
  bool storage_write_flush_shaders_ = true;
  bool storage_write_flush_pipelines_ = true;
  std::deque<int> storage_write_shader_queue_{1};
  std::deque<int> storage_write_pipeline_queue_{2};
  FILE* pipeline_storage_file_ = nullptr;
  FILE* shader_storage_file_ = nullptr;
  bool pipeline_storage_file_flush_needed_ = true;
  bool shader_storage_file_flush_needed_ = true;
  std::filesystem::path shader_storage_cache_root_{"/test/cache"};
  uint32_t shader_storage_title_id_ = 0x45410809;
  // Sentinels for data that ShutdownShaderStorage must leave resident. Actual
  // shaders and driver objects are intentionally outside this host isolation.
  std::array<uint64_t, 2> resident_objects{0xCAFE, 0xF00D};

  ShaderStorageFixture() {
    pipeline_storage_file_ = std::tmpfile();
    shader_storage_file_ = std::tmpfile();
    Check(pipeline_storage_file_ && shader_storage_file_, "host FILE fixture opens");
  }
  ~ShaderStorageFixture() { ShutdownShaderStorage(); }
  void StorageWriteThread() {
    std::unique_lock<std::mutex> lock(storage_write_request_lock_);
    storage_write_request_cond_.wait(lock, [this]() { return storage_write_thread_shutdown_; });
  }
  void StartWriter() {
#include "writer.inc"
  }
  void PrepareRecords(size_t count) {
    struct PipelineStoredDescription { uint8_t data[64]; };
    std::vector<PipelineStoredDescription> pipeline_stored_descriptions(count);
    struct { uint32_t magic, magic_api, version_swapped; } pipeline_storage_file_header{};
    constexpr uint32_t pipeline_storage_magic = 0x53504558;
    constexpr uint32_t pipeline_storage_magic_api = 0x006F6266;
    constexpr uint32_t pipeline_storage_version_swapped = 0x01000000;
    [[maybe_unused]] uint64_t pipeline_storage_valid_bytes =
        count ? sizeof(pipeline_storage_file_header) + sizeof(PipelineStoredDescription) * count : 0;
#include "truncate.inc"
  }
  void ShutdownShaderStorage();
};
#include "shutdown.inc"

void CheckStorageClosed(ShaderStorageFixture& storage, int pipeline_fd, int shader_fd) {
  Check(!storage.pipeline_storage_file_ && !storage.shader_storage_file_,
        "production cleanup clears both FILE pointers");
  Check(fcntl(pipeline_fd, F_GETFD) == -1 && errno == EBADF,
        "production cleanup closes the pipeline file descriptor");
  Check(fcntl(shader_fd, F_GETFD) == -1 && errno == EBADF,
        "production cleanup closes the shader file descriptor");
  Check(storage.storage_write_shader_queue_.empty() && storage.storage_write_pipeline_queue_.empty(),
        "no queued writes remain without a writer");
  Check(!storage.pipeline_storage_file_flush_needed_ && !storage.shader_storage_file_flush_needed_,
        "disabled storage has no outstanding flush request");
  Check(storage.shader_storage_cache_root_.empty() && storage.shader_storage_title_id_ == 0,
        "disabled storage clears the writer's identity");
  Check(storage.resident_objects == std::array<uint64_t, 2>{0xCAFE, 0xF00D},
        "record-storage cleanup preserves resident objects");
}

void CheckWriter(bool fail) {
  thread_counters.fail_at = fail ? 0 : -1;
  ShaderStorageFixture storage;
  int pipeline_fd = fileno(storage.pipeline_storage_file_);
  int shader_fd = fileno(storage.shader_storage_file_);
  storage.StartWriter();
  Check(bool(storage.storage_write_thread_) == !fail, "writer creation result is handled explicitly");
  if (fail) {
    CheckStorageClosed(storage, pipeline_fd, shader_fd);
    Check(thread_counters.named == 0, "a missing writer is never dereferenced to set its name");
    Check(markers.back() == "cache: storage writer create failed", "raw marker identifies writer failure");
  } else {
    Check(storage.pipeline_storage_file_ && storage.shader_storage_file_, "successful writer retains files");
    Check(thread_counters.named == 1, "successful writer keeps its normal name");
    storage.ShutdownShaderStorage();
    Check(thread_counters.joined == 1 && thread_counters.destroyed == 1,
          "production shutdown waits for the writer before closing streams");
    CheckStorageClosed(storage, pipeline_fd, shader_fd);
  }
}

void CheckTruncate(size_t count, int fail_at, bool write_failure) {
  ShaderStorageFixture storage;
  int pipeline_fd = fileno(storage.pipeline_storage_file_);
  int shader_fd = fileno(storage.shader_storage_file_);
  if (write_failure) {
    // A real read-only FILE produces a short fwrite after both valid truncates.
    char fd_path[64];
    std::snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", pipeline_fd);
    FILE* read_only = std::fopen(fd_path, "rb");
    Check(read_only != nullptr, "read-only FILE fixture opens");
    fclose(storage.pipeline_storage_file_);
    storage.pipeline_storage_file_ = read_only;
    pipeline_fd = fileno(read_only);
    // ftruncate on read-only would fail first, so use a wrapper that reports
    // success for the truncates only in this explicit fwrite-result test.
    rex::filesystem::bypass_actual_truncate = true;
  }
  rex::filesystem::fail_truncate_at = fail_at;
  storage.PrepareRecords(count);
  if (fail_at >= 0 || write_failure) {
    CheckStorageClosed(storage, pipeline_fd, shader_fd);
    Check(thread_counters.attempts == 0, "failed record preparation does not create a writer");
    Check(markers.back() == "cache: pipeline record truncate failed", "raw marker identifies record preparation failure");
  } else {
    Check(storage.pipeline_storage_file_ != nullptr, "valid record file stays open");
    Check(std::fflush(storage.pipeline_storage_file_) == 0, "record header flush succeeds");
    struct stat info{};
    Check(fstat(pipeline_fd, &info) == 0, "prepared record size is readable");
    Check(info.st_size == off_t(12 + 64 * count), "valid record count determines truncation size");
    Check(rex::filesystem::truncate_calls == (count ? 1 : 2), "cold and warm paths execute their real truncate calls");
  }
}
}  // namespace

int main(int argc, char** argv) {
  Check(argc == 2, "one test case is selected");
  std::string test = argv[1];
  if (test == "pool-success") CheckPool(-1);
  else if (test == "pool-first-failure") CheckPool(0);
  else if (test == "pool-partial-failure") CheckPool(2);
  else if (test == "preload-success") CheckPreload(4, -1);
  else if (test == "preload-serial") CheckPreload(1, -1);
  else if (test == "preload-first-failure") CheckPreload(4, 0);
  else if (test == "preload-partial-failure") CheckPreload(4, 2);
  else if (test == "writer-success") CheckWriter(false);
  else if (test == "writer-failure") CheckWriter(true);
  else if (test == "truncate-cold") CheckTruncate(0, -1, false);
  else if (test == "truncate-warm") CheckTruncate(3, -1, false);
  else if (test == "truncate-first-failure") CheckTruncate(3, 0, false);
  else if (test == "truncate-reset-failure") CheckTruncate(0, 1, false);
  else if (test == "truncate-write-failure") CheckTruncate(0, -1, true);
  else Check(false, "unknown test case");
  std::printf("PASS %s (%u checks)\n", test.c_str(), checks);
  return 0;
}
