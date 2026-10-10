/**
 * @file        core/threading_posix.cpp
 * @brief       POSIX platform threading implementations
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 * @license     BSD 3-Clause License
 */

#include <rex/platform.h>
#include <rex/thread.h>

static_assert(REX_PLATFORM_LINUX || REX_PLATFORM_MAC || REX_PLATFORM_PROSPERO, "This file is POSIX-only");

#include <signal.h>

#include <atomic>
#include <array>
#include <cerrno>
#include <cstddef>
#include <ctime>
#include <deque>
#include <limits>
#include <memory>

#include <pthread.h>
#include <semaphore.h>
#if !REX_PLATFORM_PROSPERO
#include <sys/eventfd.h>
#endif
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <rex/assert.h>
#include <rex/chrono/chrono_steady_cast.h>
#include <rex/logging.h>
#include <rex/thread/timer_queue.h>

#include <sched.h>
#if REX_PLATFORM_PROSPERO
#include <pthread_np.h>
#include <ps5platform/kernel.h>
#include <ps5platform/libc.h>
#include <rex/platform/signal_thread_prospero.h>
#include <rex/platform/trace_phases_prospero.h>
extern "C" void ps5p_run_thread_destructors(void);
static int pthread_getname_np(pthread_t t, char* b, size_t n) { pthread_get_name_np(t,b,n); return 0; }
static int pthread_setname_np(pthread_t t, const char* n) { pthread_set_name_np(t,n); return 0; }
#endif

#if REX_PLATFORM_ANDROID
#include <dlfcn.h>

#include <rex/main_android.h>
#include <rex/string/util.h>
#endif

#if REX_PLATFORM_LINUX
// SIGEV_THREAD_ID in timer_create(...) is a Linux extension
#define REX_HAS_SIGEV_THREAD_ID 1
#ifdef __GLIBC__
#define sigev_notify_thread_id _sigev_un._tid
#endif
#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#define gettid() syscall(SYS_gettid)
#endif
#else
#define REX_HAS_SIGEV_THREAD_ID 0
#endif

namespace rex::thread {

#if REX_PLATFORM_ANDROID
// May be null if no dynamically loaded functions are required.
static void* android_libc_;
// API 26+.
static int (*android_pthread_getname_np_)(pthread_t pthread, char* buf, size_t n);

void AndroidInitialize() {
  if (rex::GetAndroidApiLevel() >= 26) {
    android_libc_ = dlopen("libc.so", RTLD_NOW);
    assert_not_null(android_libc_);
    if (android_libc_) {
      android_pthread_getname_np_ = reinterpret_cast<decltype(android_pthread_getname_np_)>(
          dlsym(android_libc_, "pthread_getname_np"));
      assert_not_null(android_pthread_getname_np_);
    }
  }
}

void AndroidShutdown() {
  android_pthread_getname_np_ = nullptr;
  if (android_libc_) {
    dlclose(android_libc_);
    android_libc_ = nullptr;
  }
}
#endif

template <typename _Rep, typename _Period>
inline timespec DurationToTimeSpec(std::chrono::duration<_Rep, _Period> duration) {
  auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
  auto div = ldiv(nanoseconds.count(), 1000000000L);
  return timespec{div.quot, div.rem};
}

// Thread interruption is done using user-defined signals
// This implementation uses the SIGRTMAX - SIGRTMIN to signal to a thread
// gdb tip, for SIG = SIGRTMIN + SignalType : handle SIG nostop
// lldb tip, for SIG = SIGRTMIN + SignalType : process handle SIG -s false
enum class SignalType {
  kThreadSuspend,
  kThreadUserCallback,
#if REX_PLATFORM_ANDROID
  // pthread_cancel is not available on Android, using a signal handler for
  // simplified PTHREAD_CANCEL_ASYNCHRONOUS-like behavior - not disabling
  // cancellation currently, so should be enough.
  kThreadTerminate,
#endif
  k_Count
};

int GetSystemSignal(SignalType num) {
  auto result = SIGRTMIN + static_cast<int>(num);
  assert_true(result < SIGRTMAX);
  return result;
}

SignalType GetSystemSignalType(int num) {
  return static_cast<SignalType>(num - SIGRTMIN);
}

std::array<std::atomic<bool>, static_cast<size_t>(SignalType::k_Count)> signal_handler_installed =
    {};
std::mutex signal_handler_install_mutex;

static void signal_handler(int signal, siginfo_t* info, void* context);

bool install_signal_handler(SignalType type) {
  auto& installed = signal_handler_installed[static_cast<size_t>(type)];
  if (installed.load(std::memory_order_acquire)) return true;
  // A concurrent creator must wait for sigaction to finish. Publishing a
  // claimed flag before that syscall let it start a thread whose Suspend
  // signal still had the process-terminating default disposition.
  std::lock_guard lock(signal_handler_install_mutex);
  if (installed.load(std::memory_order_relaxed)) return true;
  struct sigaction action{};
  action.sa_flags = SA_SIGINFO | SA_RESTART;
  action.sa_sigaction = signal_handler;
  sigemptyset(&action.sa_mask);
  if (sigaction(GetSystemSignal(type), &action, nullptr) != 0) {
    return false;
  }
  installed.store(true, std::memory_order_release);
  return true;
}

// TODO(dougvj)
void EnableAffinityConfiguration() {}

// uint64_t ticks() { return mach_absolute_time(); }

uint32_t current_thread_system_id() {
#if REX_PLATFORM_PROSPERO
  // libkernel exports this native TID accessor, but not FreeBSD's thr_self.
  return static_cast<uint32_t>(pthread_getthreadid_np());
#else
  return static_cast<uint32_t>(syscall(SYS_gettid));
#endif
}

void MaybeYield() {
  sched_yield();
  __sync_synchronize();
}

void SyncMemory() {
  __sync_synchronize();
}

void Sleep(std::chrono::microseconds duration) {
  timespec rqtp = DurationToTimeSpec(duration);
  timespec rmtp = {};
  auto p_rqtp = &rqtp;
  auto p_rmtp = &rmtp;
  int ret = 0;
  do {
    ret = nanosleep(p_rqtp, p_rmtp);
    // Swap requested for remaining in case of signal interruption
    // in which case, we start sleeping again for the remainder
    std::swap(p_rqtp, p_rmtp);
  } while (ret == -1 && errno == EINTR);
}

// TODO(bwrsandman) Implement by allowing alert interrupts from IO operations
thread_local bool alertable_state_ = false;
bool DispatchCurrentThreadUserCallback();
SleepResult AlertableSleep(std::chrono::microseconds duration) {
  alertable_state_ = true;
  auto deadline = std::chrono::steady_clock::now() + duration;
  while (true) {
    if (DispatchCurrentThreadUserCallback()) {
      alertable_state_ = false;
      return SleepResult::kAlerted;
    }
    auto now = std::chrono::steady_clock::now();
    if (now >= deadline) {
      alertable_state_ = false;
      return SleepResult::kSuccess;
    }
    auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - now);
    Sleep(std::min(remaining, std::chrono::microseconds(1000)));
  }
}

TlsHandle AllocateTlsHandle() {
  auto key = static_cast<pthread_key_t>(-1);
  auto res = pthread_key_create(&key, nullptr);
  assert_zero(res);
  assert_true(key != static_cast<pthread_key_t>(-1));
  return static_cast<TlsHandle>(key);
}

bool FreeTlsHandle(TlsHandle handle) {
  return pthread_key_delete(static_cast<pthread_key_t>(handle)) == 0;
}

uintptr_t GetTlsValue(TlsHandle handle) {
  return reinterpret_cast<uintptr_t>(pthread_getspecific(static_cast<pthread_key_t>(handle)));
}

bool SetTlsValue(TlsHandle handle, uintptr_t value) {
  return pthread_setspecific(static_cast<pthread_key_t>(handle), reinterpret_cast<void*>(value)) ==
         0;
}

