// BSD 3-Clause License. See the project LICENSE.
#include <rex/platform/settings_prospero.h>
#include <rex/audio/prospero/prospero_audio_driver.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <system_error>

#include <rex/audio/flags.h>
#include <rex/audio/prospero/audio_out.h>
#include <rex/audio/thread_priority.h>
#include <rex/logging.h>
#include <rex/platform.h>
#include <rex/platform/trace_game_prospero.h>
#include <rex/platform/audio_metrics_prospero.h>

#if !REX_PLATFORM_PROSPERO
#error The Prospero audio backend requires a native PS5 build.
#endif

namespace rex::audio::prospero {

namespace {
// Plain snapshots are protected by a single try-claim, never a spinning lock.
// Only numeric identities are retained; the metrics worker never calls back
// into, or dereferences, an AudioDriver that may be shutting down.
struct AudioSnapshot {
  uint64_t driver = 0, instance = 0, captured_ns = 0, elapsed_ms = 0;
  uint64_t submit_calls = 0, submitted = 0, rejected = 0;
  uint64_t outputs = 0, empty = 0, silent = 0, queued = 0, peak = 0;
  bool mute = false, failed = false, final = false;
};
struct AudioDiagnosticSlot {
  std::atomic_flag busy = ATOMIC_FLAG_INIT;
  std::atomic<bool> active{false};
  bool dirty = false;
  AudioSnapshot value{};
};
constexpr uint32_t kDiagnosticSlots = 64;
constinit AudioDiagnosticSlot audio_diagnostics[kDiagnosticSlots]{};
constinit std::atomic<uint64_t> next_instance{1}, snapshot_dropped{0}, slot_unavailable{0},
    snapshots_replaced{0}, reporter_busy{0};

uint32_t ClaimDiagnosticSlot(uint64_t instance) {
  for (uint32_t i = 0; i < kDiagnosticSlots; ++i) {
    auto& slot = audio_diagnostics[i];
    if (slot.busy.test_and_set(std::memory_order_acquire)) continue;
    if (!slot.active.load(std::memory_order_relaxed)) {
      if (slot.dirty) snapshots_replaced.fetch_add(1, std::memory_order_relaxed);
      slot.value = {};
      slot.value.instance = instance;
      slot.dirty = false;
      slot.active.store(true, std::memory_order_release);
      slot.busy.clear(std::memory_order_release);
      return i;
    }
    slot.busy.clear(std::memory_order_release);
  }
  slot_unavailable.fetch_add(1, std::memory_order_relaxed);
  return UINT32_MAX;
}

void WriteAudioSnapshot(const AudioSnapshot& value) {
  prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_STATS, value.outputs, value.empty, value.silent);
  prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_QUEUE, value.submitted, value.rejected,
                            (value.peak << 32) | value.queued);
  rex::prospero::diagnostics::Line line("audio");
  line.Number("driver", value.driver); line.Number("instance", value.instance);
  line.Number("captured_ns", value.captured_ns); line.Number("final", value.final);
  line.Number("elapsed_ms", value.elapsed_ms); line.Number("submit_calls", value.submit_calls);
  line.Number("submitted", value.submitted); line.Number("rejected", value.rejected);
  line.Number("output_successes", value.outputs); line.Number("empty_queue_outputs", value.empty);
  line.Number("silent_guest_outputs", value.silent); line.Number("queued", value.queued);
  line.Number("peak_queued", value.peak); line.Number("mute", value.mute);
  line.Number("failed", value.failed); line.Write();
}
}  // namespace

ProsperoAudioDriver::ProsperoAudioDriver(memory::Memory* memory,
                                         rex::thread::Semaphore* semaphore)
    : AudioDriver(memory), semaphore_(semaphore) {}

ProsperoAudioDriver::~ProsperoAudioDriver() {
  Shutdown();
}

