#include <rex/ui/window_prospero.h>

#include <rex/cvar.h>
#include <rex/input/prospero/prospero_input_driver.h>
#include <rex/logging.h>
#include <rex/ui/keybinds.h>
#include <rex/ui/surface_prospero.h>
#include <rex/ui/windowed_app_context_prospero.h>

namespace rex::ui {

std::unique_ptr<Window> Window::Create(WindowedAppContext& app_context,
                                      std::string_view title,
                                      uint32_t desired_logical_width,
                                      uint32_t desired_logical_height) {
  if (!dynamic_cast<ProsperoWindowedAppContext*>(&app_context)) {
    REXLOG_ERROR("ProsperoWindow requires the Prospero UI context");
    return nullptr;
  }
  return std::make_unique<ProsperoWindow>(app_context, title, desired_logical_width,
                                         desired_logical_height);
}

ProsperoWindow::ProsperoWindow(WindowedAppContext& app_context, std::string_view title,
                               uint32_t desired_logical_width, uint32_t desired_logical_height)
    : Window(app_context, title, desired_logical_width, desired_logical_height),
      dispatch_state_(std::make_shared<DispatchState>()) {
  dispatch_state_->window = this;
  window_pump_token_ = static_cast<ProsperoWindowedAppContext&>(app_context).AddWindowPump(
      [state = dispatch_state_](bool poll_input) {
        if (poll_input && state->window && state->window->phase() == Phase::kOpen) {
          state->window->PollInput();
        }
        if (state->window && state->window->phase() == Phase::kOpen &&
            state->paint_pending.exchange(false, std::memory_order_acq_rel)) {
          state->window->OnPaint();
        }
      });
}

ProsperoWindow::~ProsperoWindow() {
  EnterDestructor();
  dispatch_state_->window = nullptr;
  static_cast<ProsperoWindowedAppContext&>(app_context()).RemoveWindowPump(window_pump_token_);
}

bool ProsperoWindow::OpenImpl() {
  WindowDestructionReceiver receiver(this);
  OnDesiredFullscreenUpdate(true);
  OnActualSizeUpdate(ProsperoDisplaySurface::kWidth, ProsperoDisplaySurface::kHeight, receiver);
  OnFocusUpdate(true, receiver);
  settings_chord_down_ = false;
  return true;
}

void ProsperoWindow::RequestCloseImpl() {
  WindowDestructionReceiver receiver(this);
  OnBeforeClose(receiver);
  if (!receiver.IsWindowDestroyed()) {
    OnAfterClose();
  }
}

void ProsperoWindow::ApplyNewFullscreen() {
  // There is no windowed mode on the native title's VideoOut surface.
  OnDesiredFullscreenUpdate(true);
}

void ProsperoWindow::FocusImpl() {
  WindowDestructionReceiver receiver(this);
  OnFocusUpdate(true, receiver);
}

std::unique_ptr<Surface> ProsperoWindow::CreateSurfaceImpl(Surface::TypeFlags allowed_types) {
  if (!(allowed_types & Surface::kTypeFlag_ProsperoDisplay)) {
    REXLOG_ERROR("The graphics provider cannot present to the PS5 display");
    return nullptr;
  }
  return std::make_unique<ProsperoDisplaySurface>();
}

void ProsperoWindow::RequestPaintImpl() {
  if (!dispatch_state_->paint_pending.exchange(true, std::memory_order_acq_rel)) {
    static_cast<ProsperoWindowedAppContext&>(app_context()).WakeUILoop();
  }
}

void ProsperoWindow::PollInput() {
  rex::input::X_INPUT_STATE state{};
  bool chord = false;
  if (rex::input::prospero::PollPadState(0, &state) == X_ERROR_SUCCESS) {
    constexpr uint16_t mask = rex::input::X_INPUT_GAMEPAD_LEFT_SHOULDER |
                              rex::input::X_INPUT_GAMEPAD_RIGHT_SHOULDER |
                              rex::input::X_INPUT_GAMEPAD_START;
    chord = (uint16_t(state.gamepad.buttons) & mask) == mask;
  }
  const bool pressed = chord && !settings_chord_down_;
  settings_chord_down_ = chord;
  if (pressed) {
    // Dispatch the existing settings shortcut; no separate settings state.
    const auto key = ParseVirtualKey(REXCVAR_QUERY(std::string, bind_settings));
    if (key == VirtualKey::kNone) {
      return;
    }
    KeyEvent event(this, key, 1, false, false, false, false, false);
    ProcessKeyEvent(event);
  }
}

// Native PS5 settings use ImGui, without a platform menubar. Retain menu
// models for the common API without claiming a native menu has been shown.
std::unique_ptr<MenuItem> MenuItem::Create(Type type, const std::string& text,
                                           const std::string& hotkey,
                                           std::function<void()> callback) {
  return std::unique_ptr<MenuItem>(new MenuItem(type, text, hotkey, std::move(callback)));
}

}  // namespace rex::ui
