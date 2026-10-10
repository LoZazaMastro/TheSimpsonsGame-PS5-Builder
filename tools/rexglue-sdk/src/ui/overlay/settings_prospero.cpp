// PS5 settings: typed desired values, owner-thread application, atomic saving.
// BSD 3-Clause License. See the project LICENSE.
#include <rex/ui/overlay/settings_prospero.h>
#include <prospero_settings_embedded.h>
#include <rex/platform/settings_prospero.h>
#include <rex/platform/framegen_prospero.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ui/presenter.h>
#include <rex/ui/vulkan/display_prospero.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace rex::ui {
using namespace prospero_settings;
using platform::prospero::ConfigValue;
using platform::prospero::ConfigValues;
using platform::prospero::GetLiveSettings;
using platform::prospero::LiveSetting;
using platform::prospero::LiveSettings;
using platform::prospero::SettingRequest;
using platform::prospero::SettingsConfigStore;

namespace {
ConfigValue Parse(const char* raw, ValueKind kind) {
  if (kind == ValueKind::kString) return std::string(raw);
  if (kind == ValueKind::kBoolean) {
    if (std::string_view(raw) == "true") return true;
    if (std::string_view(raw) == "false") return false;
    throw std::runtime_error("Invalid boolean setting");
  }
  std::istringstream input(raw);
  input.imbue(std::locale::classic());
  if (kind == ValueKind::kInteger) {
    int64_t value;
    if ((input >> value) && input.eof()) return value;
  } else {
    double value;
    if ((input >> value) && input.eof() && std::isfinite(value)) return value;
  }
  throw std::runtime_error("Invalid numeric setting");
}
double Number(const ConfigValue& value) {
  if (const auto* number = std::get_if<double>(&value)) return *number;
  return double(std::get<int64_t>(value));
}
bool Equal(const ConfigValue& left, const ConfigValue& right) {
  if ((std::holds_alternative<int64_t>(left) || std::holds_alternative<double>(left)) &&
      (std::holds_alternative<int64_t>(right) || std::holds_alternative<double>(right)))
    return std::abs(Number(left) - Number(right)) < 0.0000001;
  return left == right;
}
std::string Text(const ConfigValue& value) {
  if (const auto* text = std::get_if<std::string>(&value)) return *text;
  if (const auto* boolean = std::get_if<bool>(&value)) return *boolean ? "true" : "false";
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(8) << Number(value);
  return output.str();
}
// This PS5 utility menu is always English. The game's user_language setting
// only controls the game itself, including its own menus and subtitles.
constexpr const char* kNavigationHint =
    "D-pad: navigate   X: select / edit   O: back   L1 + R1 + Options: close";
constexpr const char* kFrameGenerationShortcutHint =
    "L1 + R1 + L3 + R3: toggle frame generation for this session";
std::optional<LiveSetting> LiveKey(std::string_view key) {
  if (key == "frame_rate" || key == "menu_frame_rate") return LiveSetting::kFrameRates;
  if (key == "anisotropic_override") return LiveSetting::kAnisotropy;
  if (key == "swap_post_effect") return LiveSetting::kSwapPostEffect;
  if (key == "present_effect") return LiveSetting::kPresentEffect;
  if (key == "present_fsr_sharpness_reduction") return LiveSetting::kFsrSharpnessReduction;
  if (key == "present_letterbox") return LiveSetting::kLetterbox;
  if (key == "audio_mute") return LiveSetting::kMute;
  if (key == "subtitles") return LiveSetting::kSubtitles;
  if (key == "vsync") return LiveSetting::kVSync;
  if (key == "color_saturation") return LiveSetting::kColorSaturation;
  if (key == "color_vibrance") return LiveSetting::kColorVibrance;
  if (key == "color_contrast") return LiveSetting::kColorContrast;
  if (key == "color_brightness") return LiveSetting::kColorBrightness;
  return std::nullopt;
}
std::function<void()> MakeAckLogger() {
  struct Seen {
    std::array<uint64_t, size_t(LiveSetting::kCount)> applied{}, rejected{};
  };
  auto seen = std::make_shared<Seen>();
  // Only this writer thread touches Seen. Rendering/audio only publish atomics.
  return [seen] {
    constexpr const char* names[] = {"frame_rates", "anisotropic_override", "swap_post_effect",
        "present_effect", "present_fsr_sharpness_reduction", "present_letterbox",
        "audio_mute", "subtitles", "vsync", "color_saturation", "color_vibrance",
        "color_contrast", "color_brightness"};
    static_assert(std::size(names) == size_t(LiveSetting::kCount));
    for (size_t i = 0; i < std::size(names); ++i) {
      auto applied = GetLiveSettings().Applied(LiveSetting(i));
      if (applied.version() && seen->applied[i] != applied.packed) {
        seen->applied[i] = applied.packed;
        REXLOG_INFO("PS5 settings applied by consumer: {} version={} value={}",
                    names[i], applied.version(), applied.value());
      }
      auto rejected = GetLiveSettings().Rejected(LiveSetting(i));
      if (rejected.version() && seen->rejected[i] != rejected.packed) {
        seen->rejected[i] = rejected.packed;
        REXLOG_ERROR("PS5 settings rejected by renderer: {} version={} value={}; required effect unavailable",
                     names[i], rejected.version(), rejected.value());
      }
    }
  };
}
}  // namespace

