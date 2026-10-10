// Controller-oriented PS5 settings, generated from the builder capability list.
// This utility menu is always English, independently of the game's language.
// BSD 3-Clause License. See the project LICENSE.
#pragma once
#include <filesystem>
#include <memory>
#include <span>
#include <rex/platform/settings_store_prospero.h>

struct ImGuiIO;
namespace rex::ui {
class Presenter;
class ProsperoSettingsModel;
std::shared_ptr<ProsperoSettingsModel> CreateProsperoSettingsModel(
    Presenter* presenter, const std::filesystem::path& config_path);
// Refresh file-dependent availability only when opening the dialog. Drawing
// reads this snapshot and never probes the filesystem on the rendering thread.
void RefreshProsperoSettingsAvailability(ProsperoSettingsModel& model);
void DrawProsperoSettings(ProsperoSettingsModel& model, ImGuiIO& io);

namespace prospero_settings {
enum class ValueKind { kBoolean, kInteger, kNumber, kString };
enum class Control { kChoice, kBoolean, kNumber, kColor };
enum class Apply { kLive, kRestart, kFixed };
struct ChoiceDefinition {
  const char* value;
  const char* label;
  const char* label_it;
  bool experimental;
};
struct GroupDefinition {
  const char* id;
  const char* label;
  const char* label_it;
  const char* requires_file;
};
struct SettingDefinition {
  const char* key;
  const char* label;
  const char* label_it;
  const char* group;
  const char* description;
  const char* description_it;
  const char* default_value;
  ValueKind value_kind;
  Control control;
  Apply apply;
  bool experimental;
  std::span<const ChoiceDefinition> choices;
  double minimum;
  double maximum;
  double step;
  const char* requires_key;
  const char* requires_value;
};
struct PresetValue { const char* key; const char* value; };
struct PresetDefinition {
  const char* id;
  const char* label;
  const char* label_it;
  const char* group;
  std::span<const PresetValue> values;
};
}  // namespace prospero_settings
}  // namespace rex::ui
