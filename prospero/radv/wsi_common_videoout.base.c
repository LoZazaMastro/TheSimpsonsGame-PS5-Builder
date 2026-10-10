/*
 * Copyright © 2026 Mihawk
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * VK_KHR_display on the PlayStation 5's one display, VideoOut, in place of
 * the DRM backend (wsi_common_display.c), which the console does not have.
 *
 * The display has one plane and one 3840x2160 mode at 59.94 Hz, and a 119.88
 * Hz mode first where the title's metadata declares high-frame-rate output
 * and the display refreshes at it (measured, as PS5_Vulkan's ps5vk measured
 * both). VideoOut's framebuffers are the process's: five 32 MiB buffers of
 * direct memory registered once, in the 64 KiB R_X tiles VideoOut scans out
 * (ps5platform/videoout.h). A swapchain takes the first of them as its
 * images, imported into its device as host memory
 * (VK_EXT_external_memory_host) and laid out as the display takes them (the
 * device's display swizzle, radeon_info), so a device made again, as
 * RetroArch makes one for each core, presents where the last one did and the
 * screen keeps its last frame in between.
 *
 * A present waits for the frame on a thread and flips it with
 * sceVideoOutSubmitFlip at the next vblank; FIFO is the only present mode. An
 * acquire takes a buffer that is neither on screen, nor waiting for its flip
 * to show, nor held by an application, the one flipped longest ago, and waits
 * for a flip or a vblank when none is.
 */

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c11/threads.h"
#include "util/macros.h"
#include "util/os_time.h"
#include "util/u_math.h"
#include "vk_device.h"
#include "vk_instance.h"
#include "vk_physical_device.h"
#include "vk_util.h"
#include "wsi_common_entrypoints.h"
#include "wsi_common_private.h"

#if defined(__PROSPERO__)
#include <immintrin.h>
#include <ps5platform/kernel.h>
#include <ps5platform/videoout.h>
#else
#include <sys/mman.h>
#endif

#define VIDEOOUT_WIDTH 3840
#define VIDEOOUT_HEIGHT 2160
#define VIDEOOUT_BUFFERS 5
/* A 3840x2160 four-byte image in 64 KiB tiles is 30x17 tiles, 31.9 MiB. */
#define VIDEOOUT_BUFFER_BYTES (UINT64_C(32) << 20)
#define VIDEOOUT_BUFFER_ALIGNMENT (UINT64_C(2) << 20)
#define VIDEOOUT_REFRESH_MILLIHERTZ 59940
#define VIDEOOUT_HIGH_REFRESH_MILLIHERTZ 119880
/* A period above this after the high-frame-rate mode was taken: the display
 * went on refreshing at 60 Hz, as a tester's did (PS5_Vulkan, R31). */
#define VIDEOOUT_HIGH_REFRESH_LIMIT_NS UINT64_C(12500000)
#define VIDEOOUT_PERIOD_INTERVALS 6
/* param.json's attribute3 bits for high-frame-rate output (PS5_Vulkan R31). */
#define VIDEOOUT_ATTRIBUTE3_HIGH_FRAME_RATE 0x80040u

#if !defined(__PROSPERO__)
/* The host model: flips show at once and a vblank comes every millisecond,
 * enough for the host builds to run the swapchain's logic. */
#define PS5_VIDEO_OUT_USER_SYSTEM 0xff
#define PS5_VIDEO_OUT_PIXEL_FORMAT_B8G8R8A8_SDR UINT64_C(0x8000000000000000)
#define PS5_VIDEO_OUT_TILING_64KB_R_X 0u
#define PS5_VIDEO_OUT_ATTRIBUTE_BYTES 80
#define PS5_VIDEO_OUT_FLIP_STATUS_WORDS 16
#define PS5_VIDEO_OUT_FLIP_STATUS_SHOWN_ARGUMENT 3
#define PS5_VIDEO_OUT_FLIP_VSYNC 1
#define PS5_VIDEO_OUT_MODE_HIGH_FRAME_RATE 15u
#define PS5_VIDEO_OUT_MODE_RESTORE 1u

struct ps5_video_out_buffer {
   void *data;
   void *metadata;
   void *reserved[2];
};

static int64_t host_shown_argument;

static int
sceVideoOutOpen(int32_t user, int32_t bus, int32_t index, const void *parameter)
{
   return 1;
}

static int
sceVideoOutSetFlipRate(int32_t handle, int32_t rate)
{
   return 0;
}

static void
sceVideoOutSetBufferAttribute2(void *attribute, uint64_t pixel_format, uint32_t tiling, uint32_t width,
                               uint32_t height, uint64_t reserved1, uint32_t reserved2, uint64_t reserved3)
{
}

static int
sceVideoOutRegisterBuffers2(int32_t handle, int32_t set, int32_t start, struct ps5_video_out_buffer *buffers,
                            int32_t count, void *attribute, int32_t reserved, void *option)
{
   return 0;
}

static int
sceVideoOutSubmitFlip(int32_t handle, int32_t buffer_index, uint32_t mode, int64_t argument)
{
   host_shown_argument = argument;
   return 0;
}

static int
sceVideoOutGetFlipStatus(int32_t handle, uint64_t status[PS5_VIDEO_OUT_FLIP_STATUS_WORDS])
{
   memset(status, 0, PS5_VIDEO_OUT_FLIP_STATUS_WORDS * sizeof(uint64_t));
   status[PS5_VIDEO_OUT_FLIP_STATUS_SHOWN_ARGUMENT] = host_shown_argument;
   return 0;
}

static int
sceVideoOutWaitVblank(int32_t handle)
{
   os_time_sleep(1000);
   return 0;
}

static int
sceVideoOutIsOutputSupported(int32_t handle, uint32_t mode, const void *a, const void *b, const void *c)
{
   return 0;
}