class ProsperoSettingsModel {
 public:
  struct Row {
    const SettingDefinition* definition;
    ConfigValue desired;
    ConfigValue startup;
    bool available = true;
  };

  ProsperoSettingsModel(Presenter* presenter, const std::filesystem::path& config_path)
      : presenter_(presenter), config_path_(config_path),
        store_(config_path, [](const SettingsConfigStore::Status& status) {
          if (status.error.empty())
            REXLOG_INFO("PS5 settings saved: request={} persisted={}", status.requested, status.persisted);
          else
            REXLOG_ERROR("PS5 settings save failed: request={} error={}", status.requested, status.error);
          if (!status.durability_warning.empty())
            REXLOG_WARN("PS5 settings saved with unconfirmed directory durability: request={} warning={}",
                         status.requested, status.durability_warning);
        }, MakeAckLogger()) {
    // Created by ReXApp after config loading and before starting the guest. The
    // baseline CVars stay immutable for the lifetime of this settings session.
    for (const auto& definition : kSettings) {
      auto initial = Parse(definition.default_value, definition.value_kind);
      bool available = cvar::GetFlagInfo(definition.key) != nullptr;
      if (available) {
        try { initial = Parse(cvar::GetFlagByName(definition.key).c_str(), definition.value_kind); }
        catch (const std::exception& error) {
          available = false;
          REXLOG_ERROR("PS5 settings cannot read {}: {}", definition.key, error.what());
        }
      }
      if (std::string_view(definition.key) == "frame_rate" ||
          std::string_view(definition.key) == "menu_frame_rate") {
        const auto rates = GetLiveSettings().Requested(LiveSetting::kFrameRates).value();
        initial = int64_t(std::string_view(definition.key) == "frame_rate"
            ? LiveSettings::GameFrameRate(rates) : LiveSettings::MenuFrameRate(rates));
      }
      rows_.push_back({&definition, initial, initial, available});
    }
    profile_version_ = std::max(int64_t(34), int64_t(cvar::Query<int32_t>("prospero_profile_version")));
    RefreshAvailability();
    // No DLL means frame generation could not start, even if an old TOML
    // requested it. Present that effective startup state without rewriting the
    // immutable CVars used by the game or touching the configuration on disk.
    if (!lossless_dll_present_) {
      if (auto* row = Find("frame_gen")) row->startup = false;
    }
    REXLOG_INFO("PS5 settings contract={} path={}", kCapabilitiesSha256, config_path_.string());
  }

