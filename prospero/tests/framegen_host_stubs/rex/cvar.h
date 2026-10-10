#pragma once
#include <cstdint>
#include <string_view>
namespace rex::cvar {
enum class Lifecycle { kRequiresRestart };
}
struct FramegenTestMetadata {
  FramegenTestMetadata lifecycle(rex::cvar::Lifecycle) const { return {}; }
  template<class F> FramegenTestMetadata validator(F) const { return {}; }
  FramegenTestMetadata range(int, int) const { return {}; }
};
#define REXCVAR_DEFINE_BOOL(name, value, ...) \
  bool fg_test_##name = value; \
  [[maybe_unused]] static auto fg_meta_##name = FramegenTestMetadata{}
#define REXCVAR_DEFINE_INT32(name, value, ...) \
  int32_t fg_test_##name = value; \
  [[maybe_unused]] static auto fg_meta_##name = FramegenTestMetadata{}
#define REXCVAR_GET(name) fg_test_##name
