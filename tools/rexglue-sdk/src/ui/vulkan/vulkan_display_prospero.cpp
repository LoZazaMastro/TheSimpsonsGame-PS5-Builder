#include <rex/ui/vulkan/display_prospero.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <vector>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ui/surface_prospero.h>

extern "C" int sceSystemServiceHideSplashScreen();
extern "C" uint32_t simpsons_radv_videoout_refresh_millihertz();

REXCVAR_DEFINE_UINT32(prospero_output_hz, 60, "UI/PS5",
                     "PS5 display output: 60 or 120 Hz (experimental). Requires game restart; independent of native game FPS.");

namespace rex::ui::vulkan {
namespace {
constinit std::atomic<uint32_t> requested_output_hz{60};
constinit std::atomic<uint32_t> effective_output_millihertz{0};

template <typename T, typename Enumerator>
VkResult Enumerate(Enumerator enumerate, std::vector<T>& items) {
  // Display topology is normally fixed for a title, but handle a count change
  // without reading unwritten entries or spinning forever on VK_INCOMPLETE.
  for (unsigned attempt = 0; attempt < 3; ++attempt) {
    uint32_t count = 0;
    VkResult result = enumerate(&count, nullptr);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
      return result;
    }
    items.resize(count);
    if (!count) {
      return VK_SUCCESS;
    }
    result = enumerate(&count, items.data());
    if (result == VK_INCOMPLETE) {
      continue;
    }
    if (result == VK_SUCCESS) {
      items.resize(count);
    }
    return result;
  }
  items.clear();
  return VK_INCOMPLETE;
}

bool FitsExtent(VkExtent2D extent, VkExtent2D minimum, VkExtent2D maximum) {
  return extent.width >= minimum.width && extent.height >= minimum.height &&
         extent.width <= maximum.width && extent.height <= maximum.height;
}

bool FitsOrigin(VkOffset2D minimum, VkOffset2D maximum) {
  return minimum.x <= 0 && minimum.y <= 0 && maximum.x >= 0 && maximum.y >= 0;
}

}  // namespace

bool InitializeProsperoDisplayPolicy() {
  const uint32_t configured = REXCVAR_GET(prospero_output_hz);
  const uint32_t requested = configured == 120 ? 120 : 60;
  if (configured != requested) {
    REXLOG_WARN("PS5 display: unsupported output request {} Hz; using {} Hz", configured, requested);
  }
  // The title metadata declares capability. This per-session request keeps
  // that declaration from enabling 120 Hz for the default 60 Hz profile.
  if (setenv("SIMPSONS_VIDEOOUT_120HZ", requested == 120 ? "1" : "0", 1) != 0) {
    REXLOG_ERROR("PS5 display: could not configure output request (errno {})", errno);
    return false;
  }
  requested_output_hz.store(requested, std::memory_order_relaxed);
  effective_output_millihertz.store(0, std::memory_order_relaxed);
  REXLOG_INFO("PS5 display: requested {} Hz; native game FPS are configured separately", requested);
  return true;
}

uint32_t GetProsperoRequestedOutputHz() {
  return requested_output_hz.load(std::memory_order_relaxed);
}

uint32_t GetProsperoOutputMilliHertz() {
  return effective_output_millihertz.load(std::memory_order_acquire);
}