  Row* Find(std::string_view key) {
    for (auto& row : rows_) if (key == row.definition->key) return &row;
    return nullptr;
  }
  void RefreshAvailability() {
    lossless_dll_present_ = platform::prospero::IsFrameGenerationDllPresent();
    if (!lossless_dll_present_) {
      if (auto* row = Find("frame_gen")) row->desired = false;
    }
  }
  bool RequiredFileAvailable(const Row& row) const {
    for (const auto& group : kGroups) {
      if (std::string_view(row.definition->group) != group.id) continue;
      if (!*group.requires_file) return true;
      return std::string_view(group.requires_file) == "lossless.dll" && lossless_dll_present_;
    }
    return false;
  }
  bool Enabled(const Row& row) {
    if (!row.available || row.definition->apply == Apply::kFixed || !RequiredFileAvailable(row))
      return false;
    if (!*row.definition->requires_key) return true;
    const auto* dependency = Find(row.definition->requires_key);
    return dependency && Equal(dependency->desired,
        Parse(row.definition->requires_value, dependency->definition->value_kind));
  }
  ConfigValue DefaultValue(const Row& row) const {
    // A recommended ON preset is conditional on the optional user-supplied
    // DLL. The harmless target and multiplier keep their declared defaults.
    if (std::string_view(row.definition->key) == "frame_gen" && !RequiredFileAvailable(row))
      return false;
    return Parse(row.definition->default_value, row.definition->value_kind);
  }
  bool FrameGenerationEnabledForSession() const {
    return lossless_dll_present_ &&
        platform::prospero::ReadFrameGenerationSessionRequest().enabled;
  }
  bool Experimental(const Row& row) const {
    if (row.definition->experimental) return true;
    for (const auto& choice : row.definition->choices)
      if (Equal(row.desired, Parse(choice.value, row.definition->value_kind))) return choice.experimental;
    return false;
  }
  std::string Label(const Row& row, const ConfigValue& value) const {
    if (const auto* boolean = std::get_if<bool>(&value))
      return *boolean ? "On" : "Off";
    for (const auto& choice : row.definition->choices)
      if (Equal(value, Parse(choice.value, row.definition->value_kind)))
        return choice.label;
    return Text(value);
  }
  ConfigValue Applied(const Row& row) const {
    if (row.definition->apply != Apply::kLive) return row.startup;
    const auto key = LiveKey(row.definition->key);
    if (!key) return row.startup;
    const int32_t value = GetLiveSettings().Applied(*key).value();
    switch (*key) {
      case LiveSetting::kFrameRates:
        return int64_t(std::string_view(row.definition->key) == "frame_rate"
            ? LiveSettings::GameFrameRate(value) : LiveSettings::MenuFrameRate(value));
      case LiveSetting::kAnisotropy: return int64_t(value);
      case LiveSetting::kSwapPostEffect:
        return std::string(value == 1 ? "fxaa" : value == 2 ? "fxaa_extreme" : "none");
      case LiveSetting::kPresentEffect:
        return std::string(value == 1 ? "fsr" : value == 2 ? "cas" : "bilinear");
      case LiveSetting::kFsrSharpnessReduction: return double(value) * 0.001;
      case LiveSetting::kColorSaturation:
      case LiveSetting::kColorVibrance:
      case LiveSetting::kColorContrast:
      case LiveSetting::kColorBrightness: return double(value) * 0.001;
      default: return value != 0;
    }
  }
  void ApplyLive(Row& row) {
    if (std::string_view(row.definition->key) == "frame_gen") {
      // Use the exact controller request path. Saving is separate: a shortcut
      // change must never overwrite the startup value held in row.desired.
      const auto request = platform::prospero::SetFrameGenerationForSession(
          std::get<bool>(row.desired));
      REXLOG_INFO("PS5 settings frame generation session request={} revision={}; "
                  "menu choice will also be saved for next start",
                  request.enabled ? "on" : "off", request.revision);
      return;
    }
    const auto key = LiveKey(row.definition->key);
    if (!key) throw std::runtime_error("No live consumer exists for this setting");
    int32_t value = 0;
    if (*key == LiveSetting::kFrameRates) {
      value = LiveSettings::PackFrameRates(int32_t(Number(Find("frame_rate")->desired)),
                                          int32_t(Number(Find("menu_frame_rate")->desired)));
    } else if (*key == LiveSetting::kAnisotropy) value = int32_t(Number(row.desired));
    else if (*key == LiveSetting::kSwapPostEffect) {
      const auto& effect = std::get<std::string>(row.desired);
      value = effect == "fxaa" ? 1 : effect == "fxaa_extreme" ? 2 : 0;
    } else if (*key == LiveSetting::kPresentEffect) {
      const auto& effect = std::get<std::string>(row.desired);
      value = effect == "fsr" ? 1 : effect == "cas" ? 2 : 0;
    } else if (*key == LiveSetting::kFsrSharpnessReduction ||
               *key == LiveSetting::kColorSaturation || *key == LiveSetting::kColorVibrance ||
               *key == LiveSetting::kColorContrast || *key == LiveSetting::kColorBrightness)
      value = int32_t(std::lround(Number(row.desired) * 1000));
    else value = std::get<bool>(row.desired);

    if ((*key == LiveSetting::kPresentEffect || *key == LiveSetting::kFsrSharpnessReduction) && !presenter_)
      throw std::runtime_error("Presentation is not available");
    const auto request = GetLiveSettings().Request(*key, value);
    if (request.value() != value) throw std::runtime_error("Only 30 and 60 FPS are supported");
    if (*key == LiveSetting::kPresentEffect || *key == LiveSetting::kFsrSharpnessReduction) {
      auto config = presenter_->GetGuestOutputPaintConfigFromUIThread();
      if (*key == LiveSetting::kPresentEffect) {
        config.SetEffect(value == 1 ? Presenter::GuestOutputPaintConfig::Effect::kFsr
            : value == 2 ? Presenter::GuestOutputPaintConfig::Effect::kCas
                         : Presenter::GuestOutputPaintConfig::Effect::kBilinear);
      } else config.SetFsrSharpnessReduction(float(value) * 0.001f);
      presenter_->SetGuestOutputPaintConfigFromUIThread(config);
    } else if (*key == LiveSetting::kLetterbox && presenter_) {
      presenter_->RequestUIPaintFromUIThread();
    }
    REXLOG_INFO("PS5 settings requested: {}={} version={} (awaiting consumer)",
                row.definition->key, Text(row.desired), request.version());
  }
  void Save() {
    ConfigValues values;
    for (const auto& row : rows_) if (row.available) values.emplace(row.definition->key, row.desired);
    // The version marker prevents old-profile migration from overriding newly
    // selected builder/overlay values on the next launch.
    values.emplace("prospero_profile_version", profile_version_);
    store_.Queue(std::move(values));
  }
  void Change(Row& row, ConfigValue value, bool save = true) {
    if (!row.available || row.definition->apply == Apply::kFixed) return;
    // After the controller shortcut, the live session may differ from the
    // saved startup choice. Reapplying that same saved value must still reach
    // the session setter (including Restore recommended settings).
    const bool frame_gen_session_change = std::string_view(row.definition->key) == "frame_gen" &&
        std::holds_alternative<bool>(value) && std::get<bool>(value) !=
            platform::prospero::ReadFrameGenerationSessionRequest().enabled;
    if (Equal(row.desired, value) && !frame_gen_session_change) return;
    const auto& definition = *row.definition;
    // File-dependent controls remain disabled without the supplied DLL. The
    // restore action uses the effective default: frame generation is always
    // OFF without the file, even when the normal preset recommends ON.
    if (!RequiredFileAvailable(row) && !Equal(value, DefaultValue(row))) return;
    bool valid = true;
    if (definition.control == Control::kChoice) {
      valid = std::any_of(definition.choices.begin(), definition.choices.end(), [&](const auto& choice) {
        return Equal(value, Parse(choice.value, definition.value_kind));
      });
    } else if (definition.control == Control::kNumber) {
      valid = (std::holds_alternative<int64_t>(value) || std::holds_alternative<double>(value)) &&
          std::isfinite(Number(value)) && Number(value) >= definition.minimum - 1e-8 &&
          Number(value) <= definition.maximum + 1e-8;
    } else if (definition.control == Control::kBoolean) valid = std::holds_alternative<bool>(value);
    else if (definition.control == Control::kColor) {
      const auto* hex = std::get_if<std::string>(&value);
      valid = hex && hex->size() == 6 && hex->find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
    }
    if (!valid) {
      action_error_ = "Unsupported value for this setting";
      REXLOG_ERROR("PS5 settings rejected invalid input: {}={}", definition.key, Text(value));
      return;
    }
    row.desired = std::move(value);
    action_error_.clear();
    try {
      if (row.definition->apply == Apply::kLive) ApplyLive(row);
      else REXLOG_INFO("PS5 settings staged for restart: {}={}", row.definition->key, Text(row.desired));
    } catch (const std::exception& error) {
      action_error_ = error.what();
      REXLOG_ERROR("PS5 settings request failed: {} error={}", row.definition->key, action_error_);
    }
    if (save) Save();
  }
  void RestoreDefaults() {
    for (auto& row : rows_) Change(row, DefaultValue(row), false);
    Save();
  }
  void RecoverRejectedPresenter() {
    const auto request = GetLiveSettings().Requested(LiveSetting::kPresentEffect);
    if (!presenter_ || request.packed == recovered_present_version_ ||
        GetLiveSettings().Rejected(LiveSetting::kPresentEffect).packed != request.packed) return;
    recovered_present_version_ = request.packed;
    const auto applied = GetLiveSettings().Applied(LiveSetting::kPresentEffect);
    const int32_t fallback = applied.version() ? applied.value() : 0;
    auto config = presenter_->GetGuestOutputPaintConfigFromUIThread();
    config.SetEffect(fallback == 1 ? Presenter::GuestOutputPaintConfig::Effect::kFsr
        : fallback == 2 ? Presenter::GuestOutputPaintConfig::Effect::kCas
                       : Presenter::GuestOutputPaintConfig::Effect::kBilinear);
    presenter_->SetGuestOutputPaintConfigFromUIThread(config);
    // Retain desired + rejected: saving and active policy remain distinct.
    REXLOG_WARN("PS5 settings effect unavailable; restored last usable presentation policy={}", fallback);
  }

