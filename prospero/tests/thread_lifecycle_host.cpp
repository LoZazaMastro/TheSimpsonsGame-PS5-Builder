// Compile the actual PS5 threading adapter with host pthreads. Kernel affinity,
// names and scheduling are explicit adapters, not claims of console execution.
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <semaphore.h>
#include <thread>
#include <vector>

#include <rex/thread.h>
#include <rex/logging.h>

namespace rex {
LogCategoryId RegisterLogCategory(const char*) { return LogCategoryId(0); }
spdlog::logger* GetLoggerRaw(LogCategoryId) { return spdlog::default_logger_raw(); }
}

extern "C" int __real_sem_init(sem_t*, int, unsigned);
extern "C" int __real_sem_wait(sem_t*);
extern "C" int __real_sem_post(sem_t*);
extern "C" int __real_sem_destroy(sem_t*);
extern "C" int __real_pthread_join(pthread_t, void**);
extern "C" int __real_pthread_kill(pthread_t, int);
extern "C" int __real_pthread_create(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);

static std::atomic<unsigned> wait_entries, wait_returns, posts, joins, destroys;
static std::atomic<unsigned> creates, signal_calls, schedule_calls, attribute_schedule_calls;
static std::atomic<bool> count_joins{true};
static std::atomic<bool> fail_semaphore, fail_create, fail_signal, interrupt_wait;

extern "C" int __wrap_sem_init(sem_t* sem, int shared, unsigned value) {
  if (fail_semaphore.exchange(false)) { errno = ENOSPC; return -1; }
  return __real_sem_init(sem, shared, value);
}
extern "C" int __wrap_sem_wait(sem_t* sem) {
  wait_entries.fetch_add(1);
  if (interrupt_wait.exchange(false)) { errno = EINTR; return -1; }
  const int result = __real_sem_wait(sem);
  wait_returns.fetch_add(1);
  return result;
}
extern "C" int __wrap_sem_post(sem_t* sem) {
  posts.fetch_add(1);
  return __real_sem_post(sem);
}
extern "C" int __wrap_sem_destroy(sem_t* sem) {
  destroys.fetch_add(1);
  return __real_sem_destroy(sem);
}
extern "C" int __wrap_pthread_join(pthread_t thread, void** value) {
  if (count_joins.load()) joins.fetch_add(1);
  return __real_pthread_join(thread, value);
}
extern "C" int __wrap_pthread_kill(pthread_t thread, int signal) {
  signal_calls.fetch_add(1);
  if (fail_signal.exchange(false)) return ESRCH;
  return __real_pthread_kill(thread, signal);
}
extern "C" int __wrap_pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                                       void* (*entry)(void*), void* value) {
  creates.fetch_add(1);
  if (fail_create.exchange(false)) return EAGAIN;
  return __real_pthread_create(thread, attr, entry, value);
}
// Model the range actually reported in the SDK's console probe. The test
// proves desktop values were passed / are now avoided, not native scheduling.
extern "C" int __wrap_pthread_getschedparam(pthread_t, int* policy, sched_param* param) {
  *policy = SCHED_FIFO;
  param->sched_priority = 700;
  return 0;
}
extern "C" int __wrap_pthread_setschedparam(pthread_t, int policy, const sched_param* param) {
  schedule_calls.fetch_add(1);
  return policy != SCHED_FIFO || param->sched_priority < 256 || param->sched_priority > 767
      ? EINVAL : 0;
}
extern "C" int __wrap_pthread_attr_setschedparam(pthread_attr_t*, const sched_param* param) {
  attribute_schedule_calls.fetch_add(1);
  return param->sched_priority < 256 || param->sched_priority > 767 ? EINVAL : 0;
}
extern "C" int scePthreadGetaffinity(pthread_t, uint64_t* mask) { *mask = 0x1fff; return 0; }
extern "C" int scePthreadSetaffinity(pthread_t, uint64_t) { return 0; }
extern "C" void ps5_pthread_exit(void* value) { pthread_exit(value); }