VkResult CreateProsperoDisplaySurface(const VulkanInstance& instance,
                                     VkPhysicalDevice physical_device,
                                     VkSurfaceKHR* surface_out) {
  if (!surface_out || !physical_device || !instance.instance()) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  *surface_out = VK_NULL_HANDLE;
  effective_output_millihertz.store(0, std::memory_order_release);
  const uint32_t requested_hz = GetProsperoRequestedOutputHz();
  const auto& fn = instance.functions();
  if (!instance.extensions().ext_KHR_display ||
      !fn.vkGetPhysicalDeviceDisplayPropertiesKHR ||
      !fn.vkGetPhysicalDeviceDisplayPlanePropertiesKHR ||
      !fn.vkGetDisplayPlaneSupportedDisplaysKHR || !fn.vkGetDisplayModePropertiesKHR ||
      !fn.vkGetDisplayPlaneCapabilitiesKHR || !fn.vkCreateDisplayPlaneSurfaceKHR) {
    REXGPU_ERROR("PS5 RADV does not provide the required VK_KHR_display functions");
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  std::vector<VkDisplayPropertiesKHR> displays;
  VkResult result = Enumerate<VkDisplayPropertiesKHR>(
      [&](uint32_t* count, VkDisplayPropertiesKHR* values) {
        return fn.vkGetPhysicalDeviceDisplayPropertiesKHR(physical_device, count, values);
      }, displays);
  if (result != VK_SUCCESS || displays.empty()) {
    REXGPU_ERROR("PS5 RADV reports no display (result {})", int32_t(result));
    return result == VK_SUCCESS ? VK_ERROR_INITIALIZATION_FAILED : result;
  }

  std::vector<VkDisplayPlanePropertiesKHR> planes;
  result = Enumerate<VkDisplayPlanePropertiesKHR>(
      [&](uint32_t* count, VkDisplayPlanePropertiesKHR* values) {
        return fn.vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physical_device, count, values);
      }, planes);
  if (result != VK_SUCCESS || planes.empty()) {
    REXGPU_ERROR("PS5 RADV reports no display plane (result {})", int32_t(result));
    return result == VK_SUCCESS ? VK_ERROR_INITIALIZATION_FAILED : result;
  }

  struct Candidate {
    VkDisplayKHR display;
    VkDisplayModePropertiesKHR mode;
    uint32_t distance_from_request;
  };
  std::vector<Candidate> candidates;
  for (const auto& display : displays) {
    if (!(display.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)) {
      continue;
    }
    std::vector<VkDisplayModePropertiesKHR> modes;
    result = Enumerate<VkDisplayModePropertiesKHR>(
        [&](uint32_t* count, VkDisplayModePropertiesKHR* values) {
          return fn.vkGetDisplayModePropertiesKHR(physical_device, display.display, count, values);
        }, modes);
    if (result != VK_SUCCESS) {
      continue;
    }
    for (const auto& mode : modes) {
      const auto& p = mode.parameters;
      if (p.visibleRegion.width != ProsperoDisplaySurface::kWidth ||
          p.visibleRegion.height != ProsperoDisplaySurface::kHeight ||
          !((p.refreshRate >= 59000 && p.refreshRate <= 61000) ||
            (requested_hz == 120 && p.refreshRate >= 119000 && p.refreshRate <= 121000))) {
        continue;
      }
      const uint32_t target = requested_hz * 1000;
      const uint32_t distance = p.refreshRate > target ? p.refreshRate - target
                                                      : target - p.refreshRate;
      candidates.push_back({display.display, mode, distance});
    }
  }
  std::stable_sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
    return a.distance_from_request < b.distance_from_request;
  });

  for (const auto& candidate : candidates) {
    for (uint32_t plane_index = 0; plane_index < planes.size(); ++plane_index) {
      const auto& plane = planes[plane_index];
      if (plane.currentDisplay && plane.currentDisplay != candidate.display) {
        continue;
      }
      std::vector<VkDisplayKHR> supported;
      result = Enumerate<VkDisplayKHR>(
          [&](uint32_t* count, VkDisplayKHR* values) {
            return fn.vkGetDisplayPlaneSupportedDisplaysKHR(physical_device, plane_index,
                                                            count, values);
          }, supported);
      if (result != VK_SUCCESS ||
          std::find(supported.begin(), supported.end(), candidate.display) == supported.end()) {
        continue;
      }
      VkDisplayPlaneCapabilitiesKHR capabilities{};
      result = fn.vkGetDisplayPlaneCapabilitiesKHR(physical_device, candidate.mode.displayMode,
                                                   plane_index, &capabilities);
      const VkExtent2D extent = candidate.mode.parameters.visibleRegion;
      if (result != VK_SUCCESS ||
          !FitsExtent(extent, capabilities.minSrcExtent, capabilities.maxSrcExtent) ||
          !FitsExtent(extent, capabilities.minDstExtent, capabilities.maxDstExtent) ||
          !FitsOrigin(capabilities.minSrcPosition, capabilities.maxSrcPosition) ||
          !FitsOrigin(capabilities.minDstPosition, capabilities.maxDstPosition)) {
        continue;
      }
      VkDisplayPlaneAlphaFlagBitsKHR alpha = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
      if (!(capabilities.supportedAlpha & alpha)) {
        if (capabilities.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR) {
          alpha = VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR;
        } else {
          continue;
        }
      }
      VkDisplaySurfaceCreateInfoKHR create{};
      create.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
      create.displayMode = candidate.mode.displayMode;
      create.planeIndex = plane_index;
      create.planeStackIndex = plane.currentStackIndex;
      create.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
      create.globalAlpha = 1.0f;
      create.alphaMode = alpha;
      create.imageExtent = extent;
      VkSurfaceKHR surface = VK_NULL_HANDLE;
      result = fn.vkCreateDisplayPlaneSurfaceKHR(instance.instance(), &create, nullptr, &surface);
      if (result == VK_SUCCESS && surface != VK_NULL_HANDLE) {
        const uint32_t effective = simpsons_radv_videoout_refresh_millihertz();
        if (effective != 59940 && effective != 119880) {
          fn.vkDestroySurfaceKHR(instance.instance(), surface, nullptr);
          REXGPU_ERROR("PS5 display: driver has no verified output clock");
          return VK_ERROR_INITIALIZATION_FAILED;
        }
        *surface_out = surface;
        effective_output_millihertz.store(effective, std::memory_order_release);
        REXGPU_INFO("PS5 display: {}x{}, requested {} Hz, mode {:.3f} Hz, output {:.3f} Hz, plane {}",
                    extent.width, extent.height, requested_hz,
                    double(candidate.mode.parameters.refreshRate) / 1000.0,
                    double(effective) / 1000.0, plane_index);
        if (requested_hz == 120 && effective < 119000) {
          REXLOG_WARN("PS5 display: 120 Hz is unavailable for this session; using the supported 60 Hz output");
        }
        const int splash_result = sceSystemServiceHideSplashScreen();
        if (splash_result != 0) {
          REXLOG_WARN("sceSystemServiceHideSplashScreen returned {:08X}",
                       uint32_t(splash_result));
        }
        return VK_SUCCESS;
      }
    }
  }
  REXGPU_ERROR("PS5 display has no usable 3840x2160 mode for the {} Hz request and compatible plane", requested_hz);
  return VK_ERROR_INITIALIZATION_FAILED;
}

}  // namespace rex::ui::vulkan
