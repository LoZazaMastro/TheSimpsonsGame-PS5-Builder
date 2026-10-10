#pragma once

#include <rex/ui/surface.h>

namespace rex::ui {

// Display output is independent of the Xbox video-mode and render-scale CVars.
// The pinned PS5 Mesa VideoOut WSI exposes a native 3840x2160 surface.
class ProsperoDisplaySurface final : public Surface {
 public:
  static constexpr uint32_t kWidth = 3840;
  static constexpr uint32_t kHeight = 2160;
  TypeIndex GetType() const override { return kTypeIndex_ProsperoDisplay; }

 protected:
  bool GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const override;
};

}  // namespace rex::ui
