#pragma once
#include <rex/ui/vulkan/api.h>
namespace rex::ui::vulkan {
class VulkanInstance {
 public:
  struct Functions {
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = nullptr;
    PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = nullptr;
    PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties = nullptr;
  } functions_;
  const Functions& functions() const { return functions_; }
  uint32_t api_version() const { return VK_API_VERSION_1_3; }
};
class VulkanDevice {
 public:
  struct Functions {
#define XE_UI_VULKAN_FUNCTION(name) PFN_##name name = nullptr;
#include <rex/ui/vulkan/functions/device_1_0.inc>
#undef XE_UI_VULKAN_FUNCTION
  } functions_;
  struct Properties {
    uint32_t apiVersion = VK_API_VERSION_1_3;
    bool nullDescriptor = true;
    bool vulkanMemoryModel = true;
    bool vulkanMemoryModelDeviceScope = true;
    bool vulkanMemoryModelAvailabilityVisibilityChains = true;
    bool shaderFloat16 = true;
    bool shaderStorageImageExtendedFormats = true;
    bool shaderStorageImageReadWithoutFormat = true;
    bool shaderStorageImageWriteWithoutFormat = true;
  } properties_;
  VulkanInstance instance_;
  const Functions& functions() const { return functions_; }
  const Properties& properties() const { return properties_; }
  const VulkanInstance* vulkan_instance() const { return &instance_; }
  VkDevice device() const { return reinterpret_cast<VkDevice>(uintptr_t(1)); }
  VkPhysicalDevice physical_device() const {
    return reinterpret_cast<VkPhysicalDevice>(uintptr_t(2));
  }
};
}
