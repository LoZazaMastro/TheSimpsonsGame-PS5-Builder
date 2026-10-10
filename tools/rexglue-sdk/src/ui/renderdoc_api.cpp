/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <cstdlib>

#include <rex/logging.h>
#include <rex/ui/renderdoc_api.h>

namespace rex {
namespace ui {

std::unique_ptr<RenderDocAPI> RenderDocAPI::CreateIfConnected() {
#if REX_PLATFORM_PROSPERO
  // The native title links RADV directly and does not have a RenderDoc layer.
  return nullptr;
#else
  // HAND PATCH: the "is RenderDoc attached" heuristic below dlopens
  // librenderdoc.so and treats a successful load as "attached". On systems
  // with RenderDoc installed globally (SteamOS with renderdoc-minimal:
  // /usr/lib/librenderdoc.so), the load ALWAYS succeeds, silently embedding
  // the full capture layer + auto-enabling GPU debug markers in every run --
  // significant overhead and an instability source. Make the integration
  // opt-in: only probe when explicitly requested.
  const char* renderdoc_env = std::getenv("REX_ENABLE_RENDERDOC");
  if (!renderdoc_env || renderdoc_env[0] != '1') {
    return nullptr;
  }

  std::unique_ptr<RenderDocAPI> renderdoc_api(new RenderDocAPI());

  pRENDERDOC_GetAPI get_api = nullptr;

  // Try to load the RenderDoc library. If RenderDoc is attached, the library
  // should already be loaded into the process and this will increment the
  // reference count. If not attached, the load will fail and we return nullptr.
  if (!renderdoc_api->library_.Load(platform::lib_names::kRenderDoc)) {
    return nullptr;
  }
  get_api = renderdoc_api->library_.GetSymbol<pRENDERDOC_GetAPI>("RENDERDOC_GetAPI");

  // get_api will be null if RenderDoc is not connected, or the API isn't
  // available on this platform, or there was an error.
  if (!get_api || !get_api(eRENDERDOC_API_Version_1_0_0, (void**)&renderdoc_api->api_1_0_0_) ||
      !renderdoc_api->api_1_0_0_) {
    return nullptr;
  }

  REXLOG_INFO("RenderDoc API initialized");

  return renderdoc_api;
#endif
}

RenderDocAPI::~RenderDocAPI() {
#if !REX_PLATFORM_PROSPERO
  library_.Close();
#endif
}

}  // namespace ui
}  // namespace rex
