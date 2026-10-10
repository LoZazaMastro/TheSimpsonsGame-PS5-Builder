// The runner includes the real AudioSystem methods and client declarations.
// Only the Xbox executor, guest heap, native devices and wait/thread APIs are
// host fixtures. Barriers select the race windows without a ROM or console.
#include <rex/assert.h>
#include <rex/stream.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;
using X_STATUS = uint32_t;
constexpr X_STATUS X_STATUS_SUCCESS = 0;
constexpr X_STATUS X_STATUS_UNSUCCESSFUL = 0xc0000001;
constexpr X_STATUS X_STATUS_NO_MEMORY = 0xc0000017;
constexpr X_STATUS X_STATUS_INSUFFICIENT_RESOURCES = 0xc000009a;
#define XFAILED(x) ((x) != 0)
#define SCOPE_profile_cpu_f(...)
#define SCOPE_profile_cpu_i(...)
#define REXAPU_DEBUG(...)
#define REXAPU_NOISY_DEBUG(...)
#define REXAPU_INFO(...)
#define REXAPU_WARN(...)
#define REXAPU_ERROR(...)
#define REXCVAR_GET(name) fixture_##name
static int fixture_audio_maxqframes = 8;
static bool fixture_audio_callback_pacing = true;
static bool fixture_audio_log_underruns = false;
extern "C" bool rex_audio_set_thread_priority() { return true; }

namespace {
std::atomic<unsigned> failures{0};
void Check(bool ok, const char* message) {
  if (!ok) {
    ++failures;
    std::fprintf(stderr, "FAIL %s\n", message);
  }
}

class Gate {
 public:
  void ArriveAndWait() {
    std::unique_lock lock(mutex_);
    entered_ = true;
    condition_.notify_all();
    condition_.wait(lock, [&] { return open_; });
  }
  void WaitEntered() {
    std::unique_lock lock(mutex_);
    if (!condition_.wait_for(lock, 3s, [&] { return entered_; })) {
      throw std::runtime_error("barrier was never entered");
    }
  }
  void Open() {
    std::lock_guard lock(mutex_);
    open_ = true;
    condition_.notify_all();
  }
 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  bool entered_ = false, open_ = false;
};

std::mutex progress_mutex;
std::condition_variable progress;
std::atomic<unsigned> wait_calls{0};
std::atomic<Gate*> next_wait_gate{nullptr};
std::atomic<Gate*> next_sleep_gate{nullptr};
thread_local bool is_audio_worker = false;
thread_local Gate* next_idle_notify_gate = nullptr;
template <class Predicate> void WaitUntil(Predicate predicate) {
  std::unique_lock lock(progress_mutex);
  if (!progress.wait_for(lock, 3s, predicate)) throw std::runtime_error("progress timed out");
}
}  // namespace

extern "C" int __real_nanosleep(const timespec*, timespec*);
// libstdc++'s condition_variable::notify_all is a shared-library boundary;
// wrapping pthread_cond_broadcast would miss its internal call from that DSO.
extern "C" void __real__ZNSt18condition_variable10notify_allEv(void*);
extern "C" void __wrap__ZNSt18condition_variable10notify_allEv(void* condition) {
  if (Gate* gate = std::exchange(next_idle_notify_gate, nullptr)) gate->ArriveAndWait();
  __real__ZNSt18condition_variable10notify_allEv(condition);
}
extern "C" int __wrap_nanosleep(const timespec* request, timespec* remaining) {
  if (is_audio_worker) {
    if (Gate* gate = next_sleep_gate.exchange(nullptr)) gate->ArriveAndWait();
  }
  return __real_nanosleep(request, remaining);
}