bool ProsperoAudioDriver::Initialize() {
  started_at_ = std::chrono::steady_clock::now();
  prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_INIT, 0, 0, 0);
  const int32_t initialized = sceAudioOutInit();
  if (initialized < 0 && uint32_t(initialized) != kAudioAlreadyInitialized) {
    prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_ERROR, 0, uint32_t(initialized), 0);
    REXAPU_ERROR("PS5 sceAudioOutInit failed: 0x{:08X}", uint32_t(initialized));
    return false;
  }
  port_ = sceAudioOutOpen(kAudioSystemUser, kAudioPortMain, 0, kSamplesPerFrame,
                          kSampleRate, kAudioFormatS16Stereo);
  prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_INIT, 1, uint32_t(port_), uint32_t(initialized));
  if (port_ < 0) {
    prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_ERROR, 1, uint32_t(port_), 0);
    REXAPU_ERROR("PS5 sceAudioOutOpen (48 kHz, S16 stereo, 256 samples) failed: 0x{:08X}",
                 uint32_t(port_));
    return false;
  }
  // Verify output before publishing the driver to a guest client. A stub or
  // an unusable device must not report a successful silent audio backend.
  alignas(16) const PcmFrame initial_silence{};
  const int32_t output = sceAudioOutOutput(port_, initial_silence.data());
  if (output < 0) {
    prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_ERROR, 2, uint32_t(output), uint32_t(port_));
    REXAPU_ERROR("PS5 initial sceAudioOutOutput failed: 0x{:08X}", uint32_t(output));
    Shutdown();
    return false;
  }

  const auto dump_path = REXCVAR_GET(audio_dump_file);
  if (!dump_path.empty()) {
    dump_file_ = std::fopen(dump_path.c_str(), "wb");
    dump_timestamps_ = std::fopen((dump_path + ".ts").c_str(), "wb");
    if (!dump_file_ || !dump_timestamps_) {
      prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_ERROR, 8, uint32_t(errno), uint32_t(port_));
      REXAPU_ERROR("PS5 audio dump could not open {} and its timestamp file", dump_path);
      CloseDumpFiles();
    }
  }

  stopping_ = false;
  failed_ = false;
  diagnostic_instance_ = next_instance.fetch_add(1, std::memory_order_relaxed);
  diagnostic_slot_ = ClaimDiagnosticSlot(diagnostic_instance_);
  try {
    output_thread_ = std::thread(&ProsperoAudioDriver::OutputThreadMain, this);
  } catch (const std::system_error& error) {
    prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_ERROR, 3, uint32_t(error.code().value()),
                              uint32_t(port_));
    REXAPU_ERROR("PS5 audio output thread could not start: {}", error.what());
    Shutdown();
    return false;
  }
  REXAPU_INFO("PS5 audio: 48 kHz S16 stereo, guest FL/FR/FC/LFE/BL/BR downmix");
  prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_INIT, 2, uint32_t(port_), 0);
  return true;
}

void ProsperoAudioDriver::CloseDumpFiles() {
  if (dump_file_) {
    std::fclose(dump_file_);
    dump_file_ = nullptr;
  }
  if (dump_timestamps_) {
    std::fclose(dump_timestamps_);
    dump_timestamps_ = nullptr;
  }
}

void ProsperoAudioDriver::Shutdown() {
  const bool had_device = port_ >= 0 || output_thread_.joinable();
  stopping_ = true;
  // AudioOut paces each blocking output call to a 256/48000 s hardware
  // period. Join before closing its handle or destroying its buffers.
  if (output_thread_.joinable()) {
    output_thread_.join();
  }
  if (had_device) {
    ReportDiagnostics(true);
    prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_SHUTDOWN, uintptr_t(this),
        submitted_.load(std::memory_order_relaxed), rejected_.load(std::memory_order_relaxed));
  }
  if (port_ >= 0) {
    const int32_t result = sceAudioOutClose(port_);
    if (result < 0) {
      prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_ERROR, 7, uint32_t(result), uint32_t(port_));
      REXAPU_WARN("PS5 sceAudioOutClose failed: 0x{:08X}", uint32_t(result));
    }
    port_ = -1;
  }
  {
    std::lock_guard<std::mutex> lock(frames_mutex_);
    frame_count_ = 0;
    read_position_ = 0;
  }
  CloseDumpFiles();
}

