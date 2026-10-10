// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 The Simpsons Game PS5 contributors
// PS5 presentation integration for the GPL Eden/lsfg-vk native compute chain.

#include <rex/ui/vulkan/framegen_prospero.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

#include <rex/logging.h>
#include <rex/platform/framegen_prospero.h>
#include <rex/platform/metrics_prospero.h>

#include "framegen/frame_gen_pacer.h"
#include "framegen/lsfg_chain.h"
#include "framegen/lsfg_shaders.h"
#include "framegen/vulkan_adapter.h"

namespace rex::ui::vulkan {
namespace {
namespace fg = framegen;
constexpr VkDeviceSize kFrameGenerationMemoryBudget = VkDeviceSize(512) << 20;
constexpr uint64_t kMetricsIntervalNs = uint64_t(5) * 1000000000;

struct FrameResources {
  // Allocation accounting must outlive every image and uniform buffer.
  fg::MemoryAllocator allocator;
  std::unique_ptr<fg::LsfgChain> chain;
  std::array<fg::LsfgImage, 3> outputs;
  VkExtent2D extent{};
  bool needs_initialization = true;

  FrameResources(const fg::Device& device, const fg::LsfgShaders& shaders, VkExtent2D size)
      : allocator(device, kFrameGenerationMemoryBudget), extent(size) {
    // Full-resolution optical flow preserves the selected source detail. The
    // fixed budget rejects an unsupported allocation instead of silently
    // reducing the image or changing the native render resolution.
    chain = std::make_unique<fg::LsfgChain>(device, allocator, shaders, size,
                                          fg::LSFG_DEFAULT_FORMAT, 1.0f);
    for (auto& output : outputs) output = fg::LsfgImage(device, allocator, size);
    // Populate every generation-count combination before any submission can
    // reference its descriptors. No in-flight descriptor update on a new mode.
    for (size_t count = 1; count <= outputs.size(); ++count) {
      for (size_t index = 0; index < count; ++index) {
        chain->SetTarget(device, count, index, uint32_t(index), outputs[index].View());
      }
    }
  }
  FrameResources(const FrameResources&) = delete;
  FrameResources& operator=(const FrameResources&) = delete;
};

VkImageMemoryBarrier ImageBarrier(VkImage image, VkImageLayout before, VkImageLayout after,
                                  VkAccessFlags source_access, VkAccessFlags destination_access) {
  return VkImageMemoryBarrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = source_access,
      .dstAccessMask = destination_access,
      .oldLayout = before,
      .newLayout = after,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
}

void CopyNativeInput(const VulkanDevice& native, VkCommandBuffer command_buffer,
                      const FrameGenerationSource& source, fg::LsfgImage& input) {
  const fg::vk::CommandBuffer commands(native, command_buffer);
  const std::array before{
      ImageBarrier(source.image, source.layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   source.access, VK_ACCESS_TRANSFER_READ_BIT),
      ImageBarrier(input.Handle(), input.Layout(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                   VK_ACCESS_TRANSFER_WRITE_BIT),
  };
  commands.PipelineBarrier(source.stage | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, {}, {}, before);
  const VkImageBlit region{
      .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
      .srcOffsets = {{0, 0, 0}, {int32_t(source.extent.width), int32_t(source.extent.height), 1}},
      .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
      .dstOffsets = {{0, 0, 0}, {int32_t(input.Extent().width), int32_t(input.Extent().height), 1}},
  };
  // Same dimensions: a format conversion, not a lower-resolution render. The
  // current native image keeps its existing 10-bit path through the presenter.
  native.functions().vkCmdBlitImage(command_buffer, source.image,
                                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, input.Handle(),
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);
  const std::array after{
      ImageBarrier(source.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, source.layout,
                   VK_ACCESS_TRANSFER_READ_BIT, source.access),
      ImageBarrier(input.Handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                   VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT),
  };
  commands.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                            source.stage | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, {}, {}, after);
  input.SetLayout(VK_IMAGE_LAYOUT_GENERAL);
}
}  // namespace

struct ProsperoFrameGenerator::Impl {
  const VulkanDevice& native;
  platform::prospero::FrameGenerationOptions options;
  AwaitSubmittedPaints await_submitted_paints;
  fg::FrameGenPacer pacer;
  std::unique_ptr<fg::Device> device;
  std::unique_ptr<fg::LsfgShaders> shaders;
  std::unique_ptr<FrameResources> resources;
  bool unavailable = false;
  bool overlay_active = false;
  uint64_t last_native_serial = 0;
  uint64_t last_native_time_ns = 0;
  uint64_t input_count = 0;
  size_t warm_streak = 0;
  uint32_t output_millihertz = 0;
  float observed_native_rate = 0.0f;
  float effective_output_target = 0.0f;
  size_t effective_ceiling = 0;
  const char* state = "initializing";

