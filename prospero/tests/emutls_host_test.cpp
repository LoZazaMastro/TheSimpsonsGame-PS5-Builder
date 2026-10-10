// Exercise the production LLVM/PS5 emutls adaptation and the current SDK's
// real C++ destructor registry with host pthreads. No console APIs are mocked.
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

extern "C" int ps5___cxa_thread_atexit_impl(void (*)(void*), void*, void*);
extern "C" void ps5p_run_thread_destructors(void);

// The compiler's 64-bit emutls control ABI; the two real thread_local variables
// below also make Clang generate controls and calls to the same implementation.
struct Control {
  uintptr_t size;
  uintptr_t alignment;
  uintptr_t index;
  void* initial;
};
static_assert(sizeof(Control) == 32);
extern "C" void* __emutls_get_address(Control*);

struct Payload {
  uint64_t value;
  std::array<uint64_t, 7> padding;
};
alignas(64) thread_local Payload payload{0x123456789abcdef0, {}};
alignas(4096) thread_local std::array<unsigned char, 4096> zero_page{};

constexpr unsigned kThreads = 8;
constexpr unsigned kExtraControls = 256;
std::array<Control, kExtraControls> controls;
std::atomic<unsigned> finished_destructors{0};
std::atomic<unsigned> ready{0};

void Noop(void*) {}

void NestedDestructor(void* object) {
  // The destructor registry must keep the same emutls allocation alive even
  // while destructors register more work or grow the thread's address array.
  assert(object == &payload);
  assert(payload.value >= 0x100 && payload.value < 0x100 + kThreads);
  assert(zero_page[0] == static_cast<unsigned char>(payload.value));
  finished_destructors.fetch_add(1, std::memory_order_relaxed);
}

void Destructor(void* object) {
  assert(object == &payload);
  const auto value = payload.value;
  for (unsigned i = 0; i < kExtraControls; ++i) {
    auto* slot = static_cast<uint64_t*>(__emutls_get_address(&controls[i]));
    assert(reinterpret_cast<uintptr_t>(slot) % controls[i].alignment == 0);
    assert(*slot == 0);
    *slot = value + i;
  }
  assert(&payload == object && payload.value == value);
  assert(ps5___cxa_thread_atexit_impl(NestedDestructor, object, nullptr) == 0);
  finished_destructors.fetch_add(1, std::memory_order_relaxed);
}

int main(int argc, char** argv) {
  assert(argc == 2);
  const bool registry_first = std::strcmp(argv[1], "registry-first") == 0;
  const bool explicit_drain = std::strcmp(argv[1], "explicit-drain") == 0;
  for (unsigned i = 0; i < kExtraControls; ++i) {
    controls[i] = {sizeof(uint64_t), uintptr_t(1) << (3 + i % 7), 0, nullptr};
  }
  if (registry_first) {
    // Initialize the SDK's destructor key before emutls creates its own key.
    assert(ps5___cxa_thread_atexit_impl(Noop, nullptr, nullptr) == 0);
  }
  std::vector<std::thread> workers;
  for (unsigned index = 0; index < kThreads; ++index) {
    workers.emplace_back([index, explicit_drain] {
      assert(reinterpret_cast<uintptr_t>(&payload) % 64 == 0);
      assert(reinterpret_cast<uintptr_t>(zero_page.data()) % 4096 == 0);
      assert(payload.value == 0x123456789abcdef0);
      for (const auto byte : zero_page) assert(byte == 0);
      payload.value = 0x100 + index;
      zero_page[0] = static_cast<unsigned char>(payload.value);
      assert(ps5___cxa_thread_atexit_impl(Destructor, &payload, nullptr) == 0);
      ready.fetch_add(1, std::memory_order_release);
      while (ready.load(std::memory_order_acquire) < kThreads) std::this_thread::yield();
      assert(payload.value == 0x100 + index);
      if (explicit_drain) {
        // The SDK's pthread wrappers perform this before native thread exit.
        ps5p_run_thread_destructors();
        assert(payload.value == 0x100 + index);
      }
    });
  }
  for (auto& worker : workers) worker.join();
  assert(finished_destructors.load() == kThreads * 2);
  if (registry_first) ps5p_run_thread_destructors();
  std::cout << "PASS " << argv[1]
            << ": 8 concurrent threads, initialized/zero/aligned TLS, "
               "256 controls grown during cleanup, nested SDK destructors\n";
}
