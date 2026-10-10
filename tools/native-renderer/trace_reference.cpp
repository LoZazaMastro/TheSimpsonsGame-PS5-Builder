// Reference renderer: plays a GPU frame trace through the emulated Vulkan
// backend, headless, and dumps the resulting frame. This is the ground truth
// the native replayer's output is compared against, draw for draw.
#include <memory>
#include <string>
#include <vector>

#include <cstdlib>
#include <filesystem>
#include <cstdio>

#include <rex/cvar.h>
#include <rex/graphics/flags.h>
#include <rex/memory.h>
#include <rex/graphics/trace_dump.h>
#include <rex/graphics/vulkan/graphics_system.h>

namespace rex::graphics {

class VulkanTraceDump : public TraceDump {
 protected:
  std::unique_ptr<GraphicsSystem> CreateGraphicsSystem() override {
    return std::make_unique<vulkan::VulkanGraphicsSystem>();
  }
  void BeginHostCapture() override {}
  void EndHostCapture() override {
    // With vulkan_readback_resolve on, the emulated backend has written every
    // resolve back into guest memory during playback. Dump those regions so
    // the native replayer's own resolves can be diffed against them pass by
    // pass - the first divergent resolve pinpoints the first wrong pass.
    const char* dump_dir = std::getenv("REF_RESOLVE_DUMP");
    if (!dump_dir || !emulator_) {
      return;
    }
    static const struct { uint32_t base, size; } kRegions[] = {
        {0x06614000, 2949120}, {0x06213000, 2949120}, {0x03153000, 3686400},
        {0x0F654000, 3686400}, {0x0FE36000, 3686400}, {0x03C1E000, 3686400},
        {0x04F70000, 3686400}, {0x04F6C000, 184320},  {0x05308000, 184320},
        {0x034EC000, 3686400},
    };
    std::filesystem::create_directories(dump_dir);
    for (const auto& region : kRegions) {
      const uint8_t* src = emulator_->memory()->TranslatePhysical<const uint8_t*>(region.base);
      if (!src) continue;
      char name[512];
      std::snprintf(name, sizeof(name), "%s/ref_%08X.bin", dump_dir, region.base);
      FILE* f = fopen(name, "wb");
      if (f) {
        fwrite(src, 1, region.size, f);
        fclose(f);
      }
    }
    std::printf("reference resolves dumped to %s\n", dump_dir);
  }
};

}  // namespace rex::graphics

REXCVAR_DECLARE(int32_t, vulkan_pipeline_creation_threads);

REXCVAR_DECLARE(std::string, render_target_path_vulkan);
REXCVAR_DECLARE(bool, vulkan_readback_resolve);
REXCVAR_DECLARE(std::string, aot_shader_path);
REXCVAR_DECLARE(int32_t, native_telemetry);

int main(int argc, char** argv) {
  // REF_RT_PATH overrides the render target path ("", "fsi", "native") so the
  // same trace can be rendered through any implementation headlessly - the
  // host-path color defects reproduce offline this way, no game boot needed.
  if (const char* ref_rt_path = std::getenv("REF_RT_PATH")) {
    REXCVAR_SET(render_target_path_vulkan, ref_rt_path);
  }
  // REF_NATIVE_TELEMETRY exercises the in-game native-path telemetry line
  // against a trace. A trace plays a single frame, so 1 is the useful value.
  if (const char* ref_telemetry = std::getenv("REF_NATIVE_TELEMETRY")) {
    REXCVAR_SET(native_telemetry, atoi(ref_telemetry));
  }
  if (std::getenv("REF_RESOLVE_DUMP")) {
    // Resolves must round-trip through guest memory to be dumpable.
    REXCVAR_SET(vulkan_readback_resolve, true);
  }
  // REF_AOT points at a directory of pre-translated SPIR-V so the runtime's
  // ahead-of-time serving path can be exercised headlessly - the same code the
  // game runs at boot, verifiable without launching it.
  if (const char* ref_aot = std::getenv("REF_AOT")) {
    REXCVAR_SET(aot_shader_path, ref_aot);
  }
  // REF_DUMP_SHADERS dumps the runtime's own translations - the ground truth
  // to diff a suspect ahead-of-time module against.
  if (const char* ref_dump = std::getenv("REF_DUMP_SHADERS")) {
    REXCVAR_SET(dump_shaders, ref_dump);
  }
  // Synchronous pipeline creation: with async placeholders the single traced
  // frame presents before its pipelines finish and the capture is skipped.
  REXCVAR_SET(vulkan_pipeline_creation_threads, 0);
  std::vector<std::string> args;
  for (int i = 0; i < argc; ++i) args.push_back(argv[i]);
  rex::graphics::VulkanTraceDump dump;
  return dump.Main(args);
}
