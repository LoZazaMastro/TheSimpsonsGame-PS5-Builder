/**
 * @file        gpu_profiler.cpp
 * @brief       Per-frame GPU time attribution by work category (Vulkan).
 */

#include <rex/graphics/vulkan/gpu_profiler.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include <rex/cvar.h>
#include <rex/graphics/vulkan/deferred_command_buffer.h>
#include <rex/logging.h>
#include <rex/ui/vulkan/device.h>
#include <rex/ui/vulkan/instance.h>

REXCVAR_DEFINE_INT32(gpu_profile_frames, 0, "GPU/Vulkan",
                     "Log GPU time per work category averaged over this many frames "
                     "(0 = off; diagnostic, adds timestamp queries)");

REXCVAR_DEFINE_BOOL(gpu_profile_draws, false, "GPU/Vulkan",
                    "With gpu_profile_frames, also time every draw and log the vertex/pixel "
                    "shader pairs that take the most GPU time (diagnostic)");

namespace rex::graphics::vulkan {

const char* VulkanGpuProfiler::CategoryName(Category category) {
  switch (category) {
    case Category::kDraw:
      return "draw";
    case Category::kTransfer:
      return "edram_transfer";
    case Category::kResolve:
      return "resolve";
    case Category::kTextureLoad:
      return "texture_load";
    case Category::kNativeResolve:
      return "native_resolve";
    case Category::kSwap:
      return "swap";
    default:
      return "other";
  }
}

bool VulkanGpuProfiler::Initialize(const ui::vulkan::VulkanDevice* vulkan_device,
                                   uint32_t frames_in_flight) {
  Shutdown();
  int32_t interval = REXCVAR_GET(gpu_profile_frames);
  // Offline tools (trace_dump) skip the cvar environment parsing.
  if (const char* env = std::getenv("REX_GPU_PROFILE_FRAMES")) {
    interval = std::max(interval, std::atoi(env));
  }
  if (interval <= 0) {
    return true;
  }
  const ui::vulkan::VulkanInstance* instance = vulkan_device->vulkan_instance();
  const auto& ifn = instance->functions();
  VkDevice device = vulkan_device->device();
  auto write_timestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(
      ifn.vkGetDeviceProcAddr(device, "vkCmdWriteTimestamp"));
  get_query_pool_results_ = reinterpret_cast<PFN_vkGetQueryPoolResults>(
      ifn.vkGetDeviceProcAddr(device, "vkGetQueryPoolResults"));
  if (!write_timestamp || !get_query_pool_results_) {
    REXGPU_WARN("GPU profiler: timestamp functions unavailable");
    return true;
  }
  VkPhysicalDeviceProperties properties;
  ifn.vkGetPhysicalDeviceProperties(vulkan_device->physical_device(), &properties);
  if (properties.limits.timestampPeriod <= 0.0f) {
    REXGPU_WARN("GPU profiler: timestamps unsupported on this device");
    return true;
  }
  timestamp_period_ns_ = properties.limits.timestampPeriod;
  frames_in_flight_ = frames_in_flight;
  VkQueryPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
  pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
  pool_info.queryCount = frames_in_flight * kMaxMarksPerFrame;
  const auto& dfn = vulkan_device->functions();
  if (dfn.vkCreateQueryPool(device, &pool_info, nullptr, &query_pool_) != VK_SUCCESS) {
    query_pool_ = VK_NULL_HANDLE;
    REXGPU_WARN("GPU profiler: failed to create the timestamp query pool");
    return true;
  }
  DeferredCommandBuffer::cmd_write_timestamp_ = write_timestamp;
  vulkan_device_ = vulkan_device;
  slots_.assign(frames_in_flight, FrameSlot());
  results_.resize(kMaxMarksPerFrame);
  log_interval_frames_ = uint32_t(interval);
  const char* per_draw_env = std::getenv("REX_GPU_PROFILE_DRAWS");
  per_draw_ = REXCVAR_GET(gpu_profile_draws) || per_draw_env != nullptr;
  per_render_target_ = per_draw_env != nullptr && std::strcmp(per_draw_env, "rt") == 0;
  REXGPU_INFO("GPU profiler: logging GPU time per category{} every {} frames",
              per_draw_ ? " and per draw" : "", interval);
  return true;
}

void VulkanGpuProfiler::Shutdown() {
  if (query_pool_ != VK_NULL_HANDLE && vulkan_device_) {
    vulkan_device_->functions().vkDestroyQueryPool(vulkan_device_->device(), query_pool_,
                                                    nullptr);
  }
  query_pool_ = VK_NULL_HANDLE;
  current_ = nullptr;
  slots_.clear();
}

void VulkanGpuProfiler::BeginFrame(DeferredCommandBuffer& command_buffer, uint64_t frame_index) {
  if (!enabled()) {
    return;
  }
  current_slot_index_ = uint32_t(frame_index % frames_in_flight_);
  current_ = &slots_[current_slot_index_];
  current_->frame_index = frame_index;
  current_->mark_count = 0;
  current_->draw_count = 0;
  command_buffer.CmdVkResetQueryPool(query_pool_, current_slot_index_ * kMaxMarksPerFrame,
                                     kMaxMarksPerFrame);
  Mark(command_buffer, Category::kOther);
}

void VulkanGpuProfiler::Mark(DeferredCommandBuffer& command_buffer, Category next) {
  if (!current_ || current_->mark_count >= kMaxMarksPerFrame) {
    return;
  }
  uint32_t mark = current_->mark_count++;
  current_->categories[mark] = next;
  current_->draw_ordinals[mark] = 0;
  command_buffer.CmdVkWriteTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_,
                                     current_slot_index_ * kMaxMarksPerFrame + mark);
}

