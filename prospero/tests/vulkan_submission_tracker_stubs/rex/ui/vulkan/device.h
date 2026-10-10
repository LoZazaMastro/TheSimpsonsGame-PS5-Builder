#pragma once
// Host test seam: only the Vulkan calls the real submission tracker uses.
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>

namespace rex::ui::vulkan {
class VulkanDevice {
 public:
  struct Functions {
    PFN_vkCreateFence vkCreateFence;
    PFN_vkDestroyFence vkDestroyFence;
    PFN_vkResetFences vkResetFences;
    PFN_vkGetFenceStatus vkGetFenceStatus;
    PFN_vkWaitForFences vkWaitForFences;
  };
  VulkanDevice(VkDevice device, Functions functions) : device_(device), functions_(functions) {}
  VkDevice device() const { return device_; }
  const Functions& functions() const { return functions_; }

 private:
  VkDevice device_;
  Functions functions_;
};
}  // namespace rex::ui::vulkan