  struct Window {
    uint64_t start_ns = 0;
    uint64_t native_frames = 0;
    uint64_t generated_recorded = 0;
    uint64_t generated_presented = 0;
    uint64_t native_image_presented = 0;
    uint64_t failed_images = 0;
    uint64_t native_interval_min_ns = UINT64_MAX;
    uint64_t native_interval_max_ns = 0;
    uint32_t extras_min = UINT32_MAX;
    uint32_t extras_max = 0;
  } window;

  Impl(const VulkanDevice& device_, AwaitSubmittedPaints await)
      : native(device_), options(platform::prospero::ReadFrameGenerationOptions()),
        await_submitted_paints(std::move(await)), pacer(options) {
    if (!options.enabled) {
      state = "off";
      REXLOG_INFO("framegen: off; native rendering and presentation defaults retained");
    } else {
      REXLOG_INFO("framegen: requested target={} auto_multiplier={} max_multiplier=4; "
                   "experimental, local native shader resources required",
                   options.target_rate, options.multiplier);
    }
  }

  void ResetHistory() {
    pacer.Reset();
    last_native_serial = 0;
    last_native_time_ns = 0;
    input_count = 0;
    warm_streak = 0;
    // The allocations and physical layout trackers remain untouched; earlier
    // submissions may still use them. Three newly written history slots must
    // replace old temporal data before generation can resume.
  }

  void State(const char* next) {
    if (std::strcmp(state, next) == 0) return;
    state = next;
    REXLOG_INFO("framegen: {} native_observed={:.2f} target_requested={} "
                 "target_effective={:.3f} output_millihertz={} auto_multiplier={} "
                 "effective_ceiling_extra={}",
                 state, observed_native_rate, options.target_rate, effective_output_target,
                 output_millihertz, options.multiplier, effective_ceiling);
  }

  void Fail(const char* kind, const std::string& reason) {
    if (unavailable) return;
    unavailable = true;
    state = kind;
    warm_streak = 0;
    REXLOG_WARN("framegen: {} {}; native fallback for the rest of this session", kind, reason);
    // Keep all resources alive. In particular, a command that failed to submit
    // or present must not cause another command to trust its predicted state.
  }

