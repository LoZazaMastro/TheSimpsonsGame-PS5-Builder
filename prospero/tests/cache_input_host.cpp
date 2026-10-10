// Host isolation of exact production cache parsing and teardown blocks.
// Uses real stdio, real XXH3 and the production on-disk structs. Vulkan calls
// and shader creation are capture sinks; no PS5 driver or game data is used.
#include <rex/graphics/vulkan/cache_file_reader.h>
#include <rex/graphics/xenos.h>
#include <rex/hash.h>

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <new>
#include <queue>
#include <set>
#include <thread>
#include <unordered_map>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
unsigned checks = 0;
unsigned failures = 0;
size_t allocation_limit = SIZE_MAX;
size_t rejected_allocation = 0;
void Check(bool condition, const char* message) {
  ++checks;
  if (!condition) {
    ++failures;
    std::fprintf(stderr, "FAIL: %s\n", message);
  }
}
template <typename... Args> void LogMessage(const char*, Args&&...) {}
void LogStorageStartupStage(const char*) {}
}  // namespace

// A corrupt baseline record requests nearly 8 GiB. Record and reject that
// allocation deterministically, instead of allocating or exhausting the host.
void* operator new(size_t bytes) {
  if (bytes > allocation_limit) {
    rejected_allocation = bytes;
    throw std::bad_alloc();
  }
  if (void* p = std::malloc(bytes ? bytes : 1)) return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, size_t) noexcept { std::free(p); }

#define REXGPU_INFO(...) LogMessage(__VA_ARGS__)
#define REXGPU_WARN(...) LogMessage(__VA_ARGS__)
#define REXGPU_ERROR(...) LogMessage(__VA_ARGS__)

namespace rex::filesystem {
int truncate_calls = 0;
bool truncate_failure = false;
bool truncate_bypass = false;
#include "fileops.inc"
bool TruncateStdioFile(FILE* file, uint64_t size) {
  ++truncate_calls;
  if (truncate_failure) { errno = EIO; return false; }
  return truncate_bypass || ActualTruncateStdioFile(file, size);
}
FILE* OpenFile(const std::filesystem::path& path, const char* mode) {
  return std::fopen(path.c_str(), mode);
}
}  // namespace rex::filesystem