class PosixConditionBase {
 public:
  PosixConditionBase() {
#if REX_PLATFORM_LINUX
    // Use robust mutexes so waits can recover if owner thread terminates.
    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) == 0) {
      if (pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST) == 0) {
        auto native_mutex = static_cast<pthread_mutex_t*>(mutex_.native_handle());
        pthread_mutex_destroy(native_mutex);
        pthread_mutex_init(native_mutex, &attr);
      }
      pthread_mutexattr_destroy(&attr);
    }
#endif
  }

  virtual ~PosixConditionBase() = default;
  virtual bool Signal() = 0;

  // Locks mutex_, recovering it if its owner died while holding it.
  std::unique_lock<std::mutex> LockRobust() {
#if REX_PLATFORM_LINUX
    auto native_mutex = static_cast<pthread_mutex_t*>(mutex_.native_handle());
    int lock_result = pthread_mutex_lock(native_mutex);
    if (lock_result == EOWNERDEAD) {
      pthread_mutex_consistent(native_mutex);
    } else if (lock_result != 0) {
      return std::unique_lock<std::mutex>();
    }
    return std::unique_lock<std::mutex>(mutex_, std::adopt_lock);
#else
    return std::unique_lock<std::mutex>(mutex_);
#endif
  }

  WaitResult Wait(std::chrono::milliseconds timeout) {
    bool executed;
    auto predicate = [this] { return this->signaled(); };
#if REX_PLATFORM_LINUX
    auto native_mutex = static_cast<pthread_mutex_t*>(mutex_.native_handle());
    int lock_result = pthread_mutex_lock(native_mutex);
    if (lock_result == EOWNERDEAD) {
      pthread_mutex_consistent(native_mutex);
    } else if (lock_result != 0) {
      return WaitResult::kFailed;
    }
    std::unique_lock<std::mutex> lock(mutex_, std::adopt_lock);
#else
    std::unique_lock<std::mutex> lock(mutex_);
#endif
    if (predicate()) {
      executed = true;
    } else {
      if (timeout == std::chrono::milliseconds::max()) {
        cond_.wait(lock, predicate);
        executed = true;  // Did not time out;
      } else {
        executed = cond_.wait_for(lock, timeout, predicate);
      }
    }
    if (executed) {
      post_execution();
      return WaitResult::kSuccess;
    } else {
      return WaitResult::kTimeout;
    }
  }

  static std::pair<WaitResult, size_t> WaitMultiple(std::vector<PosixConditionBase*>&& handles,
                                                    bool wait_all,
                                                    std::chrono::milliseconds timeout) {
    assert_true(!handles.empty());

    if (handles.size() == 1) {
      auto result = handles[0]->Wait(timeout);
      return std::make_pair(result, 0);
    }

    // Multi-waits used to poll: check every handle, sleep 1 ms, repeat -- and
    // once less than a millisecond of the timeout was left the sleep rounded
    // down to zero and the loop spun flat out. The Audio Worker and every
    // guest KeWaitForMultipleObjects went through it; on the title screen the
    // Audio Worker burnt half a core here, power a Steam Deck's GPU then
    // lacked, and every wake-up came up to 1 ms late. Now every state change
    // that can satisfy a wait bumps the multi-wait epoch and wakes the
    // multi-waiters (NotifyMultiWaiters), which sleep until that happens.
    const bool infinite_timeout = timeout == std::chrono::milliseconds::max();
    auto end_time = std::chrono::steady_clock::time_point::max();
    if (!infinite_timeout) {
      end_time = std::chrono::steady_clock::now() + timeout;
    }

    // Registered before the first check, so a signaler that changes a handle
    // after that check is guaranteed to see this waiter and notify.
    MultiWaitState& multi_wait = multi_wait_state();
    multi_wait.waiters.fetch_add(1, std::memory_order_seq_cst);
    struct WaiterRegistration {
      MultiWaitState& state;
      ~WaiterRegistration() { state.waiters.fetch_sub(1, std::memory_order_seq_cst); }
    } waiter_registration{multi_wait};

    std::vector<std::unique_lock<std::mutex>> locks;
    if (wait_all) {
      locks.reserve(handles.size());
    }

    while (true) {
      // Read before checking the handles: a change after the check moves it.
      uint64_t epoch = multi_wait.epoch.load(std::memory_order_seq_cst);

      if (!wait_all) {
        // One handle locked at a time, so a plain blocking lock cannot
        // deadlock against other waiters or signalers.
        for (size_t i = 0; i < handles.size(); ++i) {
          std::unique_lock<std::mutex> lock = handles[i]->LockRobust();
          if (!lock.owns_lock()) {
            return std::make_pair<WaitResult, size_t>(WaitResult::kFailed, 0);
          }
          if (handles[i]->signaled()) {
            handles[i]->post_execution();
            return std::make_pair(WaitResult::kSuccess, i);
          }
        }
      } else {
        // All handles have to be checked and taken atomically, so hold every
        // lock at once, backing off on contention instead of deadlocking.
        bool all_locked = true;
        for (size_t i = 0; i < handles.size(); ++i) {
#if REX_PLATFORM_LINUX
          auto native_mutex = static_cast<pthread_mutex_t*>(handles[i]->mutex_.native_handle());
          int result = pthread_mutex_trylock(native_mutex);
          if (result == 0 || result == EOWNERDEAD) {
            if (result == EOWNERDEAD) {
              pthread_mutex_consistent(native_mutex);
            }
            locks.emplace_back(handles[i]->mutex_, std::adopt_lock);
          } else {
            all_locked = false;
            break;
          }
#else
          locks.emplace_back(handles[i]->mutex_, std::try_to_lock);
          if (!locks.back().owns_lock()) {
            locks.pop_back();
            all_locked = false;
            break;
          }
#endif
        }
        if (!all_locked) {
          locks.clear();
          std::this_thread::sleep_for(std::chrono::microseconds(50));
          if (!infinite_timeout && std::chrono::steady_clock::now() >= end_time) {
            return std::make_pair<WaitResult, size_t>(WaitResult::kTimeout, 0);
          }
          continue;
        }
        bool all_signaled = true;
        for (size_t i = 0; i < handles.size(); ++i) {
          if (!handles[i]->signaled()) {
            all_signaled = false;
            break;
          }
        }
        if (all_signaled) {
          for (size_t i = 0; i < handles.size(); ++i) {
            handles[i]->post_execution();
          }
          locks.clear();
          return std::make_pair<WaitResult, size_t>(WaitResult::kSuccess, 0);
        }
        locks.clear();
      }

      // Nothing to take yet: sleep until some handle changes state.
      std::unique_lock<std::mutex> wait_lock(multi_wait.mutex);
      auto changed = [&multi_wait, epoch] {
        return multi_wait.epoch.load(std::memory_order_seq_cst) != epoch;
      };
      if (infinite_timeout) {
        multi_wait.cond.wait(wait_lock, changed);
      } else if (!multi_wait.cond.wait_until(wait_lock, end_time, changed)) {
        return std::make_pair<WaitResult, size_t>(WaitResult::kTimeout, 0);
      }
    }
  }

  // Wakes the threads in WaitMultiple after a state change that can satisfy
  // a wait (a signal, a release, a thread exit). Call it after releasing
  // mutex_.
  static void NotifyMultiWaiters() {
    MultiWaitState& multi_wait = multi_wait_state();
    if (!multi_wait.waiters.load(std::memory_order_seq_cst)) {
      return;
    }
    {
      std::lock_guard<std::mutex> lock(multi_wait.mutex);
      multi_wait.epoch.fetch_add(1, std::memory_order_seq_cst);
    }
    multi_wait.cond.notify_all();
  }

  virtual void* native_handle() const {
    return const_cast<std::condition_variable&>(cond_).native_handle();
  }

 protected:
  inline virtual bool signaled() const = 0;
  inline virtual void post_execution() = 0;
  std::condition_variable cond_;
  std::mutex mutex_;

 private:
  // Threads inside WaitMultiple, and the counter NotifyMultiWaiters bumps for
  // them (see WaitMultiple). Never destroyed: threads may still wait or signal
  // while the process exits, and destroying a condition variable that still
  // has waiters blocks forever.
  struct MultiWaitState {
    std::atomic<uint32_t> waiters{0};
    std::atomic<uint64_t> epoch{0};
    std::mutex mutex;
    std::condition_variable cond;
  };
  static MultiWaitState& multi_wait_state() {
    static MultiWaitState* state = new MultiWaitState();
    return *state;
  }
};

