// Headless pipeline-build harness for the native GPU renderer.
//
// Takes the AOT-compiled SPIR-V and the manifest, stands up a real Vulkan
// device on the machine's GPU with the same features the game requests, builds
// the descriptor set and pipeline layouts the reflection derived, and compiles
// a graphics pipeline for every (vertex, pixel) pair the game was observed to
// draw with.
//
// This is the proof that the driver accepts the offline-translated shaders --
// pipeline creation is where a driver rejects a module in practice, not
// vkCreateShaderModule -- and it measures how long compiling the full set
// takes, which is what the native renderer will pay once at load time instead
// of hitching mid-game.
//
// The translated vertex shaders read their vertex data out of the shared
// memory buffer themselves, addressed by gl_VertexIndex (verified: no vertex
// shader declares any Location input), so every pipeline uses empty vertex
// input state. There is no vertex attribute translation anywhere.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#define VK_NO_PROTOTYPES
#include <volk.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

std::vector<uint32_t> ReadWords(const fs::path& p) {
  std::ifstream f(p, std::ios::binary | std::ios::ate);
  if (!f) {
    return {};
  }
  std::vector<uint32_t> words(size_t(f.tellg()) / sizeof(uint32_t));
  f.seekg(0);
  f.read(reinterpret_cast<char*>(words.data()), std::streamsize(words.size() * 4));
  return words;
}

#define CHECK_VK(expr)                                                          \
  do {                                                                          \
    VkResult check_vk_result_ = (expr);                                         \
    if (check_vk_result_ != VK_SUCCESS) {                                       \
      std::fprintf(stderr, "%s failed: %d (line %d)\n", #expr,                  \
                   int(check_vk_result_), __LINE__);                            \
      return 1;                                                                 \
    }                                                                           \
  } while (0)