namespace rex::graphics::vulkan {
struct VulkanRenderTargetCache {
#include "render_pass_key.inc"
};
#include "cache_types.inc"
// Only this version constant is used by the extracted file header code.
struct SpirvShaderTranslator {
  struct Modification { static constexpr uint32_t kVersion = 12; };
};
struct VulkanShader {
  uint64_t stored_index = 0;
  uint64_t ucode_storage_index() const { return stored_index; }
  void set_ucode_storage_index(uint64_t value) { stored_index = value; }
};

struct FileFixture {
  FILE* file = std::tmpfile();
  int inspect_fd = file ? dup(fileno(file)) : -1;
  FileFixture() { Check(file && inspect_fd >= 0, "real temporary FILE opens"); }
  ~FileFixture() {
    if (file) std::fclose(file);
    if (inspect_fd >= 0) close(inspect_fd);
  }
  uint64_t Size() const {
    struct stat info{};
    Check(fstat(inspect_fd, &info) == 0, "cache descriptor can be inspected after loader cleanup");
    return uint64_t(info.st_size);
  }
  void FlushRewind() {
    Check(std::fflush(file) == 0 && std::fseek(file, 0, SEEK_SET) == 0,
          "test cache flush and rewind succeed");
  }
  template <typename T> void Write(const T& value) {
    Check(std::fwrite(&value, sizeof(value), 1, file) == 1, "test record write succeeds");
  }
  std::vector<uint8_t> Bytes() const {
    std::vector<uint8_t> data(Size());
    Check(pread(inspect_fd, data.data(), data.size(), 0) == ssize_t(data.size()),
          "cache contents are observable without changing loader position");
    return data;
  }
};

struct ShaderFixture : FileFixture {
  FILE*& shader_storage_file_ = file;
  uint64_t shader_storage_index_ = 3;
  std::unordered_map<uint64_t, VulkanShader> shaders;
  unsigned load_calls = 0;
  bool complete = false;
  bool shutdown = false;
  void ShutdownShaderStorage() {
    shutdown = true;
    if (file) std::fclose(file);
    file = nullptr;
  }
  VulkanShader* LoadShader(xenos::ShaderType, const uint32_t*, uint32_t, uint64_t hash) {
    ++load_calls;
    return &shaders[hash];
  }
  void Read() {
#include "shader_extent.inc"
#include "shader_read.inc"
    complete = true;
  }
  void Header() {
    const uint32_t value[] = {0x48534558, rex::byte_swap(ShaderStoredHeader::kVersion)};
    Write(value);
  }
  void Record(std::initializer_list<uint32_t> words, bool corrupt = false) {
    ShaderStoredHeader header{};
    header.ucode_dword_count = uint32_t(words.size());
    header.type = xenos::ShaderType::kVertex;
    header.ucode_data_hash = XXH3_64bits(words.begin(), words.size() * sizeof(uint32_t)) ^ corrupt;
    Write(header);
    Check(std::fwrite(words.begin(), sizeof(uint32_t), words.size(), file) == words.size(),
          "shader bytecode fixture write succeeds");
  }
};

struct PipelineFixture : FileFixture {
  FILE*& pipeline_storage_file_ = file;
  bool pipeline_storage_file_flush_needed_ = false;
  bool complete = false;
  bool shutdown = false;
  size_t supported = 0;
  size_t needed = 0;
  bool ArePipelineRequirementsMet(const PipelineDescription& description) {
    return !description.rasterizer_discard;
  }
  void ShutdownShaderStorage() {
    shutdown = true;
    if (file) std::fclose(file);
    file = nullptr;
  }
  void Read() {
    const bool edram_fragment_shader_interlock = false;
    std::vector<PipelineStoredDescription> pipeline_stored_descriptions;
    std::set<std::pair<uint64_t, uint64_t>> shader_translations_needed;
#include "pipeline_read.inc"
#include "pipeline_truncate.inc"
    supported = pipeline_stored_descriptions.size();
    needed = shader_translations_needed.size();
    complete = true;
  }
  void Header() {
    const uint32_t header[] = {0x53504558, 0, rex::byte_swap(PipelineDescription::kVersion)};
    Write(header);
  }
  void Record(uint64_t id, bool unsupported = false, bool corrupt = false) {
    PipelineStoredDescription record{};
    record.description.vertex_shader_hash = id;
    record.description.rasterizer_discard = unsupported;
    record.description_hash = record.description.GetHash() ^ corrupt;
    Write(record);
  }
};

void CheckShader(std::string test) {
  ShaderFixture f;
  if (test != "shader-cold" && test != "shader-write-error" && test != "shader-oversize-file") {
    f.Header();
    f.Record({0x101, 0x202, 0x303});
  }
  const uint64_t prefix = uint64_t(std::ftell(f.file));
  if (test == "shader-huge-count" || test == "shader-short-record") {
    ShaderStoredHeader bad{};
    bad.ucode_dword_count = test == "shader-huge-count" ? 0x7FFFFFFF : 37;
    f.Write(bad);
    if (test == "shader-short-record") f.Write(uint32_t(0xCD));
  } else if (test == "shader-bad-hash") {
    f.Record({0x909, 0xA0A}, true);
  } else if (test == "shader-trailing-bytes") {
    f.Write(uint8_t(0xAB));
  } else if (test == "shader-valid") {
    f.Record({0x404, 0x505});
  } else if (test == "shader-oversize-file") {
    Check(ftruncate(f.inspect_fd, cache_file::kMaxShareableCacheBytes + 1) == 0,
          "oversized optional cache is sparse, not allocated in host RAM");
  }
  f.FlushRewind();
  const uint64_t original_size = f.Size();
  std::vector<uint8_t> expected_prefix;
  if (prefix) {
    expected_prefix = f.Bytes();
    expected_prefix.resize(size_t(prefix));
  }
  if (test == "shader-truncate-error") rex::filesystem::truncate_failure = true;
  if (test == "shader-write-error") {
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/self/fd/%d", f.inspect_fd);
    std::fclose(f.file);
    f.file = std::fopen(path, "rb");
    Check(f.file, "read-only stream opens for actual fwrite error");
    rex::filesystem::truncate_bypass = true;
  }
  allocation_limit = size_t(1) << 20;
  bool oversized_allocation = false;
  try { f.Read(); }
  catch (const std::bad_alloc&) { oversized_allocation = true; }
  allocation_limit = SIZE_MAX;
  Check(!oversized_allocation, "untrusted on-disk shader count never causes a large allocation");
  if (oversized_allocation) {
    std::printf("Captured rejected allocation: %zu bytes\n", rejected_allocation);
    return;
  }
  if (test == "shader-truncate-error" || test == "shader-write-error") {
    Check(f.shutdown && !f.complete, "cache preparation failure disables further cache writes");
    return;
  }
  if (test == "shader-oversize-file") {
    Check(f.shutdown && !f.complete, "oversized cache is skipped");
    Check(f.Size() == original_size, "oversized input is preserved intact");
    Check(rex::filesystem::truncate_calls == 0, "oversized input is never truncated");
    return;
  }
  Check(f.complete && !f.shutdown, "valid cache prefix remains usable");
  if (f.file) std::fflush(f.file);
  if (test == "shader-cold") {
    Check(f.Size() == 8 && f.load_calls == 0, "cold cache gets only its checked header");
  } else if (test == "shader-valid") {
    Check(f.Size() == original_size && f.load_calls == 2, "all valid records are loaded and preserved");
  } else {
    Check(f.Size() == prefix && f.load_calls == 1, "only the corrupt tail is removed");
    Check(f.Bytes() == expected_prefix, "valid shader prefix bytes remain identical");
  }
}

void CheckPipeline(const std::string& test) {
  PipelineFixture f;
  f.Header();
  f.Record(1, test == "pipeline-all-unsupported");
  f.Record(2, true);
  if (test != "pipeline-all-unsupported") f.Record(3);
  const uint64_t prefix = uint64_t(std::ftell(f.file));
  if (test == "pipeline-corrupt-tail") {
    f.Record(4, false, true);
    f.Record(5);
  } else if (test == "pipeline-short-tail") {
    f.Write(uint8_t(0xAC));
  }
  f.FlushRewind();
  auto expected_prefix = f.Bytes();
  expected_prefix.resize(size_t(prefix));
  f.Read();
  if (f.file) std::fflush(f.file);
  Check(f.complete && !f.shutdown, "pipeline cache parser completes");
  const size_t expected_supported = test == "pipeline-all-unsupported" ? 0 : 2;
  Check(f.supported == expected_supported && f.needed == expected_supported,
        "unsupported records are excluded from prewarming");
  Check(f.Size() == prefix, "valid file prefix includes unsupported records");
  Check(f.Bytes() == expected_prefix, "pipeline record bytes are preserved through filtering");
}

struct Cookie {
  std::vector<uint8_t> bytes;
  int64_t position = 0;
  int64_t error_after = INT64_MAX;
  bool seek_error = false;
  static ssize_t Read(void* opaque, char* target, size_t size) {
    auto& c = *static_cast<Cookie*>(opaque);
    if (c.position >= c.error_after) { errno = EIO; return -1; }
    size = std::min({size, size_t(std::max(int64_t(0), int64_t(c.bytes.size()) - c.position)),
                     size_t(c.error_after - c.position)});
    std::memcpy(target, c.bytes.data() + c.position, size);
    c.position += int64_t(size);
    return ssize_t(size);
  }
  static int Seek(void* opaque, off64_t* offset, int whence) {
    auto& c = *static_cast<Cookie*>(opaque);
    if (c.seek_error) { errno = EIO; return -1; }
    const int64_t base = whence == SEEK_END ? int64_t(c.bytes.size()) :
                         whence == SEEK_CUR ? c.position : 0;
    if (base + *offset < 0) { errno = EINVAL; return -1; }
    c.position = base + *offset;
    *offset = c.position;
    return 0;
  }
  FILE* Open() {
    cookie_io_functions_t functions{};
    functions.read = Read;
    functions.seek = Seek;
    FILE* file = fopencookie(this, "r", functions);
    Check(file != nullptr && std::setvbuf(file, nullptr, _IONBF, 0) == 0,
          "stdio fault-injection stream opens");
    return file;
  }
};

void CheckIo() {
  ShaderFixture f;
  f.Header();
  f.Record({11, 12});
  f.FlushRewind();
  Cookie cookie{f.Bytes()};
  cookie.error_after = 8;
  std::fclose(f.file);
  f.file = cookie.Open();
  f.Read();
  Check(f.shutdown && !f.complete, "real stdio EIO disables cache writes");
  Check(rex::filesystem::truncate_calls == 0, "I/O errors do not truncate cached input");
}

void CheckReadHelpers() {
  using namespace cache_file;
  FileFixture f;
  const uint32_t words[] = {0xABC, 0xDEF};
  f.Write(words);
  f.FlushRewind();
  uint64_t extent = 0;
  Check(GetFileExtent(f.file, extent) && extent == sizeof(words) && std::ftell(f.file) == 0,
        "file extent check restores original cursor");
  std::vector<uint32_t> output{0x55};
  Check(ReadElements(f.file, extent, 0x7FFFFFFF, kMaxShaderDwords, output) == ReadResult::kInvalidSize,
        "31-bit corrupt count is rejected before resize");
  Check(output == std::vector<uint32_t>{0x55}, "invalid count leaves output untouched");
  Check(ReadElements(f.file, extent, 3, kMaxShaderDwords, output) == ReadResult::kTruncated,
        "record count must fit actual remaining file bytes");
  Check(output == std::vector<uint32_t>{0x55}, "short extent is rejected before resize");
  Check(ReadElements(f.file, extent, 2, kMaxShaderDwords, output) == ReadResult::kSuccess &&
        output == std::vector<uint32_t>({0xABC, 0xDEF}), "valid record is read unchanged");
  Check(ReadElements(f.file, extent, 0, kMaxShaderDwords, output) == ReadResult::kSuccess && output.empty(),
        "zero-length record at EOF needs no allocation");
  std::vector<uint8_t> blob;
  Check(ReadBlob(f.file, 7, blob) == ReadResult::kInvalidSize && blob.empty(),
        "blob byte cap is checked before allocation");
  Check(ReadBlob(f.file, 8, blob) == ReadResult::kSuccess && blob.size() == 8,
        "exact blob cap is accepted");
  Cookie cookie{{1, 2, 3, 4}};
  cookie.error_after = 2;
  FILE* faulty = cookie.Open();
  Check(ReadBlob(faulty, 8, blob) == ReadResult::kIoError && blob.empty(),
        "real fread EIO is distinguished from truncation");
  std::fclose(faulty);
  cookie.position = 0;
  cookie.seek_error = true;
  faulty = cookie.Open();
  Check(ReadBlob(faulty, 8, blob) == ReadResult::kIoError && blob.empty(),
        "failed ftell/fseek is rejected without ambiguous allocation");
  std::fclose(faulty);
  cookie.position = 0;
  cookie.seek_error = false;
  cookie.error_after = INT64_MAX;
  faulty = cookie.Open();
  std::vector<uint8_t> shortened;
  Check(ReadElements(faulty, 8, 8, 8, shortened) == ReadResult::kTruncated && shortened.empty(),
        "file shrinking after extent snapshot is detected by actual fread");
  std::fclose(faulty);
  FileFixture maximum;
  Check(ftruncate(maximum.inspect_fd, int64_t(kMaxShaderDwords) * 4) == 0,
        "largest legal PM4 program fixture opens");
  Check(ReadElements(maximum.file, uint64_t(kMaxShaderDwords) * 4, kMaxShaderDwords,
                     kMaxShaderDwords, output) == ReadResult::kSuccess && output.size() == kMaxShaderDwords,
        "largest real PM4 shader count remains accepted");
  Check(ReadElements(maximum.file, UINT64_MAX, uint64_t(kMaxShaderDwords) + 1,
                     kMaxShaderDwords, output) == ReadResult::kInvalidSize,
        "first impossible PM4 shader count is rejected");
}

bool ReadAot(FILE* aot_file, size_t& observed_size) {
#include "aot_read.inc"
  observed_size = aot_data.size();
  return true;
}
void CheckAot(const std::string& test) {
  FileFixture f;
  const uint64_t size = test == "aot-oversize" ? cache_file::kMaxShaderModuleBytes + 4 :
                        test == "aot-unaligned" ? 7 : 8;
  Check(ftruncate(f.inspect_fd, int64_t(size)) == 0, "SPIR-V module size fixture is sparse");
  FILE* file = f.file;
  f.file = nullptr;  // Production owns fclose.
  size_t observed = 0;
  allocation_limit = size_t(cache_file::kMaxShaderModuleBytes);
  bool accepted = false;
  bool oversized_allocation = false;
  try { accepted = ReadAot(file, observed); }
  catch (const std::bad_alloc&) { oversized_allocation = true; std::fclose(file); }
  allocation_limit = SIZE_MAX;
  Check(!oversized_allocation, "oversized SPIR-V is rejected before allocating its claimed file size");
  Check(accepted == (test == "aot-valid"), "real AOT read branch enforces size and dword alignment");
  if (accepted) Check(observed == size, "valid AOT input passes through unchanged");
  Check(f.Size() == size, "AOT input is left intact");
}
}  // namespace rex::graphics::vulkan

