// Exercise the actual timer queue and its PS5 lifetime branch with host
// std::thread/condition_variable. Only registry storage and thread naming are
// supplied here; no timer scheduling, waiting or callback code is mocked.
#include <rex/cvar.h>
#include <rex/thread.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

namespace rex::cvar {
std::vector<FlagEntry>& GetRegistry() {
  static std::vector<FlagEntry> entries;
  return entries;
}
std::optional<size_t> RegisterFlag(FlagEntry entry) {
  auto& entries = GetRegistry();
  entries.push_back(std::move(entry));
  return entries.size() - 1;
}
void UnregisterFlag(std::string_view name) {
  auto& entries = GetRegistry();
  std::erase_if(entries, [&](const auto& entry) { return entry.name == name; });
}
}  // namespace rex::cvar

namespace rex::thread {
void set_current_thread_name(std::string_view) {}
}  // namespace rex::thread

#ifndef TIMER_QUEUE_SOURCE
#error The runner must supply the complete production timer_queue.cpp
#endif
#include TIMER_QUEUE_SOURCE

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition);           \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

using namespace std::chrono_literals;
using TimerClock = rex::thread::TimerQueueWaitItem::clock;

namespace {
struct Signal {
  std::mutex mutex;
  std::condition_variable condition;
  int count = 0;
  TimerClock::time_point observed{};
  void Fire() {
    std::lock_guard lock(mutex);
    observed = TimerClock::now();
    ++count;
    condition.notify_all();
  }
  bool Wait(int minimum, std::chrono::milliseconds timeout = 3000ms) {
    std::unique_lock lock(mutex);
    return condition.wait_for(lock, timeout, [&] { return count >= minimum; });
  }
};

void Disarm(std::weak_ptr<rex::thread::TimerQueueWaitItem> item) {
  if (auto owned = item.lock()) owned->Disarm();
}
}  // namespace

int main(int argc, char** argv) {
  CHECK(argc == 3);
  const bool expected_default = std::strcmp(argv[2], "true") == 0;
  auto& registry = rex::cvar::GetRegistry();
  CHECK(registry.size() == 1);
  CHECK(registry[0].name == "timer_queue_sleep");
  CHECK(registry[0].getter() == (expected_default ? "true" : "false"));
  CHECK(registry[0].default_value == (expected_default ? "true" : "false"));
  CHECK(registry[0].setter(argv[1]));
  std::printf("Mode: %s; production default: %s\n", argv[1], argv[2]);

  // A newly published earlier deadline must wake a consumer waiting for a
  // distant timer. Disarm the distant timer while its userdata is still live.
  Signal late, earlier;
  auto far = rex::thread::QueueTimerOnce([](void* p) { static_cast<Signal*>(p)->Fire(); },
                                        &late, TimerClock::now() + 2s);
  std::this_thread::sleep_for(10ms);
  auto due = TimerClock::now() + 10ms;
  auto near = rex::thread::QueueTimerOnce([](void* p) { static_cast<Signal*>(p)->Fire(); },
                                         &earlier, due);
  CHECK(earlier.Wait(1, 1000ms));
  CHECK(earlier.observed >= due);
  Disarm(near);
  Disarm(far);
  CHECK(late.count == 0);
  std::puts("PASS deadline ordering and wake on earlier publication");

  // Concurrent producers exceed the ring capacity. Every one-shot callback
  // must arrive exactly once and no producer may be stranded behind a full ring.
  Signal many;
  std::vector<std::thread> producers;
  constexpr int kProducers = 4, kPerProducer = 700;
  for (int producer = 0; producer != kProducers; ++producer) {
    producers.emplace_back([&] {
      for (int i = 0; i != kPerProducer; ++i) {
        rex::thread::QueueTimerOnce([](void* p) { static_cast<Signal*>(p)->Fire(); },
                                    &many, TimerClock::now() + 1ms);
      }
    });
  }
  for (auto& producer : producers) producer.join();
  CHECK(many.Wait(kProducers * kPerProducer));
  CHECK(many.count == kProducers * kPerProducer);
  std::puts("PASS 2800 one-shot callbacks from four producers across the 512-slot ring");

  // Disarm waits for an in-flight callback, then prevents further recurrence.
  Signal entered, finished;
  std::atomic<bool> release{false}, disarmed{false};
  auto recurring = rex::thread::QueueTimerRecurring([&](void*) {
    entered.Fire();
    while (!release.load(std::memory_order_acquire)) std::this_thread::yield();
    finished.Fire();
  }, nullptr, TimerClock::now() + 5ms, 5ms);
  CHECK(entered.Wait(1));
  std::thread cancel([&] {
    Disarm(recurring);
    disarmed.store(true, std::memory_order_release);
  });
  std::this_thread::sleep_for(10ms);
  CHECK(!disarmed.load(std::memory_order_acquire));
  release.store(true, std::memory_order_release);
  cancel.join();
  CHECK(finished.Wait(1));
  const int final_count = entered.count;
  std::this_thread::sleep_for(20ms);
  CHECK(entered.count == final_count);
  std::puts("PASS cancellation waits for an active callback and prevents later calls");

  // Self-cancellation must not block the dispatcher waiting for itself.
  Signal self_fired;
  std::mutex self_mutex;
  std::weak_ptr<rex::thread::TimerQueueWaitItem> self_item;
  {
    std::lock_guard lock(self_mutex);
    self_item = rex::thread::QueueTimerRecurring([&](void*) {
      std::lock_guard callback_lock(self_mutex);
      Disarm(self_item);
      self_fired.Fire();
    }, nullptr, TimerClock::now() + 10ms, 1ms);
  }
  CHECK(self_fired.Wait(1));
  Disarm(self_item);
  std::this_thread::sleep_for(10ms);
  CHECK(self_fired.count == 1);
  std::puts("PASS self-disarm inside a recurring callback");

  // The actual global PS5 queue destructor must join its worker on return;
  // the runner applies a process timeout to catch an exit-time deadlock.
  std::puts("PASS timer contracts; returning through actual PS5 queue teardown");
  return 0;
}
