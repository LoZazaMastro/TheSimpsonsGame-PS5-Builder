// BSD 3-Clause License. See the project LICENSE.
#include <rex/platform/settings_prospero.h>
#include <rex/cvar.h>
#include <cmath>
#include <rex/logging.h>

namespace rex::platform::prospero {
namespace {
constinit LiveSettings settings;
}
LiveSettings& GetLiveSettings() { return settings; }

void InitializeLiveSettingsFromCVars() {
  const auto configured_game_rate = cvar::Query<int32_t>("frame_rate");
  const auto configured_menu_rate = cvar::Query<int32_t>("menu_frame_rate");
  const auto game_rate = LiveSettings::IsSupportedFrameRate(configured_game_rate) ? configured_game_rate : 60;
  const auto menu_rate = LiveSettings::IsSupportedFrameRate(configured_menu_rate) ? configured_menu_rate : game_rate;
  settings.Initialize(LiveSetting::kFrameRates, LiveSettings::PackFrameRates(game_rate, menu_rate));
  if (game_rate != configured_game_rate || menu_rate != configured_menu_rate)
    REXLOG_WARN("PS5 settings normalized unsupported native FPS: game {} -> {}, menu {} -> {} (only 30/60 supported)",
                 configured_game_rate, game_rate, configured_menu_rate, menu_rate);
  settings.Initialize(LiveSetting::kAnisotropy, cvar::Query<int32_t>("anisotropic_override"));
  const auto effect = cvar::Query<std::string>("swap_post_effect");
  settings.Initialize(LiveSetting::kSwapPostEffect,
                      effect == "fxaa" ? 1 : effect == "fxaa_extreme" ? 2 : 0);
  const auto present_effect = cvar::Query<std::string>("present_effect");
  settings.Initialize(LiveSetting::kPresentEffect,
                      present_effect == "fsr" ? 1 : present_effect == "cas" ? 2 : 0);
  settings.Initialize(LiveSetting::kFsrSharpnessReduction,
      int32_t(std::lround(cvar::Query<double>("present_fsr_sharpness_reduction") * 1000.0)));
  settings.Initialize(LiveSetting::kLetterbox, cvar::Query<bool>("present_letterbox"));
  settings.Initialize(LiveSetting::kMute, cvar::Query<bool>("audio_mute"));
  settings.Initialize(LiveSetting::kSubtitles, cvar::Query<bool>("subtitles"));
  settings.Initialize(LiveSetting::kVSync, cvar::Query<bool>("vsync"));
  settings.Initialize(LiveSetting::kColorSaturation,
      int32_t(std::lround(cvar::Query<double>("color_saturation") * 1000.0)));
  settings.Initialize(LiveSetting::kColorVibrance,
      int32_t(std::lround(cvar::Query<double>("color_vibrance") * 1000.0)));
  settings.Initialize(LiveSetting::kColorContrast,
      int32_t(std::lround(cvar::Query<double>("color_contrast") * 1000.0)));
  settings.Initialize(LiveSetting::kColorBrightness,
      int32_t(std::lround(cvar::Query<double>("color_brightness") * 1000.0)));
}
}  // namespace rex::platform::prospero