namespace {
std::atomic<bool> worker_active{false};
unsigned unsafe_cache_access = 0;
unsigned destroy_calls = 0;
VkPhysicalDeviceProperties properties{};
}

namespace rex::ui::vulkan {
class VulkanDevice {
 public:
  struct Functions {
    static void vkGetPhysicalDeviceProperties(VkPhysicalDevice, VkPhysicalDeviceProperties* p) {
      *p = properties;
    }
    static VkResult vkGetPipelineCacheData(VkDevice, VkPipelineCache, size_t* bytes, void* output) {
      if (worker_active.load()) ++unsafe_cache_access;
      if (!output) *bytes = 64;
      else std::memset(output, 0, *bytes);
      return VK_SUCCESS;
    }
    static void vkDestroyPipelineCache(VkDevice, VkPipelineCache, const VkAllocationCallbacks*) {
      if (worker_active.load()) ++unsafe_cache_access;
      ++destroy_calls;
    }
  };
  const Functions& functions() const { static Functions f; return f; }
  const VulkanDevice* vulkan_instance() const { return this; }
  VkPhysicalDevice physical_device() const { return VK_NULL_HANDLE; }
  VkDevice device() const { return VK_NULL_HANDLE; }
};
}  // namespace rex::ui::vulkan

namespace rex::thread {
struct Thread {
  std::thread thread;
  explicit Thread(std::function<void()> function) : thread(std::move(function)) {}
  ~Thread() { if (thread.joinable()) thread.join(); }
};
void Wait(Thread* thread, bool) { thread->thread.join(); }
}  // namespace rex::thread