void VulkanGpuProfiler::MarkKeyed(DeferredCommandBuffer& command_buffer, Category next,
                                  uint64_t key_a, uint64_t key_b) {
  if (!current_ || current_->mark_count >= kMaxMarksPerFrame) {
    return;
  }
  uint32_t mark = current_->mark_count++;
  current_->categories[mark] = next;
  current_->draw_shaders[mark] = {key_a, key_b};
  current_->draw_ordinals[mark] = ++current_->draw_count;
  command_buffer.CmdVkWriteTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_,
                                     current_slot_index_ * kMaxMarksPerFrame + mark);
}

void VulkanGpuProfiler::EndFrame(DeferredCommandBuffer& command_buffer) {
  if (!current_) {
    return;
  }
  Mark(command_buffer, Category::kOther);
  current_ = nullptr;
}

void VulkanGpuProfiler::FrameCompleted(uint64_t frame_index) {
  if (!enabled()) {
    return;
  }
  FrameSlot& slot = slots_[frame_index % frames_in_flight_];
  if (slot.frame_index != frame_index || slot.mark_count < 2 || &slot == current_) {
    return;
  }
  uint32_t first_query = uint32_t(frame_index % frames_in_flight_) * kMaxMarksPerFrame;
  VkResult result = get_query_pool_results_(
      vulkan_device_->device(), query_pool_, first_query, slot.mark_count,
      sizeof(uint64_t) * slot.mark_count, results_.data(), sizeof(uint64_t),
      VK_QUERY_RESULT_64_BIT);
  slot.frame_index = UINT64_MAX;
  if (result != VK_SUCCESS) {
    return;
  }
  double to_ms = double(timestamp_period_ns_) / 1e6;
  for (uint32_t i = 0; i + 1 < slot.mark_count; ++i) {
    uint64_t begin = results_[i], end = results_[i + 1];
    if (end < begin) {
      continue;
    }
    size_t category = size_t(slot.categories[i]);
    accumulated_ms_[category] += double(end - begin) * to_ms;
    ++accumulated_marks_[category];
    if (per_draw_ && slot.draw_ordinals[i]) {
      DrawStats& stats = draw_stats_[slot.draw_shaders[i]];
      stats.ms += double(end - begin) * to_ms;
      if (!stats.count++) {
        stats.first_ordinal = slot.draw_ordinals[i];
        stats.category = slot.categories[i];
      }
    }
  }
  if (results_[slot.mark_count - 1] >= results_[0]) {
    accumulated_total_ms_ += double(results_[slot.mark_count - 1] - results_[0]) * to_ms;
  }
  if (++accumulated_frames_ < log_interval_frames_) {
    return;
  }
  double frames = double(accumulated_frames_);
  std::string breakdown;
  for (size_t i = 0; i < size_t(Category::kCount); ++i) {
    breakdown += fmt::format(" {}={:.2f}ms/{:.0f}", CategoryName(Category(i)),
                             accumulated_ms_[i] / frames, accumulated_marks_[i] / frames);
  }
  REXGPU_INFO("[gpu-profile] {} frames: gpu frame span {:.2f}ms |{} (ms per frame / intervals per frame)",
              accumulated_frames_, accumulated_total_ms_ / frames, breakdown);
  if (per_draw_ && !draw_stats_.empty()) {
    std::vector<std::pair<std::pair<uint64_t, uint64_t>, DrawStats>> sorted(draw_stats_.begin(),
                                                                          draw_stats_.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second.ms > b.second.ms; });
    size_t shown = std::min(sorted.size(), size_t(30));
    for (size_t i = 0; i < shown; ++i) {
      const auto& [shaders, stats] = sorted[i];
      if (stats.category == Category::kDraw && per_render_target_) {
        REXGPU_INFO("[gpu-profile-rt] surface={:08X} depth={:08X} color0={:08X} mask={:08X} "
                    "{:.3f}ms {:.1f} draws per frame, first draw #{}",
                    uint32_t(shaders.first >> 32), uint32_t(shaders.first),
                    uint32_t(shaders.second >> 32), uint32_t(shaders.second), stats.ms / frames,
                    stats.count / frames, stats.first_ordinal);
      } else if (stats.category == Category::kDraw) {
        REXGPU_INFO("[gpu-profile-draw] vs={:016X} ps={:016X} {:.3f}ms {:.1f} draws per frame, "
                    "first draw #{}",
                    shaders.first, shaders.second, stats.ms / frames, stats.count / frames,
                    stats.first_ordinal);
      } else {
        REXGPU_INFO("[gpu-profile-draw] {} key={:016X}:{:016X} {:.3f}ms {:.1f} per frame, "
                    "first mark #{}",
                    CategoryName(stats.category), shaders.first, shaders.second,
                    stats.ms / frames, stats.count / frames, stats.first_ordinal);
      }
    }
    draw_stats_.clear();
  }
  accumulated_frames_ = 0;
  accumulated_total_ms_ = 0.0;
  accumulated_ms_.fill(0.0);
  accumulated_marks_.fill(0);
}

}  // namespace rex::graphics::vulkan
