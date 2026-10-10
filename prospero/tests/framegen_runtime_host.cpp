// SPDX-License-Identifier: GPL-3.0-or-later
// Recording-driver fixture for the complete production ReX LSFG adapter,
// compute stages, planner, filesystem availability and presentation entrypoint.
// This does not execute proprietary shader code or model GPU execution speed.
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <rex/platform/framegen_prospero.h>
#include <rex/ui/vulkan/framegen_prospero.h>
#include <rex/ui/vulkan/device.h>
#include "frame_gen_pacer.h"
#include "lsfg_chain.h"
#include "lsfg_shaders.h"
#include "util.h"

namespace ui = rex::ui::vulkan;
namespace fg = ui::framegen;
namespace fp = rex::platform::prospero;
namespace fs = std::filesystem;
extern bool fg_test_frame_gen;
extern int32_t fg_test_frame_gen_target_rate, fg_test_frame_gen_multiplier;

namespace framegen_test {
std::vector<std::string> logs;
uint64_t now_ns = 1000000000;
void CaptureLog(std::string message) { logs.push_back(std::move(message)); }
}
extern "C" uint64_t prospero_metrics_clock_ns() { return framegen_test::now_ns; }

static void Require(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}
#define REQUIRE(x) Require(bool(x), std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #x)