// There really is no native POSIX handle for a single wait/signal construct
// pthreads is at a lower level with more handles for such a mechanism.
// This simple wrapper class functions as our handle and uses conditional
// variables for waits and signals.
template <typename T>
class PosixCondition {};

template <>
class PosixCondition<Event> : public PosixConditionBase {
 public:
  PosixCondition(bool manual_reset, bool initial_state)
      : signal_(initial_state), manual_reset_(manual_reset) {}
  virtual ~PosixCondition() = default;

  bool Signal() override {
    {
      auto lock = std::unique_lock<std::mutex>(mutex_);
      signal_ = true;
      cond_.notify_all();
    }
    NotifyMultiWaiters();
    return true;
  }

  void Reset() {
    auto lock = std::unique_lock<std::mutex>(mutex_);
    signal_ = false;
  }

 private:
  inline bool signaled() const override { return signal_; }
  inline void post_execution() override {
    if (!manual_reset_) {
      signal_ = false;
    }
  }
  bool signal_;
  const bool manual_reset_;
};

template <>
class PosixCondition<Semaphore> : public PosixConditionBase {
 public:
  PosixCondition(uint32_t initial_count, uint32_t maximum_count)
      : count_(initial_count), maximum_count_(maximum_count) {}

  bool Signal() override { return Release(1, nullptr); }

  bool Release(uint32_t release_count, int* out_previous_count) {
    {
      auto lock = std::unique_lock<std::mutex>(mutex_);
      if (release_count > maximum_count_ - count_) {
        return false;
      }
      if (out_previous_count) {
        *out_previous_count = count_;
      }
      count_ += release_count;
      cond_.notify_all();
    }
    NotifyMultiWaiters();
    return true;
  }

 private:
  inline bool signaled() const override { return count_ > 0; }
  inline void post_execution() override {
    count_--;
    cond_.notify_all();
  }
  uint32_t count_;
  const uint32_t maximum_count_;
};

template <>
class PosixCondition<Mutant> : public PosixConditionBase {
 public:
  explicit PosixCondition(bool initial_owner) : count_(0) {
    if (initial_owner) {
      count_ = 1;
      owner_ = std::this_thread::get_id();
    }
  }

  bool Signal() override { return Release(); }

  bool Release() {
    if (owner_ == std::this_thread::get_id() && count_ > 0) {
      bool freed;
      {
        auto lock = std::unique_lock<std::mutex>(mutex_);
        --count_;
        // Free to be acquired by another thread
        freed = count_ == 0;
        if (freed) {
          cond_.notify_all();
        }
      }
      if (freed) {
        NotifyMultiWaiters();
      }
      return true;
    }
    return false;
  }

  void* native_handle() const override { return const_cast<std::mutex&>(mutex_).native_handle(); }

 private:
  inline bool signaled() const override {
    return count_ == 0 || owner_ == std::this_thread::get_id();
  }
  inline void post_execution() override {
    count_++;
    owner_ = std::this_thread::get_id();
  }
  uint32_t count_;
  std::thread::id owner_;
};

template <>
class PosixCondition<Timer> : public PosixConditionBase {
 public:
  explicit PosixCondition(bool manual_reset)
      : callback_(nullptr), signal_(false), manual_reset_(manual_reset) {}

  virtual ~PosixCondition() { Cancel(); }

  bool Signal() override {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      signal_ = true;
      cond_.notify_all();
    }
    NotifyMultiWaiters();
    return true;
  }

  void SetOnce(std::chrono::steady_clock::time_point due_time, std::function<void()> opt_callback) {
    Cancel();

    std::lock_guard<std::mutex> lock(mutex_);

    callback_ = std::move(opt_callback);
    signal_ = false;
    wait_item_ = QueueTimerOnce(&CompletionRoutine, this, due_time);
  }

  void SetRepeating(std::chrono::steady_clock::time_point due_time,
                    std::chrono::milliseconds period, std::function<void()> opt_callback) {
    Cancel();

    std::lock_guard<std::mutex> lock(mutex_);

    callback_ = std::move(opt_callback);
    signal_ = false;
    wait_item_ = QueueTimerRecurring(&CompletionRoutine, this, due_time, period);
  }

  void Cancel() {
    if (auto wait_item = wait_item_.lock()) {
      wait_item->Disarm();
    }
  }

  void* native_handle() const override {
    assert_always();
    return nullptr;
  }

 private:
  static void CompletionRoutine(void* userdata) {
    assert_not_null(userdata);
    auto timer = reinterpret_cast<PosixCondition<Timer>*>(userdata);
    timer->Signal();
    // As the callback may reset the timer, store local.
    std::function<void()> callback;
    {
      std::lock_guard<std::mutex> lock(timer->mutex_);
      callback = timer->callback_;
    }
    if (callback) {
      callback();
    }
  }

 private:
  inline bool signaled() const override { return signal_; }
  inline void post_execution() override {
    if (!manual_reset_) {
      signal_ = false;
    }
  }
  std::weak_ptr<TimerQueueWaitItem> wait_item_;
  std::function<void()> callback_;
  bool signal_;  // Protected by mutex_
  const bool manual_reset_;
};

struct ThreadStartData {
  std::function<void()> start_routine;
  bool create_suspended;
#if REX_PLATFORM_PROSPERO
  // The public handle may be closed while this thread still runs. Its
  // private facade shares the condition and is released by pthread cleanup.
  std::unique_ptr<Thread> running_thread;
#else
  Thread* thread_obj;
#endif
};

template <>
class PosixCondition<Thread>;
extern thread_local PosixCondition<Thread>* current_thread_condition_;

template <>
class PosixCondition<Thread> : public PosixConditionBase {
  enum class State {
    kUninitialized,
    kRunning,
    kSuspended,
    kFinished,
  };

 public:
  PosixCondition()
      : thread_(0),
        signaled_(false),
        exit_code_(0),
        state_(State::kUninitialized),
        suspend_count_(0) {
    suspend_sem_valid_ = sem_init(&suspend_sem_, 0, 0) == 0;
#if REX_PLATFORM_ANDROID
    android_pre_api_26_name_[0] = '\0';
#endif
  }
  bool Initialize(Thread::CreationParameters params, ThreadStartData* start_data) {
    if (!suspend_sem_valid_) return false;
    start_data->create_suspended = params.create_suspended;
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0)
      return false;
    if (pthread_attr_setstacksize(&attr, params.stack_size) != 0) {
      pthread_attr_destroy(&attr);
      return false;
    }
#if REX_PLATFORM_PROSPERO
    // Sonic's native port inherits the title's scheduling. ReX's desktop
    // priorities (1..32) are outside the PS5 range measured by the SDK
    // probes (256..767). Keep the guest-visible priority without passing an
    // invalid desktop value to libkernel or inventing an unmeasured mapping.
    logical_priority_.store(params.initial_priority ? params.initial_priority
                                                     : ThreadPriority::kNormal,
                            std::memory_order_relaxed);
#else
    if (params.initial_priority != 0) {
      sched_param sched{};
      sched.sched_priority = params.initial_priority + 1;
      if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO) != 0) {
        pthread_attr_destroy(&attr);
        return false;
      }
      if (pthread_attr_setschedparam(&attr, &sched) != 0) {
        pthread_attr_destroy(&attr);
        return false;
      }
    }
#endif
    const int create_result = pthread_create(&thread_, &attr, ThreadStartRoutine, start_data);
    if (create_result != 0) {
#if REX_PLATFORM_PROSPERO
      REXSYS_ERROR("PS5 pthread_create failed: native error {}, requested host stack {} bytes",
                   create_result, params.stack_size);
#endif
      pthread_attr_destroy(&attr);
      return false;
    }
    pthread_attr_destroy(&attr);
