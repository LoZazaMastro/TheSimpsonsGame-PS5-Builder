// Exercise the production callback/argument publication contract.
#include <rex/graphics/interrupt_callback.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

static void Check(bool value, const char* message) {
  if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::abort(); }
}

int main() {
  // A dispatch allowed between the old two stores sees mismatched words.
  struct Old { uint32_t callback = 0, data = 0; } old;
  old.callback = 0x82456830;
  Check(old.callback != 0 && old.data != 0xe3139380,
        "the old publication interleaving must expose a new callback with stale data");
  old.data = 0xe3139380;

  rex::graphics::InterruptCallbackState state;
  auto value = state.Read();
  Check(value.callback == 0 && value.user_data == 0, "initial callback disabled");
  state.Publish(0xffffffffU, 0x80000000U);
  value = state.Read();
  Check(value.callback == 0xffffffffU && value.user_data == 0x80000000U,
        "both full-width guest words retained");
  state.Publish(0, 0);

  std::atomic<bool> start{false};
  std::atomic<unsigned> writers{2};
  std::atomic<uint64_t> reads{0}, mismatches{0};
  std::vector<std::thread> workers;
  for (unsigned i = 0; i < 4; ++i) {
    workers.emplace_back([&] {
      while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
      uint64_t checked = 0, bad = 0;
      do {
        const auto snapshot = state.Read();
        if (snapshot.callback && snapshot.user_data != ~snapshot.callback) ++bad;
        ++checked;
      } while (writers.load(std::memory_order_acquire));
      reads.fetch_add(checked, std::memory_order_relaxed);
      mismatches.fetch_add(bad, std::memory_order_relaxed);
    });
  }
  for (unsigned writer = 0; writer < 2; ++writer) {
    workers.emplace_back([&, writer] {
      while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
      for (uint32_t i = 1; i <= 250000; ++i) {
        const uint32_t callback = (writer << 30) | i;
        state.Publish(callback, ~callback);
      }
      writers.fetch_sub(1, std::memory_order_release);
    });
  }
  start.store(true, std::memory_order_release);
  for (auto& worker : workers) worker.join();
  Check(mismatches.load() == 0, "dispatch must never observe words from different updates");
  Check(reads.load() >= 4, "all concurrent readers ran");
  state.Publish(0, 0x12345678);
  value = state.Read();
  Check(value.callback == 0 && value.user_data == 0x12345678, "disabled snapshot is coherent");
  std::printf("PASS: old interleaving exposed; 500000 publications, %llu concurrent reads, zero mixed pairs\n",
              static_cast<unsigned long long>(reads.load()));
}
