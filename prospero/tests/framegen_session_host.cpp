// SPDX-License-Identifier: GPL-3.0-or-later
// Session control runs independently of Vulkan and preserves saved startup CVars.
#include <rex/platform/framegen_prospero.h>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

extern bool fg_test_frame_gen;
extern int32_t fg_test_frame_gen_target_rate, fg_test_frame_gen_multiplier;
namespace fp = rex::platform::prospero;
namespace framegen_test {
std::mutex mutex;
std::vector<std::string> logs;
void CaptureLog(std::string message) {
  std::lock_guard lock(mutex);
  logs.push_back(std::move(message));
}
}
static void Require(bool condition, const char* text) {
  if (!condition) throw std::runtime_error(text);
}

int main() {
  try {
    fg_test_frame_gen = false;
    fg_test_frame_gen_target_rate = 120;
    fg_test_frame_gen_multiplier = 4;
    fp::InitializeFrameGenerationSession();
    const auto initial = fp::ReadFrameGenerationSessionRequest();
    Require(!initial.enabled && initial.revision == 1, "saved OFF initializes the session");
    Require(fp::ToggleFrameGenerationForSession(), "first chord requests ON");
    Require(!fp::ToggleFrameGenerationForSession(), "second chord requests OFF");
    const auto pair = fp::ReadFrameGenerationSessionRequest();
    Require(!pair.enabled && pair.revision == initial.revision + 2,
            "OFF/ON history is observable even when the final state matches");
    Require(!fg_test_frame_gen && fg_test_frame_gen_target_rate == 120 &&
            fg_test_frame_gen_multiplier == 4, "shortcut must preserve all saved CVars");
    std::cout << "PASS saved OFF, ON/OFF pair, revision and untouched startup options\n";

    fg_test_frame_gen = true;
    fp::InitializeFrameGenerationSession();
    Require(fp::IsFrameGenerationRequestedForSession(), "saved ON initializes the session");
    Require(!fp::ToggleFrameGenerationForSession() && fg_test_frame_gen,
            "session OFF does not persist over saved ON");
    fp::InitializeFrameGenerationSession();
    Require(fp::IsFrameGenerationRequestedForSession(), "a new session restores saved ON");
    std::cout << "PASS saved ON, temporary OFF and startup reset\n";

    fg_test_frame_gen = false;
    fp::InitializeFrameGenerationSession();
    constexpr uint64_t kThreads = 8, kToggles = 251;
    std::atomic<unsigned> running{kThreads};
    std::atomic<bool> coherent{true};
    std::vector<std::thread> workers;
    for (uint64_t i = 0; i < kThreads; ++i) {
      workers.emplace_back([&] {
        for (uint64_t n = 0; n < kToggles; ++n) {
          fp::ToggleFrameGenerationForSession();
          const auto snapshot = fp::ReadFrameGenerationSessionRequest();
          if (snapshot.enabled != bool((snapshot.revision - 1) & 1)) coherent = false;
        }
        --running;
      });
    }
    while (running.load()) {
      const auto snapshot = fp::ReadFrameGenerationSessionRequest();
      if (snapshot.enabled != bool((snapshot.revision - 1) & 1)) coherent = false;
      std::this_thread::yield();
    }
    for (auto& worker : workers) worker.join();
    const auto final = fp::ReadFrameGenerationSessionRequest();
    Require(coherent && final.revision == 1 + kThreads * kToggles && !final.enabled,
            "concurrent controller polls must not lose toggles or tear the state/revision");
    Require(!fg_test_frame_gen, "concurrent shortcuts must not change saved OFF");
    std::cout << "PASS concurrent request publication with coherent revisions and no lost toggle\n";

    const auto menu_on = fp::SetFrameGenerationForSession(true);
    Require(menu_on.enabled && menu_on.revision == final.revision + 1,
            "explicit menu ON uses the same session sequence as the shortcut");
    const auto repeat_on = fp::SetFrameGenerationForSession(true);
    Require(repeat_on.enabled && repeat_on.revision == menu_on.revision,
            "selecting an already active value must not reset framegen history");
    Require(!fp::ToggleFrameGenerationForSession(), "shortcut can disable a menu request");
    const auto menu_restore = fp::SetFrameGenerationForSession(true);
    Require(menu_restore.enabled && menu_restore.revision == menu_on.revision + 2,
            "menu restores ON after shortcut OFF even when saved startup choice was already ON");
    const auto menu_off = fp::SetFrameGenerationForSession(false);
    Require(!menu_off.enabled && menu_off.revision == menu_restore.revision + 1,
            "explicit menu OFF is immediate and versioned");
    Require(!fg_test_frame_gen && fg_test_frame_gen_target_rate == 120 &&
            fg_test_frame_gen_multiplier == 4, "menu request must not write shared startup CVars");
    std::cout << "PASS explicit menu state, idempotence, shortcut interoperation and immutable startup CVars\n";

    // Concurrent setters of the same state must publish one transition, rather
    // than several ON/OFF toggles. This also catches an implementation that
    // composes a racy read with the existing toggle API.
    std::atomic<unsigned> setters_ready{0};
    std::atomic<bool> release_setters{false};
    workers.clear();
    for (uint64_t i = 0; i < kThreads; ++i) {
      workers.emplace_back([&] {
        ++setters_ready;
        while (!release_setters.load(std::memory_order_acquire)) std::this_thread::yield();
        fp::SetFrameGenerationForSession(true);
      });
    }
    while (setters_ready.load() != kThreads) std::this_thread::yield();
    release_setters.store(true, std::memory_order_release);
    for (auto& worker : workers) worker.join();
    const auto set_final = fp::ReadFrameGenerationSessionRequest();
    Require(set_final.enabled && set_final.revision == menu_off.revision + 1,
            "concurrent explicit ON requests must converge without extra transitions");
    std::cout << "PASS concurrent explicit setters publish one state transition\n";
    Require(framegen_test::logs.front().find("chord=L1+R1+L3+R3") != std::string::npos,
            "diagnostics identify the shortcut source");
    Require(framegen_test::logs.front().find("startup configuration unchanged") != std::string::npos,
            "diagnostics distinguish session and saved settings");
    std::cout << "PASS diagnostics identify requested state, revision and session-only scope\n";
  } catch (const std::exception& error) {
    std::cerr << "FAIL " << error.what() << '\n';
    return 1;
  }
}
