/**
 * @file        rex/ui/overlay/controls_overlay.h
 * @brief       Keyboard / mouse controls overlay: the current bindings, over the
 *              game, without taking input (the game keeps playing).
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */
#pragma once
#include <rex/ui/imgui_dialog.h>

#include <chrono>

namespace rex::ui {

class ControlsOverlayDialog : public ImGuiDialog {
 public:
  // hint_only: just a short note that F1 shows the controls, which closes
  // itself after a few seconds.
  explicit ControlsOverlayDialog(ImGuiDrawer* imgui_drawer, bool hint_only = false);
  ~ControlsOverlayDialog();

  bool hint_only() const { return hint_only_; }

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  bool hint_only_;
  std::chrono::steady_clock::time_point opened_;
};

}  // namespace rex::ui
