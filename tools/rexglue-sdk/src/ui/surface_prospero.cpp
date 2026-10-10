#include <rex/ui/surface_prospero.h>

namespace rex::ui {

bool ProsperoDisplaySurface::GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const {
  width_out = kWidth;
  height_out = kHeight;
  return true;
}

}  // namespace rex::ui