#if REX_PLATFORM_PROSPERO
    native_owned_ = true;
#endif
    return true;
  }

  /// Constructor for existing thread. This should only happen once called by
  /// Thread::GetCurrentThread() on the main thread
  explicit PosixCondition(pthread_t thread)
      : thread_(thread),
        signaled_(false),
        exit_code_(0),
        state_(State::kRunning),
        suspend_count_(0) {
    suspend_sem_valid_ = sem_init(&suspend_sem_, 0, 0) == 0;
#if REX_PLATFORM_ANDROID
    android_pre_api_26_name_[0] = '\0';
#endif
  }

  virtual ~PosixCondition() {
#if REX_PLATFORM_PROSPERO
    // The running facade retains this condition through its last cleanup.
    // Dropping the public handle never waits for, kills, or invalidates the
    // worker. The SDK detach wrapper reaps its direct stack after native exit.
    if (native_owned_ && thread_ && !native_joined_) {
      const int result = pthread_detach(thread_);
      if (result != 0) {
        REXSYS_ERROR("PS5 thread detach failed during final release: native error {}", result);
      }
    }
    if (suspend_sem_valid_) sem_destroy(&suspend_sem_);
#else
    // Match Canary/Edge behavior.
    // Force-cancel/join from the condition destructor can self-join/crash
    // depending on shutdown ordering, so threads must be stopped explicitly.
    if (suspend_sem_valid_ && !thread_) {
      // Failed pthread_create still owns its successfully created semaphore.
      sem_destroy(&suspend_sem_);
    }
#endif
  }

  bool Signal() override { return true; }

  std::string name() const {
    WaitStarted();
    auto result = std::array<char, 17>{'\0'};
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (state_ != State::kUninitialized && state_ != State::kFinished) {
#if REX_PLATFORM_ANDROID
      // pthread_getname_np was added in API 26 - below that, store the name in
      // this object, which may be only modified through Xenia threading, but
      // should be enough in most cases.
      if (android_pthread_getname_np_) {
        if (android_pthread_getname_np_(thread_, result.data(), result.size() - 1) != 0) {
          assert_always();
        }
      } else {
        std::lock_guard<std::mutex> lock(android_pre_api_26_name_mutex_);
        std::strcpy(result.data(), android_pre_api_26_name_);
      }
#else
      if (pthread_getname_np(thread_, result.data(), result.size() - 1) != 0) {
        assert_always();
      }
#endif
    }
    return std::string(result.data());
  }

  void set_name(const std::string& name) {
    WaitStarted();
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (state_ != State::kUninitialized && state_ != State::kFinished) {
      pthread_setname_np(thread_, std::string(name).c_str());
#if REX_PLATFORM_ANDROID
      SetAndroidPreApi26Name(name);
#endif
    }
  }

#if REX_PLATFORM_ANDROID
  void SetAndroidPreApi26Name(const std::string_view name) {
    if (android_pthread_getname_np_) {
      return;
    }
    std::lock_guard<std::mutex> lock(android_pre_api_26_name_mutex_);
    rex::string::util_copy_truncating(android_pre_api_26_name_, name,
                                      rex::countof(android_pre_api_26_name_));
  }
#endif

  uint32_t system_id() const {
#if REX_PLATFORM_PROSPERO
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(thread_));
#else
    return static_cast<uint32_t>(thread_);