namespace rex {
template <class T, size_t N> constexpr size_t countof(const T (&)[N]) { return N; }
namespace memory {
template <class T> void store_and_swap(void* address, T value) {
  value = std::byteswap(value);
  std::memcpy(address, &value, sizeof(value));
}
class Memory {
 public:
  std::atomic<bool> fail_allocate{false};
  std::atomic<unsigned> frees{0};
  uint32_t SystemHeapAlloc(size_t size) {
    if (fail_allocate.exchange(false)) return 0;
    std::lock_guard lock(mutex_);
    const uint32_t address = next_;
    next_ += 0x1000;
    allocations_[address] = std::vector<uint8_t>(size);
    return address;
  }
  void SystemHeapFree(uint32_t address) {
    std::lock_guard lock(mutex_);
    Check(allocations_.erase(address) == 1, "guest wrapper freed exactly once");
    ++frees;
  }
  uint8_t* TranslateVirtual(uint32_t address) {
    std::lock_guard lock(mutex_);
    return allocations_.at(address).data();
  }
  bool IsLive(uint32_t address) {
    std::lock_guard lock(mutex_);
    return allocations_.contains(address);
  }
  size_t LiveCount() {
    std::lock_guard lock(mutex_);
    return allocations_.size();
  }
 private:
  std::mutex mutex_;
  uint32_t next_ = 0x10000;
  std::map<uint32_t, std::vector<uint8_t>> allocations_;
};
}  // namespace memory

namespace thread {
class global_critical_region {
 public:
  static std::recursive_mutex& mutex() { static std::recursive_mutex mutex; return mutex; }
  auto Acquire() { return std::unique_lock<std::recursive_mutex>(mutex()); }
};
enum class WaitResult { kSuccess, kTimeout, kFailed };
class WaitHandle {
 public:
  virtual ~WaitHandle() = default;
  virtual bool TryAcquire() = 0;
};
class Semaphore : public WaitHandle {
 public:
  static std::unique_ptr<Semaphore> Create(uint32_t initial, uint32_t maximum) {
    return std::unique_ptr<Semaphore>(new Semaphore(initial, maximum));
  }
  bool Release(uint32_t count, int32_t*) {
    std::lock_guard lock(mutex_);
    if (count > maximum_ - count_) return false;
    count_ += count;
    progress.notify_all();
    return true;
  }
  bool TryAcquire() override {
    std::lock_guard lock(mutex_);
    if (!count_) return false;
    --count_;
    return true;
  }
 private:
  Semaphore(uint32_t initial, uint32_t maximum) : count_(initial), maximum_(maximum) {}
  std::mutex mutex_;
  uint32_t count_, maximum_;
};
class Event : public WaitHandle {
 public:
  static std::unique_ptr<Event> CreateAutoResetEvent(bool initial) {
    return std::unique_ptr<Event>(new Event(initial));
  }
  bool Set() { signaled_ = true; progress.notify_all(); return true; }
  bool TryAcquire() override { return signaled_.exchange(false); }
 private:
  explicit Event(bool initial) : signaled_(initial) {}
  std::atomic<bool> signaled_;
};
class Fence {
 public:
  void Signal() { signaled_ = true; progress.notify_all(); }
  void Wait() { WaitUntil([&] { return signaled_.load(); }); signaled_ = false; }
 private:
  std::atomic<bool> signaled_{false};
};
WaitResult Wait(WaitHandle* handle, bool, std::chrono::milliseconds timeout = 3s) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  for (;;) {
    if (handle->TryAcquire()) return WaitResult::kSuccess;
    if (std::chrono::steady_clock::now() >= deadline) return WaitResult::kTimeout;
    std::unique_lock lock(progress_mutex);
    progress.wait_for(lock, 1ms);
  }
}
std::pair<WaitResult, size_t> WaitAny(WaitHandle* const* handles, size_t count, bool,
                                     std::chrono::milliseconds timeout) {
  ++wait_calls;
  progress.notify_all();
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  for (;;) {
    for (size_t i = 0; i < count; ++i) {
      if (handles[i]->TryAcquire()) {
        if (i < count - 1) {
          if (Gate* gate = next_wait_gate.exchange(nullptr)) gate->ArriveAndWait();
        }
        return {WaitResult::kSuccess, i};
      }
    }
    if (std::chrono::steady_clock::now() >= deadline) return {WaitResult::kTimeout, 0};
    std::unique_lock lock(progress_mutex);
    progress.wait_for(lock, 1ms);
  }
}
void Sleep(std::chrono::milliseconds time) { std::this_thread::sleep_for(time); }
}  // namespace thread

