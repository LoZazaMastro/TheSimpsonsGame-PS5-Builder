#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>

#include <rex/ui/windowed_app_context.h>

namespace rex::ui {

// The PS5 title has one fullscreen surface and no desktop window manager.
// Deferred UI work still runs exclusively on the thread that created this
// context, with a bounded wake interval for controller navigation.
class ProsperoWindowedAppContext final : public WindowedAppContext {
 public:
  ~ProsperoWindowedAppContext() override;

  void RunMainLoop();
  // Each window is pumped at most once per event-loop iteration, outside the
  // deferred-function drain. The argument requests a controller poll.
  uint64_t AddWindowPump(std::function<void(bool)> pump);
  void RemoveWindowPump(uint64_t token);
  void WakeUILoop() { NotifyUILoopOfPendingFunctions(); }

 protected:
  void NotifyUILoopOfPendingFunctions() override;
  void PlatformQuitFromUIThread() override;

 private:
  std::mutex wake_mutex_;
  std::condition_variable wake_condition_;
  uint64_t wake_generation_ = 0;
  uint64_t next_pump_token_ = 1;
  std::map<uint64_t, std::function<void(bool)>> window_pumps_;
};

}  // namespace rex::ui