VkDescriptorType ToVkType(const std::string& t) {
  if (t == "STORAGE_BUFFER") return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  if (t == "UNIFORM_BUFFER") return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  if (t == "SAMPLED_IMAGE") return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  if (t == "SAMPLER") return VK_DESCRIPTOR_TYPE_SAMPLER;
  if (t == "COMBINED_IMAGE_SAMPLER") return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  if (t == "STORAGE_IMAGE") return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

// --- Recorded-state translation ---------------------------------------------
// Maps the guest pipeline state recorded by pipeline_inventory_json (the
// engine's PipelineDescription fields, dumped verbatim at shutdown) onto
// Vulkan. This is the same mapping the emulated backend performs per draw;
// here it happens once per recorded pipeline, offline. Enum orderings verified
// against rex/graphics/xenos.h and the Vulkan headers: blend factors, compare
// ops, stencil ops and polygon modes are value-identical and cast directly.

VkBlendOp ToVkBlendOp(uint32_t guest) {
  // xenos::BlendOp: add, subtract, min, max, rev_subtract.
  static const VkBlendOp kMap[] = {VK_BLEND_OP_ADD, VK_BLEND_OP_SUBTRACT, VK_BLEND_OP_MIN,
                                   VK_BLEND_OP_MAX, VK_BLEND_OP_REVERSE_SUBTRACT};
  return guest < 5 ? kMap[guest] : VK_BLEND_OP_ADD;
}

VkPrimitiveTopology ToVkTopology(uint32_t guest) {
  // PipelinePrimitiveTopology; identical to Vulkan except patch list.
  static const VkPrimitiveTopology kMap[] = {
      VK_PRIMITIVE_TOPOLOGY_POINT_LIST,     VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
      VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,     VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
      VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY, VK_PRIMITIVE_TOPOLOGY_PATCH_LIST};
  return guest < 8 ? kMap[guest] : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

// Mirrors VulkanRenderTargetCache::GetColorVulkanFormat with the shipping
// configuration: gamma render targets stored as unorm16, no 16-bit fallbacks
// (RADV supports the SNORM16 formats).
VkFormat GuestColorFormat(uint32_t guest) {
  switch (guest) {
    case 0: return VK_FORMAT_R8G8B8A8_UNORM;                 // k_8_8_8_8
    case 1: return VK_FORMAT_R16G16B16A16_UNORM;             // k_8_8_8_8_GAMMA
    case 2: case 10: return VK_FORMAT_A8B8G8R8_UNORM_PACK32; // k_2_10_10_10(_AS_...)
    case 3: case 12: return VK_FORMAT_R16G16B16A16_SFLOAT;   // k_2_10_10_10_FLOAT(_AS_...)
    case 4: return VK_FORMAT_R16G16_SNORM;                   // k_16_16
    case 5: return VK_FORMAT_R16G16B16A16_SNORM;             // k_16_16_16_16
    case 6: return VK_FORMAT_R16G16_SFLOAT;                  // k_16_16_FLOAT
    case 7: return VK_FORMAT_R16G16B16A16_SFLOAT;            // k_16_16_16_16_FLOAT
    case 14: return VK_FORMAT_R32_SFLOAT;                    // k_32_FLOAT
    case 15: return VK_FORMAT_R32G32_SFLOAT;                 // k_32_32_FLOAT
    default: return VK_FORMAT_UNDEFINED;
  }
}

std::vector<json> ReadInventory(const fs::path& p) {
  std::vector<json> records;
  std::ifstream f(p);
  std::string line;
  while (std::getline(f, line)) {
    if (line.empty()) {
      continue;
    }
    records.push_back(json::parse(line, nullptr, false));
    if (records.back().is_discarded()) {
      records.pop_back();
    }
  }
  return records;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: pipeline_harness <spv-dir>\n");
    return 2;
  }
  fs::path dir = argv[1];
  json manifest;
  {
    std::ifstream mf(dir / "manifest.json");
    if (!mf) {
      std::fprintf(stderr, "no manifest.json in %s\n", dir.string().c_str());
      return 1;
    }
    mf >> manifest;
  }

  CHECK_VK(volkInitialize());

  VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app_info.pApplicationName = "native-renderer-harness";
  app_info.apiVersion = VK_API_VERSION_1_3;
  VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  instance_info.pApplicationInfo = &app_info;
  VkInstance instance;
  CHECK_VK(vkCreateInstance(&instance_info, nullptr, &instance));
  volkLoadInstance(instance);

  uint32_t gpu_count = 0;
  CHECK_VK(vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr));
  if (!gpu_count) {
    std::fprintf(stderr, "no Vulkan devices\n");
    return 1;
  }
  std::vector<VkPhysicalDevice> gpus(gpu_count);
  CHECK_VK(vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data()));
  VkPhysicalDevice gpu = gpus[0];

  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(gpu, &props);
  std::printf("device: %s (Vulkan %u.%u.%u)\n", props.deviceName,
              VK_API_VERSION_MAJOR(props.apiVersion), VK_API_VERSION_MINOR(props.apiVersion),
              VK_API_VERSION_PATCH(props.apiVersion));

  // Enable exactly what the device supports: querying the full feature chain
  // and passing it straight back enables everything available, which is a
  // superset of what the runtime requests, so anything the modules need that
  // the game's own device would have is on.
  VkPhysicalDeviceVulkan13Features features13 = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
  VkPhysicalDeviceVulkan12Features features12 = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
  features12.pNext = &features13;
  VkPhysicalDeviceVulkan11Features features11 = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
  features11.pNext = &features12;
  VkPhysicalDeviceFeatures2 features2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
  features2.pNext = &features11;
  vkGetPhysicalDeviceFeatures2(gpu, &features2);

  if (!features12.uniformBufferStandardLayout || !features12.scalarBlockLayout ||
      !features13.dynamicRendering) {
    std::fprintf(stderr,
                 "device lacks required features (uniformBufferStandardLayout=%d, "
                 "scalarBlockLayout=%d, dynamicRendering=%d)\n",
                 features12.uniformBufferStandardLayout, features12.scalarBlockLayout,
                 features13.dynamicRendering);
    return 1;
  }

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, nullptr);
  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_families.data());
  uint32_t queue_family = UINT32_MAX;
  for (uint32_t i = 0; i < queue_family_count; ++i) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      queue_family = i;
      break;
    }
  }
  if (queue_family == UINT32_MAX) {
    std::fprintf(stderr, "no graphics queue\n");
    return 1;
  }

  float priority = 1.0f;
  VkDeviceQueueCreateInfo queue_info = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queue_info.queueFamilyIndex = queue_family;
  queue_info.queueCount = 1;
  queue_info.pQueuePriorities = &priority;
  VkDeviceCreateInfo device_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  device_info.pNext = &features2;
  device_info.queueCreateInfoCount = 1;
  device_info.pQueueCreateInfos = &queue_info;
  VkDevice device;
  CHECK_VK(vkCreateDevice(gpu, &device_info, nullptr, &device));
  volkLoadDevice(device);

  // --- Shader modules ---------------------------------------------------------
  struct ShaderEntry {
    VkShaderModule module = VK_NULL_HANDLE;
    std::string texture_signature;
    uint32_t writes_color_targets = 0;
  };
  // key: "<hash>_<type>_<modification>"
  std::map<std::string, ShaderEntry> modules;

  auto shader_key = [](const json& s) {
    return s["hash"].get<std::string>() + "_" + s["type"].get<std::string>() + "_" +
           s["modification"].get<std::string>();
  };

  size_t module_count = 0;
  for (const auto& s : manifest["shaders"]) {
    std::vector<uint32_t> words = ReadWords(dir / s["spirv"].get<std::string>());
    if (words.empty()) {
      std::fprintf(stderr, "missing spv: %s\n", s["spirv"].get<std::string>().c_str());
      return 1;
    }
    VkShaderModuleCreateInfo module_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    module_info.codeSize = words.size() * 4;
    module_info.pCode = words.data();
    ShaderEntry entry;
    CHECK_VK(vkCreateShaderModule(device, &module_info, nullptr, &entry.module));
    entry.texture_signature = s.value("texture_set_signature", "");
    entry.writes_color_targets = s.value("writes_color_targets", 0u);
    modules[shader_key(s)] = entry;
    ++module_count;
  }
  std::printf("shader modules created: %zu\n", module_count);

  // --- Descriptor set layouts -------------------------------------------------
  // Shared sets from the manifest's reflection union.
  std::map<uint32_t, VkDescriptorSetLayout> shared_sets;
  for (const auto& set : manifest["descriptor_sets"]) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (const auto& b : set["bindings"]) {
      VkDescriptorSetLayoutBinding binding = {};
      binding.binding = b["binding"].get<uint32_t>();
      binding.descriptorType = ToVkType(b["type"].get<std::string>());
      binding.descriptorCount = b["count"].get<uint32_t>();
      binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
      bindings.push_back(binding);
    }
    VkDescriptorSetLayoutCreateInfo layout_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = uint32_t(bindings.size());
    layout_info.pBindings = bindings.data();
    VkDescriptorSetLayout layout;
    CHECK_VK(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &layout));
    shared_sets[set["set"].get<uint32_t>()] = layout;
  }

  // One empty layout to fill unused set slots (set 2 is never used; set 3 is
  // absent for the 103 shaders with no textures).
  VkDescriptorSetLayout empty_set;
  {
    VkDescriptorSetLayoutCreateInfo layout_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    CHECK_VK(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &empty_set));
  }

  // Per-texture-signature layouts, built from any one shader carrying that
  // signature (they are identical by construction of the signature).
  std::map<std::string, VkDescriptorSetLayout> texture_sets;
  texture_sets[""] = empty_set;
  for (const auto& s : manifest["shaders"]) {
    std::string signature = s.value("texture_set_signature", "");
    if (signature.empty() || texture_sets.count(signature)) {
      continue;
    }
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (const auto& d : s["descriptors"]) {
      if (d["set"].get<uint32_t>() < 2) {
        continue;
      }
      VkDescriptorSetLayoutBinding binding = {};
      binding.binding = d["binding"].get<uint32_t>();
      binding.descriptorType = ToVkType(d["type"].get<std::string>());
      binding.descriptorCount = d["count"].get<uint32_t>();
      binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
      bindings.push_back(binding);
    }
    VkDescriptorSetLayoutCreateInfo layout_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = uint32_t(bindings.size());
    layout_info.pBindings = bindings.data();
    VkDescriptorSetLayout layout;
    CHECK_VK(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &layout));
    texture_sets[signature] = layout;
  }
  std::printf("descriptor set layouts: %zu shared + %zu texture + 1 empty\n", shared_sets.size(),
              texture_sets.size() - 1);

  // Pipeline layout per texture signature: [shared0, shared1, empty, textures].
  std::map<std::string, VkPipelineLayout> pipeline_layouts;
  for (const auto& [signature, texture_layout] : texture_sets) {
    VkDescriptorSetLayout sets[4] = {shared_sets.at(0), shared_sets.at(1), empty_set,
                                     texture_layout};
    VkPipelineLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_info.setLayoutCount = signature.empty() ? 2 : 4;
    layout_info.pSetLayouts = sets;
    VkPipelineLayout layout;
    CHECK_VK(vkCreatePipelineLayout(device, &layout_info, nullptr, &layout));
    pipeline_layouts[signature] = layout;
  }

  // --- Pipelines --------------------------------------------------------------
  // Formats matching what the game renders: 1280x720, BGRA8 color, D24S8-class
  // depth. RADV has no D24, the runtime uses D32S8 there; do the same.
  const VkFormat color_format = VK_FORMAT_B8G8R8A8_UNORM;
  const VkFormat depth_format = VK_FORMAT_D32_SFLOAT_S8_UINT;

  auto find_shader = [&](const std::string& type, const json& p,
                         const char* hash_field, const char* mod_field) -> ShaderEntry* {
    std::string key =
        p[hash_field].get<std::string>() + "_" + type + "_" + p[mod_field].get<std::string>();
    auto it = modules.find(key);
    return it == modules.end() ? nullptr : &it->second;
  };

  // Persist the driver's pipeline cache across runs: the first boot pays the
  // full compile, every boot after that replays it from disk. This is the
  // mechanism that turns the mid-game pipeline-compile hitching into a
  // one-time load cost.
  fs::path cache_path = dir / "pipeline_cache.bin";
  std::vector<char> cache_data;
  {
    std::ifstream cf(cache_path, std::ios::binary | std::ios::ate);
    if (cf) {
      cache_data.resize(size_t(cf.tellg()));
      cf.seekg(0);
      cf.read(cache_data.data(), std::streamsize(cache_data.size()));
      std::printf("pipeline cache: warm (%zu bytes from disk)\n", cache_data.size());
    } else {
      std::printf("pipeline cache: cold (no file yet)\n");
    }
  }
  VkPipelineCacheCreateInfo cache_info = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
  cache_info.initialDataSize = cache_data.size();
  cache_info.pInitialData = cache_data.empty() ? nullptr : cache_data.data();
  VkPipelineCache pipeline_cache;
  CHECK_VK(vkCreatePipelineCache(device, &cache_info, nullptr, &pipeline_cache));

  // Work list: recorded full state when a play session has produced it,
  // otherwise the manifest's (vs, ps) pairs with neutral defaults so the
  // harness still proves module acceptance without a capture.
  std::vector<json> records;
  fs::path inventory_path = dir / "pipeline_inventory.jsonl";
  bool recorded_state = fs::exists(inventory_path);
  if (recorded_state) {
    records = ReadInventory(inventory_path);
    std::printf("state source: recorded (%zu pipelines from %s)\n", records.size(),
                inventory_path.filename().string().c_str());
  } else {
    for (const auto& p : manifest["pipelines"]) {
      json r;
      r["vs"] = p["vs"];
      r["vs_mod"] = p["vs_modification"];
      r["ps"] = p["ps"];
      r["ps_mod"] = p["ps_modification"];
      records.push_back(std::move(r));
    }
    std::printf("state source: manifest defaults (%zu pairs; run a capture-armed session "
                "for recorded state)\n",
                records.size());
  }

  size_t built = 0, failed = 0, skipped_gs = 0;
  auto t0 = std::chrono::steady_clock::now();

  std::vector<VkPipeline> pipelines;
  for (const auto& r : records) {
    // Geometry-shader-expanded primitives (point sprites, rect lists) need the
    // emulated backend's generated geometry shaders; those become their own
    // native-path work item once a capture shows the game actually using them.
    if (r.value("geometry_shader", 0u) != 0) {
      ++skipped_gs;
      continue;
    }

    auto module_for = [&](const char* type, const char* hash_key,
                          const char* mod_key) -> ShaderEntry* {
      std::string hash = r.value(hash_key, "0000000000000000");
      if (hash == "0000000000000000") {
        return nullptr;
      }
      auto it = modules.find(hash + "_" + type + "_" + r.value(mod_key, ""));
      return it == modules.end() ? nullptr : &it->second;
    };
    ShaderEntry* vs = module_for("vs", "vs", "vs_mod");
    ShaderEntry* ps = module_for("ps", "ps", "ps_mod");
    if (!vs) {
      std::fprintf(stderr, "no vertex module for record: %s\n", r.dump().c_str());
      ++failed;
      continue;
    }

    VkPipelineShaderStageCreateInfo stages[2] = {};
    uint32_t stage_count = 0;
    stages[stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[stage_count].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[stage_count].module = vs->module;
    stages[stage_count].pName = "main";
    ++stage_count;
    if (ps) {
      stages[stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      stages[stage_count].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      stages[stage_count].module = ps->module;
      stages[stage_count].pName = "main";
      ++stage_count;
    }

    // Empty: the shaders fetch vertex data themselves via gl_VertexIndex.
    VkPipelineVertexInputStateCreateInfo vertex_input = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly.topology = ToVkTopology(r.value("primitive_topology", 3u));
    input_assembly.primitiveRestartEnable = r.value("primitive_restart", 0u) ? VK_TRUE : VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport = {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.depthClampEnable = r.value("depth_clamp", 0u) ? VK_TRUE : VK_FALSE;
    raster.rasterizerDiscardEnable = r.value("rasterizer_discard", 0u) ? VK_TRUE : VK_FALSE;
    raster.polygonMode = VkPolygonMode(r.value("polygon_mode", 0u));
    raster.cullMode = (r.value("cull_front", 0u) ? VK_CULL_MODE_FRONT_BIT : 0u) |
                      (r.value("cull_back", 0u) ? VK_CULL_MODE_BACK_BIT : 0u);
    raster.frontFace = r.value("front_face_cw", 1u) ? VK_FRONT_FACE_CLOCKWISE
                                                    : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;

    // depth_and_color_used: bit 0 depth, bits 1..4 color attachments.
    uint32_t used = r.value("depth_and_color_used", 1u | (ps ? 2u : 0u));
    bool depth_used = (used & 1u) != 0;

    VkPipelineMultisampleStateCreateInfo multisample = {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples =
        VkSampleCountFlagBits(1u << r.value("msaa_samples", 0u));
    multisample.sampleShadingEnable = r.value("sample_rate_shading", 0u) ? VK_TRUE : VK_FALSE;
    multisample.minSampleShading = multisample.sampleShadingEnable ? 1.0f : 0.0f;

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    if (depth_used) {
      depth_stencil.depthTestEnable = VK_TRUE;
      depth_stencil.depthWriteEnable = r.value("depth_write", 1u) ? VK_TRUE : VK_FALSE;
      depth_stencil.depthCompareOp = VkCompareOp(r.value("depth_compare", 6u));
      depth_stencil.stencilTestEnable = r.value("stencil_test", 0u) ? VK_TRUE : VK_FALSE;
      auto stencil_face = [&r](const char* key) {
        VkStencilOpState s = {};
        auto arr = r.value(key, json::array({0u, 0u, 0u, 7u}));
        s.failOp = VkStencilOp(arr.at(0).get<uint32_t>());
        s.passOp = VkStencilOp(arr.at(1).get<uint32_t>());
        s.depthFailOp = VkStencilOp(arr.at(2).get<uint32_t>());
        s.compareOp = VkCompareOp(arr.at(3).get<uint32_t>());
        s.compareMask = 0xFF;
        s.writeMask = 0xFF;
        return s;
      };
      depth_stencil.front = stencil_face("stencil_front");
      depth_stencil.back = stencil_face("stencil_back");
    }

    // Color attachments and their blend state.
    VkPipelineColorBlendAttachmentState blend_attachments[4] = {};
    VkFormat color_formats[4] = {};
    uint32_t color_count = 0;
    json rts = r.value("render_targets", json::array());
    json fmts = r.value("color_formats", json::array({0u, 0u, 0u, 0u}));
    for (uint32_t i = 0; i < 4; ++i) {
      bool attachment_used =
          recorded_state ? (used & (2u << i)) != 0 : (ps && i < [&] {
            uint32_t n = 0;
            for (uint32_t mask = ps->writes_color_targets; mask; mask >>= 1) ++n;
            return n;
          }());
      if (!attachment_used) {
        continue;
      }
      VkPipelineColorBlendAttachmentState& a = blend_attachments[color_count];
      if (recorded_state && i < rts.size()) {
        const json& rt = rts.at(i);
        uint32_t src_c = rt.value("src_color", 1u), dst_c = rt.value("dst_color", 0u);
        uint32_t op_c = rt.value("color_op", 0u);
        uint32_t src_a = rt.value("src_alpha", 1u), dst_a = rt.value("dst_alpha", 0u);
        uint32_t op_a = rt.value("alpha_op", 0u);
        a.blendEnable = !(src_c == 1 && dst_c == 0 && op_c == 0 && src_a == 1 && dst_a == 0 &&
                          op_a == 0)
                            ? VK_TRUE
                            : VK_FALSE;
        a.srcColorBlendFactor = VkBlendFactor(src_c);
        a.dstColorBlendFactor = VkBlendFactor(dst_c);
        a.colorBlendOp = ToVkBlendOp(op_c);
        a.srcAlphaBlendFactor = VkBlendFactor(src_a);
        a.dstAlphaBlendFactor = VkBlendFactor(dst_a);
        a.alphaBlendOp = ToVkBlendOp(op_a);
        a.colorWriteMask = rt.value("write_mask", 0xFu);
      } else {
        a.colorWriteMask = 0xF;
      }
      color_formats[color_count] =
          recorded_state ? GuestColorFormat(fmts.at(i).get<uint32_t>()) : color_format;
      ++color_count;
    }
    VkPipelineColorBlendStateCreateInfo blend = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = color_count;
    blend.pAttachments = blend_attachments;

    VkDynamicState dynamic_states[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic = {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamic_states;

    VkPipelineRenderingCreateInfo rendering = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    rendering.colorAttachmentCount = color_count;
    rendering.pColorAttachmentFormats = color_formats;
    rendering.depthAttachmentFormat = depth_used ? depth_format : VK_FORMAT_UNDEFINED;
    rendering.stencilAttachmentFormat = depth_used ? depth_format : VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo pipeline_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_info.pNext = &rendering;
    pipeline_info.stageCount = stage_count;
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport;
    pipeline_info.pRasterizationState = &raster;
    pipeline_info.pMultisampleState = &multisample;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &blend;
    pipeline_info.pDynamicState = &dynamic;
    pipeline_info.layout = pipeline_layouts.at(ps ? ps->texture_signature : "");

    VkPipeline pipeline;
    VkResult result =
        vkCreateGraphicsPipelines(device, pipeline_cache, 1, &pipeline_info, nullptr, &pipeline);
    if (result != VK_SUCCESS) {
      std::fprintf(stderr, "vkCreateGraphicsPipelines failed (%d) for vs=%s ps=%s\n", int(result),
                   r.value("vs", "?").c_str(), r.value("ps", "?").c_str());
      ++failed;
      continue;
    }
    pipelines.push_back(pipeline);
    ++built;
  }
  if (skipped_gs) {
    std::printf("skipped %zu geometry-shader pipelines (native GS emulation not built yet)\n",
                skipped_gs);
  }

  auto t1 = std::chrono::steady_clock::now();
  double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

  std::printf("pipelines built: %zu of %zu in %.1f ms (%.2f ms avg)\n", built, built + failed, ms,
              built ? ms / double(built) : 0.0);

  {
    size_t cache_size = 0;
    if (vkGetPipelineCacheData(device, pipeline_cache, &cache_size, nullptr) == VK_SUCCESS &&
        cache_size) {
      std::vector<char> out_data(cache_size);
      if (vkGetPipelineCacheData(device, pipeline_cache, &cache_size, out_data.data()) ==
          VK_SUCCESS) {
        std::ofstream cf(cache_path, std::ios::binary);
        cf.write(out_data.data(), std::streamsize(cache_size));
        std::printf("pipeline cache: saved %zu bytes\n", cache_size);
      }
    }
  }
  vkDestroyPipelineCache(device, pipeline_cache, nullptr);

  for (VkPipeline p : pipelines) {
    vkDestroyPipeline(device, p, nullptr);
  }
  for (auto& [signature, layout] : pipeline_layouts) {
    vkDestroyPipelineLayout(device, layout, nullptr);
  }
  for (auto& [signature, layout] : texture_sets) {
    if (layout != empty_set) {
      vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }
  }
  vkDestroyDescriptorSetLayout(device, empty_set, nullptr);
  for (auto& [set, layout] : shared_sets) {
    vkDestroyDescriptorSetLayout(device, layout, nullptr);
  }
  for (auto& [key, entry] : modules) {
    vkDestroyShaderModule(device, entry.module, nullptr);
  }
  vkDestroyDevice(device, nullptr);
  vkDestroyInstance(instance, nullptr);

  return failed ? 1 : 0;
}
