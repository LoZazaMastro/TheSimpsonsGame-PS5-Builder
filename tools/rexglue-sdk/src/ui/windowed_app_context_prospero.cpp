#include <rex/ui/windowed_app_context_prospero.h>
#include <rex/platform/metrics_prospero.h>

#include <chrono>
#include <utility>
#include <vector>

namespace rex::ui {

ProsperoWindowedAppContext::~ProsperoWindowedAppContext() {
  // Drain accepted functions while the derived context is still alive.
  QuitFromUIThread();
}

void ProsperoWindowedAppContext::NotifyUILoopOfPendingFunctions() {
  {
    std::lock_guard lock(wake_mutex_);
    ++wake_generation_;
  }
  wake_condition_.notify_one();
}

void ProsperoWindowedAppContext::PlatformQuitFromUIThread() {
  NotifyUILoopOfPendingFunctions();
}

uint64_t ProsperoWindowedAppContext::AddWindowPump(std::function<void(bool)> pump) {
  assert_true(IsInUIThread());
  const uint64_t token = next_pump_token_++;
  window_pumps_.emplace(token, std::move(pump));
  return token;
}

void ProsperoWindowedAppContext::RemoveWindowPump(uint64_t token) {
  assert_true(IsInUIThread());
  window_pumps_.erase(token);
}

void ProsperoWindowedAppContext::RunMainLoop() {
  assert_true(IsInUIThread());
  using clock = std::chrono::steady_clock;
  auto next_input_poll = clock::now();
  while (!HasQuitFromUIThread()) {
    prospero_metrics_ui_tick();
    uint64_t observed_generation;
    {
      std::lock_guard lock(wake_mutex_);
      observed_generation = wake_generation_;
    }
    ExecutePendingFunctionsFromUIThread();
    if (HasQuitFromUIThread()) {
      break;
    }
    const auto now = clock::now();
    const bool poll_input = now >= next_input_poll;
    if (poll_input) {
      next_input_poll = now + std::chrono::milliseconds(8);
    }
    // Callbacks may close a window and remove themselves. Snapshot their
    // identities, then look them up again before each invocation. Painting is
    // deliberately outside ExecutePendingFunctionsFromUIThread: a continuous
    // ImGui redraw must not append another paint to a queue being drained.
    std::vector<uint64_t> tokens;
    tokens.reserve(window_pumps_.size());
    for (const auto& [token, pump] : window_pumps_) {
      tokens.push_back(token);
    }
    for (uint64_t token : tokens) {
      const auto it = window_pumps_.find(token);
      if (it != window_pumps_.end()) {
        auto pump = it->second;
        pump(poll_input);
      }
      if (HasQuitFromUIThread()) {
        break;
      }
    }
    if (HasQuitFromUIThread()) {
      break;
    }
    std::unique_lock lock(wake_mutex_);
    wake_condition_.wait_until(lock, next_input_poll, [&] {
      return wake_generation_ != observed_generation;
    });
  }
}

}  // namespace rex::ui