using namespace std::chrono_literals;
static bool Await(const std::function<bool()>& predicate, unsigned milliseconds = 2000) {
  const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
  while (!predicate()) {
    if (std::chrono::steady_clock::now() >= end) return false;
    std::this_thread::sleep_for(1ms);
  }
  return true;
}
static void Check(bool ok, const char* message) {
  if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::abort(); }
}
static void WaitThread(rex::thread::Thread* thread) {
  Check(rex::thread::Wait(thread, false, 2000ms) == rex::thread::WaitResult::kSuccess,
        "thread wait completes");
}
struct Worker {
  std::atomic<unsigned> iterations{0};
  std::atomic<bool> stop{false};
  std::unique_ptr<rex::thread::Thread> thread;
  explicit Worker(bool initially_suspended = false) {
    thread = rex::thread::Thread::Create(
        {.stack_size = 2 * 1024 * 1024, .create_suspended = initially_suspended}, [&] {
          while (!stop.load()) {
            iterations.fetch_add(1);
            std::this_thread::sleep_for(1ms);
          }
        });
    Check(bool(thread), "create worker");
    thread->set_name("lifecycle-test");
  }
  void Finish() { stop.store(true); WaitThread(thread.get()); }
  void CheckStopped(const char* message) {
    std::this_thread::sleep_for(10ms);
    const unsigned before = iterations.load();
    std::this_thread::sleep_for(20ms);
    Check(iterations.load() == before, message);
  }
};

static void InitialGateAndRepeatedWait(bool baseline) {
  Worker worker(true);
  Check(worker.iterations.load() == 0, "initial suspended gate");
  uint32_t previous = 0;
  Check(worker.thread->Resume(&previous) && previous == 1, "initial resume count");
  Check(Await([&] { return worker.iterations.load() > 8; }), "worker starts");
  std::printf("initial resume posts=%u waits=%u returns=%u\n",
              posts.load(), wait_entries.load(), wait_returns.load());
  const unsigned waits_before = wait_entries.load();
  const unsigned returns_before = wait_returns.load();
  Check(worker.thread->Suspend(&previous) && previous == 0, "first explicit suspend");
  Check(Await([&] { return wait_entries.load() > waits_before; }), "signal enters suspension");
  std::this_thread::sleep_for(10ms);
  const auto before = worker.iterations.load();
  std::this_thread::sleep_for(30ms);
  const auto after = worker.iterations.load();
  const bool escaped = after > before && wait_returns.load() > returns_before;
  std::printf("first suspend work_delta=%u wait_returns_delta=%u escaped=%d\n",
              after - before, wait_returns.load() - returns_before, escaped);
  Check(worker.thread->Resume(&previous) && previous == 1, "resume explicit suspension");
  worker.Finish();
  const unsigned joins_first = joins.load(), destroys_first = destroys.load();
  WaitThread(worker.thread.get());
  const bool repeats = joins.load() > joins_first || destroys.load() > destroys_first;
  std::printf("repeat wait joins=%u->%u semaphore_destroys=%u->%u repeated=%d\n",
              joins_first, joins.load(), destroys_first, destroys.load(), repeats);
  if (baseline) {
    Check(escaped && repeats, "both baseline lifecycle defects reproduced");
    std::puts("BASELINE DEFECTS REPRODUCED: initial wake token bypasses suspension; repeated native teardown");
  } else {
    Check(!escaped && !repeats, "suspension blocks and native teardown happens once");
    Check(!worker.thread->Suspend() && !worker.thread->Resume(), "finished thread cannot suspend or resume");
    const unsigned signals_before = signal_calls.load();
    worker.thread->QueueUserCallback([] {});
    Check(signal_calls.load() == signals_before, "finished thread does not receive an APC signal");
    Check(worker.thread->affinity_mask() == 0, "finished thread does not query native affinity");
    std::puts("PASS initial gate, explicit suspension, repeat wait and finished handle guards");
  }
}