template<class T> static uint64_t Key(T handle) {
  if constexpr (std::is_pointer_v<T>) return reinterpret_cast<uintptr_t>(handle);
  else return uint64_t(handle);
}
template<class T> static T Handle(uint64_t key) {
  if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(uintptr_t(key));
  else return T(key);
}
static uint64_t Align(uint64_t value, uint64_t alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

struct Driver {
  struct Image {
    VkImageCreateInfo info{};
    uint64_t memory = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags access = 0;
    bool fragment_sampled = false;
    bool external = false;
  };
  struct Buffer { VkDeviceSize size = 0; uint64_t memory = 0; };
  struct Memory { VkDeviceSize size = 0; uint32_t type = 0; std::vector<uint8_t> map; };
  struct Descriptor {
    VkDescriptorType type{};
    VkDescriptorImageInfo image{};
    VkDescriptorBufferInfo buffer{};
  };
  struct Set { uint64_t pool = 0, layout = 0; std::map<uint32_t, Descriptor> bindings; };
  struct Pool { uint32_t capacity = 0, allocated = 0; };
  std::map<uint64_t, Image> images;
  std::map<uint64_t, uint64_t> views;
  std::map<uint64_t, Buffer> buffers;
  std::map<uint64_t, Memory> memories;
  std::map<uint64_t, std::vector<VkDescriptorType>> set_layouts;
  std::map<uint64_t, uint64_t> pipeline_layouts, pipelines;
  std::map<uint64_t, Set> sets;
  std::map<uint64_t, Pool> pools;
  std::set<uint64_t> shaders, samplers;
  std::set<uint64_t> recorded_images, recorded_buffers, recorded_sets;
  std::set<uint64_t> pending_images, pending_buffers, pending_sets;
  std::map<std::string, size_t> calls;
  std::string failure;
  size_t failure_call = 1;
  uint64_t next = 10, bound_pipeline = 0, bound_set = 0;
  uint64_t image_payload = 0, image_requirement_bytes = 0, buffer_requirement_bytes = 0;
  uint64_t allocated_bytes = 0, allocation_peak = 0;
  size_t created_images = 0, created_buffers = 0, commands = 0, dispatches = 0, clears = 0;
  size_t writes = 0, presents = 0, output_reuse_dependencies = 0, final_dependencies = 0;
  size_t properties_queries = 0, completed_waits = 0, destroyed_objects = 0;
  uint32_t max_allocations = 4096, memory_type_bits = 7;
  bool coherent = true, format_supported = true, shader_fp16 = true;
  bool throw_recording = false;
  std::vector<std::string> events;

  template<class T> T New() { return Handle<T>(next++); }
  bool Fail(const std::string& operation) {
    const size_t n = ++calls[operation];
    return operation == failure && n == failure_call;
  }
  static uint32_t PixelBytes(VkFormat format) {
    if (format == VK_FORMAT_R8_UNORM) return 1;
    if (format == VK_FORMAT_R8G8B8A8_UNORM ||
        format == VK_FORMAT_A2B10G10R10_UNORM_PACK32) return 4;
    if (format == VK_FORMAT_R16G16B16A16_SFLOAT) return 8;
    throw std::runtime_error("unmodelled image format " + std::to_string(format));
  }
  VkImage External(VkExtent2D extent) {
    auto handle = New<VkImage>();
    Image image;
    image.info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.info.format = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    image.info.extent = {extent.width, extent.height, 1};
    image.info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image.access = VK_ACCESS_SHADER_READ_BIT;
    image.external = true;
    images.emplace(Key(handle), image);
    return handle;
  }
  void DestroyExternal(VkImage handle) {
    REQUIRE(!pending_images.contains(Key(handle)));
    REQUIRE(images.at(Key(handle)).external);
    images.erase(Key(handle));
  }
  void Begin() {
    recorded_images.clear(); recorded_buffers.clear(); recorded_sets.clear();
    bound_pipeline = bound_set = 0;
  }
  void Submit() {
    pending_images.insert(recorded_images.begin(), recorded_images.end());
    pending_buffers.insert(recorded_buffers.begin(), recorded_buffers.end());
    pending_sets.insert(recorded_sets.begin(), recorded_sets.end());
    Begin();
  }
  void Complete() {
    pending_images.clear(); pending_buffers.clear(); pending_sets.clear();
    ++completed_waits;
  }
  void Present(const ui::GeneratedFrame& frame) {
    REQUIRE(frame.image && frame.view);
    REQUIRE(views.at(Key(frame.view)) == Key(frame.image));
    auto& image = images.at(Key(frame.image));
    REQUIRE(image.layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    REQUIRE(image.access & VK_ACCESS_SHADER_READ_BIT);
    REQUIRE(image.info.format == VK_FORMAT_R8G8B8A8_UNORM);
    image.fragment_sampled = true;
    pending_images.insert(Key(frame.image));
    ++presents;
  }
  void Empty() const {
    REQUIRE(images.empty()); REQUIRE(views.empty()); REQUIRE(buffers.empty());
    REQUIRE(memories.empty()); REQUIRE(shaders.empty()); REQUIRE(samplers.empty());
    REQUIRE(pools.empty()); REQUIRE(sets.empty()); REQUIRE(pipelines.empty());
    REQUIRE(pipeline_layouts.empty()); REQUIRE(set_layouts.empty());
    REQUIRE(allocated_bytes == 0);
  }
};
static Driver* driver = nullptr;
static Driver& D() { REQUIRE(driver); return *driver; }

static ui::VulkanDevice NativeDevice() {
  ui::VulkanDevice native;
  auto& in = native.instance_.functions_;
  in.vkGetPhysicalDeviceProperties = +[](VkPhysicalDevice, VkPhysicalDeviceProperties* out) {
    ++D().properties_queries;
    *out = {};
    out->apiVersion = VK_API_VERSION_1_3;
    out->limits.maxMemoryAllocationCount = D().max_allocations;
    out->limits.maxImageDimension2D = 16384;
    out->limits.maxComputeWorkGroupInvocations = 1024;
    for (int i = 0; i < 3; ++i) {
      out->limits.maxComputeWorkGroupSize[i] = 1024;
      out->limits.maxComputeWorkGroupCount[i] = 65535;
    }
  };
  in.vkGetPhysicalDeviceMemoryProperties = +[](VkPhysicalDevice, VkPhysicalDeviceMemoryProperties* out) {
    ++D().properties_queries;
    *out = {};
    out->memoryTypeCount = 3;
    out->memoryHeapCount = 1;
    out->memoryHeaps[0].size = uint64_t(8) << 30;
    out->memoryTypes[0] = {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0};
    out->memoryTypes[1] = {VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        (D().coherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0)), 0};
    out->memoryTypes[2] = {VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 0};
  };
  in.vkGetPhysicalDeviceProperties2 = +[](VkPhysicalDevice, VkPhysicalDeviceProperties2* out) {
    ++D().properties_queries;
    for (auto* p = reinterpret_cast<VkBaseOutStructure*>(out->pNext); p; p = p->pNext) {
      if (p->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES) {
        auto* value = reinterpret_cast<VkPhysicalDeviceSubgroupProperties*>(p);
        value->subgroupSize = 64;
        value->supportedStages = VK_SHADER_STAGE_COMPUTE_BIT;
        value->supportedOperations = VK_SUBGROUP_FEATURE_BASIC_BIT;
      }
    }
  };
  in.vkGetPhysicalDeviceFormatProperties = +[](VkPhysicalDevice, VkFormat, VkFormatProperties* out) {
    ++D().properties_queries;
    *out = {};
    if (D().format_supported) out->optimalTilingFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
        VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
        VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT |
        VK_FORMAT_FEATURE_BLIT_SRC_BIT;
  };
  auto& f = native.functions_;
  f.vkCreateImage = +[](VkDevice, const VkImageCreateInfo* ci, const VkAllocationCallbacks*, VkImage* out) -> VkResult {
    *out = VK_NULL_HANDLE;
    if (D().Fail("create_image")) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    REQUIRE(ci->extent.width && ci->extent.height && ci->extent.depth == 1);
    REQUIRE(ci->mipLevels == 1 && ci->arrayLayers == 1 && ci->samples == VK_SAMPLE_COUNT_1_BIT);
    REQUIRE(ci->usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    *out = D().New<VkImage>();
    Driver::Image image; image.info = *ci;
    D().images.emplace(Key(*out), image);
    D().image_payload += uint64_t(ci->extent.width) * ci->extent.height * Driver::PixelBytes(ci->format);
    ++D().created_images;
    return VK_SUCCESS;
  };
  f.vkDestroyImage = +[](VkDevice, VkImage image, const VkAllocationCallbacks*) {
    REQUIRE(!D().pending_images.contains(Key(image)));
    for (const auto& [view, ref] : D().views) { (void)view; REQUIRE(ref != Key(image)); }
    REQUIRE(D().images.erase(Key(image)) == 1);
    D().events.push_back("image"); ++D().destroyed_objects;
  };
  f.vkGetImageMemoryRequirements = +[](VkDevice, VkImage image, VkMemoryRequirements* out) {
    const auto& info = D().images.at(Key(image)).info;
    // A deliberately documented host model, NOT actual RADV tiling: round both
    // dimensions to 16 pixels, then allocation size/alignment to 16 KiB.
    *out = {Align(Align(info.extent.width, 16) * Align(info.extent.height, 16) *
                 Driver::PixelBytes(info.format), 16384), 16384, D().memory_type_bits};
    D().image_requirement_bytes += out->size;
  };
  f.vkCreateImageView = +[](VkDevice, const VkImageViewCreateInfo* ci, const VkAllocationCallbacks*, VkImageView* out) -> VkResult {
    *out = VK_NULL_HANDLE;
    if (D().Fail("create_view")) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    REQUIRE(D().images.contains(Key(ci->image)));
    REQUIRE(ci->format == D().images.at(Key(ci->image)).info.format);
    *out = D().New<VkImageView>(); D().views.emplace(Key(*out), Key(ci->image));
    return VK_SUCCESS;
  };
  f.vkDestroyImageView = +[](VkDevice, VkImageView view, const VkAllocationCallbacks*) {
    REQUIRE(!D().pending_images.contains(D().views.at(Key(view))));
    REQUIRE(D().views.erase(Key(view)) == 1);
    D().events.push_back("view"); ++D().destroyed_objects;
  };
  f.vkCreateBuffer = +[](VkDevice, const VkBufferCreateInfo* ci, const VkAllocationCallbacks*, VkBuffer* out) -> VkResult {
    *out = VK_NULL_HANDLE;
    if (D().Fail("create_buffer")) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    REQUIRE(ci->usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    *out = D().New<VkBuffer>(); D().buffers.emplace(Key(*out), Driver::Buffer{ci->size, 0});
    ++D().created_buffers;
    return VK_SUCCESS;
  };
  f.vkDestroyBuffer = +[](VkDevice, VkBuffer buffer, const VkAllocationCallbacks*) {
    REQUIRE(!D().pending_buffers.contains(Key(buffer)));
    REQUIRE(D().buffers.erase(Key(buffer)) == 1);
    D().events.push_back("buffer"); ++D().destroyed_objects;
  };
  f.vkGetBufferMemoryRequirements = +[](VkDevice, VkBuffer buffer, VkMemoryRequirements* out) {
    *out = {Align(D().buffers.at(Key(buffer)).size, 16384), 16384, D().memory_type_bits};
    D().buffer_requirement_bytes += out->size;
  };
  f.vkAllocateMemory = +[](VkDevice, const VkMemoryAllocateInfo* ci, const VkAllocationCallbacks*, VkDeviceMemory* out) -> VkResult {
    *out = VK_NULL_HANDLE;
    if (D().Fail("allocate_memory")) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    REQUIRE(ci->allocationSize && ci->memoryTypeIndex < 3);
    *out = D().New<VkDeviceMemory>();
    D().memories.emplace(Key(*out), Driver::Memory{ci->allocationSize, ci->memoryTypeIndex, {}});
    D().allocated_bytes += ci->allocationSize;
    D().allocation_peak = std::max(D().allocation_peak, D().allocated_bytes);
    return VK_SUCCESS;
  };
  f.vkFreeMemory = +[](VkDevice, VkDeviceMemory memory, const VkAllocationCallbacks*) {
    for (const auto& [key, image] : D().images) { (void)key; REQUIRE(image.memory != Key(memory)); }
    for (const auto& [key, buffer] : D().buffers) { (void)key; REQUIRE(buffer.memory != Key(memory)); }
    D().allocated_bytes -= D().memories.at(Key(memory)).size;
    REQUIRE(D().memories.at(Key(memory)).map.empty());
    REQUIRE(D().memories.erase(Key(memory)) == 1);
    D().events.push_back("memory"); ++D().destroyed_objects;
  };
  f.vkBindImageMemory = +[](VkDevice, VkImage image, VkDeviceMemory memory, VkDeviceSize offset) -> VkResult {
    if (D().Fail("bind_image")) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    REQUIRE(offset == 0 && D().memories.contains(Key(memory)));
    REQUIRE(D().memories.at(Key(memory)).type == 0);
    D().images.at(Key(image)).memory = Key(memory); return VK_SUCCESS;
  };
  f.vkBindBufferMemory = +[](VkDevice, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset) -> VkResult {
    if (D().Fail("bind_buffer")) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    REQUIRE(offset == 0 && D().memories.contains(Key(memory)));
    REQUIRE(D().memories.at(Key(memory)).type != 0);
    D().buffers.at(Key(buffer)).memory = Key(memory); return VK_SUCCESS;
  };
  f.vkMapMemory = +[](VkDevice, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags, void** out) -> VkResult {
    *out = nullptr;
    if (D().Fail("map_memory")) return VK_ERROR_MEMORY_MAP_FAILED;
    auto& allocation = D().memories.at(Key(memory));
    REQUIRE(offset == 0 && size == VK_WHOLE_SIZE && allocation.type != 0);
    allocation.map.resize(allocation.size);
    *out = allocation.map.data(); return VK_SUCCESS;
  };
  f.vkUnmapMemory = +[](VkDevice, VkDeviceMemory memory) {
    auto& allocation = D().memories.at(Key(memory)); REQUIRE(!allocation.map.empty());
    allocation.map.clear(); D().events.push_back("unmap");
  };
  f.vkFlushMappedMemoryRanges = +[](VkDevice, uint32_t count, const VkMappedMemoryRange* ranges) -> VkResult {
    if (D().Fail("flush_memory")) return VK_ERROR_MEMORY_MAP_FAILED;
    REQUIRE(count == 1);
    REQUIRE(ranges[0].offset == 0 && ranges[0].size == VK_WHOLE_SIZE);
    REQUIRE(!D().memories.at(Key(ranges[0].memory)).map.empty());
    return VK_SUCCESS;
  };
  f.vkCreateSampler = +[](VkDevice, const VkSamplerCreateInfo*, const VkAllocationCallbacks*, VkSampler* out) -> VkResult {
    *out = VK_NULL_HANDLE;
    if (D().Fail("create_sampler")) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    *out = D().New<VkSampler>(); D().samplers.insert(Key(*out)); return VK_SUCCESS;
  };
  f.vkDestroySampler = +[](VkDevice, VkSampler sampler, const VkAllocationCallbacks*) {
    REQUIRE(D().samplers.erase(Key(sampler)) == 1); ++D().destroyed_objects;
  };
  f.vkCreateShaderModule = +[](VkDevice, const VkShaderModuleCreateInfo* ci, const VkAllocationCallbacks*, VkShaderModule* out) -> VkResult {
    *out = VK_NULL_HANDLE;
    if (D().Fail("create_shader")) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    REQUIRE(ci->codeSize == 20 && ci->pCode);
    *out = D().New<VkShaderModule>(); D().shaders.insert(Key(*out)); return VK_SUCCESS;
  };
  f.vkDestroyShaderModule = +[](VkDevice, VkShaderModule shader, const VkAllocationCallbacks*) {
    REQUIRE(D().shaders.erase(Key(shader)) == 1); ++D().destroyed_objects;
  };
  f.vkCreateDescriptorPool = +[](VkDevice, const VkDescriptorPoolCreateInfo* ci, const VkAllocationCallbacks*, VkDescriptorPool* out) -> VkResult {
    *out = VK_NULL_HANDLE;
    if (D().Fail("create_pool")) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    *out = D().New<VkDescriptorPool>(); D().pools.emplace(Key(*out), Driver::Pool{ci->maxSets, 0});
    return VK_SUCCESS;
  };
  f.vkDestroyDescriptorPool = +[](VkDevice, VkDescriptorPool pool, const VkAllocationCallbacks*) {
    for (auto it = D().sets.begin(); it != D().sets.end();) {
      if (it->second.pool != Key(pool)) { ++it; continue; }
      REQUIRE(!D().pending_sets.contains(it->first));
      it = D().sets.erase(it);
    }
    REQUIRE(D().pools.erase(Key(pool)) == 1); ++D().destroyed_objects;
  };
  f.vkCreateDescriptorSetLayout = +[](VkDevice, const VkDescriptorSetLayoutCreateInfo* ci, const VkAllocationCallbacks*, VkDescriptorSetLayout* out) -> VkResult {
    *out = VK_NULL_HANDLE;
    if (D().Fail("create_set_layout")) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    *out = D().New<VkDescriptorSetLayout>();
    std::vector<VkDescriptorType> types;
    for (uint32_t i = 0; i < ci->bindingCount; ++i) {
      REQUIRE(ci->pBindings[i].binding == i && ci->pBindings[i].descriptorCount == 1);
      REQUIRE(ci->pBindings[i].stageFlags == VK_SHADER_STAGE_COMPUTE_BIT);
      types.push_back(ci->pBindings[i].descriptorType);
    }
    D().set_layouts.emplace(Key(*out), std::move(types)); return VK_SUCCESS;
  };
  f.vkDestroyDescriptorSetLayout = +[](VkDevice, VkDescriptorSetLayout layout, const VkAllocationCallbacks*) {
    REQUIRE(D().set_layouts.erase(Key(layout)) == 1); ++D().destroyed_objects;
  };
  f.vkCreatePipelineLayout = +[](VkDevice, const VkPipelineLayoutCreateInfo* ci, const VkAllocationCallbacks*, VkPipelineLayout* out) -> VkResult {
    *out = VK_NULL_HANDLE;
    if (D().Fail("create_pipeline_layout")) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    REQUIRE(ci->setLayoutCount == 1 && ci->pushConstantRangeCount == 0);
    *out = D().New<VkPipelineLayout>();
    D().pipeline_layouts.emplace(Key(*out), Key(ci->pSetLayouts[0])); return VK_SUCCESS;
  };
  f.vkDestroyPipelineLayout = +[](VkDevice, VkPipelineLayout layout, const VkAllocationCallbacks*) {
    REQUIRE(D().pipeline_layouts.erase(Key(layout)) == 1); ++D().destroyed_objects;
  };
  f.vkCreateComputePipelines = +[](VkDevice, VkPipelineCache, uint32_t count, const VkComputePipelineCreateInfo* ci, const VkAllocationCallbacks*, VkPipeline* out) -> VkResult {
    REQUIRE(count == 1 && ci->stage.stage == VK_SHADER_STAGE_COMPUTE_BIT);
    REQUIRE(D().shaders.contains(Key(ci->stage.module)));
    REQUIRE(std::strcmp(ci->stage.pName, "main") == 0);
    const bool fail = D().Fail("create_pipeline");
    // Vulkan's partial success contract: this failure also returns a real
    // non-null handle; production must adopt it before checking VkResult.
    *out = D().New<VkPipeline>(); D().pipelines.emplace(Key(*out), Key(ci->layout));
    return fail ? VK_ERROR_OUT_OF_DEVICE_MEMORY : VK_SUCCESS;
  };
  f.vkDestroyPipeline = +[](VkDevice, VkPipeline pipeline, const VkAllocationCallbacks*) {
    REQUIRE(D().pipelines.erase(Key(pipeline)) == 1); ++D().destroyed_objects;
  };
  f.vkAllocateDescriptorSets = +[](VkDevice, const VkDescriptorSetAllocateInfo* ci, VkDescriptorSet* out) -> VkResult {
    if (D().Fail("allocate_sets")) return VK_ERROR_OUT_OF_POOL_MEMORY;
    auto& pool = D().pools.at(Key(ci->descriptorPool));
    REQUIRE(pool.allocated + ci->descriptorSetCount <= pool.capacity);
    for (uint32_t i = 0; i < ci->descriptorSetCount; ++i) {
      out[i] = D().New<VkDescriptorSet>();
      D().sets.emplace(Key(out[i]), Driver::Set{Key(ci->descriptorPool), Key(ci->pSetLayouts[i]), {}});
    }
    pool.allocated += ci->descriptorSetCount; return VK_SUCCESS;
  };
  f.vkUpdateDescriptorSets = +[](VkDevice, uint32_t count, const VkWriteDescriptorSet* writes, uint32_t copies, const VkCopyDescriptorSet*) {
    REQUIRE(copies == 0);
    for (uint32_t i = 0; i < count; ++i) {
      const auto& write = writes[i];
      REQUIRE(!D().pending_sets.contains(Key(write.dstSet)));
      auto& set = D().sets.at(Key(write.dstSet));
      REQUIRE(write.descriptorCount == 1 && write.dstArrayElement == 0);
      REQUIRE(D().set_layouts.at(set.layout).at(write.dstBinding) == write.descriptorType);
      Driver::Descriptor descriptor; descriptor.type = write.descriptorType;
      if (write.pImageInfo) descriptor.image = *write.pImageInfo;
      if (write.pBufferInfo) descriptor.buffer = *write.pBufferInfo;
      set.bindings[write.dstBinding] = descriptor;
      ++D().writes;
    }
  };
  f.vkCmdPipelineBarrier = +[](VkCommandBuffer, VkPipelineStageFlags source, VkPipelineStageFlags dest,
      VkDependencyFlags, uint32_t memories, const VkMemoryBarrier*, uint32_t buffers,
      const VkBufferMemoryBarrier*, uint32_t count, const VkImageMemoryBarrier* barriers) {
    REQUIRE(memories == 0 && buffers == 0);
    ++D().commands;
    for (uint32_t i = 0; i < count; ++i) {
      const auto& barrier = barriers[i];
      auto& image = D().images.at(Key(barrier.image));
      Require(barrier.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED || barrier.oldLayout == image.layout,
              "barrier oldLayout mismatch for image " + std::to_string(Key(barrier.image)));
      REQUIRE(barrier.srcQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);
      REQUIRE(barrier.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);
      REQUIRE(barrier.subresourceRange.levelCount == 1 && barrier.subresourceRange.layerCount == 1);
      if (image.fragment_sampled && (barrier.dstAccessMask & VK_ACCESS_SHADER_WRITE_BIT)) {
        REQUIRE(source & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        REQUIRE(barrier.srcAccessMask & VK_ACCESS_SHADER_READ_BIT);
        image.fragment_sampled = false; ++D().output_reuse_dependencies;
      }
      if (!image.external && barrier.newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        REQUIRE(source & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        REQUIRE(dest & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        REQUIRE(barrier.srcAccessMask & VK_ACCESS_SHADER_WRITE_BIT);
        REQUIRE(barrier.dstAccessMask & VK_ACCESS_SHADER_READ_BIT);
        ++D().final_dependencies;
      }
      image.layout = barrier.newLayout; image.access = barrier.dstAccessMask;
      D().recorded_images.insert(Key(barrier.image));
    }
  };
  f.vkCmdClearColorImage = +[](VkCommandBuffer, VkImage handle, VkImageLayout layout,
      const VkClearColorValue* color, uint32_t count, const VkImageSubresourceRange*) {
    ++D().commands; ++D().clears;
    auto& image = D().images.at(Key(handle));
    REQUIRE(count == 1 && layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && image.layout == layout);
    REQUIRE(image.info.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    REQUIRE(image.access & VK_ACCESS_TRANSFER_WRITE_BIT);
    REQUIRE(color->uint32[0] == 0 && color->uint32[1] == 0 && color->uint32[2] == 0 && color->uint32[3] == 0);
    D().recorded_images.insert(Key(handle));
    if (D().throw_recording) throw std::bad_alloc();
  };
  f.vkCmdBlitImage = +[](VkCommandBuffer, VkImage src, VkImageLayout src_layout, VkImage dst,
      VkImageLayout dst_layout, uint32_t count, const VkImageBlit* regions, VkFilter filter) {
    ++D().commands;
    auto& a = D().images.at(Key(src)); auto& b = D().images.at(Key(dst));
    REQUIRE(count == 1 && filter == VK_FILTER_NEAREST);
    REQUIRE(a.layout == src_layout && src_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    REQUIRE(b.layout == dst_layout && dst_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    REQUIRE(a.info.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    REQUIRE(b.info.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    REQUIRE(a.access & VK_ACCESS_TRANSFER_READ_BIT); REQUIRE(b.access & VK_ACCESS_TRANSFER_WRITE_BIT);
    REQUIRE(regions->srcOffsets[1].x == int(a.info.extent.width));
    REQUIRE(regions->dstOffsets[1].x == int(b.info.extent.width));
    REQUIRE(a.info.extent.width == b.info.extent.width && a.info.extent.height == b.info.extent.height);
    D().recorded_images.insert(Key(src)); D().recorded_images.insert(Key(dst));
  };
  f.vkCmdBindPipeline = +[](VkCommandBuffer, VkPipelineBindPoint point, VkPipeline pipeline) {
    ++D().commands; REQUIRE(point == VK_PIPELINE_BIND_POINT_COMPUTE);
    REQUIRE(D().pipelines.contains(Key(pipeline))); D().bound_pipeline = Key(pipeline);
  };
  f.vkCmdBindDescriptorSets = +[](VkCommandBuffer, VkPipelineBindPoint point, VkPipelineLayout layout,
      uint32_t first, uint32_t count, const VkDescriptorSet* sets, uint32_t dynamic, const uint32_t*) {
    ++D().commands;
    REQUIRE(point == VK_PIPELINE_BIND_POINT_COMPUTE && first == 0 && count == 1 && dynamic == 0);
    REQUIRE(D().pipeline_layouts.at(Key(layout)) == D().sets.at(Key(*sets)).layout);
    D().bound_set = Key(*sets);
  };
  f.vkCmdDispatch = +[](VkCommandBuffer, uint32_t x, uint32_t y, uint32_t z) {
    ++D().commands; ++D().dispatches;
    REQUIRE(x && y && z == 1 && x <= 65535 && y <= 65535);
    REQUIRE(D().bound_pipeline && D().bound_set);
    auto& set = D().sets.at(D().bound_set);
    REQUIRE(D().pipeline_layouts.at(D().pipelines.at(D().bound_pipeline)) == set.layout);
    REQUIRE(set.bindings.size() == D().set_layouts.at(set.layout).size());
    D().recorded_sets.insert(D().bound_set);
    for (const auto& [binding, descriptor] : set.bindings) {
      (void)binding;
      if (descriptor.type == VK_DESCRIPTOR_TYPE_SAMPLER) {
        REQUIRE(D().samplers.contains(Key(descriptor.image.sampler)));
      } else if (descriptor.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
        const auto& buffer = D().buffers.at(Key(descriptor.buffer.buffer));
        REQUIRE(descriptor.buffer.range == 48 && descriptor.buffer.offset == 0 && buffer.size == 48);
        REQUIRE(!D().memories.at(buffer.memory).map.empty());
        D().recorded_buffers.insert(Key(descriptor.buffer.buffer));
      } else {
        if (!descriptor.image.imageView) {
          // Eden uses optional previous-level sampled inputs. Real shader
          // construction separately requires the enabled nullDescriptor feature.
          REQUIRE(descriptor.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
          continue;
        }
        const uint64_t image_key = D().views.at(Key(descriptor.image.imageView));
        auto& image = D().images.at(image_key);
        REQUIRE(image.layout == VK_IMAGE_LAYOUT_GENERAL && descriptor.image.imageLayout == image.layout);
        const bool storage = descriptor.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        REQUIRE(image.access & (storage ? VK_ACCESS_SHADER_WRITE_BIT : VK_ACCESS_SHADER_READ_BIT));
        D().recorded_images.insert(image_key);
      }
    }
  };
  return native;
}

// These binding groups are independently transcribed from the pinned Eden
// compute stages, not read from the implementation under test at runtime.
// The separate framegen-shaders suite exercises the real PE and SPIR-V parser,
// validator and constructor. This suite substitutes only that shader boundary.
static std::vector<VkDescriptorType> ExpectedBindings(uint32_t id) {
  std::map<uint32_t, std::array<unsigned, 4>> table = {
    {255,{1,1,1,7}}, {256,{1,2,5,1}}, {280,{1,2,5,3}}, {281,{1,2,6,1}},
    {282,{0,1,3,2}}, {283,{0,1,2,2}}, {284,{0,1,2,2}}, {285,{1,2,4,1}},
    {286,{0,1,3,2}}, {287,{0,1,2,2}}, {288,{0,1,2,2}}, {289,{1,2,4,1}},
    {290,{0,1,1,1}}, {291,{0,1,1,1}}, {292,{0,1,1,2}}, {293,{0,1,2,2}},
    {294,{0,1,1,1}}, {295,{0,1,1,1}}, {296,{0,1,1,1}}, {297,{1,2,2,1}},
    {298,{0,1,6,2}}, {299,{0,1,2,2}}, {300,{0,1,2,2}}, {301,{0,1,2,2}},
    {302,{1,1,2,6}}
  };
  const std::array<VkDescriptorType,4> types{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      VK_DESCRIPTOR_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE};
  std::vector<VkDescriptorType> result;
  for (size_t i = 0; i < 4; ++i) result.insert(result.end(), table.at(id)[i], types[i]);
  return result;
}
namespace rex::ui::vulkan::framegen {
LsfgShaders::LsfgShaders(const Device& device, const std::filesystem::path&) {
  variant_ = D().shader_fp16 ? ShaderVariant::NativeFp16 : ShaderVariant::NativeFp32;
  source_bytes_ = 0; // synthetic shader boundary, never a proprietary library
  for (uint32_t id = 255; id <= 302; ++id) {
    if (id > 256 && id < 280) continue;
    std::array<uint32_t, 5> placeholder{0x07230203, 0, 0, 0, id};
    modules_.emplace(id, CreateWrappedShaderModule(device, placeholder));
    interfaces_.emplace(id, ShaderInterface{ExpectedBindings(id), {1,1,1}});
  }
}
VkShaderModule LsfgShaders::Get(uint32_t id) const { return *modules_.at(id); }
void LsfgShaders::ValidateInterface(uint32_t id, std::span<const VkDescriptorType> expected) const {
  const auto& actual = interfaces_.at(id).bindings;
  Require(actual.size() == expected.size() && std::equal(actual.begin(), actual.end(), expected.begin()),
          "stage descriptor contract differs from pinned Eden for shader " + std::to_string(id));
}
}

static fs::path fixture_root;
static fs::path SetDll() {
  const auto user = fixture_root / "user";
  fs::create_directories(user / "lossless");
  std::ofstream(user / "lossless/Lossless.dll", std::ios::binary).put('x');
  fp::ConfigureFrameGenerationPaths(user, {});
  return user;
}
static void Options(bool enabled, uint32_t target = 0, uint32_t multiplier = 2) {
  fg_test_frame_gen = enabled;
  fg_test_frame_gen_target_rate = int32_t(target);
  fg_test_frame_gen_multiplier = int32_t(multiplier);
}
struct Session {
  Driver model;
  ui::VulkanDevice native;
  std::unique_ptr<ui::ProsperoFrameGenerator> generator;
  ui::FrameGenerationSource source;
  uint64_t interval_ns = 16666667;
  uint64_t serial = 0;
  uint32_t output = 119880;
  bool wait_result = true;
  size_t waits = 0;
  explicit Session(VkExtent2D extent = {1280,720}, bool instantiate = true) {
    REQUIRE(!driver); driver = &model; native = NativeDevice();
    source.image = model.External(extent);
    source.format = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    source.extent = extent;
    if (instantiate) Create();
  }
  void Create() {
    generator = std::make_unique<ui::ProsperoFrameGenerator>(native, [&] {
      ++waits;
      if (!wait_result) return false;
      model.Complete(); return true;
    });
  }
  ui::FrameGenerationBatch Step(bool submit = true, bool notify = true) {
    model.Begin(); source.native_serial = ++serial;
    source.native_time_ns = 1000000000 + serial * interval_ns;
    framegen_test::now_ns = source.native_time_ns;
    auto result = generator->RecordNativeFrame(Handle<VkCommandBuffer>(3), source, output);
    if (!result.discard_recording) {
      const auto& image = model.images.at(Key(source.image));
      REQUIRE(image.layout == source.layout && image.access == source.access);
    }
    if (submit && !result.discard_recording) {
      model.Submit();
      for (uint32_t i = 0; i < result.count; ++i) {
        model.Present(result.frames[i]);
        if (notify) generator->NotifyFramePresented(true, true);
      }
      if (notify) generator->NotifyFramePresented(false, true);
    }
    return result;
  }
  uint64_t Run(size_t frames) {
    uint64_t generated = 0;
    for (size_t i = 0; i < frames; ++i) generated += Step().count;
    return generated;
  }
  ~Session() {
    model.Complete(); generator.reset(); model.DestroyExternal(source.image);
    model.Empty(); driver = nullptr;
  }
};

static size_t tests = 0;
static void Test(const std::string& name, const std::function<void()>& body) {
  framegen_test::logs.clear();
  try { body(); }
  catch (...) {
    std::cerr << "FAILED " << name << '\n';
    for (const auto& line : framegen_test::logs) std::cerr << line << '\n';
    throw;
  }
  ++tests; std::cout << "PASS " << name << '\n';
}

static void TestPacer() {
  struct Cadence { unsigned native, output, multiplier, target; double expected; const char* name; };
  const Cadence cases[] = {
    {60,119880,2,0,119.88,"60_to_119880_auto2"},
    {30,59940,4,0,59.94,"30_to_59940_auto4_capped"},
    {30,119880,4,0,119.88,"30_to_119880_auto4"},
    {30,119880,3,0,90.0,"30_to_119880_auto3"},
    {60,59940,4,0,60.0,"60_fills_actual_59940"},
    {60,240000,4,0,120.0,"actual_rate_capped_at_120"},
    {30,119880,2,120,119.88,"explicit120_uses_global4_not_auto2"},
    {60,119880,4,60,60.0,"explicit60_already_filled"}
  };
  for (const auto& c : cases) Test(std::string("pacer_") + c.name, [&] {
    fg::FrameGenPacer pacer(fp::FrameGenerationOptions{true,c.target,c.multiplier});
    uint64_t total = 0; size_t sampled = 0, largest = 0;
    const uint64_t step = uint64_t(std::llround(1.0e9 / c.native));
    for (size_t i = 0; i < 1800; ++i) {
      const auto plan = pacer.Plan(3,1000000000+i*step,c.output);
      REQUIRE(plan.generations <= 3);
      REQUIRE(plan.output_target <= 120.0f);
      pacer.CommitGeneratedCount(plan.generations);
      if (i >= 900) { total += 1 + plan.generations; ++sampled; largest = std::max(largest,plan.generations); }
    }
    const double rate = double(total) / sampled * c.native;
    Require(std::abs(rate - c.expected) < .20, "unexpected steady planned rate " + std::to_string(rate));
    if (c.native == 30 && c.target == 120) REQUIRE(largest == 3);
  });
  Test("pacer_unknown_disabled_capacity_and_timestamp_guards", [] {
    fg::FrameGenPacer p(fp::FrameGenerationOptions{true,0,2});
    REQUIRE(std::string(p.Plan(3,1,0).reason) == "output_unknown");
    REQUIRE(std::string(p.Plan(0,1,120000).reason) == "no_capacity");
    REQUIRE(std::string(p.Plan(3,UINT64_MAX,120000).reason) == "invalid_native_timestamp");
    REQUIRE(!p.Plan(3,1000000000,120000).warm);
    REQUIRE(p.Plan(3,1016666667,120000).warm);
    REQUIRE(std::string(p.Plan(3,1016666667,120000).reason) == "non_monotonic_native_time");
    REQUIRE(!p.Plan(3,1033333334,120000).warm);
    p.Reset(); REQUIRE(!p.Plan(3,100,120000).warm);
    fg::FrameGenPacer disabled(fp::FrameGenerationOptions{});
    REQUIRE(!disabled.Plan(3,1000000000,120000).warm);
  });
  Test("pacer_pause_gap_burst_and_recovery", [] {
    fg::FrameGenPacer p(fp::FrameGenerationOptions{true,0,2});
    uint64_t now = 1000000000;
    for (int i=0;i<10;++i) { auto plan=p.Plan(3,now,120000);p.CommitGeneratedCount(plan.generations);now+=16666667; }
    REQUIRE(std::string(p.Plan(3,now+500000000,120000).reason) == "native_gap");
    now+=516666667;
    REQUIRE(!p.Plan(3,now,120000).warm);
    for (int i=0;i<70;++i) { now+=16666667; auto plan=p.Plan(3,now,120000);p.CommitGeneratedCount(plan.generations); }
    now+=16666667; REQUIRE(p.Plan(3,now,120000).warm);
    REQUIRE(std::string(p.Plan(3,now+1,120000).reason) == "native_burst");
  });
}

static void TestPaths() {
  Test("presence_empty_and_arbitrary_file_no_binary_validation", [] {
    const auto user = SetDll();
    REQUIRE(fp::IsFrameGenerationDllPresent());
    std::ofstream(user / "lossless/Lossless.dll",std::ios::trunc).close();
    REQUIRE(fp::IsFrameGenerationDllPresent());
    REQUIRE(fp::ResolveFrameGenerationDllPath() == user / "lossless/Lossless.dll");
  });
  Test("presence_primary_then_readonly_title_fallback", [] {
    const auto user = SetDll(); const auto title = fixture_root / "title";
    fs::create_directories(title / "userdata/lossless");
    std::ofstream(title / "userdata/lossless/Lossless.dll").put('z');
    fp::ConfigureFrameGenerationPaths(user,title);
    REQUIRE(fp::ResolveFrameGenerationDllPath() == user / "lossless/Lossless.dll");
    fs::remove(user / "lossless/Lossless.dll");
    REQUIRE(fp::ResolveFrameGenerationDllPath() == title / "userdata/lossless/Lossless.dll");
    fp::ConfigureFrameGenerationPaths({},{}); REQUIRE(!fp::IsFrameGenerationDllPresent());
  });
  Test("presence_symlink_fifo_directory_missing_rejected_without_wait", [] {
    const auto user = SetDll(); const auto path = user / "lossless/Lossless.dll";
    fs::remove(path); fs::create_symlink(fixture_root / "title/userdata/lossless/Lossless.dll",path);
    REQUIRE(!fp::IsFrameGenerationDllPresent()); fs::remove(path);
    REQUIRE(mkfifo(path.c_str(),0600)==0); REQUIRE(!fp::IsFrameGenerationDllPresent()); fs::remove(path);
    fs::create_directory(path); REQUIRE(!fp::IsFrameGenerationDllPresent()); fs::remove(path);
    REQUIRE(!fp::IsFrameGenerationDllPresent()); SetDll();
  });
  Test("presence_path_pair_publication_is_synchronized", [] {
    const auto user = SetDll(); const auto title = fixture_root / "title";
    std::atomic<bool> failed{false};
    std::thread writer([&] {
      for (int i=0;i<2000;++i) fp::ConfigureFrameGenerationPaths(i%2 ? user : fs::path{},title);
    });
    for (int i=0;i<2000;++i) {
      const auto path = fp::ResolveFrameGenerationDllPath();
      if (path != user/"lossless/Lossless.dll" && path != title/"userdata/lossless/Lossless.dll") failed=true;
    }
    writer.join(); REQUIRE(!failed); fp::ConfigureFrameGenerationPaths(user,{});
  });
  Test("options_invalid_manual_values_are_clamped", [] {
    Options(true); fg_test_frame_gen_target_rate=90; fg_test_frame_gen_multiplier=100;
    auto options=fp::ReadFrameGenerationOptions(); REQUIRE(options.target_rate==0 && options.multiplier==4);
    fg_test_frame_gen_multiplier=-1; REQUIRE(fp::ReadFrameGenerationOptions().multiplier==2);
  });
}

static void TestRuntime() {
  Test("disabled_performs_no_gpu_queries_allocations_or_commands", [] {
    Options(false); Session s; REQUIRE(s.Run(90)==0);
    s.generator->SetOverlayActive(true); s.generator->NotifySubmissionFailure();
    s.generator->NotifyFramePresented(true,true);
    REQUIRE(s.model.properties_queries==0 && s.model.commands==0 && s.model.calls.empty());
  });
  Test("enabled_native60_fills_actual60_without_allocating_chain", [] {
    Options(true,0,4); Session s; s.output=59940; REQUIRE(s.Run(90)==0);
    REQUIRE(s.model.properties_queries==0 && s.model.created_images==0 && s.model.commands==0);
  });
  Test("session_on_after_saved_off_uses_runtime_request_without_mutating_toml_options", [] {
    Options(false,0,2); Session s;
    REQUIRE(s.Run(12)==0 && s.model.created_images==0 && s.model.commands==0);
    s.generator->SetEnabledForSession(true);
    for(int i=0;i<4;++i) REQUIRE(s.Step().count==0);
    REQUIRE(s.Run(12)>0 && s.model.created_images==155);
    REQUIRE(!fp::ReadFrameGenerationOptions().enabled);
  });
  Test("session_off_flushes_metrics_keeps_pending_resources_and_on_rewarms", [] {
    Options(true); Session s; REQUIRE(s.Run(12)>0);
    const auto bytes=s.model.allocated_bytes, destroyed=s.model.destroyed_objects;
    const auto commands=s.model.commands, clears=s.model.clears;
    REQUIRE(!s.model.pending_images.empty());
    framegen_test::logs.clear();
    s.generator->SetEnabledForSession(false);
    REQUIRE(std::any_of(framegen_test::logs.begin(),framegen_test::logs.end(),
        [](const auto& line){return line.find("framegen: metrics")!=std::string::npos;}));
    REQUIRE(s.Run(12)==0 && s.model.commands==commands);
    REQUIRE(s.model.allocated_bytes==bytes && s.model.destroyed_objects==destroyed);
    REQUIRE(!s.model.pending_images.empty());
    s.generator->SetEnabledForSession(true);
    for(int i=0;i<4;++i) REQUIRE(s.Step().count==0);
    REQUIRE(s.Run(12)>0 && s.model.clears==clears);
    REQUIRE(s.model.allocated_bytes==bytes && s.model.destroyed_objects==destroyed);
    REQUIRE(fp::ReadFrameGenerationOptions().enabled);
  });
  Test("session_revision_off_on_pair_restarts_warmup_with_same_final_enabled_state", [] {
    Options(true); Session s; REQUIRE(s.Run(12)>0);
    const auto bytes=s.model.allocated_bytes, destroyed=s.model.destroyed_objects;
    s.generator->SetEnabledForSession(true,true);
    for(int i=0;i<4;++i) REQUIRE(s.Step().count==0);
    REQUIRE(s.Run(12)>0);
    REQUIRE(s.model.allocated_bytes==bytes && s.model.destroyed_objects==destroyed);
  });
  Test("session_enable_during_overlay_waits_for_native_overlay_to_close", [] {
    Options(false); Session s; s.generator->SetOverlayActive(true);
    s.generator->SetEnabledForSession(true);
    REQUIRE(s.Run(12)==0 && s.model.commands==0 && s.model.created_images==0);
    s.generator->SetOverlayActive(false);
    for(int i=0;i<4;++i) REQUIRE(s.Step().count==0);
    REQUIRE(s.Run(12)>0);
  });
  Test("session_off_after_recording_still_quarantines_failed_submission_until_restart", [] {
    Options(true); Session s; REQUIRE(s.Run(12)>0);
    s.Step(false); // Commands and predicted layouts exist but were not submitted.
    const auto bytes=s.model.allocated_bytes, destroyed=s.model.destroyed_objects;
    const auto commands=s.model.commands;
    s.generator->SetEnabledForSession(false);
    s.generator->NotifySubmissionFailure();
    REQUIRE(s.generator->IsUnavailable());
    s.generator->SetEnabledForSession(true);
    REQUIRE(s.generator->IsUnavailable());
    REQUIRE(s.Run(12)==0 && s.model.commands==commands);
    REQUIRE(s.model.allocated_bytes==bytes && s.model.destroyed_objects==destroyed);
    REQUIRE(std::any_of(framegen_test::logs.begin(),framegen_test::logs.end(),
        [](const auto& line){return line.find("unavailable until game restart")!=std::string::npos;}));
  });
  Test("native60_to120_warmup_source_restore_ordered_output_reuse", [] {
    Options(true,0,2); Session s;
    REQUIRE(!s.generator->IsUnavailable());
    for(int i=0;i<4;++i) REQUIRE(s.Step().count==0);
    REQUIRE(s.Run(80)>70);
    REQUIRE(s.model.created_images==155 && s.model.clears==155);
    REQUIRE(s.model.output_reuse_dependencies>60 && s.model.final_dependencies>70);
    REQUIRE(s.model.dispatches>0 && s.model.pending_images.size()>0);
    const auto commands=s.model.commands; auto duplicate=s.generator->RecordNativeFrame(
      Handle<VkCommandBuffer>(3),s.source,s.output);
    REQUIRE(duplicate.count==0 && s.model.commands==commands);
    const auto writes=s.model.writes; REQUIRE(s.Run(20)>17); REQUIRE(s.model.writes==writes);
    REQUIRE(!s.generator->IsUnavailable());
  });
  Test("native30_auto4_caps_generation_to_actual60", [] {
    Options(true,0,4); Session s; s.interval_ns=33333333; s.output=59940;
    uint64_t count=s.Run(150); REQUIRE(count>=140 && count<=146);
  });
  Test("native30_auto4_exposes_three_distinct_chronological_outputs", [] {
    Options(true,0,4); Session s; s.interval_ns=33333333;
    s.Run(12); auto batch=s.Step(); REQUIRE(batch.count==3);
    REQUIRE(batch.frames[0].image!=batch.frames[1].image && batch.frames[1].image!=batch.frames[2].image);
    REQUIRE(batch.frames[0].extent.width==1280 && s.model.output_reuse_dependencies>10);
  });
  for (bool fp16 : {true,false}) Test(std::string("4k_allocation_requirements_")+(fp16?"fp16":"fp32"), [=] {
    Options(true,0,4); Session s({3840,2160},false); s.model.shader_fp16=fp16; s.Create(); s.interval_ns=33333333;
    REQUIRE(s.Run(10)>0);
    REQUIRE(s.model.created_images==155);
    REQUIRE(s.model.image_payload==274018500);
    REQUIRE(s.model.allocated_bytes<=(uint64_t(512)<<20));
    REQUIRE(s.model.allocation_peak==s.model.allocated_bytes);
    std::cout << "MEMORY " << (fp16?"fp16":"fp32") << " images=" << s.model.created_images
      << " buffers=" << s.model.created_buffers << " logical_image_bytes=" << s.model.image_payload
      << " image_requirement_bytes=" << s.model.image_requirement_bytes
      << " uniform_requirement_bytes=" << s.model.buffer_requirement_bytes
      << " allocation_peak=" << s.model.allocation_peak << " budget=" << (uint64_t(512)<<20) << '\n';
  });
  Test("resize_waits_submitted_paints_before_releasing_old_resources", [] {
    Options(true); Session s; REQUIRE(s.Run(12)>0);
    const auto old=s.source.image; s.source.image=s.model.External({1920,1080}); s.source.extent={1920,1080};
    REQUIRE(s.Step().count==0 && s.waits==1);
    s.model.DestroyExternal(old);
    REQUIRE(s.Run(10)>0 && s.model.created_images==310 && s.model.clears==310);
  });
  Test("resize_wait_failure_retains_inflight_resources_and_disables_fg", [] {
    Options(true); Session s; REQUIRE(s.Run(10)>0);
    s.wait_result=false;
    const auto memory=s.model.allocated_bytes, destroyed=s.model.destroyed_objects;
    const auto old=s.source.image; s.source.image=s.model.External({1920,1080}); s.source.extent={1920,1080};
    const auto batch=s.Step(); REQUIRE(batch.count==0 && !batch.discard_recording && s.waits==1);
    REQUIRE(s.generator->IsUnavailable());
    REQUIRE(s.model.allocated_bytes==memory && s.model.destroyed_objects==destroyed);
    const auto commands=s.model.commands; REQUIRE(s.Run(10)==0 && s.model.commands==commands);
    s.model.Complete(); s.model.DestroyExternal(old);
  });
  Test("submission_failure_quarantines_predicted_history_until_teardown", [] {
    Options(true); Session s; REQUIRE(s.Run(12)>0);
    const auto bytes=s.model.allocated_bytes, destroyed=s.model.destroyed_objects;
    s.Step(false); s.generator->NotifySubmissionFailure();
    REQUIRE(s.generator->IsUnavailable());
    const auto commands=s.model.commands;
    REQUIRE(s.Run(10)==0 && s.model.commands==commands);
    REQUIRE(s.model.allocated_bytes==bytes && s.model.destroyed_objects==destroyed);
    s.generator->SetOverlayActive(true); s.generator->SetOverlayActive(false);
    REQUIRE(s.generator->IsUnavailable());
  });
  Test("overlay_short_gap_resets_history_without_freeing_resources", [] {
    Options(true); Session s; REQUIRE(s.Run(12)>0);
    const auto bytes=s.model.allocated_bytes, destroyed=s.model.destroyed_objects, commands=s.model.commands;
    s.generator->SetOverlayActive(true); REQUIRE(s.Step().count==0 && s.model.commands==commands);
    s.generator->SetOverlayActive(false);
    for(int i=0;i<4;++i) REQUIRE(s.Step().count==0);
    REQUIRE(s.Run(10)>0 && s.model.allocated_bytes==bytes && s.model.destroyed_objects==destroyed);
    REQUIRE(s.model.clears==155);
  });
  Test("history_restarts_after_native_serial_rewind_and_large_gap", [] {
    Options(true); Session s; REQUIRE(s.Run(12)>0);
    s.serial=0; for(int i=0;i<4;++i) REQUIRE(s.Step().count==0);
    REQUIRE(s.Run(10)>0);
    s.serial+=120; REQUIRE(s.Step().count==0);
    REQUIRE(s.Run(30)==0); REQUIRE(s.Run(55)>0);
  });
  Test("recording_allocation_exception_requires_discard_and_permanent_fallback", [] {
    Options(true); Session s; s.model.throw_recording=true;
    REQUIRE(s.Step().count==0); auto batch=s.Step(false);
    REQUIRE(batch.count==0 && batch.discard_recording);
    REQUIRE(s.generator->IsUnavailable());
    REQUIRE(s.model.allocated_bytes>0);
    const auto commands=s.model.commands;
    s.model.throw_recording=false;
    REQUIRE(s.Run(10)==0 && s.model.commands==commands);
  });
  const std::vector<std::pair<std::string,size_t>> failures = {
    {"create_shader",24}, {"create_pool",1}, {"create_image",50}, {"create_view",50},
    {"allocate_memory",75}, {"bind_image",75}, {"create_buffer",2}, {"map_memory",2},
    {"bind_buffer",2}, {"create_sampler",2}, {"create_set_layout",10},
    {"create_pipeline_layout",10}, {"create_pipeline",10}, {"allocate_sets",10}
  };
  for (const auto& [operation,call] : failures) Test("allocation_failure_cleanup_"+operation,[=] {
    Options(true); Session s; s.model.failure=operation; s.model.failure_call=call;
    REQUIRE(s.Step().count==0); auto batch=s.Step();
    REQUIRE(batch.count==0 && !batch.discard_recording && s.model.commands==0);
    REQUIRE(s.model.calls.at(operation)>=call);
    REQUIRE(s.model.allocated_bytes==0 && s.model.views.empty() && s.model.pipelines.empty());
    const auto calls=s.model.calls;
    REQUIRE(s.Run(10)==0 && s.model.calls==calls);
  });
  Test("allocation_count_limit_keeps_partial_failure_before_recording", [] {
    Options(true); Session s({},false); s.model.DestroyExternal(s.source.image);
    s.source.extent={1280,720}; s.source.image=s.model.External(s.source.extent);
    s.model.max_allocations=32; s.Create(); s.Step(); auto b=s.Step();
    REQUIRE(!b.discard_recording && b.count==0 && s.model.commands==0 && s.model.allocated_bytes==0);
    REQUIRE(s.model.calls["allocate_memory"]==8);
  });
  Test("unsupported_format_or_memory_type_falls_back_without_commands", [] {
    Options(true);
    { Session s; s.model.format_supported=false; REQUIRE(s.Run(4)==0 && s.model.commands==0 && s.model.created_images==0); }
    { Session s; s.model.memory_type_bits=0; REQUIRE(s.Run(4)==0 && s.model.commands==0 && s.model.allocated_bytes==0); }
  });
  Test("noncoherent_uniforms_are_flushed_and_released_after_buffers", [] {
    Options(true); Session s({1280,720},false); s.model.coherent=false; s.Create(); REQUIRE(s.Run(10)>0);
    REQUIRE(s.model.calls["flush_memory"]==s.model.created_buffers && s.model.created_buffers>0);
  });
  Test("noncoherent_flush_failure_unmaps_and_falls_back_before_commands", [] {
    Options(true); Session s({1280,720},false); s.model.coherent=false;
    s.model.failure="flush_memory"; s.Create(); REQUIRE(s.Run(4)==0 && s.model.commands==0 && s.model.allocated_bytes==0);
  });
  Test("missing_dll_and_oversized_source_leave_native_path_available", [] {
    Options(true); fp::ConfigureFrameGenerationPaths({},{});
    { Session s; REQUIRE(s.Run(4)==0 && s.model.commands==0 && s.model.created_images==0);
      REQUIRE(s.generator->IsUnavailable()); }
    SetDll();
    { Session s({7680,4320}); REQUIRE(s.Run(4)==0 && s.model.commands==0 && s.model.created_images==0);
      REQUIRE(s.generator->IsUnavailable()); }
  });
  Test("move_assignment_releases_old_view_before_image_before_memory", [] {
    Driver model; REQUIRE(!driver); driver=&model;
    {
      auto native=NativeDevice(); fg::Device device(native); fg::MemoryAllocator allocator(device);
      {
        fg::LsfgImage a(device,allocator,{32,32}), b(device,allocator,{64,64});
        model.events.clear(); a=std::move(b);
        REQUIRE(model.events==std::vector<std::string>({"view","image","memory"}));
        REQUIRE(allocator.Budget().count==1);
      }
      REQUIRE(allocator.Budget().used==0 && allocator.Budget().count==0);
    }
    model.Empty(); driver=nullptr;
  });
  Test("metrics_do_not_promote_recorded_or_black_images_to_presented", [] {
    Options(true); Session s;
    for(int i=0;i<15;++i) s.Step(true,false);
    s.generator->NotifyFramePresented(true,false);
    s.generator->NotifyFramePresented(true,true);
    s.generator->NotifyFramePresented(false,true);
    s.generator->NotifyFramePresented(false,true); // same native image UI redraw
    framegen_test::now_ns+=6000000000;
    s.generator->NotifyFramePresented(false,false);
    auto it=std::find_if(framegen_test::logs.begin(),framegen_test::logs.end(),[](const std::string& l){return l.find("framegen: metrics")!=std::string::npos;});
    REQUIRE(it!=framegen_test::logs.end());
    REQUIRE(it->find("generated_drawn_presented=1 ")!=std::string::npos);
    REQUIRE(it->find("native_image_drawn_presented=2 ")!=std::string::npos);
    REQUIRE(it->find("failed_images=2 ")!=std::string::npos);
    REQUIRE(it->find("not_guest_FPS")!=std::string::npos);
  });
}

int main(int argc, char** argv) {
  try {
    REQUIRE(argc==2); fixture_root=fs::path(argv[1]); fs::create_directories(fixture_root);
    TestPaths(); SetDll(); TestPacer(); TestRuntime();
    std::cout << "ALL_PASS " << tests << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n'; return 1;
  }
}