static int
sceVideoOutConfigureOutput(int32_t handle, uint32_t mode, const void *a, const void *b, const void *c)
{
   return -1;
}
#endif

/* The display and its modes: one each per process, as VideoOut is. */
struct wsi_videoout_mode {
   uint32_t refresh_millihertz;
   bool high_frame_rate;
};

struct wsi_videoout_display {
   int dummy;
};

static struct wsi_videoout_display videoout_display;
static struct wsi_videoout_mode videoout_mode = {.refresh_millihertz = VIDEOOUT_REFRESH_MILLIHERTZ};
static struct wsi_videoout_mode videoout_high_mode = {
   .refresh_millihertz = VIDEOOUT_HIGH_REFRESH_MILLIHERTZ,
   .high_frame_rate = true,
};

static VkDisplayKHR
videoout_display_handle(void)
{
   return (VkDisplayKHR)(uintptr_t)&videoout_display;
}

static VkDisplayModeKHR
videoout_mode_handle(struct wsi_videoout_mode *mode)
{
   return (VkDisplayModeKHR)(uintptr_t)mode;
}

static struct wsi_videoout_mode *
videoout_mode_from_handle(VkDisplayModeKHR mode)
{
   return (struct wsi_videoout_mode *)(uintptr_t)mode;
}

static const VkExtent2D videoout_extent = {VIDEOOUT_WIDTH, VIDEOOUT_HEIGHT};

struct wsi_videoout_swapchain;

/* A present waiting for its frame before its flip. */
struct wsi_videoout_present {
   struct wsi_videoout_swapchain *chain;
   uint32_t buffer;
   VkFence fence;
};

/* VideoOut, its framebuffers and what each holds, under lock. */
struct wsi_videoout_output {
   mtx_t lock;
   cnd_t changed;
   bool opened;
   int handle;
   bool high_frame_rate;
   bool modes_settled;
   bool high_mode_offered;
   uint8_t *buffers;
   /* The argument of the latest flip submitted; each flip's is one more. */
   int64_t flip_argument;
   struct {
      /* The argument this buffer was last flipped with, 0 before its first flip. */
      int64_t argument;
      /* An application holds it, acquired and not yet presented. */
      bool held;
      /* Presented, its flip waiting for the frame. */
      bool pending;
   } buffer[VIDEOOUT_BUFFERS];
   /* The swapchain presenting now; older ones are retired. */
   struct wsi_videoout_swapchain *current;

   /* The flip thread and its queue. */
   bool thread_started;
   thrd_t thread;
   struct wsi_videoout_present queue[VIDEOOUT_BUFFERS * 2];
   uint32_t queue_head, queue_count;
};

static struct wsi_videoout_output videoout_output = {
   .handle = -1,
};
static once_flag videoout_once = ONCE_FLAG_INIT;

static void
videoout_output_init_once(void)
{
   mtx_init(&videoout_output.lock, mtx_plain);
   cnd_init(&videoout_output.changed);
}

struct wsi_videoout_swapchain {
   struct wsi_swapchain base;
   bool retired;
   /* Presents queued and not yet flipped. */
   uint32_t in_flight;
   struct wsi_image images[];
};

struct wsi_videoout {
   struct wsi_interface base;
};

/* --- VideoOut ------------------------------------------------------------ */

static bool
videoout_title_declares_high_frame_rate(void)
{
   FILE *const file = fopen("/app0/sce_sys/param.json", "rb");
   if (file == NULL)
      return false;
   char text[16384];
   const size_t length = fread(text, 1, sizeof(text) - 1, file);
   fclose(file);
   text[length] = '\0';
   const char *at = strstr(text, "\"attribute3\"");
   if (at == NULL || (at = strchr(at, ':')) == NULL)
      return false;
   const unsigned long value = strtoul(at + 1, NULL, 0);
   return (value & VIDEOOUT_ATTRIBUTE3_HIGH_FRAME_RATE) == VIDEOOUT_ATTRIBUTE3_HIGH_FRAME_RATE;
}

/* The refresh period: the median of a few intervals between vblank waits,
 * the first wait finding the phase. 0 when a wait fails. */
static uint64_t
videoout_vblank_period_ns(int handle)
{
   uint64_t intervals[VIDEOOUT_PERIOD_INTERVALS];
   if (sceVideoOutWaitVblank(handle) != 0)
      return 0;
   uint64_t previous = os_time_get_nano();
   for (unsigned at = 0; at < VIDEOOUT_PERIOD_INTERVALS; at++) {
      if (sceVideoOutWaitVblank(handle) != 0)
         return 0;
      const uint64_t now = os_time_get_nano();
      intervals[at] = now - previous;
      previous = now;
   }
   for (unsigned i = 1; i < VIDEOOUT_PERIOD_INTERVALS; i++)
      for (unsigned j = i; j > 0 && intervals[j - 1] > intervals[j]; j--) {
         const uint64_t swap = intervals[j];
         intervals[j] = intervals[j - 1];
         intervals[j - 1] = swap;
      }
   return intervals[VIDEOOUT_PERIOD_INTERVALS / 2];
}

/* Opens VideoOut once per process, in the high-frame-rate mode where the
 * title declares it, the output takes it and the display then refreshes at
 * it. Under lock. */