namespace runtime {
class FunctionDispatcher {
 public:
  explicit FunctionDispatcher(rex::memory::Memory* memory) : memory_(memory) {}
  rex::memory::Memory* memory() { return memory_; }
  std::function<void(uint32_t, uint32_t)> execute;
  void Execute(void*, uint32_t callback, uint64_t* args, size_t count) {
    Check(count == 1, "guest audio callback gets one argument");
    execute(callback, static_cast<uint32_t>(args[0]));
  }
 private:
  rex::memory::Memory* memory_;
};
}  // namespace runtime
namespace system {
class XHostThread {
 public:
  std::atomic<bool> exited{false};
  std::atomic<bool> caught_exception{false};
  ~XHostThread() { if (native_.joinable()) native_.join(); }
  void Start(std::function<void()> work) {
    native_ = std::thread([this, work = std::move(work)] {
      is_audio_worker = true;
      try { work(); } catch (const std::exception&) { caught_exception = true; }
      exited = true;
      progress.notify_all();
    });
  }
  void* thread_state() { return nullptr; }
  void Wait(int, int, int, void*) { if (native_.joinable()) native_.join(); }
  void Terminate(int) { Check(false, "audio shutdown must not cancel the callback thread"); }
 private:
  std::thread native_;
};
}  // namespace system
namespace audio {
constexpr uint32_t kAudioSaveSignature = 0x58415544;
class AudioDriver {
 public:
  virtual ~AudioDriver() = default;
  virtual void SubmitFrame(uint32_t) = 0;
};
class XmaDecoder {
 public:
  explicit XmaDecoder(runtime::FunctionDispatcher*) {}
  void Shutdown() {}
  void Pause() {}
  void Resume() {}
};
class AudioSystem {
 public:
  explicit AudioSystem(runtime::FunctionDispatcher*);
  virtual ~AudioSystem();
  void WorkerThreadMain();
  void Initialize();
  void Shutdown();
  X_STATUS RegisterClient(uint32_t, uint32_t, size_t*);
  void UnregisterClient(size_t);
  void SubmitFrame(size_t, uint32_t);
  void Pause();
  void Resume();
  bool Restore(stream::ByteStream*);
  memory::Memory* memory() const { return memory_; }
  virtual X_STATUS CreateDriver(size_t, rex::thread::Semaphore*, AudioDriver**) = 0;
  virtual void DestroyDriver(AudioDriver*) = 0;
  static constexpr size_t kMaximumQueuedFrames = 64;
  memory::Memory* memory_;
  runtime::FunctionDispatcher* function_dispatcher_;
  std::unique_ptr<XmaDecoder> xma_decoder_;
  uint32_t queued_frames_;
  std::atomic<bool> worker_running_{false};
  std::shared_ptr<system::XHostThread> worker_thread_;
#include "client_state.inc"
  int FindFreeClient();
  std::unique_ptr<rex::thread::Semaphore> client_semaphores_[kMaximumClientCount];
  std::unique_ptr<rex::thread::Event> shutdown_event_;
  rex::thread::WaitHandle* wait_handles_[kMaximumClientCount + 1];
  bool paused_ = false;
  rex::thread::Fence pause_fence_;
  std::unique_ptr<rex::thread::Event> resume_event_;
};
#include "audio_methods.inc"
}  // namespace audio
}  // namespace rex

