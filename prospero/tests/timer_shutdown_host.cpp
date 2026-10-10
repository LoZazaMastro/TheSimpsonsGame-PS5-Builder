// Scheduler gates expose the real publication/stop race in the complete
// production TimerQueue. No ring, wait-strategy or callback logic is replaced.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace timer_shutdown_test {
struct Hooks {
  std::atomic<void*> queue{nullptr};
  std::mutex mutex;
  std::condition_variable condition;
  bool shutdown = false;
  bool pause_publication = false;
  bool publication_paused = false;
  bool release_publication = false;
  bool nonempty_wait = false;
  bool post_wake_wait = false;
  bool exited = false;
  bool claim_entered = false;
  bool stop_before_claim = false;
} hooks;

void BeforeWait(void* queue, uint64_t sequence, bool empty) {
  if (hooks.queue.load() != queue) return;
  std::lock_guard lock(hooks.mutex);
  hooks.nonempty_wait |= !empty;
  // Sequence 0 is the distant timer; sequence 1 is the destructor wakeup.
  hooks.post_wake_wait |= hooks.shutdown && !empty && sequence >= 2;
  hooks.condition.notify_all();
}
void BeforeClaim(void* queue, bool stopped) {
  if (hooks.queue.load() != queue) return;
  std::lock_guard lock(hooks.mutex);
  if (hooks.shutdown) {
    hooks.claim_entered = true;
    hooks.stop_before_claim |= stopped;
    hooks.condition.notify_all();
  }
}
void AfterShutdownPublish(void* queue) {
  if (hooks.queue.load() != queue) return;
  std::unique_lock lock(hooks.mutex);
  if (!hooks.pause_publication) return;
  hooks.publication_paused = true;
  hooks.condition.notify_all();
  hooks.condition.wait(lock, [] { return hooks.release_publication; });
}
void Exited(void* queue) {
  if (hooks.queue.load() != queue) return;
  std::lock_guard lock(hooks.mutex);
  hooks.exited = true;
  hooks.condition.notify_all();
}
}  // namespace timer_shutdown_test

// Reuse the exact host registry/thread-name adapters already used by the
// timer contract suite. Its own main is not executed by this harness.
#define main TimerQueueContractMain
#include "timer_queue_host.cpp"
#undef main

namespace {
template <typename F> bool AwaitHook(F predicate, std::chrono::milliseconds timeout = 1500ms) {
  std::unique_lock lock(timer_shutdown_test::hooks.mutex);
  return timer_shutdown_test::hooks.condition.wait_for(lock, timeout, predicate);
}

void Watch(rex::thread::TimerQueue* queue, bool pause) {
  auto& h = timer_shutdown_test::hooks;
  std::lock_guard lock(h.mutex);
  h.pause_publication = pause;
  h.queue.store(queue);
}

bool DistantTimerShutdown() {
  auto queue = std::make_unique<rex::thread::TimerQueue>();
  Watch(queue.get(), true);
  auto& h = timer_shutdown_test::hooks;
  std::atomic<unsigned> calls{0};
  auto future = std::make_shared<rex::thread::TimerQueueWaitItem>(
      [&](void*) { ++calls; }, nullptr, queue.get(), TimerClock::now() + 3s, TimerClock::duration::zero());
  future->Disarm();
  queue->QueueTimer(std::move(future));
  CHECK(AwaitHook([&] { return h.nonempty_wait; }));
  {
    std::lock_guard lock(h.mutex);
    h.shutdown = true;
  }
  std::atomic<bool> finished{false};
  std::thread destroyer([owned = queue.release(), &finished] {
    delete owned;
    finished.store(true);
    timer_shutdown_test::hooks.condition.notify_all();
  });
  CHECK(AwaitHook([&] { return h.publication_paused; }));
  // Hold the producer after publication until the worker either re-enters
  // the long wait (baseline) or observes stop and exits (fixed).
  CHECK(AwaitHook([&] { return h.post_wake_wait || h.exited; }));
  bool waited_after_wake;
  {
    std::lock_guard lock(h.mutex);
    waited_after_wake = h.post_wake_wait;
    h.release_publication = true;
    h.condition.notify_all();
  }
  const auto released = TimerClock::now();
  const bool prompt = AwaitHook([&] { return finished.load(); }, 300ms);
  // A finite distant deadline lets even the baseline join safely. The test
  // diagnoses the missed wake without killing or detaching a live thread.
  destroyer.join();
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(TimerClock::now() - released);
  h.queue.store(nullptr);
  CHECK(calls.load() == 0);
  std::printf("shutdown after published wake: post_wake_wait=%d prompt=%d elapsed_host_ms=%lld\n",
              waited_after_wake, prompt, static_cast<long long>(elapsed.count()));
  return prompt && !waited_after_wake;
}

bool FullRingShutdown() {
  auto queue = std::make_unique<rex::thread::TimerQueue>();
  Watch(queue.get(), false);
  auto& h = timer_shutdown_test::hooks;
  Signal entered;
  std::atomic<bool> release_callback{false};
  auto callback = std::make_shared<rex::thread::TimerQueueWaitItem>(
      [&](void*) {
        entered.Fire();
        while (!release_callback.load()) std::this_thread::yield();
      }, nullptr, queue.get(), TimerClock::now(), TimerClock::duration::zero());
  queue->QueueTimer(std::move(callback));
  CHECK(entered.Wait(1));
  // The dispatcher has published consumption of the first slot and is held
  // inside its callback. Fill every one of the remaining 512 ring slots.
  for (unsigned i = 0; i < 512; ++i) {
    auto pending = std::make_shared<rex::thread::TimerQueueWaitItem>(
        [](void*) {}, nullptr, queue.get(), TimerClock::now() + 10s, TimerClock::duration::zero());
    pending->Disarm();
    queue->QueueTimer(std::move(pending));
  }
  {
    std::lock_guard lock(h.mutex);
    h.shutdown = true;
  }
  std::atomic<bool> finished{false};
  std::thread destroyer([owned = queue.release(), &finished] {
    delete owned;
    finished.store(true);
    timer_shutdown_test::hooks.condition.notify_all();
  });
  CHECK(AwaitHook([&] { return h.claim_entered; }));
  CHECK(!h.stop_before_claim);
  CHECK(!finished.load());
  release_callback.store(true);
  CHECK(AwaitHook([&] { return finished.load(); }));
  destroyer.join();
  h.queue.store(nullptr);
  std::puts("PASS full 512-slot ring: dispatcher remains live until shutdown slot is claimed");
  return true;
}
}  // namespace

int main(int argc, char** argv) {
  CHECK(argc == 3);
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  CHECK(rex::cvar::GetRegistry()[0].setter(argv[1]));
  const bool pass = std::strcmp(argv[2], "distant") == 0 ? DistantTimerShutdown() : FullRingShutdown();
  std::printf("%s %s/%s\n", pass ? "PASS" : "FAIL", argv[1], argv[2]);
  return pass ? 0 : 1;
}