static bool
videoout_open_locked(void)
{
   struct wsi_videoout_output *out = &videoout_output;
   if (out->opened)
      return out->handle >= 0;
   out->opened = true;
   out->handle = sceVideoOutOpen(PS5_VIDEO_OUT_USER_SYSTEM, 0, 0, NULL);
   if (out->handle < 0) {
      fprintf(stderr, "wsi/videoout: sceVideoOutOpen failed: 0x%08x\n", (unsigned)out->handle);
      return false;
   }
   if (videoout_title_declares_high_frame_rate()) {
      const int supported =
         sceVideoOutIsOutputSupported(out->handle, PS5_VIDEO_OUT_MODE_HIGH_FRAME_RATE, NULL, NULL, NULL);
      const int configured =
         supported > 0
            ? sceVideoOutConfigureOutput(out->handle, PS5_VIDEO_OUT_MODE_HIGH_FRAME_RATE, NULL, NULL, NULL)
            : supported;
      out->high_frame_rate = configured == 0;
      if (out->high_frame_rate) {
         const uint64_t period = videoout_vblank_period_ns(out->handle);
         if (period > VIDEOOUT_HIGH_REFRESH_LIMIT_NS) {
            sceVideoOutConfigureOutput(out->handle, PS5_VIDEO_OUT_MODE_RESTORE, NULL, NULL, NULL);
            out->high_frame_rate = false;
            fprintf(stderr,
                    "wsi/videoout: 119.88 Hz accepted, but a vblank comes every %.3f ms; 59.94 Hz restored\n",
                    (double)period / 1e6);
         }
      } else {
         fprintf(stderr, "wsi/videoout: 119.88 Hz %s (%d); presenting at 59.94 Hz\n",
                 supported > 0 ? "refused" : "not supported", configured);
      }
   }
   return true;
}

static void
videoout_settle_modes(void)
{
   call_once(&videoout_once, videoout_output_init_once);
   mtx_lock(&videoout_output.lock);
   if (!videoout_output.modes_settled) {
      videoout_output.modes_settled = true;
      videoout_output.high_mode_offered = videoout_open_locked() && videoout_output.high_frame_rate;
   }
   mtx_unlock(&videoout_output.lock);
}

/* The framebuffers, cleared and registered once per process. Under lock. */
static VkResult
videoout_register_locked(void)
{
   struct wsi_videoout_output *out = &videoout_output;
   if (out->buffers)
      return VK_SUCCESS;
   if (!videoout_open_locked())
      return VK_ERROR_INITIALIZATION_FAILED;

   const size_t bytes = VIDEOOUT_BUFFERS * VIDEOOUT_BUFFER_BYTES;
   uint8_t *buffers = NULL;
#if defined(__PROSPERO__)
   int64_t physical = -1;
   if (sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), bytes, VIDEOOUT_BUFFER_ALIGNMENT,
                                     PS5_KERNEL_DIRECT_TYPE_CPU, &physical) != 0)
      return VK_ERROR_OUT_OF_DEVICE_MEMORY;
   void *address = NULL;
   if (sceKernelMapDirectMemory(&address, bytes,
                                PS5_KERNEL_PROT_CPU_READ | PS5_KERNEL_PROT_CPU_WRITE | PS5_KERNEL_PROT_GPU_READ |
                                   PS5_KERNEL_PROT_GPU_WRITE,
                                0, physical, VIDEOOUT_BUFFER_ALIGNMENT) != 0) {
      sceKernelReleaseDirectMemory(physical, bytes);
      return VK_ERROR_OUT_OF_DEVICE_MEMORY;
   }
   buffers = address;
   /* The display reads memory, not the CPU's cache. */
   memset(buffers, 0, bytes);
   for (size_t at = 0; at < bytes; at += 64)
      _mm_clflush(buffers + at);
   _mm_mfence();
#else
   buffers = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   if (buffers == MAP_FAILED)
      return VK_ERROR_OUT_OF_DEVICE_MEMORY;
#endif

   int result = sceVideoOutSetFlipRate(out->handle, 0);
   struct ps5_video_out_buffer registered[VIDEOOUT_BUFFERS];
   for (uint32_t i = 0; i < VIDEOOUT_BUFFERS; i++)
      registered[i] = (struct ps5_video_out_buffer){.data = buffers + i * VIDEOOUT_BUFFER_BYTES};
   uint8_t attribute[PS5_VIDEO_OUT_ATTRIBUTE_BYTES] = {0};
   sceVideoOutSetBufferAttribute2(attribute, PS5_VIDEO_OUT_PIXEL_FORMAT_B8G8R8A8_SDR,
                                  PS5_VIDEO_OUT_TILING_64KB_R_X, VIDEOOUT_WIDTH, VIDEOOUT_HEIGHT, 0, 0, 0);
   if (result == 0)
      result = sceVideoOutRegisterBuffers2(out->handle, 0, 0, registered, VIDEOOUT_BUFFERS, attribute, 0, NULL);
   if (result != 0) {
      fprintf(stderr, "wsi/videoout: the framebuffers were not registered: 0x%08x\n", (unsigned)result);
      /* The memory stays: VideoOut may hold a buffer it was given. */
      return VK_ERROR_INITIALIZATION_FAILED;
   }
   out->buffers = buffers;
   return VK_SUCCESS;
}

static int64_t
videoout_shown_argument_locked(void)
{
   uint64_t status[PS5_VIDEO_OUT_FLIP_STATUS_WORDS] = {0};
   if (videoout_output.flip_argument == 0 || sceVideoOutGetFlipStatus(videoout_output.handle, status) != 0)
      return 0;
   return (int64_t)status[PS5_VIDEO_OUT_FLIP_STATUS_SHOWN_ARGUMENT];
}

