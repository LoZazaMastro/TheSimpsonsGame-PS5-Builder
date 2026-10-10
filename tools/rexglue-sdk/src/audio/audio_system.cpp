/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/assert.h>
#include <rex/audio/audio_driver.h>
#include <rex/audio/audio_system.h>
#include <rex/audio/flags.h>
#include <rex/audio/thread_priority.h>
#include <rex/audio/xma/decoder.h>
#include <rex/dbg.h>
#include <rex/logging.h>
#include <rex/math.h>
#include <rex/memory/ring_buffer.h>
#include <rex/stream.h>
#include <rex/string/buffer.h>
#include <rex/system/thread_state.h>
#include <rex/thread.h>
#include <rex/cvar.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <thread>
#include <utility>

#if !REX_PLATFORM_PROSPERO
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_timer.h>
#endif
#if REX_PLATFORM_LINUX
#include <SDL3/SDL_system.h>
#include <sys/syscall.h>
#endif

REXCVAR_DEFINE_INT32(
    audio_maxqframes, 8, "Audio",
    "Max buffered audio frames (range 4-64). Lower reduces latency but may cause stuttering.");

REXCVAR_DEFINE_BOOL(audio_callback_pacing, true, "Audio",
                    "Ask the game for audio frames at the hardware's steady rate instead of as "
                    "fast as queue space frees up (bursts make the game submit silent frames)");

// As with normal Microsoft, there are like twelve different ways to access
// the audio APIs. Early games use XMA*() methods almost exclusively to touch
// decoders. Later games use XAudio*() and direct memory writes to the XMA
// structures (as opposed to the XMA* calls), meaning that we have to support
// both.
//
// For ease of implementation, most audio related processing is handled in
// AudioSystem, and the functions here call off to it.
// The XMA*() functions just manipulate the audio system in the guest context
// and let the normal AudioSystem handling take it, to prevent duplicate
// implementations. They can be found in xboxkrnl_audio_xma.cc

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <cerrno>
#endif

extern "C" bool rex_audio_set_thread_priority() {
#if defined(_WIN32)
  return SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) != 0;
#elif REX_PLATFORM_PROSPERO
  // Keep the native scheduling policy. Prospero uses smaller numeric thread
  // priorities for higher priority; desktop SCHED_FIFO priority 10 is invalid.
  int policy = 0;
  struct sched_param sp{};
  if (pthread_getschedparam(pthread_self(), &policy, &sp) != 0) {
    return false;
  }
  const int bound_a = sched_get_priority_min(policy);
  const int bound_b = sched_get_priority_max(policy);
  if (bound_a < 0 || bound_b < 0) {
    return false;
  }
  sp.sched_priority = std::clamp(sp.sched_priority - 16, std::min(bound_a, bound_b),
                                std::max(bound_a, bound_b));
  return pthread_setschedparam(pthread_self(), policy, &sp) == 0;
#else
  struct sched_param sp{};
  sp.sched_priority = 10;
  if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp) == 0) {
    return true;
  }
#if REX_PLATFORM_LINUX
  if (SDL_SetLinuxThreadPriorityAndPolicy(syscall(SYS_gettid), SDL_THREAD_PRIORITY_TIME_CRITICAL,
                                          SCHED_RR)) {
    return true;
  }
#endif
  return SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH);
#endif
}