void ProsperoAudioDriver::SubmitFrame(uint32_t samples_ptr) {
  if (submit_calls_.fetch_add(1, std::memory_order_relaxed) == 0) {
    prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_FIRST_SUBMIT, uintptr_t(this), samples_ptr, 0);
  }
  if (stopping_ || failed_) {
    rejected_.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  // Copy the guest frame on submission: the mixer may reuse its planar
  // source buffer before AudioOut consumes the queued frame.
  const float* samples = memory_->TranslateVirtual<float*>(samples_ptr);
  PcmFrame frame = ConvertGuestFrame(samples);
  if (dump_file_) {
    const uint64_t timestamp = uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
    const bool ok =
        std::fwrite(samples, sizeof(float), kSamplesPerFrame * kGuestChannels, dump_file_) ==
            kSamplesPerFrame * kGuestChannels &&
        std::fwrite(&timestamp, sizeof(timestamp), 1, dump_timestamps_) == 1;
    if (!ok) {
      prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_ERROR, 8, uint32_t(errno), uint32_t(port_));
      REXAPU_ERROR("PS5 audio dump write failed; disabling dump");
      CloseDumpFiles();
    }
  }
  std::lock_guard<std::mutex> lock(frames_mutex_);
  if (stopping_ || failed_) {
    rejected_.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  if (frame_count_ == kFrameCapacity) {
    // The client semaphore permits at most 64 outstanding frames. A full
    // queue here is a broken accounting contract, not permission to overwrite
    // audio still in use or synthesize extra credits.
    REXAPU_ERROR("PS5 audio queue overflow; stopping this audio device");
    rejected_.fetch_add(1, std::memory_order_relaxed);
    prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_ERROR, 6, frame_count_, uint32_t(port_));
    failed_ = true;
    return;
  }
  frames_[(read_position_ + frame_count_) % kFrameCapacity] = frame;
  ++frame_count_;
  queue_peak_ = std::max(queue_peak_, frame_count_);
  submitted_.fetch_add(1, std::memory_order_relaxed);
}

void ProsperoAudioDriver::ReportDiagnostics(bool final) {
  AudioSnapshot value;
  value.driver = uintptr_t(this); value.instance = diagnostic_instance_;
  const auto now = std::chrono::steady_clock::now();
  value.captured_ns = uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
      now.time_since_epoch()).count());
  value.elapsed_ms = uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
      now - started_at_).count());
  {
    std::lock_guard<std::mutex> lock(frames_mutex_);
    value.queued = frame_count_;
    value.peak = queue_peak_;
  }
  value.submit_calls = submit_calls_.load(std::memory_order_relaxed);
  value.submitted = submitted_.load(std::memory_order_relaxed);
  value.rejected = rejected_.load(std::memory_order_relaxed);
  value.outputs = output_successes_;
  // A queue-empty output is locally generated silence. This is not a PS5
  // hardware underrun measurement, nor proof that the guest should be audible.
  value.empty = empty_periods_; value.silent = silent_guest_frames_;
  value.mute = rex::platform::prospero::GetLiveSettings().Applied(
      rex::platform::prospero::LiveSetting::kMute).value() != 0; value.failed = failed_.load(std::memory_order_relaxed);
  value.final = final;
  if (final) {
    // Shutdown has already joined AudioOut. One final normal-context write is
    // safe here and does not wait for another metrics interval before exit.
    WriteAudioSnapshot(value);
    if (diagnostic_slot_ < kDiagnosticSlots) {
      auto& slot = audio_diagnostics[diagnostic_slot_];
      if (!slot.busy.test_and_set(std::memory_order_acquire)) {
        slot.dirty = false;
        slot.busy.clear(std::memory_order_release);
      }
      slot.active.store(false, std::memory_order_release);
      diagnostic_slot_ = UINT32_MAX;
    }
    return;
  }
  // The 5.33 ms paced AudioOut worker performs no periodic diagnostic disk I/O.
  if (diagnostic_slot_ >= kDiagnosticSlots) return;
  auto& slot = audio_diagnostics[diagnostic_slot_];
  if (slot.busy.test_and_set(std::memory_order_acquire)) {
    snapshot_dropped.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  if (slot.dirty) snapshots_replaced.fetch_add(1, std::memory_order_relaxed);
  slot.value = value;
  slot.dirty = true;
  slot.busy.clear(std::memory_order_release);
}