/* The flip thread: each present's flip once its frame is done, in order. */
static int
videoout_flip_thread(void *data)
{
   struct wsi_videoout_output *out = &videoout_output;
   mtx_lock(&out->lock);
   for (;;) {
      while (out->queue_count == 0)
         cnd_wait(&out->changed, &out->lock);
      const struct wsi_videoout_present present = out->queue[out->queue_head];
      mtx_unlock(&out->lock);

      struct wsi_videoout_swapchain *chain = present.chain;
      const struct wsi_device *wsi = chain->base.wsi;
      wsi->WaitForFences(chain->base.device, 1, &present.fence, VK_TRUE, UINT64_MAX);

      mtx_lock(&out->lock);
      const int64_t argument = out->flip_argument + 1;
      const int result = sceVideoOutSubmitFlip(out->handle, (int32_t)present.buffer, PS5_VIDEO_OUT_FLIP_VSYNC,
                                               argument);
      if (result == 0) {
         out->flip_argument = argument;
         out->buffer[present.buffer].argument = argument;
      } else {
         fprintf(stderr, "wsi/videoout: sceVideoOutSubmitFlip(%u) failed: 0x%08x\n", present.buffer,
                 (unsigned)result);
      }
      out->buffer[present.buffer].pending = false;
      out->queue_head = (out->queue_head + 1) % ARRAY_SIZE(out->queue);
      out->queue_count--;
      chain->in_flight--;
      cnd_broadcast(&out->changed);
   }
   return 0;
}

/* --- VK_KHR_display -------------------------------------------------------- */

VKAPI_ATTR VkResult VKAPI_CALL
wsi_GetPhysicalDeviceDisplayPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                          VkDisplayPropertiesKHR *pProperties)
{
   VK_OUTARRAY_MAKE_TYPED(VkDisplayPropertiesKHR, out, pProperties, pPropertyCount);
   vk_outarray_append_typed(VkDisplayPropertiesKHR, &out, prop)
   {
      *prop = (VkDisplayPropertiesKHR){
         .display = videoout_display_handle(),
         .displayName = "PS5 VideoOut",
         /* The size of the screen VideoOut drives is not known. */
         .physicalDimensions = {0, 0},
         .physicalResolution = videoout_extent,
         .supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
         .planeReorderPossible = VK_FALSE,
         .persistentContent = VK_FALSE,
      };
   }
   return vk_outarray_status(&out);
}

VKAPI_ATTR VkResult VKAPI_CALL
wsi_GetPhysicalDeviceDisplayProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                           VkDisplayProperties2KHR *pProperties)
{
   VK_OUTARRAY_MAKE_TYPED(VkDisplayProperties2KHR, out, pProperties, pPropertyCount);
   vk_outarray_append_typed(VkDisplayProperties2KHR, &out, prop)
   {
      uint32_t count = 1;
      wsi_GetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &count, &prop->displayProperties);
   }
   return vk_outarray_status(&out);
}

VKAPI_ATTR VkResult VKAPI_CALL
wsi_GetPhysicalDeviceDisplayPlanePropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                               VkDisplayPlanePropertiesKHR *pProperties)
{
   VK_OUTARRAY_MAKE_TYPED(VkDisplayPlanePropertiesKHR, out, pProperties, pPropertyCount);
   vk_outarray_append_typed(VkDisplayPlanePropertiesKHR, &out, prop)
   {
      *prop = (VkDisplayPlanePropertiesKHR){
         .currentDisplay = videoout_display_handle(),
         .currentStackIndex = 0,
      };
   }
   return vk_outarray_status(&out);
}

VKAPI_ATTR VkResult VKAPI_CALL
wsi_GetPhysicalDeviceDisplayPlaneProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
                                                VkDisplayPlaneProperties2KHR *pProperties)
{
   VK_OUTARRAY_MAKE_TYPED(VkDisplayPlaneProperties2KHR, out, pProperties, pPropertyCount);
   vk_outarray_append_typed(VkDisplayPlaneProperties2KHR, &out, prop)
   {
      uint32_t count = 1;
      wsi_GetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, &count, &prop->displayPlaneProperties);
   }
   return vk_outarray_status(&out);
}

VKAPI_ATTR VkResult VKAPI_CALL
wsi_GetDisplayPlaneSupportedDisplaysKHR(VkPhysicalDevice physicalDevice, uint32_t planeIndex,
                                        uint32_t *pDisplayCount, VkDisplayKHR *pDisplays)
{
   VK_OUTARRAY_MAKE_TYPED(VkDisplayKHR, out, pDisplays, pDisplayCount);
   if (planeIndex == 0) {
      vk_outarray_append_typed(VkDisplayKHR, &out, display)
         *display = videoout_display_handle();
   }
   return vk_outarray_status(&out);
}

/* The fastest mode first: an application taking the first mode gets 119.88
 * Hz where it is offered. */
static uint32_t
videoout_modes(struct wsi_videoout_mode *modes[2])
{
   videoout_settle_modes();
   uint32_t count = 0;
   if (videoout_output.high_mode_offered)
      modes[count++] = &videoout_high_mode;
   modes[count++] = &videoout_mode;
   return count;
}

VKAPI_ATTR VkResult VKAPI_CALL
wsi_GetDisplayModePropertiesKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, uint32_t *pPropertyCount,
                                VkDisplayModePropertiesKHR *pProperties)
{
   struct wsi_videoout_mode *modes[2];
   const uint32_t mode_count = videoout_modes(modes);
   VK_OUTARRAY_MAKE_TYPED(VkDisplayModePropertiesKHR, out, pProperties, pPropertyCount);
   for (uint32_t i = 0; i < mode_count; i++) {
      vk_outarray_append_typed(VkDisplayModePropertiesKHR, &out, prop)
      {
         *prop = (VkDisplayModePropertiesKHR){
            .displayMode = videoout_mode_handle(modes[i]),
            .parameters = {.visibleRegion = videoout_extent, .refreshRate = modes[i]->refresh_millihertz},
         };
      }
   }
   return vk_outarray_status(&out);
}

VKAPI_ATTR VkResult VKAPI_CALL
wsi_GetDisplayModeProperties2KHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, uint32_t *pPropertyCount,
                                 VkDisplayModeProperties2KHR *pProperties)
{
   struct wsi_videoout_mode *modes[2];
   const uint32_t mode_count = videoout_modes(modes);
   VK_OUTARRAY_MAKE_TYPED(VkDisplayModeProperties2KHR, out, pProperties, pPropertyCount);
   for (uint32_t i = 0; i < mode_count; i++) {
      vk_outarray_append_typed(VkDisplayModeProperties2KHR, &out, prop)
      {
         prop->displayModeProperties = (VkDisplayModePropertiesKHR){
            .displayMode = videoout_mode_handle(modes[i]),
            .parameters = {.visibleRegion = videoout_extent, .refreshRate = modes[i]->refresh_millihertz},
         };
      }
   }
   return vk_outarray_status(&out);
}