#endif
  }

  uint64_t affinity_mask() {
    WaitStarted();
    std::unique_lock<std::mutex> state_lock(state_mutex_);
    if (state_ == State::kFinished) return 0;
#if REX_PLATFORM_PROSPERO
    uint64_t mask = 0;
    if (scePthreadGetaffinity(thread_, &mask) != 0) return 0;
    return mask;
#else
    cpu_set_t cpu_set;
#if REX_PLATFORM_ANDROID
    if (sched_getaffinity(pthread_gettid_np(thread_), sizeof(cpu_set_t), &cpu_set) != 0) {
      assert_always();
    }
#else
    if (pthread_getaffinity_np(thread_, sizeof(cpu_set_t), &cpu_set) != 0) {
      assert_always();
    }
#endif
    uint64_t result = 0;
    auto cpu_count = std::min(CPU_SETSIZE, 64);
    for (auto i = 0u; i < cpu_count; i++) {
      auto set = CPU_ISSET(i, &cpu_set);
      result |= set << i;
    }
    return result;
#endif
  }

  void set_affinity_mask(uint64_t mask) {
    WaitStarted();
    std::unique_lock<std::mutex> state_lock(state_mutex_);
    if (state_ == State::kFinished) return;
#if REX_PLATFORM_PROSPERO
    const int result = scePthreadSetaffinity(thread_, mask);
    if (result != 0) REXSYS_WARN("PS5 thread affinity failed ({})", result);
#else
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    for (auto i = 0u; i < 64; i++) {
      if (mask & (1ULL << i)) {
        CPU_SET(i, &cpu_set);
      }
    }
#if REX_PLATFORM_ANDROID
    if (sched_setaffinity(pthread_gettid_np(thread_), sizeof(cpu_set_t), &cpu_set) != 0) {
      assert_always();
    }
#else
    if (pthread_setaffinity_np(thread_, sizeof(cpu_set_t), &cpu_set) != 0) {
      assert_always();
    }
#endif
#endif
  }

  int priority() {
    WaitStarted();
#if REX_PLATFORM_PROSPERO
    return logical_priority_.load(std::memory_order_relaxed);
#else
    int policy;
    sched_param param{};
    int ret = pthread_getschedparam(thread_, &policy, &param);
    if (ret != 0) {
      return -1;
    }

    return param.sched_priority;
#endif
  }

  void set_priority(int new_priority) {
    WaitStarted();
#if REX_PLATFORM_PROSPERO
    // Preserve the inherited native policy and priority, as Sonic does.
    // Audio's separate native-priority helper is unaffected by this adapter.
    logical_priority_.store(new_priority, std::memory_order_relaxed);
#else
    sched_param param{};
    param.sched_priority = new_priority;
    int result = pthread_setschedparam(thread_, SCHED_FIFO, &param);
    if (result != 0) {
      switch (result) {
        case EPERM:
          REXSYS_WARN("set_priority: permission denied");
          break;
        case EINVAL:
          assert_always();
          break;
        default:
          REXSYS_WARN("set_priority: pthread_setschedparam failed ({})", result);
          break;
      }
    }
#endif
  }

  void QueueUserCallback(std::function<void()> callback) {
    WaitStarted();
    std::unique_lock<std::mutex> state_lock(state_mutex_);
    if (state_ == State::kFinished) return;
    {
      std::unique_lock<std::mutex> lock(callback_mutex_);
      user_callbacks_.push_back(std::move(callback));
      has_pending_user_callbacks_.store(true, std::memory_order_release);
    }

    // If the callback is queued on the current thread, don't self-signal.
    // Alertable waits drain this queue in normal thread context.
    if (pthread_equal(thread_, pthread_self())) {
      state_lock.unlock();
      if (alertable_state_) {
        DispatchQueuedUserCallbacks();
      }
      return;
    }

    sigval value{};
    value.sival_ptr = this;
#if REX_PLATFORM_ANDROID
    int result = sigqueue(pthread_gettid_np(thread_),
                          GetSystemSignal(SignalType::kThreadUserCallback), value);
#else
#if REX_PLATFORM_PROSPERO
    // The callback lives in user_callbacks_; the signal is only a wakeup.
    int result = pthread_kill(thread_, GetSystemSignal(SignalType::kThreadUserCallback));
#else
    int result = pthread_sigqueue(thread_, GetSystemSignal(SignalType::kThreadUserCallback), value);
#endif
#endif
    if (result != 0) {
      REXSYS_WARN("QueueUserCallback: signal delivery failed ({})", result);
    }
  }

  bool DispatchQueuedUserCallbacks() {
    if (!has_pending_user_callbacks_.load(std::memory_order_acquire)) {
      return false;
    }

    std::function<void()> callback;
    {
      std::unique_lock<std::mutex> lock(callback_mutex_);
      if (user_callbacks_.empty()) {
        has_pending_user_callbacks_.store(false, std::memory_order_release);
        return false;
      }
      callback = std::move(user_callbacks_.front());
      user_callbacks_.pop_front();
      has_pending_user_callbacks_.store(!user_callbacks_.empty(), std::memory_order_release);
    }
    if (callback) {
      callback();
    }
    return true;
  }

  bool Resume(uint32_t* out_previous_suspend_count = nullptr) {
    if (out_previous_suspend_count) {
      *out_previous_suspend_count = 0;
    }
    WaitStarted();
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (state_ == State::kFinished || suspend_count_.load(std::memory_order_relaxed) == 0) {
      return false;
    }
    if (out_previous_suspend_count) {
      *out_previous_suspend_count = suspend_count_.load(std::memory_order_relaxed);
    }
    --suspend_count_;
    if (suspend_count_.load(std::memory_order_relaxed) == 0 && state_ == State::kSuspended) {
      state_ = State::kRunning;
      // Async-signal-safe wakeup for WaitSuspended() from signal handler path.
      sem_post(&suspend_sem_);
    }
    state_signal_.notify_all();
    return true;
  }

  bool Suspend(uint32_t* out_previous_suspend_count = nullptr) {
    if (out_previous_suspend_count) {
      *out_previous_suspend_count = 0;
    }
    WaitStarted();
    bool is_current_thread = pthread_self() == thread_;
    {
      std::unique_lock<std::mutex> lock(state_mutex_);
      if (state_ == State::kFinished ||
          suspend_count_.load(std::memory_order_relaxed) == UINT32_MAX) return false;
      const uint32_t previous = suspend_count_.load(std::memory_order_relaxed);
      if (out_previous_suspend_count) {
        *out_previous_suspend_count = previous;
      }
      state_ = State::kSuspended;
      ++suspend_count_;
      if (previous == 0 && !is_current_thread) {
        // The handler reads only the atomic count, never state_mutex_. Keep
        // the state locked until delivery succeeds so a rejected signal can
        // roll back before another Suspend/Resume observes the transition.
        const int result = pthread_kill(thread_, GetSystemSignal(SignalType::kThreadSuspend));
        if (result != 0) {
          suspend_count_.store(0, std::memory_order_release);
          state_ = State::kRunning;
          return false;
        }
      }
    }

    if (is_current_thread) {
      WaitSuspended();
      return true;
    }

    return true;
  }

  void Terminate(int exit_code) {
#if REX_PLATFORM_PROSPERO
    // Startup publishes its state only after installing native cleanup and
    // the signal context. Cancellation before that point could leak both.
    WaitStarted();
#endif
    // Native pthread identifiers may be recycled after join while this
    // signaled ReX object is still retained by guest handles or waiters.
    // The bound condition identifies the actual logical current thread.
    const bool is_current_thread = current_thread_condition_ == this;
    {
      std::unique_lock<std::mutex> lock(state_mutex_);
      if (state_ == State::kFinished) {
        if (is_current_thread) {
          // This is really bad. Some thread must have called Terminate() on us
          // just before we decided to terminate ourselves
          assert_always();
          for (;;) {
            // Wait for pthread_cancel() to actually happen.
          }
        }
        return;
      }
      state_ = State::kFinished;
#if REX_PLATFORM_PROSPERO
      // Native cancellation may run cleanup without unwinding C++ locks.
      // Publish this independently so that cleanup never needs to reacquire
      // a state mutex held by the interrupted thread after this point.
      termination_requested_.store(true, std::memory_order_release);
#endif
    }

    {
      std::lock_guard<std::mutex> lock(mutex_);

      exit_code_ = exit_code;
      signaled_ = true;
      cond_.notify_all();
    }
    NotifyMultiWaiters();
    if (is_current_thread) {
#if REX_PLATFORM_PROSPERO
      (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
      prospero_signal_thread_clear();
      // Drain the SDK's C++ thread-local destructors before emutls storage goes
      // away. Do not globally alias pthread_exit: the SDK calls the real import.
      ps5_pthread_exit(reinterpret_cast<void*>(exit_code));
#else
      pthread_exit(reinterpret_cast<void*>(exit_code));
#endif
    } else {
#if REX_PLATFORM_ANDROID
      if (pthread_kill(thread_, GetSystemSignal(SignalType::kThreadTerminate)) != 0) {
        assert_always();
      }
#else
      if (pthread_cancel(thread_) != 0) {
        assert_always();
      }
#endif
    }
  }

  void WaitStarted() const {
    std::unique_lock<std::mutex> lock(state_mutex_);
    state_signal_.wait(lock, [this] { return state_ != State::kUninitialized; });
  }

  /// Uses sem_wait because it may be called from signal handler context.
  void WaitSuspended() {
    // Tokens are wakeup hints. Resume can beat signal delivery or the initial
    // gate, so an older token must never release a later suspension. Recheck
    // the count after every token/EINTR without taking a mutex interrupted by
    // this same signal handler.
    static_assert(std::atomic<uint32_t>::is_always_lock_free);
    while (suspend_count_.load(std::memory_order_acquire) != 0) {
      if (sem_wait(&suspend_sem_) == -1 && errno != EINTR) {
        // A broken wait cannot safely grant this thread access to guest state.
        std::abort();
      }
    }
  }

  void* native_handle() const override { return reinterpret_cast<void*>(thread_); }

 private:
  static void* ThreadStartRoutine(void* parameter);
#if REX_PLATFORM_PROSPERO
  static void ThreadCleanupRoutine(void* parameter);
#endif
  inline bool signaled() const override { return signaled_; }
  inline void post_execution() override {
    // Thread objects remain signaled for every later waiter. The native join
    // and semaphore destruction each consume their resource only once.
    // mutex_, held by every Wait variant, serializes this transition.
    if (thread_ && !native_joined_) {
      const int result = pthread_join(thread_, nullptr);
#if REX_PLATFORM_PROSPERO
      if (result != 0) {
        // A successful ReX wait must mean native TLS/closure cleanup ended.
        // Returning success after a failed join lets title teardown release
        // guest memory and KernelState while that cleanup can still use them.
        REXSYS_ERROR("PS5 native thread join failed: error {}", result);
        std::abort();
      }
#endif
      if (result == 0) {
        native_joined_ = true;
        if (suspend_sem_valid_) {
          sem_destroy(&suspend_sem_);
          suspend_sem_valid_ = false;
        }
      }
    }
  }
  pthread_t thread_;
  bool signaled_;
  int exit_code_;
  State state_;             // Protected by state_mutex_
  // Mutations are serialized by state_mutex_; the signal handler only loads.
  std::atomic<uint32_t> suspend_count_;
  sem_t suspend_sem_;
  bool suspend_sem_valid_ = false;
  bool native_joined_ = false;
#if REX_PLATFORM_PROSPERO
  bool native_owned_ = false;
  std::atomic<bool> termination_requested_{false};
  std::atomic<int32_t> logical_priority_{ThreadPriority::kNormal};
#endif
  mutable std::mutex state_mutex_;
  mutable std::mutex callback_mutex_;
  mutable std::condition_variable state_signal_;
  std::deque<std::function<void()>> user_callbacks_;
  std::atomic<bool> has_pending_user_callbacks_{false};
#if REX_PLATFORM_ANDROID
  // Name accessible via name() on Android before API 26 which added
  // pthread_getname_np.
  mutable std::mutex android_pre_api_26_name_mutex_;
  char android_pre_api_26_name_[16];
#endif
};

class PosixWaitHandle {
 public:
  virtual ~PosixWaitHandle();
  virtual PosixConditionBase& condition() = 0;
};

PosixWaitHandle::~PosixWaitHandle() = default;

