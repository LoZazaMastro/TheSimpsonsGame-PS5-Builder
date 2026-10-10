// Headless execution of the real PS5 ImGui menu. The native presenter and CVar
// registry are test boundaries; Vulkan submission is covered by source/target
// review and mailbox tests, not represented as hardware execution here.
#include "../../tools/rexglue-sdk/src/ui/overlay/settings_prospero.cpp"
#include <imgui_internal.h>
#include <test_settings_font.h>
#include <cassert>
#include <fstream>
#include <iostream>

namespace {
std::map<std::string, std::string> flags;
rex::platform::prospero::LiveSettings live;
bool dll_present = false;
size_t availability_probes = 0;
uint32_t output_millihertz = 59940;
// The session API is a test boundary here; its real atomic implementation has
// separate concurrency/idempotence tests. This fixture verifies UI usage.
rex::platform::prospero::FrameGenerationSessionRequest framegen_session{true, 1};
size_t framegen_set_calls = 0;
}
namespace rex {
LogCategoryId RegisterLogCategory(const char*) { return LogCategoryId(0); }
spdlog::logger* GetLoggerRaw(LogCategoryId) { return nullptr; }
namespace cvar {
const FlagEntry* GetFlagInfo(std::string_view name) {
  static FlagEntry entry;
  return flags.contains(std::string(name)) ? &entry : nullptr;
}
std::string GetFlagByName(std::string_view name) { return flags.at(std::string(name)); }
template <> uint32_t Query<uint32_t>(std::string_view name) { return uint32_t(std::stoul(GetFlagByName(name))); }
template <> int32_t Query<int32_t>(std::string_view name) { return int32_t(std::stol(GetFlagByName(name))); }
}
namespace platform::prospero {
LiveSettings& GetLiveSettings() { return live; }
bool IsFrameGenerationDllPresent() { ++availability_probes; return dll_present; }
FrameGenerationSessionRequest ReadFrameGenerationSessionRequest() { return framegen_session; }
FrameGenerationSessionRequest SetFrameGenerationForSession(bool enabled) {
  ++framegen_set_calls;
  if (framegen_session.enabled != enabled) {
    framegen_session.enabled = enabled;
    ++framegen_session.revision;
  }
  return framegen_session;
}
}
namespace ui {
namespace vulkan {
uint32_t GetProsperoOutputMilliHertz() { return output_millihertz; }
}
void Presenter::SetGuestOutputPaintConfigFromUIThread(const GuestOutputPaintConfig& config) {
  guest_output_paint_config_ = config;
}
void Presenter::RequestUIPaintFromUIThread() {}
}
}
std::string Frame(rex::ui::ProsperoSettingsModel& model, bool capture_text = false) {
  ImGui::NewFrame();
  if (capture_text) ImGui::LogToBuffer();
  rex::ui::DrawProsperoSettings(model, ImGui::GetIO());
  const std::string text = capture_text ? ImGui::GetCurrentContext()->LogBuffer.c_str() : "";
  if (capture_text) ImGui::LogFinish();
  const auto* window = ImGui::FindWindowByName("The Simpsons Game###ps5_settings");
  assert(window);
  // The controller hints are the final two items in the parent, outside both
  // scrolling children. Prove they fit in the actual rendered content area.
  assert(window->DC.CursorMaxPos.y <= window->InnerRect.Max.y - window->WindowPadding.y + 1.0f);
  assert(window->DC.CursorMaxPos.x <= window->InnerRect.Max.x - window->WindowPadding.x + 1.0f);
  ImGui::Render();
  const auto* draw = ImGui::GetDrawData();
  assert(draw && draw->Valid && draw->TotalVtxCount > 100 && draw->TotalIdxCount > 100);
  for (const auto* list : draw->CmdLists) for (const auto& command : list->CmdBuffer) {
    assert(command.ClipRect.x >= 0 && command.ClipRect.y >= 0);
    assert(command.ClipRect.z <= draw->DisplaySize.x + 1 && command.ClipRect.w <= draw->DisplaySize.y + 1);
  }
  return text;
}
int main(int argc, char** argv) {
  assert(argc == 2);
  const std::filesystem::path path = std::filesystem::absolute(argv[1]);
  for (const auto& def : rex::ui::prospero_settings::kSettings) flags[def.key] = def.default_value;
  flags["prospero_profile_version"] = "48";
  flags["frame_rate"] = "120";
  flags["menu_frame_rate"] = "0";
  flags["user_language"] = "6";
  flags["frame_gen"] = "true"; // A requested ON value cannot bypass the missing-DLL gate.
  const auto original_flags = flags;
  using namespace rex::platform::prospero;
  live.Initialize(LiveSetting::kFrameRates, LiveSettings::PackFrameRates(60, 30));
  live.Initialize(LiveSetting::kAnisotropy, 5);
  live.Initialize(LiveSetting::kSwapPostEffect, 1);
  live.Initialize(LiveSetting::kPresentEffect, 1);
  live.Initialize(LiveSetting::kFsrSharpnessReduction, 0);
  live.Initialize(LiveSetting::kLetterbox, 1);
  live.Initialize(LiveSetting::kMute, 0);
  live.Initialize(LiveSetting::kSubtitles, 0);
  live.Initialize(LiveSetting::kVSync, 0);
  live.Initialize(LiveSetting::kColorSaturation, 1000);
  live.Initialize(LiveSetting::kColorVibrance, 0);
  live.Initialize(LiveSetting::kColorContrast, 1000);
  live.Initialize(LiveSetting::kColorBrightness, 0);

  ImGui::CreateContext();
  auto& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
  io.DisplaySize = ImVec2(1280, 720);
  io.DeltaTime = 1.0f / 60.0f;
  ImFontConfig config;
  config.OversampleH = config.OversampleV = 1;
  config.PixelSnapH = true;
  const ImWchar ranges[] = {0x20, 0xFF, 0};
  auto* font = io.Fonts->AddFontFromMemoryCompressedBase85TTF(
      rex::ui::kProggyTinyCompressedDataBase85, 10.0f, &config, ranges);
  unsigned char* pixels; int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  io.Fonts->SetTexID(ImTextureID(1));

  auto model = rex::ui::CreateProsperoSettingsModel(nullptr, path);
  assert(flags.at("user_language") == "6");
  assert(std::get<int64_t>(model->Find("frame_rate")->desired) == 60);
  assert(std::get<int64_t>(model->Find("menu_frame_rate")->desired) == 30);
  for (const auto& [key, value] : {std::pair{"present_effect", "fsr"},
                                  std::pair{"swap_post_effect", "fxaa"},
                                  std::pair{"eye_shading", "clean"}}) {
    assert(std::get<std::string>(model->Find(key)->desired) == value);
    assert(std::get<std::string>(model->Applied(*model->Find(key))) == value);
  }
  assert(std::get<int64_t>(model->Find("frame_gen_target_rate")->desired) == 120);
  for (const auto* key : {"frame_gen", "frame_gen_target_rate", "frame_gen_multiplier"})
    assert(!model->Enabled(*model->Find(key)));
  assert(!std::get<bool>(model->Find("frame_gen")->desired));
  assert(!model->FrameGenerationEnabledForSession());
  assert(framegen_set_calls == 0);
  model->Change(*model->Find("frame_gen"), true);
  assert(!std::get<bool>(model->Find("frame_gen")->desired));
  model->Change(*model->Find("frame_gen_target_rate"), int64_t(0));
  assert(std::get<int64_t>(model->Find("frame_gen_target_rate")->desired) == 120);
  model->RestoreDefaults();
  assert(!std::get<bool>(model->Find("frame_gen")->desired));
  assert(!framegen_session.enabled);
  assert(std::get<int64_t>(model->Find("frame_gen_target_rate")->desired) == 120);
  assert(std::get<int64_t>(model->Find("frame_gen_multiplier")->desired) == 2);
  std::cout << "PASS missing DLL forces frame generation OFF despite an ON preset or saved CVar, including Restore recommended settings\n";
  const size_t probes_before_draw = availability_probes;
  Frame(*model); Frame(*model);
  assert(availability_probes == probes_before_draw);
  const auto old_nav = ImGui::GetCurrentContext()->NavId;
  io.AddKeyEvent(ImGuiKey_GamepadDpadDown, true); Frame(*model);
  io.AddKeyEvent(ImGuiKey_GamepadDpadDown, false); Frame(*model);
  assert(ImGui::GetCurrentContext()->NavId && ImGui::GetCurrentContext()->NavId != old_nav);
  io.AddKeyEvent(ImGuiKey_GamepadFaceDown, true); Frame(*model);
  io.AddKeyEvent(ImGuiKey_GamepadFaceDown, false); Frame(*model);
  std::cout << "PASS real ImGui gamepad navigation moves focus and activates controls\n";

  std::ofstream menu_text(path.parent_path() / "ui-menu-english.txt");
  assert(menu_text);
  for (const auto size : {ImVec2(1280, 720), ImVec2(1920, 1080), ImVec2(3840, 2160)}) {
    io.DisplaySize = size;
    for (size_t group = 0; group < std::size(rex::ui::prospero_settings::kGroups); ++group) {
      model->group_ = group; Frame(*model); Frame(*model);
      const auto text = Frame(*model, true);
      const auto& definition = rex::ui::prospero_settings::kGroups[group];
      assert(text.find("Settings") != std::string::npos);
      assert(text.find("Fullscreen") != std::string::npos);
      assert(text.find(rex::ui::kNavigationHint) != std::string::npos);
      assert(text.find(rex::ui::kFrameGenerationShortcutHint) != std::string::npos);
      assert(text.find("Impostazioni") == std::string::npos);
      assert(text.find("Schermo intero") == std::string::npos);
      assert(text.find("Ripristina") == std::string::npos);
      for (const auto& row : model->rows_) {
        if (std::string_view(row.definition->group) == definition.id) {
          assert(text.find(row.definition->label) != std::string::npos);
          if (row.definition->control == rex::ui::prospero_settings::Control::kChoice) {
            const auto choice = model->Label(row, row.desired);
            assert(text.find(choice) != std::string::npos);
          }
        }
      }
      if (std::string_view(definition.requires_file) == "lossless.dll") {
        const auto first_hint = text.find(rex::ui::kFrameGenerationShortcutHint);
        assert(text.find(rex::ui::kFrameGenerationShortcutHint, first_hint + 1) != std::string::npos);
        assert(text.find("Release all four buttons before pressing again.") != std::string::npos);
        assert(text.find("The shortcut does not change the saved startup setting.") != std::string::npos);
        assert(text.find("Applied after restarting the game") != std::string::npos);
        // The on/off control itself is live; only target/multiplier retain
        // restart notices. A section-wide restart label would be misleading.
        assert(text.find("Applied after restarting the game") >= text.find("Generated frame target"));
      }
      menu_text << "=== " << int(size.x) << 'x' << int(size.y) << " / " << definition.id << " ===\n" << text << '\n';
    }
  }
  assert(flags.at("user_language") == "6");
  std::cout << "PASS all quick-menu groups render English with Italian gameplay selected; both controller hints fit outside scrolling lists at 720p/1080p/4K\n";
  dll_present = true;
  rex::ui::RefreshProsperoSettingsAvailability(*model);
  for (const auto* key : {"frame_gen", "frame_gen_target_rate", "frame_gen_multiplier"})
    assert(model->Enabled(*model->Find(key)));
  model->RestoreDefaults();
  assert(std::get<bool>(model->Find("frame_gen")->desired));
  assert(model->FrameGenerationEnabledForSession());
  assert(framegen_session.enabled);
  model->Change(*model->Find("frame_gen_target_rate"), int64_t(0));
  model->Change(*model->Find("frame_gen_multiplier"), int64_t(4));
  assert(model->FrameGenerationEnabledForSession());
  assert(std::get<int64_t>(model->Find("frame_gen_target_rate")->desired) == 0);
  assert(std::get<int64_t>(model->Applied(*model->Find("frame_gen_target_rate"))) == 120);
  assert(live.Requested(LiveSetting::kFrameRates).value() == LiveSettings::PackFrameRates(60, 30));
  output_millihertz = 119880;
  Frame(*model);
  dll_present = false;
  rex::ui::RefreshProsperoSettingsAvailability(*model);
  for (const auto* key : {"frame_gen", "frame_gen_target_rate", "frame_gen_multiplier"})
    assert(!model->Enabled(*model->Find(key)));
  assert(!std::get<bool>(model->Find("frame_gen")->desired));
  model->Change(*model->Find("frame_gen"), true);
  assert(!std::get<bool>(model->Find("frame_gen")->desired));
  dll_present = true;
  rex::ui::RefreshProsperoSettingsAvailability(*model);
  model->Change(*model->Find("frame_gen"), true);
  assert(model->Enabled(*model->Find("prospero_output_hz")));
  std::cout << "PASS DLL presence gates live framegen and restart-only target/multiplier; output refresh stays independent; drawing performs no file probes\n";

  for (size_t group = 0; group < std::size(rex::ui::prospero_settings::kGroups); ++group)
    if (std::string_view(rex::ui::prospero_settings::kGroups[group].id) == "framegen") model->group_ = group;
  auto* framegen = model->Find("frame_gen");
  assert(framegen->definition->apply == rex::ui::prospero_settings::Apply::kLive);
  assert(std::get<bool>(framegen->desired));
  // Simulate a controller hotkey between menu frames. Drawing must reflect it
  // while preserving both the saved value and the save request count.
  const auto save_before_shortcut = model->store_.GetStatus().requested;
  framegen_session.enabled = false; ++framegen_session.revision;
  const auto off_text = Frame(*model, true);
  assert(!model->FrameGenerationEnabledForSession() && std::get<bool>(framegen->desired));
  assert(off_text.find("Off for this session.") != std::string::npos);
  assert(off_text.find("Saved for next start: On") != std::string::npos);
  assert(model->store_.GetStatus().requested == save_before_shortcut);
  const auto calls_before_restore = framegen_set_calls;
  model->Change(*framegen, true); // Saved ON already, but session OFF.
  assert(framegen_session.enabled && std::get<bool>(framegen->desired));
  assert(framegen_set_calls == calls_before_restore + 1);
  const auto idempotent_revision = framegen_session.revision;
  model->Change(*framegen, true);
  assert(framegen_session.revision == idempotent_revision);
  framegen_session.enabled = false; ++framegen_session.revision;
  model->RestoreDefaults(); // Reapply an unchanged saved ON to the session.
  assert(framegen_session.enabled && std::get<bool>(framegen->desired));
  assert(Frame(*model, true).find("On for this session; pauses while this menu is open.") != std::string::npos);
  // Restore reset the restart-only pair too; stage a real change again.
  model->Change(*model->Find("frame_gen_target_rate"), int64_t(0));
  model->Change(*model->Find("frame_gen_multiplier"), int64_t(4));
  model->Change(*framegen, false);
  assert(!framegen_session.enabled && !std::get<bool>(framegen->desired));
  const auto save_before_second_shortcut = model->store_.GetStatus().requested;
  framegen_session.enabled = true; ++framegen_session.revision;
  const auto on_text = Frame(*model, true);
  assert(on_text.find("Saved for next start: Off") != std::string::npos);
  assert(model->store_.GetStatus().requested == save_before_second_shortcut);
  assert(!std::get<bool>(framegen->desired));
  std::cout << "PASS live framegen checkbox follows session hotkeys, same saved value can restore the session, and drawing never persists shortcut changes\n";

  struct ColorCase { const char* key; LiveSetting slot; double selected; };
  const auto brightness_before_invalid = live.Requested(LiveSetting::kColorBrightness);
  const auto brightness_saved_before_invalid = model->Find("color_brightness")->desired;
  model->Change(*model->Find("color_brightness"), -0.15); // Below the catalog minimum of -0.10.
  assert(live.Requested(LiveSetting::kColorBrightness).packed == brightness_before_invalid.packed);
  assert(model->Find("color_brightness")->desired == brightness_saved_before_invalid);
  assert(!model->action_error_.empty());
  for (const auto& color : {ColorCase{"color_saturation", LiveSetting::kColorSaturation, 1.2},
                            ColorCase{"color_vibrance", LiveSetting::kColorVibrance, 0.25},
                            ColorCase{"color_contrast", LiveSetting::kColorContrast, 0.8},
                            ColorCase{"color_brightness", LiveSetting::kColorBrightness, -0.05}}) {
    auto* row = model->Find(color.key);
    assert(row->definition->apply == rex::ui::prospero_settings::Apply::kLive);
    const auto previous = model->Applied(*row);
    model->Change(*row, color.selected);
    const auto request = live.Requested(color.slot);
    assert(request.value() == int32_t(std::lround(color.selected * 1000)));
    assert(model->Applied(*row) == previous); // UI waits for the owning renderer.
    live.Reject(color.slot, request);
    for (size_t group = 0; group < std::size(rex::ui::prospero_settings::kGroups); ++group)
      if (std::string_view(rex::ui::prospero_settings::kGroups[group].id) == row->definition->group) model->group_ = group;
    assert(Frame(*model, true).find("Effect unavailable; fallback in use") != std::string::npos);
    // An optional colour pass may recover without another user edit. The same
    // request's successful acknowledgement must remove the stale UI error.
    live.Acknowledge(color.slot, request);
    assert(live.Rejected(color.slot).version() == 0);
    assert(std::abs(std::get<double>(model->Applied(*row)) - color.selected) < 0.0000001);
    assert(Frame(*model, true).find("Effect unavailable; fallback in use") == std::string::npos);
    const double revised = color.selected + 0.01;
    model->Change(*row, revised);
    live.Acknowledge(color.slot, live.Requested(color.slot));
    assert(std::abs(std::get<double>(model->Applied(*row)) - revised) < 0.0000001);
  }
  auto* vsync = model->Find("vsync");
  assert(vsync->definition->apply == rex::ui::prospero_settings::Apply::kLive);
  const auto previous_vsync = model->Applied(*vsync);
  model->Change(*vsync, true);
  const auto vsync_request = live.Requested(LiveSetting::kVSync);
  assert(vsync_request.value() == 1 && model->Applied(*vsync) == previous_vsync);
  live.Acknowledge(LiveSetting::kVSync, vsync_request);
  assert(std::get<bool>(model->Applied(*vsync)));
  assert(flags.at("prospero_output_hz") == original_flags.at("prospero_output_hz"));
  assert(framegen_session.enabled && !std::get<bool>(framegen->desired));
  std::cout << "PASS VSync and four colour controls use real live slots and signed thousandths, invalid values are rejected, and renderer recovery clears fallback without another edit\n";
  // Check every static English capability string against the same font and
  // glyph range shipped by ImGuiDrawer, not a substituted desktop font.
  auto* baked = font->GetFontBaked(10.0f);
  size_t checked = 0;
  auto check_glyphs = [&](const char* text) {
    while (*text) {
      unsigned int code = 0;
      const int length = ImTextCharFromUtf8(&code, text, nullptr);
      assert(length > 0);
      text += length;
      if (code >= 0x20 && code != ' ') {
        if (!baked->FindGlyphNoFallback(ImWchar(code))) {
          std::cerr << "Missing glyph U+" << std::hex << code << '\n'; std::abort();
        }
        ++checked;
      }
    }
  };
  check_glyphs(rex::ui::kNavigationHint);
  check_glyphs(rex::ui::kFrameGenerationShortcutHint);
  for (const auto& group : rex::ui::prospero_settings::kGroups) check_glyphs(group.label);
  for (const auto& def : rex::ui::prospero_settings::kSettings) {
    for (const auto* text : {def.label, def.description}) check_glyphs(text);
    for (const auto& choice : def.choices) check_glyphs(choice.label);
  }
  std::cout << "PASS all groups draw at720p/1080p/4K with valid clipping; " << checked << " glyphs supported by shipped font\n";

  auto* render = model->Find("resolution_scale");
  model->Change(*render, int64_t(2));
  assert(std::get<int64_t>(render->desired) == 2 && std::get<int64_t>(model->Applied(*render)) == 3);
  auto* fps = model->Find("frame_rate");
  model->Change(*fps, int64_t(30));
  const auto requested = live.Requested(LiveSetting::kFrameRates);
  assert(LiveSettings::GameFrameRate(requested.value()) == 30 && live.Applied(LiveSetting::kFrameRates).packed != requested.packed);
  for (int invalid : {0, 90, 120, 144, 240}) model->Change(*fps, int64_t(invalid));
  assert(std::get<int64_t>(fps->desired) == 30 && live.Requested(LiveSetting::kFrameRates).packed == requested.packed);
  assert(model->action_error_ == "Unsupported value for this setting");
  Frame(*model); Frame(*model);
  assert(flags == original_flags);  // Neither restart nor live changes write raw storage.
  live.Consume(LiveSetting::kFrameRates);
  assert(std::get<int64_t>(model->Applied(*fps)) == 30);
  // Dialog closure leaves this app-owned model alive. Its final destruction
  // drains persistence, including restart-only render resolution.
  model.reset();
  const auto stored = ReadSettingsValues(path);
  assert(std::get<int64_t>(stored.at("resolution_scale")) == 2);
  assert(std::get<int64_t>(stored.at("frame_rate")) == 30);
  assert(std::get<int64_t>(stored.at("menu_frame_rate")) == 30);
  assert(std::get<int64_t>(stored.at("user_language")) == 6);
  assert(std::get<int64_t>(stored.at("prospero_profile_version")) == 48);
  assert(!std::get<bool>(stored.at("frame_gen")));
  assert(framegen_session.enabled); // Later unrelated saves did not persist the hotkey ON.
  assert(std::get<bool>(stored.at("vsync")));
  assert(std::abs(std::get<double>(stored.at("color_brightness")) + 0.04) < 0.0000001);
  assert(std::abs(std::get<double>(stored.at("color_saturation")) - 1.21) < 0.0000001);
  assert(std::get<int64_t>(stored.at("frame_gen_target_rate")) == 0);
  assert(std::get<int64_t>(stored.at("frame_gen_multiplier")) == 4);
  std::cout << "PASS real menu model stages restart, waits for live consumer, rejects unsupported FPS, preserves raw CVars and future marker\n";
  ImGui::DestroyContext();
}
