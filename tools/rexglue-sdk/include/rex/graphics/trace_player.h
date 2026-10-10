#pragma once
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

#include <atomic>
#include <string>
#include <vector>

#include <rex/graphics/trace_protocol.h>
#include <rex/graphics/trace_reader.h>
#include <rex/thread.h>

namespace rex::graphics {

class GraphicsSystem;

enum class TracePlaybackMode {
  kUntilEnd,
  kBreakOnSwap,
};

class TracePlayer : public TraceReader {
 public:
  TracePlayer(GraphicsSystem* graphics_system);

  GraphicsSystem* graphics_system() const { return graphics_system_; }
  int current_frame_index() const { return current_frame_index_; }
  int current_command_index() const { return current_command_index_; }
  bool is_playing_trace() const { return playing_trace_; }
  const Frame* current_frame() const;

  // Only valid if playing_trace is true.
  // Scalar from 0-10000
  uint32_t playback_percent() const { return playback_percent_; }

  void SeekFrame(int target_frame);
  void SeekCommand(int target_command);

  // Plays the frame from its start up to and including the command, even if
  // that's the current position, keeping the caches. With
  // skip_unchanged_memory, memory the trace would overwrite with the bytes it
  // already holds is left alone, so cached copies of it stay valid - a replay
  // then behaves like the next frame of a running game, not a cold start.
  void ReplayFrame(int target_frame, int target_command, bool skip_unchanged_memory);

  void WaitOnPlayback();

 private:
  void PlayTrace(const uint8_t* trace_data, size_t trace_size, TracePlaybackMode playback_mode,
                 bool clear_caches, bool skip_unchanged_memory = false);
  void PlayTraceOnThread(const uint8_t* trace_data, size_t trace_size,
                         TracePlaybackMode playback_mode, bool clear_caches,
                         bool skip_unchanged_memory);

  GraphicsSystem* graphics_system_;
  int current_frame_index_;
  int current_command_index_;
  bool playing_trace_ = false;
  std::atomic<uint32_t> playback_percent_ = {0};
  std::unique_ptr<rex::thread::Event> playback_event_;
  // Only used on the command processor thread.
  std::vector<uint8_t> memory_compare_buffer_;
};

}  // namespace rex::graphics