  Presenter* presenter_;
  std::filesystem::path config_path_;
  SettingsConfigStore store_;
  std::vector<Row> rows_;
  bool lossless_dll_present_ = false;
  int64_t profile_version_ = 34;
  size_t group_ = 1;
  uint64_t recovered_present_version_ = 0;
  std::string action_error_;
};

std::shared_ptr<ProsperoSettingsModel> CreateProsperoSettingsModel(
    Presenter* presenter, const std::filesystem::path& config_path) {
  return std::make_shared<ProsperoSettingsModel>(presenter, config_path);
}

void RefreshProsperoSettingsAvailability(ProsperoSettingsModel& model) {
  model.RefreshAvailability();
}

void DrawProsperoSettings(ProsperoSettingsModel& model, ImGuiIO& io) {
  model.RecoverRejectedPresenter();
  const float scale = std::clamp(io.DisplaySize.y / 540.0f, 1.0f, 4.0f);
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.06f, io.DisplaySize.y * 0.05f));
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.88f, io.DisplaySize.y * 0.90f));
  ImGui::Begin("The Simpsons Game###ps5_settings", nullptr,
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
  ImGui::SetWindowFontScale(scale);
  ImGui::TextUnformatted("Settings");
  ImGui::SameLine();
  const uint32_t actual_output_millihertz = vulkan::GetProsperoOutputMilliHertz();
  if (actual_output_millihertz) {
    ImGui::TextDisabled("  Vulkan / RADV  |  Fullscreen  |  %.2f Hz",
        double(actual_output_millihertz) * 0.001);
  } else {
    ImGui::TextDisabled("  Vulkan / RADV  |  Fullscreen  |  Output initializing");
  }
  ImGui::Separator();
  // Both controller hints stay outside the scrollable settings list. Reserve
  // their actual wrapped height so the shortcut remains visible at 720p too.
  const float footer_width = ImGui::GetContentRegionAvail().x;
  const float footer = ImGui::GetTextLineHeightWithSpacing() * 3.3f +
      ImGui::CalcTextSize(kNavigationHint, nullptr, false, footer_width).y +
      ImGui::CalcTextSize(kFrameGenerationShortcutHint, nullptr, false, footer_width).y;
  const float body_height = std::max(100.0f, ImGui::GetContentRegionAvail().y - footer);
  const float sidebar = std::min(ImGui::GetContentRegionAvail().x * 0.25f, 235.0f * scale);
  ImGui::BeginChild("groups", ImVec2(sidebar, body_height), true);
  for (size_t i = 0; i < std::size(kGroups); ++i) {
    const auto& group = kGroups[i];
    if (ImGui::Selectable(group.label, model.group_ == i, 0,
                           ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 1.65f))) model.group_ = i;
  }
  ImGui::EndChild();
  ImGui::SameLine();
  ImGui::BeginChild("options", ImVec2(0, body_height), true);
  const auto& group = kGroups[model.group_];
  if (std::string_view(group.requires_file) == "lossless.dll" && !model.lossless_dll_present_) {
    ImGui::TextWrapped("Add Lossless.dll in the builder to enable these three settings.");
    ImGui::Separator();
  }
  if (std::string_view(group.requires_file) == "lossless.dll") {
    ImGui::TextWrapped("Turn frame generation on or off for this session below. "
                       "Changes here are also saved for the next start. "
                       "Target and multiplier changes apply after restarting the game.");
    ImGui::TextWrapped("Frame generation pauses while this menu is open.");
    ImGui::Separator();
  }

  // Derived presets only assign their real underlying settings. No synthetic
  // color_preset/ink_color_preset key can enter the configuration file.
  bool any_preset = false;
  for (const auto& preset : kPresets) {
    if (std::string_view(preset.group) != group.id) continue;
    if (!any_preset) { ImGui::TextUnformatted("Presets"); any_preset = true; }
    ImGui::PushID(preset.id);
    if (ImGui::Button(preset.label)) {
      for (const auto& value : preset.values) if (auto* row = model.Find(value.key))
        model.Change(*row, Parse(value.value, row->definition->value_kind), false);
      model.Save();
    }
    ImGui::PopID();
    ImGui::SameLine();
  }
  if (any_preset) { ImGui::NewLine(); ImGui::Separator(); }

  for (auto& row : model.rows_) {
    const auto& def = *row.definition;
    if (std::string_view(def.group) != group.id) continue;
    ImGui::PushID(def.key);
    ImGui::TextUnformatted(def.label);
    if (model.Experimental(row)) {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f), "  Experimental");
    }
    const bool enabled = model.Enabled(row);
    ImGui::BeginDisabled(!enabled);
    ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x, 430.0f * scale));
    if (def.control == Control::kChoice) {
      const auto current = model.Label(row, row.desired);
      if (ImGui::BeginCombo("##value", current.c_str())) {
        for (const auto& choice : def.choices) {
          auto value = Parse(choice.value, def.value_kind);
          const bool selected = Equal(row.desired, value);
          std::string label = choice.label;
          if (choice.experimental) label += "  [Experimental]";
          if (ImGui::Selectable(label.c_str(), selected)) model.Change(row, std::move(value));
          if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
    } else if (def.control == Control::kBoolean) {
      // The checkbox follows the live session, including changes made by the
      // controller shortcut. Keep row.desired as the saved startup value.
      bool value = std::string_view(def.key) == "frame_gen"
          ? model.FrameGenerationEnabledForSession() : std::get<bool>(row.desired);
      if (ImGui::Checkbox("Enabled", &value)) model.Change(row, value);
    } else if (def.control == Control::kNumber) {
      const int maximum = int(std::lround((def.maximum - def.minimum) / def.step));
      int position = int(std::lround((Number(row.desired) - def.minimum) / def.step));
      if (ImGui::SliderInt("##value", &position, 0, maximum, "", ImGuiSliderFlags_NoInput))
        model.Change(row, def.minimum + double(position) * def.step);
      ImGui::SameLine();
      ImGui::Text("%.2f", Number(row.desired));
    } else if (def.control == Control::kColor) {
      const auto& hex = std::get<std::string>(row.desired);
      unsigned rgb = 0;
      std::from_chars(hex.data(), hex.data() + hex.size(), rgb, 16);
      int colors[3] = {int((rgb >> 16) & 255), int((rgb >> 8) & 255), int(rgb & 255)};
      bool changed = false;
      const char* channels[] = {"Red", "Green", "Blue"};
      for (size_t i = 0; i < 3; ++i) {
        ImGui::SetNextItemWidth(std::min(ImGui::GetContentRegionAvail().x * 0.72f, 360.0f * scale));
        changed |= ImGui::SliderInt(channels[i], &colors[i], 0, 255, "%d", ImGuiSliderFlags_NoInput);
      }
      ImGui::ColorButton("##swatch", ImVec4(colors[0] / 255.f, colors[1] / 255.f, colors[2] / 255.f, 1.f));
      ImGui::SameLine(); ImGui::Text("#%02X%02X%02X", colors[0], colors[1], colors[2]);
      if (changed) {
        char selected[7];
        std::snprintf(selected, sizeof(selected), "%02X%02X%02X", colors[0], colors[1], colors[2]);
        model.Change(row, std::string(selected));
      }
    }
    ImGui::EndDisabled();
    ImGui::PushTextWrapPos(0);
    ImGui::TextDisabled("%s", def.description);
    if (std::string_view(def.key) == "frame_gen") {
      ImGui::TextColored(ImVec4(.9f, .85f, .55f, 1.f), "%s", kFrameGenerationShortcutHint);
      ImGui::TextDisabled("Release all four buttons before pressing again. "
                          "The shortcut does not change the saved startup setting.");
    }
    if (std::string_view(def.key) == "frame_gen") {
      if (!row.available) {
        ImGui::TextColored(ImVec4(1.f, .45f, .35f, 1.f), "Unavailable in this build");
      } else if (!model.RequiredFileAvailable(row)) {
        ImGui::TextColored(ImVec4(1.f, .8f, .35f, 1.f), "Unavailable without Lossless.dll");
      } else {
        // This is requested session state, not a fabricated renderer ack.
        // Generation is intentionally paused for the entire open overlay.
        const bool session_enabled = model.FrameGenerationEnabledForSession();
        ImGui::TextColored(ImVec4(.6f, .8f, 1.f, 1.f), "%s", session_enabled
            ? "On for this session; pauses while this menu is open."
            : "Off for this session.");
        if (session_enabled != std::get<bool>(row.desired)) {
          ImGui::TextDisabled("Saved for next start: %s", model.Label(row, row.desired).c_str());
        } else {
          ImGui::TextDisabled("Changes here are saved for the next start.");
        }
      }
    } else if (!row.available) {
      ImGui::TextColored(ImVec4(1.f, .45f, .35f, 1.f), "Unavailable in this build");
    } else if (def.apply == Apply::kRestart) {
      ImGui::TextColored(ImVec4(1.f, .8f, .35f, 1.f), "Applied after restarting the game");
      if (!Equal(row.desired, row.startup)) {
        const char* startup_label = std::string_view(def.key) == "frame_gen"
            ? "At game startup" : "Currently active";
        ImGui::TextDisabled("%s: %s", startup_label,
                            model.Label(row, row.startup).c_str());
      }
    } else if (def.apply == Apply::kLive && enabled) {
      const auto key = LiveKey(def.key);
      if (key) {
        const auto requested = GetLiveSettings().Requested(*key);
        const auto applied = GetLiveSettings().Applied(*key);
        const auto rejected = GetLiveSettings().Rejected(*key);
        if (requested.packed == rejected.packed && rejected.version()) {
          ImGui::TextColored(ImVec4(1.f, .45f, .35f, 1.f), "Effect unavailable; fallback in use");
        } else if (requested.packed == applied.packed && Equal(row.desired, model.Applied(row))) {
          ImGui::TextColored(ImVec4(.4f, 1.f, .65f, 1.f), "Applied");
        } else {
          ImGui::TextColored(ImVec4(.6f, .8f, 1.f, 1.f), "Waiting for the game to use this setting");
        }
      }
    }
    ImGui::PopTextWrapPos();
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    ImGui::PopID();
  }
  if (ImGui::Button("Restore recommended settings")) {
    model.RestoreDefaults();
  }
  ImGui::EndChild();
  ImGui::Separator();
  const auto status = model.store_.GetStatus();
  if (!status.error.empty()) {
    ImGui::TextColored(ImVec4(1.f, .45f, .35f, 1.f), "Settings were not saved");
    ImGui::SameLine();
    if (ImGui::Button("Retry save")) model.Save();
    ImGui::TextWrapped("%s", status.error.c_str());
  } else if (status.requested != status.persisted) {
    ImGui::TextUnformatted("Saving settings...");
  } else {
    ImGui::TextUnformatted("Settings saved automatically");
  }
  if (!status.durability_warning.empty()) {
    ImGui::TextColored(ImVec4(1.f, .8f, .35f, 1.f),
        "Saved. Protection against sudden power loss could not be confirmed.");
  }
  if (!model.action_error_.empty()) ImGui::TextWrapped("%s", model.action_error_.c_str());
  ImGui::TextWrapped("%s", kNavigationHint);
  ImGui::TextWrapped("%s", kFrameGenerationShortcutHint);
  ImGui::End();
}
}  // namespace rex::ui
