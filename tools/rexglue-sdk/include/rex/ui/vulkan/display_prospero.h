#pragma once

#include <rex/ui/vulkan/instance.h>

namespace rex::ui::vulkan {

// The output clock is separate from the guest game's 30/60 FPS clock. The
// request is captured after configuration loading, before the driver opens
// VideoOut. The effective clock is published only after creating a surface.
bool InitializeProsperoDisplayPolicy();
uint32_t GetProsperoRequestedOutputHz();
uint32_t GetProsperoOutputMilliHertz();

// Selects an advertised 3840x2160 mode and compatible plane. A 120 Hz request
// prefers 119.88 Hz and falls back to 59.94 Hz when that is what the display
// supports. The default remains the working 60 Hz presentation policy.
VkResult CreateProsperoDisplaySurface(const VulkanInstance& instance,
                                     VkPhysicalDevice physical_device,
                                     VkSurfaceKHR* surface_out);

}  // namespace rex::ui::vulkan
