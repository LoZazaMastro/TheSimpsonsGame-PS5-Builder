// Standalone extraction of the runtime's generated pass-through geometry
// shaders (point sprites, rectangle lists, quad lists). The generation logic
// is the emulated backend's own, lifted verbatim from the Vulkan pipeline
// cache with the device queries pinned to Vulkan 1.2+ values; keeping it
// byte-compatible means the native renderer's expansion of these primitives
// is exactly the shipping behavior.
#include <cstdint>
#include <memory>
#include <vector>

#include <rex/graphics/pipeline/shader/spirv_builder.h>
#include <rex/graphics/pipeline/shader/spirv_translator.h>

namespace rex::graphics::nativegs {

enum class PipelineGeometryShader : uint32_t {
  kNone,
  kPointList,
  kRectangleList,
  kQuadList,
};

union GeometryShaderKey {
  uint32_t key;
  struct {
    PipelineGeometryShader type : 2;
    uint32_t interpolator_count : 5;
    uint32_t user_clip_plane_count : 3;
    uint32_t user_clip_plane_cull : 1;
    uint32_t has_vertex_kill_and : 1;
    uint32_t has_point_size : 1;
    uint32_t has_point_coordinates : 1;
    uint32_t point_ps_ucp_mode : 2;
  };
  GeometryShaderKey() : key(0) {}
};

std::vector<unsigned int> GenerateGeometryShader(GeometryShaderKey key) {
#include "gs_gen_body.inc"
  return shader_code;
}

}  // namespace rex::graphics::nativegs
