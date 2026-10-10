// Exercises the real SharedMemory implementation. The runner substitutes only
// the guest-memory provider, CVar registration, logging and profiling plumbing;
// page-validity, invalidation, watching, streaming and range merging are real.
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include <rex/graphics/shared_memory.h>

namespace rex::thread {
std::recursive_mutex& global_critical_region::mutex() {
  static std::recursive_mutex instance;
  return instance;
}
}  // namespace rex::thread

namespace {
void Require(bool condition, const char* message) {
  if (!condition) {
    std::fprintf(stderr, "FAIL %s\n", message);
    std::abort();
  }
}

class TestSharedMemory final : public rex::graphics::SharedMemory {
 public:
  explicit TestSharedMemory(rex::memory::Memory& memory)
      : SharedMemory(memory), mirror(memory.bytes.size(), 0xCD) {
    InitializeCommon();
    streamed_page_shadows_supported_ = true;
  }
  std::vector<uint8_t> mirror;
  size_t upload_calls = 0;
  uint64_t uploaded_bytes = 0;
  void KeepGpuPagesForTrace() { PrepareForTraceDownload(); }

 protected:
  bool UploadRanges(const std::vector<std::pair<uint32_t, uint32_t>>& ranges) override {
    ++upload_calls;
    for (const auto& [first, count] : ranges) {
      uint32_t address = first << page_size_log2();
      uint32_t bytes = count << page_size_log2();
      Require(uint64_t(address) + bytes <= mirror.size(), "host memory fixture bound");
      MakeRangeValid(address, bytes, false);
      CopyPagesForUpload(first, count, mirror.data() + address);
      uploaded_bytes += bytes;
    }
    return true;
  }
};

void FunctionalSuite() {
  rex::memory::Memory memory;
  TestSharedMemory shared(memory);
  const uint32_t page = uint32_t(rex::memory::page_size());
  std::fill(memory.bytes.begin(), memory.bytes.end(), uint8_t(0x27));
  Require(shared.RequestRange(128, 64), "initial upload succeeds");
  Require(shared.upload_calls == 1 && shared.uploaded_bytes == page,
          "upload uses host page size");
  Require(shared.mirror[128] == 0x27, "initial GPU snapshot");
  Require(shared.RequestRange(128, 64) && shared.upload_calls == 1,
          "valid page fast path avoids duplicate upload");

  memory.bytes[128] = 0x81;
  shared.MemoryInvalidationCallback(128, 1, true);
  Require(shared.RequestRange(128, 64) && shared.upload_calls == 2,
          "guest invalidation requires fresh upload");
  Require(shared.mirror[128] == 0x81, "rewritten vertex bytes become visible");

  // At 16 KiB, these two addresses are different guest 4 KiB subpages of the
  // same tracked host page; at 4 KiB, they are independent tracked pages.
  Require(shared.RequestRange(4096 + 128, 64), "neighbor guest subpage upload");
  auto before = shared.upload_calls;
  shared.MemoryInvalidationCallback(128, 1, true);
  Require(shared.RequestRange(4096 + 128, 64), "neighbor subpage after invalidation");
  Require(shared.upload_calls == before + (page > 4096 ? 1 : 0),
          "host-page invalidation granularity is consistent");

  const uint32_t gpu_address = page * 4;
  Require(shared.RequestRange(gpu_address, 128), "GPU destination initialized");
  shared.RangeWrittenByGpu(gpu_address, 128);
  shared.mirror[gpu_address] = 0xA6;
  memory.bytes[gpu_address] = 0x39;
  shared.SetSystemPageBlocksValidWithGpuDataWritten();
  before = shared.upload_calls;
  Require(shared.RequestRange(gpu_address, 128) && shared.upload_calls == before,
          "frame-end keeps GPU-owned contents valid");
  Require(shared.mirror[gpu_address] == 0xA6, "GPU-owned bytes never replaced by CPU copy");
  shared.KeepGpuPagesForTrace();
  Require(shared.IsRangeGpuWritten(gpu_address, 1), "trace preparation keeps GPU ownership");
  shared.InvalidateAllPages();
  Require(shared.RequestRange(gpu_address, 128) && shared.upload_calls == before + 1,
          "full invalidation drops validity");
  Require(shared.mirror[gpu_address] == 0x39, "full invalidation permits fresh CPU bytes");

  size_t watch_count = 0;
  auto watch = shared.WatchMemoryRange(page * 10 + 8, page + 10,
      [](const auto&, void* context, void*, uint64_t argument, bool gpu) {
        Require(argument == 74207 && !gpu, "watch callback metadata");
        ++*static_cast<size_t*>(context);
      }, &watch_count, nullptr, 74207);
  Require(watch != nullptr, "watch installed");
  shared.MemoryInvalidationCallback(page * 11 + 2, 1, true);
  shared.MemoryInvalidationCallback(page * 11 + 2, 1, true);
  Require(watch_count == 1, "overlapping watch fires once and is removed");

  const std::pair<uint32_t, uint32_t> ranges[] = {
      {page * 20 + 32, page + 64}, {page * 20, 256}, {page * 22 - 20, 40}};
  before = shared.upload_calls;
  Require(shared.RequestRanges(ranges, 3), "unsorted overlapping ranges merge");
  Require(shared.upload_calls == before + 1, "merged ranges use one upload transaction");
  Require(shared.RequestRange(0, 0), "empty range succeeds");
  Require(!shared.RequestRange(rex::graphics::SharedMemory::kBufferSize - 1, 2), "end overflow rejected");
  Require(!shared.RequestRange(UINT32_MAX, 1), "invalid start rejected");
  std::printf("PASS functional page=%u validity, invalidation, GPU ownership, watches, range bounds\n",
              page);
}

void ConcurrentSuite() {
  rex::memory::Memory memory;
  TestSharedMemory shared(memory);
  Require(shared.RequestRange(128, 64), "concurrency initial upload");
  std::atomic<bool> ready{false};
  std::thread guest([&] {
    while (!ready.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
    for (uint32_t i = 0; i < 40000; ++i) {
      shared.MemoryInvalidationCallback(128, 1, true);
      if (!(i & 31)) std::this_thread::yield();
    }
  });
  ready.store(true, std::memory_order_release);
  for (uint32_t i = 0; i < 40000; ++i) {
    Require(shared.RequestRange(128, 64), "concurrent vertex request succeeds");
    if (!(i & 31)) std::this_thread::yield();
  }
  guest.join();
  // A synchronized final invalidation must always be seen on the next draw.
  memory.bytes[128] = 0xF4;
  shared.MemoryInvalidationCallback(128, 1, true);
  Require(shared.RequestRange(128, 64) && shared.mirror[128] == 0xF4,
          "final invalidation cannot leave stale vertex data");
  std::printf("PASS concurrency page=%zu 40000 guest invalidations, 40000 vertex requests\n",
              rex::memory::page_size());
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) return 2;
  rex::memory::test_page_size = std::strtoul(argv[1], nullptr, 10);
  if (rex::memory::test_page_size != 4096 && rex::memory::test_page_size != 16384) return 2;
  if (!std::strcmp(argv[2], "functional")) FunctionalSuite();
  else if (!std::strcmp(argv[2], "concurrent")) ConcurrentSuite();
  else return 2;
  return 0;
}