thread_local PosixCondition<Thread>* current_thread_condition_ = nullptr;

bool DispatchCurrentThreadUserCallback() {
  if (!current_thread_condition_) {
    Thread::GetCurrentThread();
  }
  return current_thread_condition_ && current_thread_condition_->DispatchQueuedUserCallbacks();
}

namespace {

constexpr auto kAlertablePollSlice = std::chrono::milliseconds(1);

class ScopedAlertableState {
 public:
  explicit ScopedAlertableState(bool alertable) : alertable_(alertable) {
    if (alertable_) {
      alertable_state_ = true;
    }
  }
  ~ScopedAlertableState() {
    if (alertable_) {
      alertable_state_ = false;
    }
  }

 private:
  bool alertable_;
};

std::chrono::steady_clock::time_point ComputeAlertableDeadline(std::chrono::milliseconds timeout) {
  if (timeout == std::chrono::milliseconds::max()) {
    return std::chrono::steady_clock::time_point::max();
  }
  return std::chrono::steady_clock::now() + timeout;
}

bool HasAlertableTimeoutElapsed(std::chrono::steady_clock::time_point deadline) {
  return deadline != std::chrono::steady_clock::time_point::max() &&
         std::chrono::steady_clock::now() >= deadline;
}

std::chrono::milliseconds ComputeAlertableWaitTimeout(
    std::chrono::steady_clock::time_point deadline) {
  if (deadline == std::chrono::steady_clock::time_point::max()) {
    return kAlertablePollSlice;
  }
  auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
      deadline - std::chrono::steady_clock::now());
  if (remaining <= std::chrono::milliseconds::zero()) {
    return std::chrono::milliseconds::zero();
  }
  return std::min(remaining, kAlertablePollSlice);
}

}  // namespace

// This wraps a condition object as our handle because posix has no single
// native handle for higher level concurrency constructs such as semaphores
template <typename T>
class PosixConditionHandle : public T, public PosixWaitHandle {
 public:
  PosixConditionHandle() = default;
  explicit PosixConditionHandle(bool);
  explicit PosixConditionHandle(pthread_t thread);
  PosixConditionHandle(bool manual_reset, bool initial_state);
  PosixConditionHandle(uint32_t initial_count, uint32_t maximum_count);
  ~PosixConditionHandle() override = default;

  PosixCondition<T>& condition() override { return handle_; }
  void* native_handle() const override { return handle_.native_handle(); }

 protected:
  PosixCondition<T> handle_;
  friend PosixCondition<T>;
};

#if REX_PLATFORM_PROSPERO
// A Windows-style thread handle is independent of native execution lifetime.
// Keep its wait state alive in a private running facade when the public
// unique_ptr is closed, including closure on the worker itself.
template <>
class PosixConditionHandle<Thread> : public Thread, public PosixWaitHandle {
 public:
  PosixConditionHandle()
      : condition_state_(std::make_shared<PosixCondition<Thread>>()),
        handle_(*condition_state_) {}
  explicit PosixConditionHandle(pthread_t thread)
      : condition_state_(std::make_shared<PosixCondition<Thread>>(thread)),
        handle_(*condition_state_) {}
  explicit PosixConditionHandle(std::shared_ptr<PosixCondition<Thread>> condition)
      : condition_state_(std::move(condition)), handle_(*condition_state_) {}
  PosixCondition<Thread>& condition() override { return handle_; }
  void* native_handle() const override { return handle_.native_handle(); }

 protected:
  std::shared_ptr<PosixCondition<Thread>> condition_state_;
  PosixCondition<Thread>& handle_;
  friend PosixCondition<Thread>;
};
#endif

template <>
PosixConditionHandle<Semaphore>::PosixConditionHandle(uint32_t initial_count,
                                                      uint32_t maximum_count)
    : handle_(initial_count, maximum_count) {}

template <>
PosixConditionHandle<Mutant>::PosixConditionHandle(bool initial_owner) : handle_(initial_owner) {}

template <>
PosixConditionHandle<Timer>::PosixConditionHandle(bool manual_reset) : handle_(manual_reset) {}

template <>
PosixConditionHandle<Event>::PosixConditionHandle(bool manual_reset, bool initial_state)
    : handle_(manual_reset, initial_state) {}

#if !REX_PLATFORM_PROSPERO
template <>
PosixConditionHandle<Thread>::PosixConditionHandle(pthread_t thread) : handle_(thread) {}
#endif

WaitResult Wait(WaitHandle* wait_handle, bool is_alertable, std::chrono::milliseconds timeout) {
  auto posix_wait_handle = dynamic_cast<PosixWaitHandle*>(wait_handle);
  if (posix_wait_handle == nullptr) {
    return WaitResult::kFailed;
  }
  if (!is_alertable) {
    return posix_wait_handle->condition().Wait(timeout);
  }

  ScopedAlertableState alertable_state_guard(true);
  auto deadline = ComputeAlertableDeadline(timeout);

  while (true) {
    if (DispatchCurrentThreadUserCallback()) {
      return WaitResult::kUserCallback;
    }
    if (HasAlertableTimeoutElapsed(deadline)) {
      return WaitResult::kTimeout;
    }
    auto result = posix_wait_handle->condition().Wait(ComputeAlertableWaitTimeout(deadline));
    if (result != WaitResult::kTimeout) {
      return result;
    }
  }
}

WaitResult SignalAndWait(WaitHandle* wait_handle_to_signal, WaitHandle* wait_handle_to_wait_on,
                         bool is_alertable, std::chrono::milliseconds timeout) {
  auto result = WaitResult::kFailed;
  auto posix_wait_handle_to_signal = dynamic_cast<PosixWaitHandle*>(wait_handle_to_signal);
  auto posix_wait_handle_to_wait_on = dynamic_cast<PosixWaitHandle*>(wait_handle_to_wait_on);
  if (posix_wait_handle_to_signal == nullptr || posix_wait_handle_to_wait_on == nullptr) {
    return WaitResult::kFailed;
  }
  if (!posix_wait_handle_to_signal->condition().Signal()) {
    return WaitResult::kFailed;
  }

  if (!is_alertable) {
    return posix_wait_handle_to_wait_on->condition().Wait(timeout);
  }

  ScopedAlertableState alertable_state_guard(true);
  auto deadline = ComputeAlertableDeadline(timeout);
  while (true) {
    if (DispatchCurrentThreadUserCallback()) {
      return WaitResult::kUserCallback;
    }
    if (HasAlertableTimeoutElapsed(deadline)) {
      return WaitResult::kTimeout;
    }
    result = posix_wait_handle_to_wait_on->condition().Wait(ComputeAlertableWaitTimeout(deadline));
    if (result != WaitResult::kTimeout) {
      return result;
    }
  }
}

std::pair<WaitResult, size_t> WaitMultiple(WaitHandle* wait_handles[], size_t wait_handle_count,
                                           bool wait_all, bool is_alertable,
                                           std::chrono::milliseconds timeout) {
  std::vector<PosixConditionBase*> conditions;
  conditions.reserve(wait_handle_count);
  for (size_t i = 0u; i < wait_handle_count; ++i) {
    auto handle = dynamic_cast<PosixWaitHandle*>(wait_handles[i]);
    if (handle == nullptr) {
      return std::make_pair(WaitResult::kFailed, 0);
    }
    conditions.push_back(&handle->condition());
  }
  if (!is_alertable) {
    return PosixConditionBase::WaitMultiple(std::move(conditions), wait_all, timeout);
  }

  ScopedAlertableState alertable_state_guard(true);
  auto deadline = ComputeAlertableDeadline(timeout);
  while (true) {
    if (DispatchCurrentThreadUserCallback()) {
      return std::make_pair(WaitResult::kUserCallback, 0);
    }
    if (HasAlertableTimeoutElapsed(deadline)) {
      return std::make_pair(WaitResult::kTimeout, 0);
    }
    auto result = PosixConditionBase::WaitMultiple(std::vector<PosixConditionBase*>(conditions),
                                                   wait_all, ComputeAlertableWaitTimeout(deadline));
    if (result.first != WaitResult::kTimeout) {
      return result;
    }
  }
}