VKAPI_ATTR VkResult VKAPI_CALL
wsi_CreateDisplayModeKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display,
                         const VkDisplayModeCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                         VkDisplayModeKHR *pMode)
{
   /* VideoOut's modes are the ones reported and it makes no others: one
    * asked for with a reported mode's parameters is that mode. */
   struct wsi_videoout_mode *modes[2];
   const uint32_t mode_count = videoout_modes(modes);
   const VkDisplayModeParametersKHR *parameters = &pCreateInfo->parameters;
   for (uint32_t i = 0; i < mode_count; i++) {
      if (parameters->visibleRegion.width == videoout_extent.width &&
          parameters->visibleRegion.height == videoout_extent.height &&
          parameters->refreshRate == modes[i]->refresh_millihertz) {
         *pMode = videoout_mode_handle(modes[i]);
         return VK_SUCCESS;
      }
   }
   return VK_ERROR_INITIALIZATION_FAILED;
}

VKAPI_ATTR VkResult VKAPI_CALL
wsi_GetDisplayPlaneCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkDisplayModeKHR mode, uint32_t planeIndex,
                                   VkDisplayPlaneCapabilitiesKHR *pCapabilities)
{
   *pCapabilities = (VkDisplayPlaneCapabilitiesKHR){
      .supportedAlpha = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR,
      .minSrcPosition = {0, 0},
      .maxSrcPosition = {0, 0},
      .minSrcExtent = videoout_extent,
      .maxSrcExtent = videoout_extent,
      .minDstPosition = {0, 0},
      .maxDstPosition = {0, 0},
      .minDstExtent = videoout_extent,
      .maxDstExtent = videoout_extent,
   };
   return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
wsi_GetDisplayPlaneCapabilities2KHR(VkPhysicalDevice physicalDevice,
                                    const VkDisplayPlaneInfo2KHR *pDisplayPlaneInfo,
                                    VkDisplayPlaneCapabilities2KHR *pCapabilities)
{
   return wsi_GetDisplayPlaneCapabilitiesKHR(physicalDevice, pDisplayPlaneInfo->mode, pDisplayPlaneInfo->planeIndex,
                                             &pCapabilities->capabilities);
}

VKAPI_ATTR VkResult VKAPI_CALL
wsi_CreateDisplayPlaneSurfaceKHR(VkInstance _instance, const VkDisplaySurfaceCreateInfoKHR *pCreateInfo,
                                 const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface)
{
   VK_FROM_HANDLE(vk_instance, instance, _instance);

   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR);

   VkIcdSurfaceDisplay *surface =
      vk_zalloc2(&instance->alloc, pAllocator, sizeof(*surface), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (surface == NULL)
      return VK_ERROR_OUT_OF_HOST_MEMORY;

   surface->base.platform = VK_ICD_WSI_PLATFORM_DISPLAY;
   surface->displayMode = pCreateInfo->displayMode;
   surface->planeIndex = pCreateInfo->planeIndex;
   surface->planeStackIndex = pCreateInfo->planeStackIndex;
   surface->transform = pCreateInfo->transform;
   surface->globalAlpha = pCreateInfo->globalAlpha;
   surface->alphaMode = pCreateInfo->alphaMode;
   surface->imageExtent = pCreateInfo->imageExtent;

   *pSurface = VkIcdSurfaceBase_to_handle(&surface->base);
   return VK_SUCCESS;
}

/* --- Surfaces ------------------------------------------------------------ */

static VkResult
videoout_surface_get_support(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device, uint32_t queueFamilyIndex,
                             VkBool32 *pSupported)
{
   /* Flips come from the CPU once a frame is done, from any queue's work. */
   *pSupported = VK_TRUE;
   return VK_SUCCESS;
}

static VkResult
videoout_surface_get_capabilities2(VkIcdSurfaceBase *icd_surface, struct wsi_device *wsi_device,
                                   const void *info_next, VkSurfaceCapabilities2KHR *caps)
{
   VkSurfaceCapabilitiesKHR *const c = &caps->surfaceCapabilities;
   const VkSurfacePresentModeKHR *present_mode = vk_find_struct_const(info_next, SURFACE_PRESENT_MODE_KHR);

   c->minImageCount = 2;
   c->maxImageCount = VIDEOOUT_BUFFERS;
   c->currentExtent = videoout_extent;
   c->minImageExtent = videoout_extent;
   c->maxImageExtent = videoout_extent;
   c->maxImageArrayLayers = 1;
   c->supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
   c->currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
   c->supportedCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

   VkImageUsageFlags image_usage = wsi_caps_get_image_usage();
   VkImageUsageFlags2CreateInfoKHR *usage2 = vk_find_struct(caps->pNext, IMAGE_USAGE_FLAGS_2_CREATE_INFO_KHR);
   if (usage2)
      usage2->usage = image_usage;
   else
      c->supportedUsageFlags = image_usage;

   vk_foreach_struct (ext, caps->pNext) {
      switch (ext->sType) {
      case VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR: {
         VkSurfaceProtectedCapabilitiesKHR *protected = (void *)ext;
         protected->supportsProtected = VK_FALSE;
         break;
      }
      case VK_STRUCTURE_TYPE_SURFACE_PRESENT_SCALING_CAPABILITIES_KHR: {
         VkSurfacePresentScalingCapabilitiesKHR *scaling = (void *)ext;
         scaling->supportedPresentScaling = 0;
         scaling->supportedPresentGravityX = 0;
         scaling->supportedPresentGravityY = 0;
         scaling->minScaledImageExtent = c->minImageExtent;
         scaling->maxScaledImageExtent = c->maxImageExtent;
         break;
      }
      case VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_COMPATIBILITY_KHR: {
         /* FIFO only. */
         VkSurfacePresentModeCompatibilityKHR *compat = (void *)ext;
         if (compat->pPresentModes) {
            if (compat->presentModeCount) {
               assert(present_mode);
               compat->pPresentModes[0] = present_mode->presentMode;
               compat->presentModeCount = 1;
            }
         } else {
            compat->presentModeCount = 1;
         }
         break;
      }
      default:
         break;
      }
   }
   return VK_SUCCESS;
}

static const VkFormat videoout_formats[] = {
   VK_FORMAT_B8G8R8A8_UNORM,
   VK_FORMAT_B8G8R8A8_SRGB,
};

static VkResult
videoout_surface_get_formats(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device, uint32_t *pSurfaceFormatCount,
                             VkSurfaceFormatKHR *pSurfaceFormats)
{
   VK_OUTARRAY_MAKE_TYPED(VkSurfaceFormatKHR, out, pSurfaceFormats, pSurfaceFormatCount);
   for (unsigned i = 0; i < ARRAY_SIZE(videoout_formats); i++) {
      vk_outarray_append_typed(VkSurfaceFormatKHR, &out, f)
      {
         f->format = videoout_formats[i];
         f->colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      }
   }
   return vk_outarray_status(&out);
}

static VkResult
videoout_surface_get_formats2(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device, const void *info_next,
                              uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats)
{
   VK_OUTARRAY_MAKE_TYPED(VkSurfaceFormat2KHR, out, pSurfaceFormats, pSurfaceFormatCount);
   for (unsigned i = 0; i < ARRAY_SIZE(videoout_formats); i++) {
      vk_outarray_append_typed(VkSurfaceFormat2KHR, &out, f)
      {
         assert(f->sType == VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR);
         f->surfaceFormat.format = videoout_formats[i];
         f->surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      }
   }
   return vk_outarray_status(&out);
}

static VkResult
videoout_surface_get_present_modes(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device,
                                   uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes)
{
   VK_OUTARRAY_MAKE_TYPED(VkPresentModeKHR, out, pPresentModes, pPresentModeCount);
   vk_outarray_append_typed(VkPresentModeKHR, &out, mode)
      *mode = VK_PRESENT_MODE_FIFO_KHR;
   return vk_outarray_status(&out);
}

static VkResult
videoout_surface_get_present_rectangles(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device,
                                        uint32_t *pRectCount, VkRect2D *pRects)
{
   VK_OUTARRAY_MAKE_TYPED(VkRect2D, out, pRects, pRectCount);
   vk_outarray_append_typed(VkRect2D, &out, rect)
      *rect = (VkRect2D){.offset = {0, 0}, .extent = videoout_extent};
   return vk_outarray_status(&out);
}

/* --- Images -------------------------------------------------------------- */

/* A swapchain image: the framebuffer of its index, imported as host memory. */
static VkResult
videoout_create_image_mem(const struct wsi_swapchain *wsi_chain, const struct wsi_image_info *info,
                          struct wsi_image *image)
{
   const struct wsi_videoout_swapchain *chain = (const struct wsi_videoout_swapchain *)wsi_chain;
   const struct wsi_device *wsi = wsi_chain->wsi;
   const uint32_t index = image - chain->images;
   void *const pointer = videoout_output.buffers + index * VIDEOOUT_BUFFER_BYTES;

   VkMemoryRequirements reqs;
   wsi->GetImageMemoryRequirements(wsi_chain->device, image->image, &reqs);
   if (reqs.size > VIDEOOUT_BUFFER_BYTES || reqs.alignment > VIDEOOUT_BUFFER_ALIGNMENT)
      return VK_ERROR_INITIALIZATION_FAILED;

   VkMemoryHostPointerPropertiesEXT host_props = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT,
   };
   VkResult result = wsi->GetMemoryHostPointerPropertiesEXT(
      wsi_chain->device, VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT, pointer, &host_props);
   if (result != VK_SUCCESS)
      return result;

   const uint32_t type_bits = reqs.memoryTypeBits & host_props.memoryTypeBits;
   if (!type_bits)
      return VK_ERROR_INITIALIZATION_FAILED;

   const VkImportMemoryHostPointerInfoEXT import = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT,
      .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT,
      .pHostPointer = pointer,
   };
   const VkMemoryAllocateInfo allocate = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &import,
      .allocationSize = VIDEOOUT_BUFFER_BYTES,
      .memoryTypeIndex = wsi_select_memory_type(wsi, 0, 0, type_bits),
   };
   return wsi->AllocateMemory(wsi_chain->device, &allocate, &wsi_chain->alloc, &image->memory);
}

