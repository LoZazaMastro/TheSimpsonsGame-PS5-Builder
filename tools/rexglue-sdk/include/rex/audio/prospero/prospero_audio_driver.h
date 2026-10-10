// BSD 3-Clause License. See the project LICENSE.
#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>

#include <rex/audio/audio_driver.h>
#include <rex/audio/prospero/pcm_conversion.h>
#include <rex/thread.h>

namespace rex::audio::prospero {

class ProsperoAudioDriver final : public AudioDriver {
 public:
  ProsperoAudioDriver(memory::Memory* memory, rex::thread::Semaphore* semaphore);
  ~ProsperoAudioDriver() override;

  bool Initialize();
  void Shutdown();
  void SubmitFrame(uint32_t samples_ptr) override;

 private:
  void OutputThreadMain();
  void CloseDumpFiles();
  void ReportDiagnostics(bool final);

  static constexpr size_t kFrameCapacity = 64;
  rex::thread::Semaphore* semaphore_;
  int32_t port_ = -1;
  std::atomic<bool> stopping_{false};
  std::atomic<bool> failed_{false};
  std::thread output_thread_;
  std::mutex frames_mutex_;
  std::array<PcmFrame, kFrameCapacity> frames_{};
  size_t read_position_ = 0;
  size_t frame_count_ = 0;
  size_t queue_peak_ = 0;  // protected by frames_mutex_
  std::atomic<uint64_t> submit_calls_{0}, submitted_{0}, rejected_{0};
  // Written only by the output worker; Shutdown reads after joining it.
  uint64_t output_successes_ = 0, empty_periods_ = 0, silent_guest_frames_ = 0;
  std::chrono::steady_clock::time_point started_at_{};
  uint32_t diagnostic_slot_ = UINT32_MAX;
  uint64_t diagnostic_instance_ = 0;
  FILE* dump_file_ = nullptr;
  FILE* dump_timestamps_ = nullptr;
};

}  // namespace rex::audio::prospero