class PosixEvent : public PosixConditionHandle<Event> {
 public:
  PosixEvent(bool manual_reset, bool initial_state)
      : PosixConditionHandle(manual_reset, initial_state) {}
  ~PosixEvent() override = default;
  void Set() override { handle_.Signal(); }
  void Reset() override { handle_.Reset(); }
  void Pulse() override {
    using namespace std::chrono_literals;
    handle_.Signal();
    MaybeYield();
    Sleep(10us);
    handle_.Reset();
  }
};

std::unique_ptr<Event> Event::CreateManualResetEvent(bool initial_state) {
  return std::make_unique<PosixEvent>(true, initial_state);
}

std::unique_ptr<Event> Event::CreateAutoResetEvent(bool initial_state) {
  return std::make_unique<PosixEvent>(false, initial_state);
}

class PosixSemaphore : public PosixConditionHandle<Semaphore> {
 public:
  PosixSemaphore(int initial_count, int maximum_count)
      : PosixConditionHandle(static_cast<uint32_t>(initial_count),
                             static_cast<uint32_t>(maximum_count)) {}
  ~PosixSemaphore() override = default;
  bool Release(int release_count, int* out_previous_count) override {
    if (release_count < 1) {
      return false;
    }
    return handle_.Release(static_cast<uint32_t>(release_count), out_previous_count);
  }
};

std::unique_ptr<Semaphore> Semaphore::Create(int initial_count, int maximum_count) {
  if (initial_count < 0 || initial_count > maximum_count || maximum_count <= 0) {
    return nullptr;
  }
  return std::make_unique<PosixSemaphore>(initial_count, maximum_count);
}

class PosixMutant : public PosixConditionHandle<Mutant> {
 public:
  explicit PosixMutant(bool initial_owner) : PosixConditionHandle(initial_owner) {}
  ~PosixMutant() override = default;
  bool Release() override { return handle_.Release(); }
};

std::unique_ptr<Mutant> Mutant::Create(bool initial_owner) {
  return std::make_unique<PosixMutant>(initial_owner);
}

class PosixTimer : public PosixConditionHandle<Timer> {
  using WClock_ = Timer::WClock_;
  using GClock_ = Timer::GClock_;

 public:
  explicit PosixTimer(bool manual_reset) : PosixConditionHandle(manual_reset) {}
  ~PosixTimer() override = default;

  bool SetOnceAfter(rex::chrono::hundrednanoseconds rel_time,
                    std::function<void()> opt_callback = nullptr) override {
    return SetOnceAt(GClock_::now() + rel_time, std::move(opt_callback));
  }
  bool SetOnceAt(WClock_::time_point due_time,
                 std::function<void()> opt_callback = nullptr) override {
    return SetOnceAt(rex::chrono::clock_cast<GClock_>(due_time), std::move(opt_callback));
  };
  bool SetOnceAt(GClock_::time_point due_time,
                 std::function<void()> opt_callback = nullptr) override {
    handle_.SetOnce(due_time, std::move(opt_callback));
    return true;
  }

  bool SetRepeatingAfter(rex::chrono::hundrednanoseconds rel_time, std::chrono::milliseconds period,
                         std::function<void()> opt_callback = nullptr) override {
    return SetRepeatingAt(GClock_::now() + rel_time, period, std::move(opt_callback));
  }
  bool SetRepeatingAt(WClock_::time_point due_time, std::chrono::milliseconds period,
                      std::function<void()> opt_callback = nullptr) override {
    return SetRepeatingAt(rex::chrono::clock_cast<GClock_>(due_time), period,
                          std::move(opt_callback));
  }
  bool SetRepeatingAt(GClock_::time_point due_time, std::chrono::milliseconds period,
                      std::function<void()> opt_callback = nullptr) override {
    handle_.SetRepeating(due_time, period, std::move(opt_callback));
    return true;
  }
  bool Cancel() override {
    handle_.Cancel();
    return true;
  }
};

std::unique_ptr<Timer> Timer::CreateManualResetTimer() {
  return std::make_unique<PosixTimer>(true);
}

std::unique_ptr<Timer> Timer::CreateSynchronizationTimer() {
  return std::make_unique<PosixTimer>(false);
}

class PosixThread : public PosixConditionHandle<Thread> {
 public:
  PosixThread() = default;
  explicit PosixThread(pthread_t thread) : PosixConditionHandle(thread) {}
#if REX_PLATFORM_PROSPERO
  explicit PosixThread(std::shared_ptr<PosixCondition<Thread>> condition)
      : PosixConditionHandle(std::move(condition)) {}
#endif
  ~PosixThread() override = default;

  bool Initialize(CreationParameters params, std::function<void()> start_routine) {
#if REX_PLATFORM_PROSPERO
    auto start_data = std::make_unique<ThreadStartData>(ThreadStartData{
        std::move(start_routine), false, std::make_unique<PosixThread>(condition_state_)});
#else
    auto start_data = std::make_unique<ThreadStartData>(
        ThreadStartData{std::move(start_routine), false, this});
#endif
    if (!handle_.Initialize(params, start_data.get())) return false;
    // The start routine owns this only after pthread_create succeeds. A
    // failed creation must also release the captured function and its data.
    start_data.release();
    return true;
  }

  void set_name(std::string name) override {
    handle_.WaitStarted();
    Thread::set_name(name);
    if (name.length() > 15) {
      name = name.substr(0, 15);
    }
    handle_.set_name(name);
  }

  uint32_t system_id() const override { return handle_.system_id(); }

  uint64_t affinity_mask() override { return handle_.affinity_mask(); }
  void set_affinity_mask(uint64_t mask) override { handle_.set_affinity_mask(mask); }

  int priority() override { return handle_.priority(); }
  void set_priority(int new_priority) override { handle_.set_priority(new_priority); }

  void QueueUserCallback(std::function<void()> callback) override {
    handle_.QueueUserCallback(std::move(callback));
  }

  bool Resume(uint32_t* out_previous_suspend_count) override {
    return handle_.Resume(out_previous_suspend_count);
  }

  bool Suspend(uint32_t* out_previous_suspend_count) override {
    return handle_.Suspend(out_previous_suspend_count);
  }

  void Terminate(int exit_code) override { handle_.Terminate(exit_code); }

  void WaitSuspended() { handle_.WaitSuspended(); }
};

thread_local PosixThread* current_thread_ = nullptr;