VkResult
wsi_videoout_configure_image(const struct wsi_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo,
                             const struct wsi_videoout_image_params *params, struct wsi_image_info *info)
{
   VkResult result =
      wsi_configure_image(chain, pCreateInfo, VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT, info);
   if (result != VK_SUCCESS)
      return result;

   /* Laid out as the display scans out. */
   info->wsi.scanout = true;
   info->create_mem = videoout_create_image_mem;
   return VK_SUCCESS;
}

/* --- Swapchains ---------------------------------------------------------- */

static struct wsi_image *
videoout_swapchain_get_wsi_image(struct wsi_swapchain *wsi_chain, uint32_t image_index)
{
   struct wsi_videoout_swapchain *chain = (struct wsi_videoout_swapchain *)wsi_chain;
   return &chain->images[image_index];
}

/* A buffer of the first count an application may render into, under lock:
 * neither held, nor waiting for its flip, nor flipped past the flip shown
 * last, nor on screen; the one flipped longest ago. UINT32_MAX when none is,
 * with *vblank set when only a flip still to show holds one. */
static uint32_t
videoout_free_buffer_locked(uint32_t count, bool *vblank)
{
   struct wsi_videoout_output *out = &videoout_output;
   const int64_t shown = videoout_shown_argument_locked();

   /* On screen: the buffer of the latest flip shown. */
   uint32_t on_screen = UINT32_MAX;
   for (uint32_t i = 0; i < VIDEOOUT_BUFFERS; i++)
      if (out->buffer[i].argument != 0 && out->buffer[i].argument <= shown &&
          (on_screen == UINT32_MAX || out->buffer[i].argument > out->buffer[on_screen].argument))
         on_screen = i;

   uint32_t best = UINT32_MAX;
   *vblank = false;
   for (uint32_t i = 0; i < count; i++) {
      if (out->buffer[i].held || out->buffer[i].pending || i == on_screen)
         continue;
      if (out->buffer[i].argument > shown) {
         *vblank = true;
         continue;
      }
      if (best == UINT32_MAX || out->buffer[i].argument < out->buffer[best].argument)
         best = i;
   }
   return best;
}

