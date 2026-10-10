#pragma once
#include <rex/ui/vulkan/device.h>

namespace rex::ui::vulkan::util {
template <typename Function, typename Parent, typename Object>
inline bool DestroyAndNullHandle(Function* function, Parent parent, Object& handle) {
  if (handle == VK_NULL_HANDLE) return false;
  function(parent, handle, nullptr);
  handle = VK_NULL_HANDLE;
  return true;
}
}  // namespace rex::ui::vulkan::util