namespace rex::graphics::vulkan {
#include "driver_read.inc"

struct NamedFile {
  std::filesystem::path path;
  int fd = -1;
  NamedFile() {
    char value[] = "/tmp/simpsons-cache-input-XXXXXX";
    fd = mkstemp(value);
    path = value;
    Check(fd >= 0, "named cache file opens");
  }
  ~NamedFile() { if (fd >= 0) close(fd); unlink(path.c_str()); }
};

void CheckDriver() {
  NamedFile f;
  ui::vulkan::VulkanDevice device;
  properties.vendorID = 0x1002;
  properties.deviceID = 0xDA7A;
  properties.pipelineCacheUUID[0] = 0x78;
  VkPipelineCacheHeaderVersionOne header{};
  header.headerSize = sizeof(header);
  header.headerVersion = VK_PIPELINE_CACHE_HEADER_VERSION_ONE;
  header.vendorID = properties.vendorID;
  header.deviceID = properties.deviceID;
  std::memcpy(header.pipelineCacheUUID, properties.pipelineCacheUUID, VK_UUID_SIZE);
  std::vector<uint8_t> data(sizeof(header) + 17, 0x3C);
  auto store_header = [&] {
    std::memcpy(data.data(), &header, sizeof(header));
    Check(pwrite(f.fd, data.data(), data.size(), 0) == ssize_t(data.size()), "driver fixture writes");
    Check(ftruncate(f.fd, data.size()) == 0, "driver fixture size resets");
  };
  store_header();
  Check(ReadDriverCacheFile(device, f.path) == data, "compatible driver cache bytes reach Vulkan unchanged");
  header.vendorID ^= 1;
  store_header();
  Check(ReadDriverCacheFile(device, f.path).empty(), "different vendor cache is rejected");
  header.vendorID ^= 1;
  header.deviceID ^= 1;
  store_header();
  Check(ReadDriverCacheFile(device, f.path).empty(), "different device cache is rejected");
  header.deviceID ^= 1;
  header.pipelineCacheUUID[0] ^= 1;
  store_header();
  Check(ReadDriverCacheFile(device, f.path).empty(), "different driver UUID cache is rejected");
  header.pipelineCacheUUID[0] ^= 1;
  header.headerSize = 1;
  store_header();
  Check(ReadDriverCacheFile(device, f.path).empty(), "undersized driver header is rejected");
  header.headerSize = uint32_t(data.size() + 1);
  store_header();
  Check(ReadDriverCacheFile(device, f.path).empty(), "driver header cannot exceed actual file");
  header.headerSize = sizeof(header);
  header.headerVersion = VkPipelineCacheHeaderVersion(2);
  store_header();
  Check(ReadDriverCacheFile(device, f.path).empty(), "unrecognized driver header version is rejected");
  Check(ftruncate(f.fd, sizeof(header) - 1) == 0, "short driver fixture truncates");
  Check(ReadDriverCacheFile(device, f.path).empty(), "truncated driver header is rejected");
  Check(ftruncate(f.fd, cache_file::kMaxDriverCacheBytes + 1) == 0, "oversized driver fixture is sparse");
  Check(ReadDriverCacheFile(device, f.path).empty(), "oversized driver cache is skipped");
  struct stat info{};
  Check(fstat(f.fd, &info) == 0 && uint64_t(info.st_size) == cache_file::kMaxDriverCacheBytes + 1,
        "oversized driver input remains intact");
}

struct ShutdownFixture {
  struct CommandProcessor {
    ui::vulkan::VulkanDevice device;
    const ui::vulkan::VulkanDevice* GetVulkanDevice() const { return &device; }
  } command_processor_;
  std::mutex creation_request_lock_;
  std::condition_variable creation_request_cond_;
  std::vector<std::unique_ptr<rex::thread::Thread>> creation_threads_;
  std::queue<int> creation_queue_;
  size_t creation_threads_shutdown_from_ = SIZE_MAX;
  unsigned creation_threads_busy_ = 1;
  bool startup_loading_ = true;
  bool creation_completion_set_event_ = true;
  std::unique_ptr<int> creation_completion_event_ = std::make_unique<int>(1);
  VkPipelineCache vk_pipeline_cache_ = VkPipelineCache(uintptr_t(0x10));
  std::filesystem::path vk_pipeline_cache_path_;
  void Shutdown() {
#include "cache_shutdown.inc"
  }
  void Start() {
    creation_threads_.push_back(std::make_unique<rex::thread::Thread>([this] {
      std::unique_lock<std::mutex> lock(creation_request_lock_);
      worker_active = true;
      creation_request_cond_.notify_all();
      creation_request_cond_.wait(lock, [this] { return creation_threads_shutdown_from_ == 0; });
      worker_active = false;
    }));
    std::unique_lock<std::mutex> lock(creation_request_lock_);
    creation_request_cond_.wait(lock, [] { return worker_active.load(); });
  }
};
void CheckShutdown(bool warm_path) {
  NamedFile file;
  ShutdownFixture f;
  if (warm_path) f.vk_pipeline_cache_path_ = file.path;
  f.Start();
  f.Shutdown();
  Check(!worker_active && f.creation_threads_.empty(), "production shutdown joins active creation workers");
  Check(unsafe_cache_access == 0, "driver cache is saved and destroyed only after active workers finish");
  Check(destroy_calls == 1 && f.vk_pipeline_cache_ == VK_NULL_HANDLE,
        "driver cache ownership is released exactly once");
  Check(f.creation_threads_busy_ == 0 && !f.startup_loading_ && !f.creation_completion_event_,
        "creation state is reset after worker join");
  if (unsafe_cache_access) std::printf("Captured premature driver cache accesses: %u\n", unsafe_cache_access);
}
}  // namespace rex::graphics::vulkan

int main(int argc, char** argv) {
  using namespace rex::graphics::vulkan;
  Check(argc == 2, "exactly one test scenario selected");
  if (argc != 2) return 2;
  std::string test = argv[1];
  if (test == "read-helpers") CheckReadHelpers();
  else if (test == "shader-io-error") CheckIo();
  else if (test.starts_with("shader-")) CheckShader(test);
  else if (test.starts_with("pipeline-")) CheckPipeline(test);
  else if (test.starts_with("aot-")) CheckAot(test);
  else if (test == "driver") CheckDriver();
  else if (test == "shutdown-cold") CheckShutdown(false);
  else if (test == "shutdown-warm") CheckShutdown(true);
  else Check(false, "known scenario");
  std::printf("%s %s (%u checks, %u failures)\n", failures ? "FAIL" : "PASS", test.c_str(), checks, failures);
  return failures ? 1 : 0;
}