static VkResult
videoout_swapchain_acquire_next_image(struct wsi_swapchain *wsi_chain, const VkAcquireNextImageInfoKHR *info,
                                      uint32_t *image_index)
{
   struct wsi_videoout_swapchain *chain = (struct wsi_videoout_swapchain *)wsi_chain;
   struct wsi_videoout_output *out = &videoout_output;
   const uint64_t deadline = os_time_get_absolute_timeout(info->timeout);

   mtx_lock(&out->lock);
   for (;;) {
      if (chain->retired) {
         mtx_unlock(&out->lock);
         return VK_ERROR_OUT_OF_DATE_KHR;
      }
      bool vblank;
      const uint32_t index = videoout_free_buffer_locked(chain->base.image_count, &vblank);
      if (index != UINT32_MAX) {
         out->buffer[index].held = true;
         mtx_unlock(&out->lock);
         *image_index = index;
         return VK_SUCCESS;
      }
      if (info->timeout == 0) {
         mtx_unlock(&out->lock);
         return VK_NOT_READY;
      }
      if (os_time_get_nano() >= deadline) {
         mtx_unlock(&out->lock);
         return VK_TIMEOUT;
      }
      if (vblank) {
         /* A flip still to show frees one at a vblank. */
         const int handle = out->handle;
         mtx_unlock(&out->lock);
         sceVideoOutWaitVblank(handle);
         mtx_lock(&out->lock);
      } else {
         /* A flip waiting for its frame, or an image held: wait for either. */
         struct timespec until;
         const uint64_t wake = MIN2(deadline, os_time_get_nano() + UINT64_C(20000000));
         until.tv_sec = wake / 1000000000;
         until.tv_nsec = wake % 1000000000;
         cnd_timedwait(&out->changed, &out->lock, &until);
      }
   }
}

static VkResult
videoout_swapchain_release_images(struct wsi_swapchain *wsi_chain, uint32_t count, const uint32_t *indices)
{
   struct wsi_videoout_output *out = &videoout_output;
   mtx_lock(&out->lock);
   for (uint32_t i = 0; i < count; i++)
      out->buffer[indices[i]].held = false;
   cnd_broadcast(&out->changed);
   mtx_unlock(&out->lock);
   return VK_SUCCESS;
}

static VkResult
videoout_swapchain_queue_present(struct wsi_swapchain *wsi_chain, uint32_t image_index, uint64_t present_id,
                                 const VkPresentRegionKHR *damage)
{
   struct wsi_videoout_swapchain *chain = (struct wsi_videoout_swapchain *)wsi_chain;
   struct wsi_videoout_output *out = &videoout_output;

   mtx_lock(&out->lock);
   out->buffer[image_index].held = false;
   if (chain->retired) {
      cnd_broadcast(&out->changed);
      mtx_unlock(&out->lock);
      return VK_ERROR_OUT_OF_DATE_KHR;
   }
   if (!out->thread_started) {
      if (thrd_create(&out->thread, videoout_flip_thread, NULL) != thrd_success) {
         mtx_unlock(&out->lock);
         return VK_ERROR_OUT_OF_HOST_MEMORY;
      }
      out->thread_started = true;
   }
   assert(out->queue_count < ARRAY_SIZE(out->queue));
   out->queue[(out->queue_head + out->queue_count) % ARRAY_SIZE(out->queue)] = (struct wsi_videoout_present){
      .chain = chain,
      .buffer = image_index,
      .fence = chain->base.fences[image_index],
   };
   out->queue_count++;
   out->buffer[image_index].pending = true;
   chain->in_flight++;
   cnd_broadcast(&out->changed);
   mtx_unlock(&out->lock);
   return VK_SUCCESS;
}

static VkResult
videoout_swapchain_wait_for_present(struct wsi_swapchain *wsi_chain, uint64_t present_id, uint64_t timeout)
{
   return wsi_swapchain_wait_for_present_semaphore(wsi_chain, present_id, timeout);
}

static VkResult
videoout_swapchain_destroy(struct wsi_swapchain *wsi_chain, const VkAllocationCallbacks *pAllocator)
{
   struct wsi_videoout_swapchain *chain = (struct wsi_videoout_swapchain *)wsi_chain;
   struct wsi_videoout_output *out = &videoout_output;

   /* Its flips use its fences: they go first. */
   mtx_lock(&out->lock);
   while (chain->in_flight)
      cnd_wait(&out->changed, &out->lock);
   for (uint32_t i = 0; i < chain->base.image_count; i++)
      if (chain->images[i].acquired)
         out->buffer[i].held = false;
   if (out->current == chain)
      out->current = NULL;
   cnd_broadcast(&out->changed);
   mtx_unlock(&out->lock);

   for (uint32_t i = 0; i < chain->base.image_count; i++) {
      if (chain->images[i].image != VK_NULL_HANDLE)
         wsi_destroy_image(&chain->base, &chain->images[i]);
   }
   wsi_swapchain_finish(&chain->base);
   vk_free(pAllocator, chain);
   return VK_SUCCESS;
}