static void NestedAndRapid() {
  Worker worker;
  Check(Await([&] { return worker.iterations.load() > 4; }), "worker runs");
  uint32_t previous = 0;
  const unsigned waits_before = wait_entries.load(), signals_before = signal_calls.load();
  interrupt_wait.store(true);
  Check(worker.thread->Suspend(&previous) && previous == 0, "first suspension");
  Check(Await([&] { return wait_entries.load() >= waits_before + 2; }), "EINTR retries suspension wait");
  worker.CheckStopped("first suspension holds after EINTR");
  Check(worker.thread->Suspend(&previous) && previous == 1, "nested suspension count");
  Check(signal_calls.load() == signals_before + 1, "nested suspension sends no second signal");
  Check(worker.thread->Resume(&previous) && previous == 2, "first nested resume");
  worker.CheckStopped("partial nested resume stays suspended");
  const unsigned before = worker.iterations.load();
  Check(worker.thread->Resume(&previous) && previous == 1, "final nested resume");
  Check(Await([&] { return worker.iterations.load() > before; }), "nested suspension released");
  for (unsigned i = 0; i < 128; ++i) {
    Check(worker.thread->Suspend(&previous) && previous == 0, "rapid suspension count");
    Check(worker.thread->Resume(&previous) && previous == 1, "rapid resume count");
  }
  const unsigned rapid_end = worker.iterations.load();
  Check(Await([&] { return worker.iterations.load() > rapid_end + 4; }), "rapid cycles do not strand worker");
  fail_signal.store(true);
  Check(!worker.thread->Suspend(), "failed signal does not report successful suspension");
  const unsigned failed_end = worker.iterations.load();
  Check(Await([&] { return worker.iterations.load() > failed_end + 4; }), "failed signal restores runnable state");
  Check(!worker.thread->Resume(), "failed signal leaves no suspend count");
  worker.Finish();
  std::puts("PASS nested suspension, EINTR, 128 rapid cycles and failed signal rollback");
}

static void ConcurrentWaits() {
  Worker worker;
  Check(Await([&] { return worker.iterations.load() > 4; }), "worker starts");
  std::atomic<unsigned> ready{0}, completed{0};
  std::vector<std::thread> waiters;
  for (unsigned i = 0; i < 8; ++i) {
    waiters.emplace_back([&] {
      ready.fetch_add(1);
      WaitThread(worker.thread.get());
      WaitThread(worker.thread.get());
      completed.fetch_add(1);
    });
  }
  Check(Await([&] { return ready.load() == 8; }), "all waiters ready");
  worker.stop.store(true);
  Check(Await([&] { return completed.load() == 8; }), "all repeated waiters finish");
  Check(joins.load() == 1 && destroys.load() == 1, "concurrent waits reap native resources once");
  count_joins.store(false);
  for (auto& waiter : waiters) waiter.join();
  count_joins.store(true);
  std::puts("PASS 8 simultaneous waiters / 16 waits with one native join and semaphore destroy");
}

static void Scheduling(bool baseline) {
  Worker worker;
  const int initial = worker.thread->priority();
  for (int priority : {0, 1, 8, 16, 24, 32}) worker.thread->set_priority(priority);
  const int updated = worker.thread->priority();
  worker.Finish();
  std::atomic<bool> ran{false};
  auto requested = rex::thread::Thread::Create(
      {.stack_size = 2 * 1024 * 1024, .initial_priority = 24}, [&] { ran.store(true); });
  std::printf("scheduler native_sets=%u attribute_sets=%u query_initial=%d query_final=%d requested_thread=%d\n",
              schedule_calls.load(), attribute_schedule_calls.load(), initial, updated, bool(requested));
  if (baseline) {
    Check(initial == 700 && updated == 700 && schedule_calls.load() == 6 &&
          attribute_schedule_calls.load() == 1 && !requested,
          "desktop priority numbers rejected by PS5 range model");
    std::puts("BASELINE DEFECT REPRODUCED: invalid desktop priorities and native number exposed to ReX");
  } else {
    Check(initial == rex::thread::ThreadPriority::kNormal && updated == 32 &&
          !schedule_calls.load() && !attribute_schedule_calls.load() && requested,
          "inherit native scheduler and preserve ReX priority values");
    Check(requested->priority() == 24, "initial logical priority retained");
    WaitThread(requested.get());
    Check(ran.load(), "requested-priority worker starts");
    std::puts("PASS inherited native scheduling and guest-visible logical priorities (host adapter only)");
  }
}

static void CreationFailures() {
  fail_semaphore.store(true);
  auto invalid_sem = rex::thread::Thread::Create({}, [] {});
  Check(!invalid_sem && creates.load() == 0 && destroys.load() == 0,
        "semaphore failure prevents native thread creation");
  auto owner = std::make_shared<int>(42);
  std::weak_ptr<int> weak = owner;
  fail_create.store(true);
  auto rejected = rex::thread::Thread::Create({}, [held = std::move(owner)] {});
  Check(!rejected && weak.expired() && creates.load() == 1 && destroys.load() == 1,
        "pthread creation failure frees capture and semaphore once");
  std::puts("PASS sem_init / pthread_create failures and capture ownership");
}