namespace {
bool GlobalLockIsAvailableToAnotherThread() {
  bool available = false;
  std::thread other([&] {
    auto& mutex = rex::thread::global_critical_region::mutex();
    available = mutex.try_lock();
    if (available) mutex.unlock();
  });
  other.join();
  return available;
}

class AudioFixture : public rex::audio::AudioSystem {
 public:
  explicit AudioFixture(rex::runtime::FunctionDispatcher* dispatcher) : AudioSystem(dispatcher) {}
  ~AudioFixture() override { Shutdown(); }
  std::atomic<unsigned> created{0}, destroyed{0}, submitted{0};
  std::atomic<bool> fail_create{false};
  Gate* create_gate = nullptr;
  Gate* submit_gate = nullptr;
  Gate* destroy_notify_gate = nullptr;
  struct Driver final : rex::audio::AudioDriver {
    explicit Driver(AudioFixture* owner) : owner(owner) {}
    void SubmitFrame(uint32_t) override {
      Check(GlobalLockIsAvailableToAnotherThread(), "SubmitFrame does not hold the global lock");
      if (owner->submit_gate) owner->submit_gate->ArriveAndWait();
      ++owner->submitted;
    }
    AudioFixture* owner;
  };
  X_STATUS CreateDriver(size_t, rex::thread::Semaphore*, rex::audio::AudioDriver** output) override {
    Check(GlobalLockIsAvailableToAnotherThread(), "device creation does not hold the global lock");
    if (create_gate) create_gate->ArriveAndWait();
    *output = nullptr;
    if (fail_create.exchange(false)) return X_STATUS_UNSUCCESSFUL;
    *output = new Driver(this);
    ++created;
    return X_STATUS_SUCCESS;
  }
  void DestroyDriver(rex::audio::AudioDriver* driver) override {
    Check(GlobalLockIsAvailableToAnotherThread(), "device destruction does not hold the global lock");
    Check(driver != nullptr, "no null driver destruction");
    delete driver;
    ++destroyed;
    progress.notify_all();
    next_idle_notify_gate = destroy_notify_gate;
  }
  void Start() {
    worker_thread_ = std::make_shared<rex::system::XHostThread>();
    worker_running_ = true;
    worker_thread_->Start([this] { WorkerThreadMain(); });
  }
  void Stop() { Shutdown(); }
  size_t Register(uint32_t callback = 1) {
    size_t index = size_t(-1);
    Check(RegisterClient(callback, 123, &index) == X_STATUS_SUCCESS, "register succeeds");
    return index;
  }
};

void Run(const std::string& scenario) {
  rex::memory::Memory memory;
  rex::runtime::FunctionDispatcher dispatcher(&memory);
  AudioFixture audio(&dispatcher);
  std::atomic<unsigned> callbacks{0}, stale{0};
  auto observe = [&](uint32_t, uint32_t arg) {
    if (!memory.IsLive(arg)) ++stale;
    ++callbacks;
    progress.notify_all();
  };
  dispatcher.execute = observe;
  auto restore = [&](std::initializer_list<uint32_t> words) {
    std::vector<uint32_t> data(words);
    rex::stream::ByteStream stream(reinterpret_cast<uint8_t*>(data.data()), data.size() * 4);
    return audio.Restore(&stream);
  };
  if (scenario == "unregister-before-execute") {
    size_t index = audio.Register();
    Gate sleeping;
    next_sleep_gate = &sleeping;
    audio.Start();
    sleeping.WaitEntered();
    audio.UnregisterClient(index);
    sleeping.Open();
    WaitUntil([&] { return wait_calls.load() >= 3; });
    audio.Stop();
    Check(callbacks == 1 && stale == 0, "retired paced callback never executes");
  } else if (scenario == "unregister-during-callback" || scenario == "reregister-pending" ||
             scenario == "shutdown-active") {
    Gate callback;
    dispatcher.execute = [&](uint32_t cb, uint32_t arg) {
      callback.ArriveAndWait();
      observe(cb, arg);
    };
    size_t index = audio.Register();
    audio.Start();
    callback.WaitEntered();
    if (scenario == "shutdown-active") {
      std::atomic<bool> complete{false};
      std::thread closer([&] { audio.Stop(); complete = true; });
      std::this_thread::sleep_for(10ms);
      Check(!complete && memory.LiveCount() == 1 && audio.destroyed == 0,
            "shutdown waits for the running callback with its resources alive");
      callback.Open();
      closer.join();
    } else {
      audio.UnregisterClient(index);
      Check(memory.LiveCount() == 1 && audio.destroyed == 0,
            "unregister retains resources used by the running callback");
      size_t new_index = size_t(-1);
      if (scenario == "reregister-pending") {
        new_index = audio.Register(2);
        Check(new_index != index, "retired in-flight slot cannot be reused");
      }
      callback.Open();
      WaitUntil([&] { return audio.destroyed.load() >= 1; });
      if (new_index != size_t(-1)) audio.UnregisterClient(new_index);
      audio.Stop();
    }
    Check(stale == 0, "callback wrapper stays live until Execute returns");
  } else if (scenario == "self-unregister" || scenario == "callback-exception") {
    size_t index = audio.Register();
    dispatcher.execute = [&](uint32_t cb, uint32_t arg) {
      audio.UnregisterClient(index);
      observe(cb, arg);
      Check(audio.destroyed == 0, "self-unregister defers its own driver destruction");
      if (scenario == "callback-exception") throw std::runtime_error("guest executor fixture");
    };
    audio.Start();
    WaitUntil([&] { return audio.destroyed.load() >= 1; });
    audio.Stop();
    Check(callbacks == 1 && stale == 0, "self-unregister executes once with live wrapper");
  } else if (scenario == "submit-after-retire") {
    size_t index = audio.Register();
    audio.UnregisterClient(index);
    audio.SubmitFrame(index, 123);
    audio.SubmitFrame(1000, 123);
    Check(audio.submitted == 0, "late and out-of-range frames are rejected");
  } else if (scenario == "submit-retire-concurrent") {
    Gate submission;
    audio.submit_gate = &submission;
    size_t index = audio.Register();
    std::thread producer([&] { audio.SubmitFrame(index, 123); });
    submission.WaitEntered();
    audio.UnregisterClient(index);
    Check(audio.destroyed == 0, "in-flight native submit retains the device");
    submission.Open();
    producer.join();
    Check(audio.submitted == 1 && audio.destroyed == 1, "submit release completes deferred cleanup");
  } else if (scenario == "stale-wait-credit") {
    size_t old_index = audio.Register(1);
    Gate wait;
    next_wait_gate = &wait;
    audio.Start();
    wait.WaitEntered();
    audio.UnregisterClient(old_index);
    size_t new_index = audio.Register(2);
    Check(new_index == old_index, "fully retired slot may be reused");
    wait.Open();
    WaitUntil([&] { return wait_calls.load() >= 10; });
    audio.Stop();
    Check(callbacks == 8, "old wait result cannot synthesize an extra new-client frame credit");
  } else if (scenario == "registration-failure") {
    size_t index = size_t(-1);
    audio.fail_create = true;
    Check(XFAILED(audio.RegisterClient(1, 123, &index)), "native device failure propagates");
    memory.fail_allocate = true;
    Check(XFAILED(audio.RegisterClient(1, 123, &index)), "guest wrapper allocation failure propagates");
    index = audio.Register();
    Check(index == 0, "failed registration releases the reserved slot");
    audio.UnregisterClient(index);
  } else if (scenario == "registration-shutdown") {
    Gate creation;
    audio.create_gate = &creation;
    X_STATUS result = X_STATUS_SUCCESS;
    std::thread registrar([&] { size_t index; result = audio.RegisterClient(1, 123, &index); });
    creation.WaitEntered();
    std::atomic<bool> complete{false};
    std::thread closer([&] { audio.Stop(); complete = true; });
    std::this_thread::sleep_for(10ms);
    Check(!complete, "shutdown keeps an in-progress registration alive");
    creation.Open();
    registrar.join();
    closer.join();
    Check(XFAILED(result), "in-progress registration cannot publish after shutdown");
  } else if (scenario == "shutdown-paused") {
    audio.Start();
    audio.Pause();
    audio.Stop();
  } else if (scenario == "restore-retiring") {
    Gate callback;
    dispatcher.execute = [&](uint32_t cb, uint32_t arg) {
      callback.ArriveAndWait();
      observe(cb, arg);
    };
    const auto index = audio.Register();
    audio.Start();
    callback.WaitEntered();
    audio.UnregisterClient(index);
    Check(!restore({rex::audio::kAudioSaveSignature, 1, uint32_t(index), 2, 123, 0}),
          "restore rejects a retired slot whose callback still owns it");
    Check(memory.LiveCount() == 1 && audio.destroyed == 0 && audio.created == 1,
          "rejected restore preserves the in-flight registration");
    callback.Open();
    WaitUntil([&] { return audio.destroyed.load() >= 1; });
    audio.Stop();
    Check(stale == 0, "restore does not free an in-flight callback wrapper");
  } else if (scenario == "restore-invalid") {
    Check(!audio.Restore(nullptr), "restore rejects null stream");
    Check(!restore({}), "restore rejects short signature");
    Check(!restore({rex::audio::kAudioSaveSignature, 9}), "restore rejects excessive client count");
    Check(!restore({rex::audio::kAudioSaveSignature, 1, 8, 2, 123, 0}),
          "restore rejects an out-of-range slot");
    Check(!restore({rex::audio::kAudioSaveSignature, 1, 0}), "restore rejects truncated record");
    Check(!restore({rex::audio::kAudioSaveSignature, 2, 0, 2, 123, 0, 0, 2, 123, 0}),
          "restore rejects duplicate slots before publishing any driver");
    Check(audio.created == 0, "malformed savestates create no native drivers");
  } else if (scenario == "restore-idle") {
    const uint32_t wrapped_arg = memory.SystemHeapAlloc(4);
    rex::memory::store_and_swap<uint32_t>(memory.TranslateVirtual(wrapped_arg), 123);
    Check(restore({rex::audio::kAudioSaveSignature, 1, 0, 2, 123, wrapped_arg}),
          "quiescent restore publishes a complete registration");
    audio.SubmitFrame(0, 123);
    Check(audio.submitted == 1, "restored driver accepts audio frames");
    audio.UnregisterClient(0);
  } else if (scenario == "restore-shutdown") {
    Gate creation;
    audio.create_gate = &creation;
    const uint32_t wrapped_arg = memory.SystemHeapAlloc(4);
    bool result = true;
    std::thread restorer([&] {
      result = restore({rex::audio::kAudioSaveSignature, 1, 0, 2, 123, wrapped_arg});
    });
    creation.WaitEntered();
    std::atomic<bool> complete{false};
    std::thread closer([&] { audio.Stop(); complete = true; });
    std::this_thread::sleep_for(10ms);
    Check(!complete, "shutdown waits for in-progress restore driver initialization");
    creation.Open();
    restorer.join();
    closer.join();
    Check(!result && audio.created == audio.destroyed,
          "restore cannot publish after shutdown and closes its driver");
    Check(memory.IsLive(wrapped_arg), "unpublished restore leaves its supplied heap pointer alone");
    memory.SystemHeapFree(wrapped_arg);
  } else if (scenario == "cleanup-notify-shutdown") {
    Gate notifying;
    audio.destroy_notify_gate = &notifying;
    const auto index = audio.Register();
    std::jthread retiring([&] { audio.UnregisterClient(index); });
    notifying.WaitEntered();
    Check(!GlobalLockIsAvailableToAnotherThread(),
          "idle completion keeps the owner lock until notify_all has finished");
    std::atomic<bool> complete{false};
    std::jthread closer([&] { audio.Stop(); complete = true; });
    std::this_thread::sleep_for(10ms);
    Check(!complete, "shutdown cannot return while cleanup still uses its condition variable");
    notifying.Open();
    retiring.join();
    closer.join();
  } else {
    throw std::runtime_error("unknown scenario");
  }
  audio.Stop();
  Check(memory.LiveCount() == 0, "all guest callback wrappers released");
  Check(audio.created == audio.destroyed, "every native audio driver destroyed exactly once");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  try { Run(argv[1]); }
  catch (const std::exception& error) {
    std::fprintf(stderr, "fixture failed: %s\n", error.what());
    return 2;
  }
  std::printf("%s %s (%u failed checks)\n", failures ? "FAIL" : "PASS", argv[1], failures.load());
  return failures ? 1 : 0;
}