void ProsperoAudioDriver::OutputThreadMain() {
  prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_WORKER, uintptr_t(this), 1, uint32_t(port_));
  ReportDiagnostics(false);
  if (!rex_audio_set_thread_priority()) {
    REXAPU_WARN("PS5 AudioOut worker uses its inherited thread priority");
  }
  auto report_at = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (!stopping_ && !failed_) {
    alignas(16) PcmFrame output{};
    bool had_frame = false;
    {
      std::lock_guard<std::mutex> lock(frames_mutex_);
      if (frame_count_) {
        output = frames_[read_position_];
        read_position_ = (read_position_ + 1) % kFrameCapacity;
        --frame_count_;
        had_frame = true;
      }
    }
    const bool silent_guest = had_frame &&
        std::all_of(output.begin(), output.end(), [](int16_t v) { return v == 0; });
    if (rex::platform::prospero::GetLiveSettings().Consume(
            rex::platform::prospero::LiveSetting::kMute)) {
      output.fill(0);
    }
    const int32_t result = sceAudioOutOutput(port_, output.data());
    if (result < 0) {
      failed_ = true;
      prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_ERROR, 4, uint32_t(result), uint32_t(port_));
      REXAPU_ERROR("PS5 sceAudioOutOutput failed: 0x{:08X}; audio client stopped", uint32_t(result));
      // Stop requesting guest frames for a failed device. Reporting empty
      // buffers as successfully played would hide a native output failure.
      break;
    }
    ++output_successes_;
    if (!had_frame) ++empty_periods_;
    if (silent_guest) ++silent_guest_frames_;
    if (had_frame && !stopping_) {
      if (!semaphore_->Release(1, nullptr)) {
        failed_ = true;
        prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_ERROR, 5, 0, uint32_t(port_));
        REXAPU_ERROR("PS5 audio frame semaphore overflow; audio client stopped");
        break;
      }
    }
    const auto now = std::chrono::steady_clock::now();
    if (now >= report_at) {
      ReportDiagnostics(false);
      report_at = now + std::chrono::seconds(5);
    }
  }
  ReportDiagnostics(false);
  prospero_trace_checkpoint(PROSPERO_GAME_AUDIO_WORKER, uintptr_t(this), 0, uint32_t(port_));
}

}  // namespace rex::audio::prospero

extern "C" void prospero_audio_report_diagnostics(void) noexcept {
  using namespace rex::audio::prospero;
  for (auto& slot : audio_diagnostics) {
    if (slot.busy.test_and_set(std::memory_order_acquire)) {
      reporter_busy.fetch_add(1, std::memory_order_relaxed);
      continue;
    }
    const bool dirty = slot.dirty;
    const AudioSnapshot value = slot.value;
    slot.dirty = false;
    slot.busy.clear(std::memory_order_release);
    if (dirty) WriteAudioSnapshot(value);
  }
  // Always explicit, including zero: readers can distinguish unavailable
  // snapshots from a genuinely idle or silent audio device.
  rex::prospero::diagnostics::Line line("audio_diagnostics");
  line.Number("snapshot_dropped", snapshot_dropped.load(std::memory_order_relaxed));
  line.Number("slots_unavailable", slot_unavailable.load(std::memory_order_relaxed));
  line.Number("unreported_replaced", snapshots_replaced.load(std::memory_order_relaxed));
  line.Number("reporter_busy", reporter_busy.load(std::memory_order_relaxed));
  line.Write();
}