#if REX_PLATFORM_PROSPERO
void PosixCondition<Thread>::ThreadCleanupRoutine(void* parameter) {
  // pthread cleanup runs for return, pthread_exit and cancellation. Unlike
  // relying on C++ stack unwinding, it is part of the native pthread contract.
  (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
  auto* start_data = static_cast<ThreadStartData*>(parameter);
  auto* thread = static_cast<PosixThread*>(start_data->running_thread.get());
  auto& condition = thread->handle_;
  // Stop accepting suspend signals as soon as teardown begins. Destructors
  // may publish SEH state again, so clear once more after draining them.
  prospero_signal_thread_clear();
  // Captures may own the XThread and its guest ThreadState. Drain the SDK
  // list while those objects, the running facade and emulated TLS all remain
  // alive. TLS destructors may still use either current-thread binding.
  // Thread::Exit may have drained it already; an empty second drain is safe.
  ps5p_run_thread_destructors();
  prospero_signal_thread_clear();
  // Capture destructors may close the public handle or register additional
  // TLS destructors. Drain those too while the ReX running facade is alive.
  start_data->start_routine = {};
  ps5p_run_thread_destructors();
  prospero_signal_thread_clear();
  bool notify = false;
  if (!condition.termination_requested_.load(std::memory_order_acquire)) {
    std::lock_guard lock(condition.state_mutex_);
    // Terminate already publishes its requested exit code before asking the
    // native thread to exit. Do not reacquire its wait mutex in that path:
    // a waiter may already hold it while joining this very cleanup.
    if (condition.state_ != State::kFinished) {
      condition.state_ = State::kFinished;
      notify = true;
    }
  }
  if (notify) {
    {
      std::lock_guard lock(condition.mutex_);
      condition.exit_code_ = 0;
      condition.signaled_ = true;
      condition.cond_.notify_all();
    }
    PosixConditionBase::NotifyMultiWaiters();
  }
  // A late suspension signal must never consult a freed condition, and a
  // reused pthread id must not inherit this thread's SEH/suspend pointers.
  prospero_signal_thread_clear();
  current_thread_ = nullptr;
  current_thread_condition_ = nullptr;
  delete start_data;
}
#endif

void* PosixCondition<Thread>::ThreadStartRoutine(void* parameter) {
#if REX_PLATFORM_PROSPERO
  // Do not allow cancellation until the native cleanup record and all
  // thread-owned state are installed. Terminate waits for the publication.
  (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
#endif
#if !REX_PLATFORM_ANDROID
  if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr) != 0) {
    assert_always();
  }
#endif
  set_current_thread_name("");

  auto start_data = static_cast<ThreadStartData*>(parameter);
  assert_not_null(start_data);
#if REX_PLATFORM_PROSPERO
  auto* thread = static_cast<PosixThread*>(start_data->running_thread.get());
  assert_not_null(thread);
  pthread_cleanup_push(ThreadCleanupRoutine, start_data);
  current_thread_ = thread;
  current_thread_condition_ = &thread->handle_;
  const int bind_result = prospero_signal_bind_suspend(
      &thread->handle_, [](void* context) {
        static_cast<PosixCondition<Thread>*>(context)->WaitSuspended();
      });
  if (bind_result != 0) {
    prospero_trace_checkpoint(PROSPERO_PHASE_SIGNAL_THREAD_REGISTRATION_FAILED,
                             uintptr_t(&thread->handle_), bind_result, 0);
    std::abort();
  }
  prospero_trace_checkpoint(PROSPERO_PHASE_SIGNAL_THREAD_BOUND,
                           uintptr_t(&thread->handle_), 0, 0);
  {
    std::lock_guard lock(thread->handle_.state_mutex_);
    thread->handle_.state_ = start_data->create_suspended ? State::kSuspended : State::kRunning;
    if (start_data->create_suspended) thread->handle_.suspend_count_ = 1;
    thread->handle_.state_signal_.notify_all();
  }
  (void)pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
  if (start_data->create_suspended) thread->WaitSuspended();
  start_data->start_routine();
  pthread_cleanup_pop(1);
#else
  assert_not_null(start_data->thread_obj);

  auto thread = dynamic_cast<PosixThread*>(start_data->thread_obj);
  auto start_routine = std::move(start_data->start_routine);
  auto create_suspended = start_data->create_suspended;
  delete start_data;

  current_thread_ = thread;
  current_thread_condition_ = &thread->handle_;
  {
    std::unique_lock<std::mutex> lock(thread->handle_.state_mutex_);
    thread->handle_.state_ = create_suspended ? State::kSuspended : State::kRunning;
    // The suspend count must change together with the state. Resume() may run
    // as soon as WaitStarted() sees the new state, and with a zero count it
    // does nothing, which left the thread suspended forever.
    if (create_suspended) {
      thread->handle_.suspend_count_ = 1;
    }
    thread->handle_.state_signal_.notify_all();
  }
  if (create_suspended) thread->WaitSuspended();

  start_routine();

  {
    std::unique_lock<std::mutex> lock(thread->handle_.state_mutex_);
    thread->handle_.state_ = State::kFinished;
  }

  {
    std::unique_lock<std::mutex> lock(thread->handle_.mutex_);
    thread->handle_.exit_code_ = 0;
    thread->handle_.signaled_ = true;
    thread->handle_.cond_.notify_all();
  }
  PosixConditionBase::NotifyMultiWaiters();

  current_thread_ = nullptr;
  current_thread_condition_ = nullptr;
#endif
  return nullptr;
}

std::unique_ptr<Thread> Thread::Create(CreationParameters params,
                                       std::function<void()> start_routine) {
  if (!install_signal_handler(SignalType::kThreadSuspend) ||
      !install_signal_handler(SignalType::kThreadUserCallback)) {
    REXSYS_ERROR("Unable to install the native thread signal handlers");
    return nullptr;
  }
#if REX_PLATFORM_ANDROID
  if (!install_signal_handler(SignalType::kThreadTerminate)) return nullptr;
#endif
  auto thread = std::make_unique<PosixThread>();
  if (!thread->Initialize(params, std::move(start_routine)))
    return nullptr;
  assert_not_null(thread);
  return thread;
}

Thread* Thread::GetCurrentThread() {
  if (current_thread_) {
    return current_thread_;
  }

  // Threads not created by Thread::Create (typically main thread) still need
  // process-wide signal handlers used by suspend/APC machinery.
  if (!install_signal_handler(SignalType::kThreadSuspend) ||
      !install_signal_handler(SignalType::kThreadUserCallback)) {
    REXSYS_ERROR("Unable to install the native current-thread signal handlers");
    std::abort();
  }
#if REX_PLATFORM_ANDROID
  if (!install_signal_handler(SignalType::kThreadTerminate)) std::abort();
#endif

  // Should take this route only for threads not created by Thread::Create.
  // The only thread not created by Thread::Create should be the main thread.
  pthread_t handle = pthread_self();

  current_thread_ = new PosixThread(handle);
  current_thread_condition_ = &current_thread_->condition();
#if REX_PLATFORM_PROSPERO
  const int bind_result = prospero_signal_bind_suspend(
      current_thread_condition_, [](void* context) {
        static_cast<PosixCondition<Thread>*>(context)->WaitSuspended();
      });
  if (bind_result != 0) {
    prospero_trace_checkpoint(PROSPERO_PHASE_SIGNAL_THREAD_REGISTRATION_FAILED,
                             uintptr_t(current_thread_condition_), bind_result, 0);
    std::abort();
  }
  prospero_trace_checkpoint(PROSPERO_PHASE_SIGNAL_THREAD_BOUND,
                           uintptr_t(current_thread_condition_), 0, 1);
#endif
  // TODO(bwrsandman): Disabling deleting thread_local current thread to prevent
  //                   assert in destructor. Since this is thread local, the
  //                   "memory leaking" is controlled.
  // atexit([] { delete current_thread_; });

  return current_thread_;
}

void Thread::Exit(int exit_code) {
  if (current_thread_) {
    current_thread_->Terminate(exit_code);
  } else {
    // Should only happen with the main thread
#if REX_PLATFORM_PROSPERO
    ps5_pthread_exit(reinterpret_cast<void*>(exit_code));
#else
    pthread_exit(reinterpret_cast<void*>(exit_code));
#endif
  }
  // Function must not return
  assert_always();
}

void set_current_thread_name(const std::string_view name) {
  pthread_setname_np(pthread_self(), std::string(name).c_str());
#if REX_PLATFORM_ANDROID
  if (!android_pthread_getname_np_ && current_thread_) {
    current_thread_->condition().SetAndroidPreApi26Name(name);
  }
#endif
}

static void signal_handler(int signal, siginfo_t* /*info*/, void* /*context*/) {
  switch (GetSystemSignalType(signal)) {
    case SignalType::kThreadSuspend: {
#if REX_PLATFORM_PROSPERO
      prospero_signal_dispatch_suspend();
#else
      if (!current_thread_) {
        return;
      }
      current_thread_->WaitSuspended();
#endif
    } break;
    case SignalType::kThreadUserCallback: {
      // Callbacks are drained from alertable waits in normal thread context.
      // This signal is only used as a wakeup hint.
    } break;
#if REX_PLATFORM_ANDROID
    case SignalType::kThreadTerminate: {
      pthread_exit(reinterpret_cast<void*>(-1));
    } break;
#endif
    default:
      assert_always();
  }
}

}  // namespace rex::thread