static VkResult
videoout_surface_create_swapchain(VkIcdSurfaceBase *icd_surface, VkDevice device, struct wsi_device *wsi_device,
                                  const VkSwapchainCreateInfoKHR *pCreateInfo,
                                  const VkAllocationCallbacks *pAllocator, struct wsi_swapchain **swapchain_out)
{
   VkIcdSurfaceDisplay *surface = (VkIcdSurfaceDisplay *)icd_surface;
   struct wsi_videoout_output *out = &videoout_output;

   assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
   if (pCreateInfo->imageExtent.width != VIDEOOUT_WIDTH || pCreateInfo->imageExtent.height != VIDEOOUT_HEIGHT ||
       pCreateInfo->minImageCount > VIDEOOUT_BUFFERS || !wsi_device->GetMemoryHostPointerPropertiesEXT)
      return VK_ERROR_INITIALIZATION_FAILED;

   /* The output in the surface's mode: a mode other than the output's was not
    * offered. */
   videoout_settle_modes();
   const struct wsi_videoout_mode *mode = videoout_mode_from_handle(surface->displayMode);
   if (mode->high_frame_rate && !out->high_mode_offered)
      return VK_ERROR_INITIALIZATION_FAILED;

   mtx_lock(&out->lock);
   VkResult result = videoout_register_locked();
   if (result == VK_SUCCESS && out->current && out->current != (struct wsi_videoout_swapchain *)
                                                 wsi_swapchain_from_handle(pCreateInfo->oldSwapchain))
      result = VK_ERROR_NATIVE_WINDOW_IN_USE_KHR;
   mtx_unlock(&out->lock);
   if (result != VK_SUCCESS)
      return result;

   const uint32_t num_images = MAX2(pCreateInfo->minImageCount, 3);
   const size_t size = sizeof(struct wsi_videoout_swapchain) + num_images * sizeof(struct wsi_image);
   struct wsi_videoout_swapchain *chain = vk_zalloc(pAllocator, size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (chain == NULL)
      return VK_ERROR_OUT_OF_HOST_MEMORY;

   struct wsi_videoout_image_params image_params = {
      .base.image_type = WSI_IMAGE_TYPE_VIDEOOUT,
   };
   result = wsi_swapchain_init(wsi_device, &chain->base, device, pCreateInfo, &image_params.base, pAllocator);
   if (result != VK_SUCCESS) {
      vk_free(pAllocator, chain);
      return result;
   }

   chain->base.destroy = videoout_swapchain_destroy;
   chain->base.get_wsi_image = videoout_swapchain_get_wsi_image;
   chain->base.acquire_next_image = videoout_swapchain_acquire_next_image;
   chain->base.release_images = videoout_swapchain_release_images;
   chain->base.queue_present = videoout_swapchain_queue_present;
   chain->base.wait_for_present = videoout_swapchain_wait_for_present;
   chain->base.present_mode = VK_PRESENT_MODE_FIFO_KHR;
   chain->base.image_count = num_images;

   for (uint32_t i = 0; i < num_images; i++) {
      result = wsi_create_image(&chain->base, &chain->base.image_info, &chain->images[i]);
      if (result != VK_SUCCESS) {
         /* wsi_create_image destroyed the one that failed. */
         for (uint32_t j = 0; j < i; j++)
            wsi_destroy_image(&chain->base, &chain->images[j]);
         wsi_swapchain_finish(&chain->base);
         vk_free(pAllocator, chain);
         return result;
      }
   }

   /* The old swapchain is retired: its acquires and presents are out of date. */
   mtx_lock(&out->lock);
   if (out->current)
      out->current->retired = true;
   out->current = chain;
   mtx_unlock(&out->lock);

   *swapchain_out = &chain->base;
   return VK_SUCCESS;
}

/* --- The interface ------------------------------------------------------- */

VkResult
wsi_display_init_wsi(struct wsi_device *wsi_device, const VkAllocationCallbacks *alloc, int display_fd)
{
   struct wsi_videoout *wsi = vk_zalloc(alloc, sizeof(*wsi), 8, VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
   if (!wsi) {
      wsi_device->wsi[VK_ICD_WSI_PLATFORM_DISPLAY] = NULL;
      return VK_ERROR_OUT_OF_HOST_MEMORY;
   }

   call_once(&videoout_once, videoout_output_init_once);

   wsi->base.get_support = videoout_surface_get_support;
   wsi->base.get_capabilities2 = videoout_surface_get_capabilities2;
   wsi->base.get_formats = videoout_surface_get_formats;
   wsi->base.get_formats2 = videoout_surface_get_formats2;
   wsi->base.get_present_modes = videoout_surface_get_present_modes;
   wsi->base.get_present_rectangles = videoout_surface_get_present_rectangles;
   wsi->base.create_swapchain = videoout_surface_create_swapchain;

   wsi_device->wsi[VK_ICD_WSI_PLATFORM_DISPLAY] = &wsi->base;
   return VK_SUCCESS;
}

void
wsi_display_finish_wsi(struct wsi_device *wsi_device, const VkAllocationCallbacks *alloc)
{
   struct wsi_videoout *wsi = (struct wsi_videoout *)wsi_device->wsi[VK_ICD_WSI_PLATFORM_DISPLAY];
   if (wsi)
      vk_free(alloc, wsi);
}

void
wsi_display_setup_syncobj_fd(struct wsi_device *wsi_device, int fd)
{
}