namespace rex::audio {

AudioSystem::AudioSystem(runtime::FunctionDispatcher* function_dispatcher)
    : memory_(function_dispatcher->memory()),
      function_dispatcher_(function_dispatcher),
      worker_running_(false) {
  std::memset(clients_, 0, sizeof(clients_));

  queued_frames_ =
      uint32_t(std::clamp(REXCVAR_GET(audio_maxqframes), 4, int32_t(kMaximumQueuedFrames)));

  for (size_t i = 0; i < kMaximumClientCount; ++i) {
    client_semaphores_[i] = rex::thread::Semaphore::Create(0, queued_frames_);
    assert_not_null(client_semaphores_[i]);
    wait_handles_[i] = client_semaphores_[i].get();
  }
  shutdown_event_ = rex::thread::Event::CreateAutoResetEvent(false);
  assert_not_null(shutdown_event_);
  wait_handles_[kMaximumClientCount] = shutdown_event_.get();

  xma_decoder_ = std::make_unique<rex::audio::XmaDecoder>(function_dispatcher_);

  resume_event_ = rex::thread::Event::CreateAutoResetEvent(false);
  assert_not_null(resume_event_);
}

AudioSystem::~AudioSystem() {
  if (xma_decoder_) {
    xma_decoder_->Shutdown();
  }
}

X_STATUS AudioSystem::Setup(system::KernelState* kernel_state) {
  X_STATUS result = xma_decoder_->Setup(kernel_state);
  if (result) {
    return result;
  }

  worker_running_ = true;
  worker_thread_ = system::object_ref<system::XHostThread>(
      new system::XHostThread(kernel_state, 128 * 1024, 0, [this]() {
        WorkerThreadMain();
        return 0;
      }));

  worker_thread_->set_name("Audio Worker");
  result = worker_thread_->Create();
  if (XFAILED(result)) {
    worker_running_ = false;
    worker_thread_.reset();
    xma_decoder_->Shutdown();
    return result;
  }

  return X_STATUS_SUCCESS;
}

void AudioSystem::WorkerThreadMain() {
  // HAND PATCH: raise the audio worker to realtime-ish priority so guest
  // audio callbacks aren't starved by GPU/streaming threads (audible
  // stutter under load). SCHED_FIFO needs privileges we may not have, so
  // use SDL's native scheduling helper, including the desktop priority broker.
  if (!rex_audio_set_thread_priority()) {
#if REX_PLATFORM_PROSPERO
    REXAPU_WARN("PS5 AudioWorker uses its inherited thread priority");
#else
    REXAPU_WARN("AudioWorker: cannot raise thread priority: {}", SDL_GetError());
#endif
  }
  // Initialize driver and ringbuffer.
  Initialize();

  // The hardware asks for one frame (256 samples) every 5.33 ms of playback,
  // and the game's mixer renders at that pace: called again right away, its
  // callback submits a frame of silence. But the audio driver frees queue
  // slots in bursts (a PipeWire period is four frames), so callbacks are
  // spread out to just under the frame duration, which keeps the queue full.
  constexpr auto kCallbackInterval = std::chrono::microseconds(5333 * 39 / 40);
  std::chrono::steady_clock::time_point next_callback[kMaximumClientCount] = {};

  // audio_log_underruns: callbacks per second, how long the game's callback
  // runs and how late the paced wait wakes (a coarse timer shows here).
  struct PacingDiag {
    uint32_t callbacks = 0, waits = 0, unpaced = 0;
    double callback_sum_us = 0, callback_max_us = 0, late_sum_us = 0, late_max_us = 0;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point last_callback = {};
  } pacing_diag;

  // Main run loop.
  uint32_t diag_pump_count = 0;
  while (worker_running_) {
    uint64_t wait_generations[kMaximumClientCount];
    {
      auto global_lock = global_critical_region_.Acquire();
      std::copy(std::begin(client_generations_), std::end(client_generations_), wait_generations);
    }
    // These handles signify the number of submitted samples. Once we reach
    // 64 samples, we wait until our audio backend releases a semaphore
    // (signaling a sample has finished playing)
    auto result = rex::thread::WaitAny(wait_handles_, rex::countof(wait_handles_), true,
                                       std::chrono::milliseconds(500));
    if (result.first == rex::thread::WaitResult::kFailed) {
      REXAPU_WARN("AudioWorker: WaitAny failed");
      continue;
    }

    if (result.first == rex::thread::WaitResult::kTimeout) {
      if (diag_pump_count < 5) {
        REXAPU_NOISY_DEBUG("AudioWorker: WaitAny timed out (no semaphore signals)");
      }
    }

    if (result.first == thread::WaitResult::kSuccess && result.second == kMaximumClientCount) {
      // Shutdown event signaled.
      if (paused_) {
        pause_fence_.Signal();
        thread::Wait(resume_event_.get(), false);
      }

      continue;
    }

    // Number of clients pumped
    bool pumped = false;
    if (result.first == rex::thread::WaitResult::kSuccess) {
      auto index = result.second;

      auto global_lock = global_critical_region_.Acquire();
      if (clients_shutting_down_) break;
      if (client_generations_[index] != wait_generations[index]) {
        // The slot changed while WaitAny was asleep or returning. Restore a
        // credit if it was taken from the new registration. If that queue is
        // already full, the consumed credit belonged to the old one instead.
        // The next wait snapshots the new generation before dispatching it.
        if (clients_[index].in_use && !client_retiring_[index]) {
          client_semaphores_[index]->Release(1, nullptr);
        }
        continue;
      }
      const uint64_t generation = client_generations_[index];
      const bool has_callback = clients_[index].in_use && !client_retiring_[index] &&
                                clients_[index].callback != 0;
      global_lock.unlock();

      if (has_callback) {
        if (REXCVAR_GET(audio_callback_pacing)) {
          auto now = std::chrono::steady_clock::now();
          if (now < next_callback[index]) {
            // SDL uses a high-resolution timer on Windows. Keep the existing
            // schedule and measure the actual wake rather than the deadline.
#if REX_PLATFORM_PROSPERO
            std::this_thread::sleep_until(next_callback[index]);
#else
            SDL_DelayNS(uint64_t(
                std::chrono::duration_cast<std::chrono::nanoseconds>(next_callback[index] - now)
                    .count()));
#endif
            now = std::chrono::steady_clock::now();
            if (REXCVAR_GET(audio_log_underruns)) {
              const double late =
                  std::chrono::duration<double, std::micro>(now - next_callback[index]).count();
              pacing_diag.late_sum_us += std::max(0.0, late);
              pacing_diag.late_max_us = std::max(pacing_diag.late_max_us, late);
              ++pacing_diag.waits;
            }
          } else if (REXCVAR_GET(audio_log_underruns)) {
            ++pacing_diag.unpaced;
          }
          // Keep the schedule, so waking late cannot lower the average rate
          // below the hardware's, but never ask again within half a frame:
          // called again right away, the game's mixer returns silence. After
          // a stall, this restarts from now instead of catching up in a burst.
          next_callback[index] =
              std::max(next_callback[index] + kCallbackInterval, now + kCallbackInterval / 2);
        }
        // Recheck after pacing: UnregisterClient may have retired this slot
        // during the sleep. The lease keeps both driver and wrapped argument
        // alive through Execute, including self-unregistration and exceptions.
        auto client = RetainClient(index, generation);
        if (!client || !client.callback()) continue;
        const uint32_t client_callback = client.callback();
        const uint32_t client_callback_arg = client.wrapped_arg();
        if (diag_pump_count < 10) {
          REXAPU_DEBUG("AudioWorker: dispatching callback {:08X} with arg {:08X} for client {}",
                       client_callback, client_callback_arg, index);
        }
        SCOPE_profile_cpu_i("apu", "rex::audio::AudioSystem->client_callback");
        uint64_t args[] = {client_callback_arg};
        const bool log_pacing = REXCVAR_GET(audio_log_underruns);
        const auto callback_start =
            log_pacing ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
        function_dispatcher_->Execute(worker_thread_->thread_state(), client_callback, args,
                                      rex::countof(args));
        if (log_pacing) {
          auto end = std::chrono::steady_clock::now();
          // Do not average a pause, loading gap or disabled diagnostic period
          // into the callback rate reported for active playback.
          if (pacing_diag.last_callback == std::chrono::steady_clock::time_point{} ||
              callback_start - pacing_diag.last_callback > std::chrono::milliseconds(250)) {
            pacing_diag = {};
            pacing_diag.start = callback_start;
          }
          pacing_diag.last_callback = end;
          double took = std::chrono::duration<double, std::micro>(end - callback_start).count();
          pacing_diag.callback_sum_us += took;
          pacing_diag.callback_max_us = std::max(pacing_diag.callback_max_us, took);
          ++pacing_diag.callbacks;
          double elapsed = std::chrono::duration<double>(end - pacing_diag.start).count();
          if (elapsed >= 5.0) {
            REXAPU_INFO(
                "[audio-pacing] {:.1f} callbacks/s (hardware: 187.5), callback avg {:.0f} us "
                "max {:.0f} us; paced wait woke late avg {:.0f} us max {:.0f} us over {} waits, "
                "{} callbacks already late",
                pacing_diag.callbacks / elapsed,
                pacing_diag.callback_sum_us / pacing_diag.callbacks, pacing_diag.callback_max_us,
                pacing_diag.waits ? pacing_diag.late_sum_us / pacing_diag.waits : 0.0,
                pacing_diag.late_max_us, pacing_diag.waits, pacing_diag.unpaced);
            pacing_diag = {};
            pacing_diag.start = pacing_diag.last_callback = end;
          }
        }
        if (diag_pump_count < 10) {
          REXAPU_DEBUG("AudioWorker: callback returned for client {}", index);
        }
        diag_pump_count++;
      } else {
        REXAPU_DEBUG("AudioWorker: semaphore signaled for client {} but callback is 0", index);
      }

      pumped = true;
    }

    if (!worker_running_) {
      break;
    }

    if (!pumped) {
      SCOPE_profile_cpu_i("apu", "Sleep");
      rex::thread::Sleep(std::chrono::milliseconds(500));
    }
  }
  worker_running_ = false;

  // TODO(benvanik): call module API to kill?
}

int AudioSystem::FindFreeClient() {
  for (size_t i = 0; i < kMaximumClientCount; i++) {
    auto& client = clients_[i];
    if (!client.in_use && !client_retiring_[i] && !client_cleanup_running_[i]) {
      return i;
    }
  }

  return -1;
}

void AudioSystem::Initialize() {}

void AudioSystem::Shutdown() {
  {
    auto global_lock = global_critical_region_.Acquire();
    clients_shutting_down_ = true;
    // Existing leases remain valid until they finish; no new callback or
    // submit can begin once shutdown starts.
    for (size_t i = 0; i < kMaximumClientCount; ++i) {
      if (clients_[i].in_use) {
        clients_[i].in_use = false;
        client_retiring_[i] = true;
      }
    }
  }

  worker_running_ = false;
  shutdown_event_->Set();
  // A paused worker must also be released before joining.
  resume_event_->Set();
  if (worker_thread_) {
    // Asynchronous cancellation may interrupt a native lock or a live guest
    // callback. Let its stack unwind normally before reclaiming its resources.
    // The app's existing process watchdog handles a guest callback that never
    // returns; it must not be made to look like a successful audio shutdown.
    worker_thread_->Wait(0, 0, 0, nullptr);
    worker_thread_.reset();
  }

  // A callback already in progress may still submit XMA work or wait for it.
  // Keep decoder contexts alive until the audio callback worker has joined.
  if (xma_decoder_) {
    xma_decoder_->Shutdown();
  }

  // Destroy all active client drivers (closes SDL audio devices, stopping
  // callback threads) before the semaphores they reference are destroyed.
  for (size_t i = 0; i < kMaximumClientCount; i++) {
    CleanupRetiredClient(i, client_generations_[i]);
  }
  // A submit or registration may already have left the critical region when
  // shutdown began. Keep this object and its semaphores alive until those
  // operations finish their normal cleanup as well.
  auto global_lock = global_critical_region_.Acquire();
  clients_idle_.wait(global_lock, [this] {
    for (size_t i = 0; i < kMaximumClientCount; ++i) {
      if (client_in_flight_[i] || client_retiring_[i] || client_cleanup_running_[i]) return false;
    }
    return true;
  });
}

X_STATUS AudioSystem::RegisterClient(uint32_t callback, uint32_t callback_arg, size_t* out_index) {
  REXAPU_DEBUG("AudioSystem::RegisterClient: callback={:08X} callback_arg={:08X}", callback,
               callback_arg);
  auto global_lock = global_critical_region_.Acquire();

  if (clients_shutting_down_) return X_STATUS_UNSUCCESSFUL;

  auto index = FindFreeClient();
  if (index < 0) {
    return X_STATUS_INSUFFICIENT_RESOURCES;
  }
  REXAPU_DEBUG("AudioSystem::RegisterClient: using client index={} queued_frames={}", index,
               queued_frames_);

  auto client_semaphore = client_semaphores_[index].get();
  // Reserve the numeric slot before opening the hardware device. Native
  // output initialization and its failure cleanup may block; neither belongs
  // inside the system-wide guest critical region.
  client_cleanup_running_[index] = true;
  if (++client_generations_[index] == 0) ++client_generations_[index];
  global_lock.unlock();
  AudioDriver* driver = nullptr;
  uint32_t ptr = 0;
  auto discard_registration = [&](X_STATUS status) {
    if (driver) DestroyDriver(driver);
    if (ptr) memory()->SystemHeapFree(ptr);
    {
      auto cleanup_lock = global_critical_region_.Acquire();
      client_cleanup_running_[index] = false;
      clients_idle_.notify_all();
    }
    return status;
  };
  try {
    auto result = CreateDriver(index, client_semaphore, &driver);
    if (XFAILED(result)) {
      return discard_registration(result);
    }
    if (!driver) return discard_registration(X_STATUS_UNSUCCESSFUL);

    ptr = memory()->SystemHeapAlloc(0x4);
    if (!ptr) {
      return discard_registration(X_STATUS_NO_MEMORY);
    }
    memory::store_and_swap<uint32_t>(memory()->TranslateVirtual(ptr), callback_arg);
  } catch (...) {
    discard_registration(X_STATUS_UNSUCCESSFUL);
    throw;
  }

  global_lock.lock();
  if (clients_shutting_down_) {
    global_lock.unlock();
    return discard_registration(X_STATUS_UNSUCCESSFUL);
  }
  clients_[index] = {driver, callback, callback_arg, ptr, true};
  client_cleanup_running_[index] = false;

  // Publish a complete client before granting callback credits. On native
  // device failure a retry must not inherit credits from a failed registration.
  if (!client_semaphore->Release(queued_frames_, nullptr)) {
    clients_[index].in_use = false;
    client_retiring_[index] = true;
    const uint64_t generation = client_generations_[index];
    global_lock.unlock();
    CleanupRetiredClient(index, generation);
    return X_STATUS_UNSUCCESSFUL;
  }

  if (out_index) {
    *out_index = index;
  }

  return X_STATUS_SUCCESS;
}

void AudioSystem::SubmitFrame(size_t index, uint32_t samples_ptr) {
  SCOPE_profile_cpu_f("apu");

  static std::atomic<uint32_t> submit_count{0};
  if (submit_count.fetch_add(1, std::memory_order_relaxed) < 10) {
    REXAPU_DEBUG("AudioSystem::SubmitFrame called: index={} samples_ptr={:08X}", index,
                 samples_ptr);
  }

  auto client = RetainClient(index);
  if (!client) {
    REXAPU_WARN("Ignoring audio frame for inactive client {}", index);
    return;
  }
  // Serialize submissions (including optional audio dumping) without holding
  // the system-wide guest critical region over conversion or host I/O. Declare
  // this after the lease so it is released before deferred driver destruction.
  std::lock_guard lock(client_submit_mutexes_[index]);
  client.driver()->SubmitFrame(samples_ptr);
}

void AudioSystem::UnregisterClient(size_t index) {
  SCOPE_profile_cpu_f("apu");

  uint64_t generation;
  {
    auto global_lock = global_critical_region_.Acquire();
    if (index >= kMaximumClientCount || !clients_[index].in_use) return;
    clients_[index].in_use = false;
    client_retiring_[index] = true;
    generation = client_generations_[index];
  }
  CleanupRetiredClient(index, generation);
}

AudioSystem::ClientLease::ClientLease(ClientLease&& other) noexcept
    : owner_(std::exchange(other.owner_, nullptr)), index_(other.index_),
      generation_(other.generation_), driver_(other.driver_), callback_(other.callback_),
      wrapped_arg_(other.wrapped_arg_) {}

AudioSystem::ClientLease::~ClientLease() {
  if (owner_) owner_->ReleaseClient(index_, generation_);
}

AudioSystem::ClientLease AudioSystem::RetainClient(size_t index, uint64_t expected_generation) {
  auto global_lock = global_critical_region_.Acquire();
  if (clients_shutting_down_ || index >= kMaximumClientCount || !clients_[index].in_use ||
      client_retiring_[index] || !clients_[index].driver ||
      (expected_generation && client_generations_[index] != expected_generation)) {
    return {};
  }
  ++client_in_flight_[index];
  const auto& client = clients_[index];
  return ClientLease(this, index, client_generations_[index], client.driver, client.callback,
                     client.wrapped_callback_arg);
}

void AudioSystem::ReleaseClient(size_t index, uint64_t generation) {
  {
    auto global_lock = global_critical_region_.Acquire();
    assert_true(index < kMaximumClientCount && client_generations_[index] == generation &&
                client_in_flight_[index] != 0);
    --client_in_flight_[index];
  }
  CleanupRetiredClient(index, generation);
}

void AudioSystem::CleanupRetiredClient(size_t index, uint64_t generation) {
  AudioDriver* driver;
  uint32_t wrapped_arg;
  {
    auto global_lock = global_critical_region_.Acquire();
    if (client_generations_[index] != generation || !client_retiring_[index] ||
        client_in_flight_[index] || client_cleanup_running_[index]) {
      return;
    }
    client_cleanup_running_[index] = true;
    driver = clients_[index].driver;
    wrapped_arg = clients_[index].wrapped_callback_arg;
  }

  // Output device destruction can join a hardware-paced thread. Neither the
  // global critical region nor the per-slot submission mutex is held here.
  if (driver) DestroyDriver(driver);
  if (wrapped_arg) memory()->SystemHeapFree(wrapped_arg);

  // Drain the semaphore of its count.
  auto client_semaphore = client_semaphores_[index].get();
  rex::thread::WaitResult wait_result;
  do {
    wait_result = rex::thread::Wait(client_semaphore, false, std::chrono::milliseconds(0));
  } while (wait_result == rex::thread::WaitResult::kSuccess);
  assert_true(wait_result == rex::thread::WaitResult::kTimeout);
  {
    auto global_lock = global_critical_region_.Acquire();
    clients_[index] = {nullptr, 0, 0, 0, false};
    client_retiring_[index] = false;
    client_cleanup_running_[index] = false;
    // Publish completion and notify under the same lock. Shutdown may destroy
    // this object as soon as its idle predicate observes the cleared flags.
    clients_idle_.notify_all();
  }
}

bool AudioSystem::Save(stream::ByteStream* stream) {
  stream->Write(kAudioSaveSignature);

  // Count the number of used clients first.
  // Any gaps should be handled gracefully.
  uint32_t used_clients = 0;
  for (size_t i = 0; i < kMaximumClientCount; i++) {
    if (clients_[i].in_use) {
      used_clients++;
    }
  }

  stream->Write(used_clients);
  for (uint32_t i = 0; i < kMaximumClientCount; i++) {
    auto& client = clients_[i];
    if (!client.in_use) {
      continue;
    }

    stream->Write(i);
    stream->Write(client.callback);
    stream->Write(client.callback_arg);
    stream->Write(client.wrapped_callback_arg);
  }

  return true;
}

bool AudioSystem::Restore(stream::ByteStream* stream) {
  if (!stream || stream->offset() > stream->data_length() ||
      stream->data_length() - stream->offset() < 2 * sizeof(uint32_t)) {
    return false;
  }
  if (stream->Read<uint32_t>() != kAudioSaveSignature) {
    REXAPU_ERROR("AudioSystem::Restore - Invalid magic value!");
    return false;
  }

  const uint32_t num_clients = stream->Read<uint32_t>();
  if (num_clients > kMaximumClientCount ||
      num_clients > (stream->data_length() - stream->offset()) / (4 * sizeof(uint32_t))) {
    return false;
  }
  struct SavedClient {
    uint32_t id, callback, callback_arg, wrapped_arg;
  };
  std::array<SavedClient, kMaximumClientCount> saved{};
  std::array<bool, kMaximumClientCount> seen{};
  for (uint32_t i = 0; i < num_clients; ++i) {
    const uint32_t id = stream->Read<uint32_t>();
    if (id >= kMaximumClientCount || seen[id]) return false;
    seen[id] = true;
    saved[i] = {id, stream->Read<uint32_t>(), stream->Read<uint32_t>(),
                stream->Read<uint32_t>()};
  }
  {
    auto global_lock = global_critical_region_.Acquire();
    if (clients_shutting_down_) return false;
    for (uint32_t i = 0; i < num_clients; ++i) {
      const auto id = saved[i].id;
      // Savestate callers must quiesce callbacks/submits before restoring.
      // A retired registration may still own this slot and its driver.
      if (client_in_flight_[id] || client_retiring_[id] || client_cleanup_running_[id]) {
        return false;
      }
    }
  }
  for (uint32_t i = 0; i < num_clients; ++i) {
    const auto [id, callback, callback_arg, wrapped_arg] = saved[i];
    UnregisterClient(id);
    {
      auto global_lock = global_critical_region_.Acquire();
      // Recheck after teardown: a concurrent operation may have retained or
      // reused the slot after the initial validation. Never overwrite it.
      if (clients_shutting_down_ || clients_[id].in_use || client_in_flight_[id] ||
          client_retiring_[id] || client_cleanup_running_[id]) {
        return false;
      }
      client_cleanup_running_[id] = true;
      if (++client_generations_[id] == 0) ++client_generations_[id];
    }
    auto client_semaphore = client_semaphores_[id].get();
    AudioDriver* driver = nullptr;
    auto discard_driver = [&] {
      if (driver) DestroyDriver(std::exchange(driver, nullptr));
      {
        auto cleanup_lock = global_critical_region_.Acquire();
        client_cleanup_running_[id] = false;
        clients_idle_.notify_all();
      }
      return false;
    };
    try {
      const auto status = CreateDriver(id, client_semaphore, &driver);
      if (XFAILED(status) || !driver) return discard_driver();
    } catch (...) {
      discard_driver();
      throw;
    }
    auto global_lock = global_critical_region_.Acquire();
    if (clients_shutting_down_) {
      global_lock.unlock();
      return discard_driver();
    }
    // The restored guest heap owns wrapped_arg until this publication.
    clients_[id] = {driver, callback, callback_arg, wrapped_arg, true};
    client_cleanup_running_[id] = false;
    if (!client_semaphore->Release(queued_frames_, nullptr)) {
      clients_[id].in_use = false;
      client_retiring_[id] = true;
      const uint64_t generation = client_generations_[id];
      global_lock.unlock();
      CleanupRetiredClient(id, generation);
      return false;
    }
  }

  return true;
}

void AudioSystem::Pause() {
  if (paused_) {
    return;
  }
  paused_ = true;

  // Kind of a hack, but it works.
  shutdown_event_->Set();
  pause_fence_.Wait();

  xma_decoder_->Pause();
}

void AudioSystem::Resume() {
  if (!paused_) {
    return;
  }
  paused_ = false;

  resume_event_->Set();

  xma_decoder_->Resume();
}

}  // namespace rex::audio
