// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 The Simpsons Game PS5 contributors
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>

#include <rex/ui/vulkan/api.h>

namespace rex::ui::vulkan {
class VulkanDevice;

struct FrameGenerationSource {
  VkImage image = VK_NULL_HANDLE;
  VkFormat format = VK_FORMAT_UNDEFINED;
  VkExtent2D extent{};
  VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  VkPipelineStageFlags stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  VkAccessFlags access = VK_ACCESS_SHADER_READ_BIT;
  // A successful native guest refresh, not an allocation version or UI redraw.
  uint64_t native_serial = 0;
  uint64_t native_time_ns = 0;
};

struct GeneratedFrame {
  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkExtent2D extent{};
};

struct FrameGenerationBatch {
  std::array<GeneratedFrame, 3> frames{};
  uint32_t count = 0;
  // A failure after commands were recorded requires abandoning that recording.
  // Nothing using it may be submitted. Start the normal native path in a fresh
  // command buffer. Previously submitted paints remain valid and must complete.
  bool discard_recording = false;
};

class ProsperoFrameGenerator {
 public:
  using AwaitSubmittedPaints = std::function<bool()>;
  ProsperoFrameGenerator(const VulkanDevice& device,
                        AwaitSubmittedPaints await_submitted_paints);
  ~ProsperoFrameGenerator();
  ProsperoFrameGenerator(const ProsperoFrameGenerator&) = delete;
  ProsperoFrameGenerator& operator=(const ProsperoFrameGenerator&) = delete;

  // Call under the presenter consumer/paint lock before the first render pass.
  // The source is restored to its original layout/stage/access. The returned
  // RGBA8 images are shader-readable and must be presented in order before the
  // current native frame. Retain the native source and its configuration across
  // the entire batch; do not consume another mailbox image between its frames.
  // The same graphics queue orders history, compute and all paint submissions.
  // Output reuse includes fragment-read -> compute-write dependencies.
  // Actual output rate comes from the selected display mode, not its request.
  FrameGenerationBatch RecordNativeFrame(VkCommandBuffer command_buffer,
                                         const FrameGenerationSource& source,
                                         uint32_t actual_output_millihertz);

  // If a recorded batch cannot be submitted/presented, its predicted layouts
  // and history are not trusted. Disable FG for this session and keep resources
  // alive until the presenter's normal, fence-complete teardown.
  void NotifySubmissionFailure();

  // A terminal failure must also bypass the presenter's optional batch path.
  // Query under the same consumer/paint lock as RecordNativeFrame. Retaining
  // this object keeps resources referenced by submitted commands alive until
  // normal fence-complete teardown; observing failure does not destroy them.
  bool IsUnavailable() const;

  // Session-only request, applied under the presenter paint lock. Changing
  // state, or observing an intervening OFF/ON revision, restarts CPU pacing
  // and temporal warmup without destroying images or resetting their tracked
  // layouts while older submissions may still reference them. A terminal
  // unavailable state cannot be cleared by this request.
  void SetEnabledForSession(bool enabled, bool reset_history = false);

  // Called once per attempted image presentation, including native/UI redraws.
  // Success requires the image to have been drawn and vkQueuePresentKHR to have
  // succeeded; a successful clear-only present is not an image success. This
  // never counts a generated image or a UI redraw as a native guest refresh.
  void NotifyFramePresented(bool generated, bool success);

  // Overlay frames use the native presentation path. Log only the transition,
  // and restart temporal warmup after an overlay without freeing in-flight GPU
  // resources or assuming a large timestamp gap.
  void SetOverlayActive(bool active);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace rex::ui::vulkan
