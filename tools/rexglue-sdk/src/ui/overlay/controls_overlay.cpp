/**
 * @file        ui/overlay/controls_overlay.cpp
 * @brief       Keyboard / mouse controls overlay. See controls_overlay.h.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */
#include <rex/ui/overlay/controls_overlay.h>

#include <rex/cvar.h>

#include <imgui.h>

#include <algorithm>
#include <string>
#include <string_view>

namespace rex::ui {

namespace {

struct ControlRow {
  const char* button;  // as the game's prompts show it
  const char* cvar;    // nullptr: the mouse
  const char* action;
  bool hide_unbound = false;  // optional extras, listed only once bound
};

constexpr ControlRow kControlRows[] = {
    {"Left stick up", "keybind_lstick_up", "Move forward"},
    {"Left stick down", "keybind_lstick_down", "Move back"},
    {"Left stick left", "keybind_lstick_left", "Move left"},
    {"Left stick right", "keybind_lstick_right", "Move right"},
    {"Left stick, gently", "keybind_walk", "Walk (hold with a move key)"},
    {"Right stick", nullptr, "Look around"},
    {"Right stick up", "keybind_rstick_up", "Look up", true},
    {"Right stick down", "keybind_rstick_down", "Look down", true},
    {"Right stick left", "keybind_rstick_left", "Look left", true},
    {"Right stick right", "keybind_rstick_right", "Look right", true},
    {"A", "keybind_a", "Jump, confirm"},
    {"B", "keybind_b", "Special attack (hold), back"},
    {"X", "keybind_x", "Attack"},
    {"Y", "keybind_y", "Action: talk, use, pick up"},
    {"LT", "keybind_left_trigger", "Target (hold)"},
    {"RT", "keybind_right_trigger", ""},
    {"LB", "keybind_left_shoulder", ""},
    {"RB", "keybind_right_shoulder", ""},
    {"D-pad up", "keybind_dpad_up", "Switch character"},
    {"D-pad right", "keybind_dpad_right", "Switch character"},
    {"D-pad down", "keybind_dpad_down", "Switch character"},
    {"D-pad left", "keybind_dpad_left", "Switch character"},
    {"Back", "keybind_back", "To-do list"},
    {"Start", "keybind_start", "Pause"},
    {"Left stick click", "keybind_lstick_press", ""},
    {"Right stick click", "keybind_rstick_press", ""},
};

// ImGui's font is drawn for about 720 lines; scale it up on bigger windows.
float FontScale(const ImGuiIO& io) {
  return std::max(1.0f, io.DisplaySize.y / 720.0f);
}

// The binding (key names separated by commas) the way the launcher shows it.
std::string FormatBinding(std::string_view binding) {
  static const struct {
    std::string_view name;
    const char* label;
  } kLabels[] = {
      {"LMB", "Left click"},  {"RMB", "Right click"}, {"MMB", "Middle click"},
      {"Mouse4", "Mouse 4"},  {"Mouse5", "Mouse 5"},  {"WheelUp", "Wheel up"},
      {"WheelDown", "Wheel down"}, {"Control", "Ctrl"},  {"Escape", "Esc"},
      {"Return", "Enter"},    {"Left", "Left arrow"}, {"Right", "Right arrow"},
      {"Up", "Up arrow"},     {"Down", "Down arrow"},   {"Backtick", "`"},
  };
  std::string result;
  size_t start = 0;
  while (start <= binding.size()) {
    size_t end = binding.find(',', start);
    if (end == std::string_view::npos) {
      end = binding.size();
    }
    std::string_view key = binding.substr(start, end - start);
    while (!key.empty() && key.front() == ' ') {
      key.remove_prefix(1);
    }
    while (!key.empty() && key.back() == ' ') {
      key.remove_suffix(1);
    }
    if (!key.empty()) {
      if (!result.empty()) {
        result += "  /  ";
      }
      const char* label = nullptr;
      for (const auto& entry : kLabels) {
        if (entry.name == key) {
          label = entry.label;
          break;
        }
      }
      result += label ? std::string(label) : std::string(key);
    }
    start = end + 1;
  }
  return result.empty() ? std::string("(unbound)") : result;
}

constexpr std::chrono::seconds kHintDuration(10);

// The key an overlay keybind (bind_controls, bind_settings) is on now.
std::string BindKey(const char* cvar) {
  return FormatBinding(rex::cvar::GetFlagByName(cvar));
}

}  // namespace

ControlsOverlayDialog::ControlsOverlayDialog(ImGuiDrawer* imgui_drawer, bool hint_only)
    : ImGuiDialog(imgui_drawer), hint_only_(hint_only) {}

ControlsOverlayDialog::~ControlsOverlayDialog() = default;

void ControlsOverlayDialog::OnDraw(ImGuiIO& io) {
  const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_AlwaysAutoResize;
  if (hint_only_) {
    // Counted from the first frame it is drawn in, not from window creation,
    // so the hint isn't over before the game shows anything.
    const auto now = std::chrono::steady_clock::now();
    if (opened_ == std::chrono::steady_clock::time_point()) {
      opened_ = now;
    }
    if (now - opened_ > kHintDuration) {
      return;
    }
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.06f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.65f);
    if (ImGui::Begin("##controls_hint", nullptr, flags)) {
      ImGui::SetWindowFontScale(FontScale(io));
      std::string hint = "Keyboard & mouse: press " + BindKey("bind_controls") + " to see the controls";
      ImGui::TextUnformatted(hint.c_str());
    }
    ImGui::End();
    return;
  }
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                          ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowBgAlpha(0.85f);
  if (ImGui::Begin("##controls", nullptr, flags)) {
    ImGui::SetWindowFontScale(FontScale(io));
    ImGui::TextUnformatted("KEYBOARD & MOUSE CONTROLS");
    ImGui::Separator();
    const ImVec4 dim(0.65f, 0.65f, 0.65f, 1.0f);
    if (ImGui::BeginTable("##controls_table", 3, ImGuiTableFlags_SizingFixedFit)) {
      ImGui::TableSetupColumn("Button");
      ImGui::TableSetupColumn("Keys");
      ImGui::TableSetupColumn("Does");
      ImGui::TableHeadersRow();
      for (const ControlRow& row : kControlRows) {
        if (row.hide_unbound && rex::cvar::GetFlagByName(row.cvar).empty()) {
          continue;
        }
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextColored(dim, "%s", row.button);
        ImGui::TableSetColumnIndex(1);
        std::string binding =
            row.cvar ? FormatBinding(rex::cvar::GetFlagByName(row.cvar)) : std::string("Mouse");
        ImGui::TextUnformatted(binding.c_str());
        ImGui::TableSetColumnIndex(2);
        ImGui::TextUnformatted(row.action);
      }
      ImGui::EndTable();
    }
    ImGui::Separator();
    std::string change = "Change keys in the launcher, or in game with " + BindKey("bind_settings") +
                         " (Input > Keybinds).";
    std::string close = BindKey("bind_controls") + " closes this.";
    ImGui::TextUnformatted(change.c_str());
    ImGui::TextUnformatted(close.c_str());
  }
  ImGui::End();
}

}  // namespace rex::ui