static void ExplicitExit() {
  std::atomic<bool> entered{false}, returned{false};
  auto thread = rex::thread::Thread::Create({.stack_size = 2 * 1024 * 1024}, [&] {
    entered.store(true);
    rex::thread::Thread::Exit(42);
    returned.store(true);
  });
  Check(bool(thread), "create explicit-exit worker");
  WaitThread(thread.get());
  WaitThread(thread.get());
  Check(entered.load() && !returned.load(), "Thread::Exit leaves through the native exit path");
  Check(joins.load() == 1 && destroys.load() == 1,
        "explicit exit and repeated wait consume native resources once");
  Check(!thread->Suspend() && !thread->Resume(), "explicitly exited handle rejects suspension");
  std::puts("PASS explicit Thread::Exit and repeated join with one semaphore destruction");
}

static void TerminateSuspended() {
  for (bool initial_gate : {true, false}) {
    const auto waits_before = wait_entries.load();
    Worker worker(initial_gate);
    if (!initial_gate) {
      Check(Await([&] { return worker.iterations.load() > 4; }), "cancel target starts");
      Check(worker.thread->Suspend(), "cancel target enters signal suspension");
    }
    Check(Await([&] { return wait_entries.load() > waits_before; }),
          "cancel target is blocked in suspension semaphore");
    const auto joins_before = joins.load(), destroys_before = destroys.load();
    worker.thread->Terminate(7);
    WaitThread(worker.thread.get());
    WaitThread(worker.thread.get());
    Check(joins.load() == joins_before + 1 && destroys.load() == destroys_before + 1,
          "cancelled suspended thread joins and destroys its semaphore once");
    Check(!worker.thread->Suspend() && !worker.thread->Resume(),
          "cancelled suspended thread rejects later native operations");
    if (initial_gate) Check(worker.iterations.load() == 0, "cancelled initial gate never runs user work");
  }
  std::puts("PASS external Terminate during initial gate and signal suspension, then repeated wait");
}

static void ReusedNativeHandle() {
  Worker finished;
  finished.Finish();
  void* old_handle = finished.thread->native_handle();
  std::atomic<bool> reused{false}, returned{false};
  for (unsigned attempt = 0; attempt != 32 && !reused.load(); ++attempt) {
    auto replacement = rex::thread::Thread::Create(
        {.stack_size = 2 * 1024 * 1024, .create_suspended = true}, [&] {
          if (reinterpret_cast<void*>(pthread_self()) != old_handle) return;
          reused.store(true);
          std::puts("OBSERVED actual pthread handle reuse while old ReX object remains alive");
          finished.thread->Terminate(99);
          returned.store(true);
        });
    Check(bool(replacement), "create replacement for actual handle reuse");
    Check(replacement->Resume(), "resume replacement thread");
    WaitThread(replacement.get());
  }
  Check(reused.load() && returned.load(),
        "reused native identity cannot enter the finished object's self-termination branch");
  std::puts("PASS actual pthread identity reuse keeps logical thread objects distinct");
}

int main(int argc, char** argv) {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  Check(argc >= 2, "test case supplied");
  const bool baseline = argc > 2 && std::strcmp(argv[2], "baseline") == 0;
  if (std::strcmp(argv[1], "lifecycle") == 0) InitialGateAndRepeatedWait(baseline);
  else if (std::strcmp(argv[1], "nested") == 0) NestedAndRapid();
  else if (std::strcmp(argv[1], "concurrent") == 0) ConcurrentWaits();
  else if (std::strcmp(argv[1], "scheduling") == 0) Scheduling(baseline);
  else if (std::strcmp(argv[1], "failures") == 0) CreationFailures();
  else if (std::strcmp(argv[1], "exit") == 0) ExplicitExit();
  else if (std::strcmp(argv[1], "terminate-suspended") == 0) TerminateSuspended();
  else if (std::strcmp(argv[1], "reused-identity") == 0) ReusedNativeHandle();
  else Check(false, "unknown case");
  return 0;
}