  void CheckFormats(VkFormat source_format) {
    const auto& functions = native.vulkan_instance()->functions();
    const VkFormatFeatureFlags compute_features = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
        VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
        VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    for (const VkFormat format : {fg::LSFG_DEFAULT_FORMAT, fg::LSFG_FLOW_FORMAT,
                                  fg::LSFG_MOTION_FORMAT}) {
      VkFormatProperties properties{};
      functions.vkGetPhysicalDeviceFormatProperties(native.physical_device(), format, &properties);
      const VkFormatFeatureFlags required = compute_features |
          (format == fg::LSFG_DEFAULT_FORMAT ? VK_FORMAT_FEATURE_BLIT_DST_BIT : 0);
      if ((properties.optimalTilingFeatures & required) != required) {
        throw std::runtime_error("format " + std::to_string(int(format)) +
                                 " lacks required compute/sample/transfer support");
      }
    }
    VkFormatProperties source{};
    functions.vkGetPhysicalDeviceFormatProperties(native.physical_device(), source_format, &source);
    if (!(source.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
      throw std::runtime_error("native source format cannot be converted for LSFG");
    }
  }

  void EnsureResources(const FrameGenerationSource& source) {
    if (!source.extent.width || !source.extent.height || source.extent.width > 3840 ||
        source.extent.height > 2160) {
      throw std::runtime_error("source extent outside supported 1..3840 x 1..2160 framegen range");
    }
    if (!device) {
      const auto path = platform::prospero::ResolveFrameGenerationDllPath();
      if (path.empty()) throw std::runtime_error("local Lossless.dll absent or unreadable");
      device = std::make_unique<fg::Device>(native);
      CheckFormats(source.format);
      shaders = std::make_unique<fg::LsfgShaders>(*device, path);
      REXLOG_INFO("framegen: shaders_ready modules=25 variant={} source_bytes={} path={}",
                   shaders->Variant() == fg::ShaderVariant::NativeFp16 ? "native_fp16" : "native_fp32",
                   shaders->SourceBytes(), path.string());
    }
    if (resources && resources->extent.width == source.extent.width &&
        resources->extent.height == source.extent.height) return;
    if (resources) {
      if (!await_submitted_paints || !await_submitted_paints()) {
        throw std::runtime_error("cannot wait for submitted paints before framegen resize");
      }
      resources.reset();
    }
    resources = std::make_unique<FrameResources>(*device, *shaders, source.extent);
    input_count = 0;
    warm_streak = 0;
    REXLOG_INFO("framegen: resources_ready source={}x{} output={}x{} flow_scale=1.0 "
                 "device_image_uniform_bytes={} allocations={} budget_bytes={} "
                 "format=RGBA8 history_slots=3 max_extra=3",
                 source.extent.width, source.extent.height, source.extent.width, source.extent.height,
                 resources->allocator.Budget().used, resources->allocator.Budget().count,
                 resources->allocator.Budget().limit);
  }

  void ObserveNative(const FrameGenerationSource& source) {
    const uint64_t now = prospero_metrics_clock_ns();
    if (!window.start_ns) window.start_ns = now;
    ++window.native_frames;
    if (last_native_time_ns && source.native_time_ns > last_native_time_ns) {
      const uint64_t delta = source.native_time_ns - last_native_time_ns;
      window.native_interval_min_ns = std::min(window.native_interval_min_ns, delta);
      window.native_interval_max_ns = std::max(window.native_interval_max_ns, delta);
    }
  }

  void LogWindow(bool force = false) {
    if (!options.enabled || !window.start_ns) return;
    const uint64_t now = prospero_metrics_clock_ns();
    if (now <= window.start_ns || (!force && now - window.start_ns < kMetricsIntervalNs)) return;
    const double seconds = double(now - window.start_ns) / 1.0e9;
    REXLOG_INFO("framegen: metrics seconds={:.3f} state={} output_millihertz={} "
                 "native_observed_fps={:.2f} native_interval_low_fps={:.2f} "
                 "native_interval_high_fps={:.2f} generated_recorded={} generated_drawn_presented={} "
                 "generated_drawn_presented_fps={:.2f} native_image_drawn_presented={} "
                 "total_drawn_presented_fps={:.2f} failed_images={} requested_auto_multiplier={} "
                 "effective_extra_min={} effective_extra_max={} "
                 "note=native_image_presentations_include_UI_redraws_not_guest_FPS",
                 seconds, state, output_millihertz, double(window.native_frames) / seconds,
                 window.native_interval_max_ns ? 1.0e9 / double(window.native_interval_max_ns) : 0.0,
                 window.native_interval_min_ns != UINT64_MAX
                     ? 1.0e9 / double(window.native_interval_min_ns) : 0.0,
                 window.generated_recorded, window.generated_presented,
                 double(window.generated_presented) / seconds, window.native_image_presented,
                 double(window.generated_presented + window.native_image_presented) / seconds,
                 window.failed_images, options.multiplier,
                 window.extras_min == UINT32_MAX ? 0 : window.extras_min, window.extras_max);
    window = {};
    window.start_ns = now;
  }
};

ProsperoFrameGenerator::ProsperoFrameGenerator(const VulkanDevice& device,
                                               AwaitSubmittedPaints await_submitted_paints)
    : impl_(std::make_unique<Impl>(device, std::move(await_submitted_paints))) {}

ProsperoFrameGenerator::~ProsperoFrameGenerator() {
  // The presenter waits all submitted paints before destroying this object.
  impl_->LogWindow(true);
}

FrameGenerationBatch ProsperoFrameGenerator::RecordNativeFrame(
    VkCommandBuffer command_buffer, const FrameGenerationSource& source,
    uint32_t actual_output_millihertz) {
  auto& impl = *impl_;
  FrameGenerationBatch batch;
  if (!impl.options.enabled || impl.unavailable || impl.overlay_active ||
      !source.image || !source.native_serial) return batch;
  if (source.native_serial == impl.last_native_serial) return batch;
  if (source.native_serial < impl.last_native_serial) impl.ResetHistory();
  impl.ObserveNative(source);
  impl.last_native_serial = source.native_serial;
  impl.last_native_time_ns = source.native_time_ns;
  impl.output_millihertz = actual_output_millihertz;
  const auto plan = impl.pacer.Plan(batch.frames.size(), source.native_time_ns,
                                     actual_output_millihertz);
  impl.observed_native_rate = plan.native_rate;
  impl.effective_output_target = plan.output_target;
  impl.effective_ceiling = plan.ceiling;
  if (!plan.warm) {
    impl.warm_streak = 0;
    impl.input_count = 0;
    impl.pacer.CommitGeneratedCount(0);
    impl.State(plan.reason);
    return batch;
  }

  bool recording_started = false;
  try {
    // Every allocation and descriptor write finishes before the first recorded
    // command. A failure here leaves the current native command buffer usable.
    impl.EnsureResources(source);
    auto& resources = *impl.resources;
    const fg::vk::CommandBuffer commands(impl.native, command_buffer);
    recording_started = true;
    if (resources.needs_initialization) {
      resources.allocator.InitializeImages(commands);
      resources.needs_initialization = false;
    }
    const uint64_t input_index = impl.input_count++;
    CopyNativeInput(impl.native, command_buffer, source, resources.chain->Input(input_index));
    if (impl.input_count < 2) {
      impl.pacer.CommitGeneratedCount(0);
      impl.State("warmup_native_pair");
      return batch;
    }
    resources.chain->DispatchShared(commands, input_index);
    ++impl.warm_streak;
    if (impl.warm_streak < fg::LSFG_HISTORY_SLOTS) {
      impl.pacer.CommitGeneratedCount(0);
      impl.State("warmup_history");
      return batch;
    }
    batch.count = uint32_t(std::min(plan.generations, batch.frames.size()));
    for (uint32_t index = 0; index < batch.count; ++index) {
      auto& output = resources.outputs[index];
      resources.chain->DispatchGeneration(commands, input_index, batch.count, index, index,
                                           output.Handle(), output.Extent());
      output.SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      batch.frames[index] = {output.Handle(), output.View(), output.Extent()};
    }
    impl.pacer.CommitGeneratedCount(batch.count);
    impl.window.generated_recorded += batch.count;
    impl.window.extras_min = std::min(impl.window.extras_min, batch.count);
    impl.window.extras_max = std::max(impl.window.extras_max, batch.count);
    impl.State("active");
  } catch (const std::exception& error) {
    batch = {};
    batch.discard_recording = recording_started;
    impl.Fail(recording_started ? "error" : "unavailable", error.what());
  } catch (...) {
    batch = {};
    batch.discard_recording = recording_started;
    impl.Fail("error", "unknown exception while preparing or recording LSFG");
  }
  return batch;
}

void ProsperoFrameGenerator::NotifySubmissionFailure() {
  // OFF may arrive after optional commands were recorded but before their
  // submission. A failed submission still invalidates those predicted image
  // layouts, even though no further generation is currently requested.
  impl_->Fail("error", "recorded frame was not submitted/presented; temporal state invalidated");
}

bool ProsperoFrameGenerator::IsUnavailable() const {
  return impl_->unavailable;
}

void ProsperoFrameGenerator::SetEnabledForSession(bool enabled, bool reset_history) {
  auto& impl = *impl_;
  if (impl.options.enabled == enabled && !reset_history) return;
  // Report the completed measurement window before disabling its collection.
  impl.LogWindow(true);
  impl.options.enabled = enabled;
  // The pacer owns no Vulkan resources. Give it the new request while retaining
  // the physical image layouts and all pending GPU allocations unchanged.
  impl.pacer = fg::FrameGenPacer(impl.options);
  impl.ResetHistory();
  impl.window = {};
  impl.observed_native_rate = 0.0f;
  impl.effective_output_target = 0.0f;
  impl.effective_ceiling = 0;
  if (impl.unavailable) {
    REXLOG_WARN("framegen: session request={}; unavailable until game restart after "
                 "the earlier error; native presentation retained",
                 enabled ? "on" : "off");
    return;
  }
  impl.State(!enabled ? "off_session" : impl.overlay_active
      ? "suspended_overlay" : "warmup_session_enabled");
}

void ProsperoFrameGenerator::NotifyFramePresented(bool generated, bool success) {
  if (!impl_->options.enabled) return;
  if (!impl_->window.start_ns) impl_->window.start_ns = prospero_metrics_clock_ns();
  if (!success) ++impl_->window.failed_images;
  else if (generated) ++impl_->window.generated_presented;
  else ++impl_->window.native_image_presented;
  impl_->LogWindow();
}

void ProsperoFrameGenerator::SetOverlayActive(bool active) {
  if (impl_->overlay_active == active) return;
  impl_->overlay_active = active;
  if (!impl_->options.enabled || impl_->unavailable) return;
  impl_->ResetHistory();
  impl_->State(active ? "suspended_overlay" : "warmup_after_overlay");
}
}  // namespace rex::ui::vulkan
