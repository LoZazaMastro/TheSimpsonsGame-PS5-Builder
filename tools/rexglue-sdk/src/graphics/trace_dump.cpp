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

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <string_view>
#include <vector>

#include <rex/cvar.h>
#include <rex/dbg.h>
#include <rex/filesystem.h>
#include <rex/graphics/command_processor.h>
#include <rex/graphics/graphics_system.h>
#include <rex/graphics/trace_dump.h>
#include <rex/logging.h>
#include <rex/memory.h>
#include <rex/string.h>
#include <rex/system/kernel_state.h>
#include <rex/thread.h>
#include <rex/ui/presenter.h>
#include <rex/ui/window.h>


REXCVAR_DEFINE_STRING(target_trace_file, "", "GPU", "Specifies the trace file to load.");
REXCVAR_DEFINE_STRING(trace_dump_path, "", "GPU", "Output path for dumped files.");

namespace rex::graphics {

using namespace rex::graphics::xenos;

TraceDump::TraceDump() = default;

TraceDump::~TraceDump() = default;

int TraceDump::Main(const std::vector<std::string>& args) {
  // TRACE_CVARS="name=value;name=value" sets cvars before the graphics system
  // is created, so any renderer option can be compared on the same trace.
  if (const char* cvars_env = std::getenv("TRACE_CVARS")) {
    std::string_view cvars(cvars_env);
    while (!cvars.empty()) {
      size_t end = cvars.find(';');
      std::string_view assignment = cvars.substr(0, end);
      cvars = end == std::string_view::npos ? std::string_view() : cvars.substr(end + 1);
      size_t equals = assignment.find('=');
      if (equals == std::string_view::npos) {
        continue;
      }
      std::string_view name = assignment.substr(0, equals);
      std::string_view value = assignment.substr(equals + 1);
      if (!rex::cvar::SetFlagByName(name, value)) {
        REXGPU_ERROR("TraceDump: could not set cvar {} to {}", name, value);
      }
    }
  }

  // Grab path from the flag or unnamed argument.
  std::filesystem::path path;
  std::filesystem::path output_path;
  if (!REXCVAR_GET(target_trace_file).empty()) {
    // Passed as a named argument.
    // TODO(benvanik): find something better than gflags that supports
    // unicode.
    path = REXCVAR_GET(target_trace_file);
  } else if (args.size() >= 2) {
    // Passed as an unnamed argument.
    path = rex::to_path(args[1]);

    if (args.size() >= 3) {
      output_path = rex::to_path(args[2]);
    }
  }

  if (path.empty()) {
    REXGPU_ERROR("No trace file specified");
    return 5;
  }

  // Normalize the path and make absolute.
  auto abs_path = std::filesystem::absolute(path);
  REXGPU_INFO("Loading trace file {}...", rex::path_to_utf8(abs_path));

  if (!Setup()) {
    REXGPU_ERROR("Unable to setup trace dump tool");
    return 4;
  }
  if (!Load(std::move(abs_path))) {
    REXGPU_ERROR("Unable to load trace file; not found?");
    return 5;
  }

  // Root file name for outputs.
  if (output_path.empty()) {
    base_output_path_ = REXCVAR_GET(trace_dump_path);
    auto output_name = path.filename().replace_extension();

    base_output_path_ = base_output_path_ / output_name;
  } else {
    base_output_path_ = output_path;
  }

  // Ensure output path exists.
  rex::filesystem::CreateParentFolder(base_output_path_);

  return Run();
}

bool TraceDump::Setup() {
  // Headless runtime with just the graphics backend - no game module. The
  // offscreen presenter must exist before the runtime marks the guest GPU
  // headless, so set up presentation first.
  emulator_ = std::make_unique<Runtime>("", "", "");
  auto graphics = CreateGraphicsSystem();
  auto* graphics_raw = static_cast<GraphicsSystem*>(graphics.get());
  graphics_raw->SetupPresentation(nullptr);
  RuntimeConfig config;
  config.graphics = std::move(graphics);
  config.tool_mode = false;
  X_STATUS result = emulator_->Setup(std::move(config));
  if (XFAILED(result)) {
    REXGPU_ERROR("Failed to setup emulator: {:08X}", result);
    return false;
  }
  graphics_system_ = static_cast<GraphicsSystem*>(emulator_->graphics_system());
  player_ = std::make_unique<TracePlayer>(graphics_system_);
  return true;
}

bool TraceDump::Load(const std::filesystem::path& trace_file_path) {
  trace_file_path_ = trace_file_path;

  if (!player_->Open(rex::path_to_utf8(trace_file_path_))) {
    REXGPU_ERROR("Could not load trace file");
    return false;
  }

  return true;
}

int TraceDump::Run() {
  // TRACE_SHADER_STORAGE=<cache root>:<title ID, hex>: load a shader storage
  // first, as the game does at boot - every stored shader translated, every
  // stored pipeline created (with aot_export_path, to export a complete
  // ahead-of-time shader set). The storage is appended to, so pass a copy.
  if (const char* storage_env = std::getenv("TRACE_SHADER_STORAGE")) {
    std::string storage_spec(storage_env);
    size_t separator = storage_spec.rfind(':');
    if (separator != std::string::npos) {
      uint32_t title_id =
          uint32_t(std::strtoul(storage_spec.c_str() + separator + 1, nullptr, 16));
      std::filesystem::path cache_root(storage_spec.substr(0, separator));
      REXGPU_INFO("TraceDump: loading the shader storage of title {:08X} from {}", title_id,
                  cache_root.string());
      graphics_system_->InitializeShaderStorage(cache_root, title_id, true);
    }
  }
  BeginHostCapture();
  const TraceReader::Frame* frame = player_->frame_count() ? player_->frame(0) : nullptr;
  if (!frame || frame->commands.empty()) {
    REXGPU_ERROR("TraceDump: the trace has no frame to play");
    return 1;
  }
  // TRACE_STOP: stop playback at an arbitrary command index instead of the
  // end of the frame - lets a reference renderer dump the EDRAM state at any
  // point mid-frame for divergence bisection against a native replayer.
  int stop_command = static_cast<int>(frame->commands.size() - 1);
  if (const char* stop_env = std::getenv("TRACE_STOP")) {
    int requested = atoi(stop_env);
    if (requested >= 0 && requested < stop_command) stop_command = requested;
    REXGPU_INFO("TraceDump: stopping at command {} of {}", stop_command, frame->commands.size());
  }
  player_->ReplayFrame(0, stop_command, false);
  player_->WaitOnPlayback();
  EndHostCapture();

  // TRACE_BENCH: replay the same frame N more times and report the steady-state
  // cost. A single playback is swamped by process and device setup (over a
  // second), which makes wall-clock comparisons between render target paths
  // meaningless. Looping amortizes that away and turns this tool into an
  // offline benchmark: the same frame, the same GPU, one variable at a time.
  if (const char* bench_env = std::getenv("TRACE_BENCH")) {
    int iterations = atoi(bench_env);
    if (iterations > 0) {
      // One warm-up pass is already done above (pipelines built, textures
      // resident), so every timed iteration is steady state.
      std::vector<double> samples;
      samples.reserve(size_t(iterations));
      for (int i = 0; i < iterations; ++i) {
        auto t0 = std::chrono::steady_clock::now();
        player_->ReplayFrame(0, stop_command, true);
        player_->WaitOnPlayback();
        auto t1 = std::chrono::steady_clock::now();
        samples.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
      }
      std::sort(samples.begin(), samples.end());
      double total = 0.0;
      for (double s : samples) total += s;
      REXGPU_INFO(
          "TraceDump: bench {} iterations - mean {:.2f} ms, median {:.2f} ms, "
          "min {:.2f} ms, max {:.2f} ms",
          iterations, total / double(samples.size()), samples[samples.size() / 2],
          samples.front(), samples.back());
    }
  }

  // Capture.
  int result = 0;
  ui::Presenter* presenter = graphics_system_->presenter();
  ui::RawImage raw_image;
  if (presenter && presenter->CaptureGuestOutput(raw_image)) {
    // Save the framebuffer as a PPM - no image library needed, and every
    // viewer and diff tool reads it.
    auto ppm_path = base_output_path_.replace_extension(".ppm");
    auto handle = filesystem::OpenFile(ppm_path, "wb");
    fprintf(handle, "P6\n%u %u\n255\n", uint32_t(raw_image.width), uint32_t(raw_image.height));
    for (uint32_t y = 0; y < raw_image.height; ++y) {
      const uint8_t* row = raw_image.data.data() + y * raw_image.stride;
      for (uint32_t x = 0; x < raw_image.width; ++x) {
        fputc(row[x * 4 + 0], handle);
        fputc(row[x * 4 + 1], handle);
        fputc(row[x * 4 + 2], handle);
      }
    }
    fclose(handle);
  } else {
    result = 1;
  }

  player_.reset();
  emulator_.reset();
  return result;
}

}  // namespace rex::graphics
