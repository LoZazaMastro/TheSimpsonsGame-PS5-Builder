// Host fixture boundary. Vulkan structs and PFN types are the repository's real
// Vulkan C headers; no SDK ABI, loader, driver or GPU is exercised here.
#pragma once
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
