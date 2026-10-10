// Full production Prospero input driver, plus byte-for-byte XAM/InputSystem
// methods selected by the runner. Only the native scePad/UserService calls,
// optional injection and UI capture state are fixtures. Guest structs retain
// the production big-endian fields and native PadData retains its exact ABI.
#include <rex/input/input_system.h>
#include <rex/input/prospero/prospero_input_driver.h>
#include <rex/ui/overlay_input.h>
#include <rex/ui/virtual_key.h>
#include <rex/platform/framegen_prospero.h>
#include <rex/platform/trace_prospero.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <unistd.h>

#define SCOPE_profile_cpu_f(...)
#define REXKRNL_TRACE(...)
namespace {
unsigned checks = 0, failures = 0;
void Check(bool ok, const char* message) {
  ++checks;
  if (!ok) {
    ++failures;
    std::fprintf(stderr, "FAIL %s\n", message);
  }
}
std::array<int, 4> users{44, 99, -1, -1};
std::vector<int> opened_users;
int last_read_user = -1, last_rumble_user = -1;
rex::input::prospero::PadVibration last_vibration{};
bool overlay_active = false;
bool pad_connected = true;
uint32_t pad_buttons = 0;
bool explicit_pad_buttons = false;
uint8_t pad_lx = 128;
int pad_read_error = 0;
unsigned rumble_calls = 0;
std::atomic<unsigned> framegen_toggles{0}, framegen_unlocked_probes{0};
bool framegen_probe_unlocked = false;
}  // namespace

extern "C" {
int scePadInit() { return 0; }
int sceUserServiceInitialize(void*) { return 0; }
int sceUserServiceGetLoginUserIdList(int output[4]) {
  std::copy(users.begin(), users.end(), output);
  return 0;
}
int sceUserServiceGetInitialUser(int* user) { *user = 99; return 0; }
int scePadOpen(int user, int, int, void*) { opened_users.push_back(user); return user + 1000; }
int scePadGetHandle(int user, int, int) { return user + 1000; }
int scePadReadState(int handle, rex::input::prospero::PadData* state) {
  using namespace rex::input::prospero;
  last_read_user = handle - 1000;
  if (pad_read_error) return pad_read_error;
  *state = {};
  state->connected = pad_connected;
  state->left_stick = {pad_lx, 128};
  state->right_stick = {128, 128};
  state->buttons = explicit_pad_buttons || pad_buttons ? pad_buttons :
      (last_read_user == 99 ? kPadCross : kPadCircle);
  return 0;
}
int scePadSetVibrationMode(int, int) { return 0; }
int scePadSetVibration(int handle, const rex::input::prospero::PadVibration* vibration) {
  ++rumble_calls;
  last_rumble_user = handle - 1000;
  last_vibration = *vibration;
  return 0;
}
int scePadClose(int) { return 0; }
}

namespace rex::ui {
bool IsOverlayInputActive() { return overlay_active; }
}
namespace rex::platform::prospero {
bool ToggleFrameGenerationForSession() {
  const unsigned count = framegen_toggles.fetch_add(1) + 1;
  if (framegen_probe_unlocked) {
    // Reentering the native poll would deadlock if the callback ran under its
    // mutex. The same held press must also not request a recursive toggle.
    input::X_INPUT_STATE state{};
    if (input::prospero::PollPadState(0, &state) == X_ERROR_SUCCESS) {
      framegen_unlocked_probes.fetch_add(1);
    }
  }
  return (count & 1) != 0;
}
}
namespace rex::input {
namespace {
struct DisabledInjector {
  bool enabled() const { return false; }
  bool Apply(X_INPUT_STATE&) const { return false; }
} g_input_injector;
void FillInjectedCapabilities(X_INPUT_CAPABILITIES*) { std::abort(); }
}
#include "input_system_methods.inc"
}  // namespace rex::input

namespace rex::kernel::xam {
using u32 = uint32_t;
template <class T> using ppc_ptr_t = T*;
using input::X_INPUT_CAPABILITIES;
using input::X_INPUT_STATE;
using input::X_INPUT_VIBRATION;
using input::X_INPUT_KEYSTROKE;
input::InputSystem* fixture_input = nullptr;
input::InputSystem* input_system() { return fixture_input; }
#include "xam_input_methods.inc"
}  // namespace rex::kernel::xam

static void CheckFrameGenerationShortcut(rex::input::prospero::ProsperoInputDriver* driver) {
  using namespace rex;
  using namespace rex::input;
  const unsigned began_checks = checks, began_failures = failures;
  constexpr uint32_t native_chord = prospero::kPadL1 | prospero::kPadR1 |
                                    prospero::kPadL3 | prospero::kPadR3;
  constexpr uint16_t guest_chord = X_INPUT_GAMEPAD_LEFT_SHOULDER |
      X_INPUT_GAMEPAD_RIGHT_SHOULDER | X_INPUT_GAMEPAD_LEFT_THUMB | X_INPUT_GAMEPAD_RIGHT_THUMB;
  constexpr uint16_t freecam_chord = X_INPUT_GAMEPAD_LEFT_THUMB | X_INPUT_GAMEPAD_RIGHT_THUMB;
  X_INPUT_STATE state{};
  X_INPUT_KEYSTROKE key{};
  explicit_pad_buttons = true;
  pad_buttons = 0;
  pad_lx = 128;
  framegen_probe_unlocked = true;
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS, "neutral poll arms framegen shortcut");
  for (unsigned i = 0; i < 40; ++i) {
    if (driver->GetKeystroke(0, 0, &key) != X_ERROR_SUCCESS) break;
  }

  // This first trigger uses guest input only: no UI tick or rendered image.
  pad_buttons = native_chord;
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS && framegen_toggles == 1,
        "guest-only framegen shortcut requests one toggle without UI polling");
  Check((uint16_t(state.gamepad.buttons) & guest_chord) == 0,
        "framegen shortcut is consumed before guest/freecam sees L3+R3");
  bool held_clean = true;
  for (unsigned i = 0; i < 8; ++i) {
    held_clean &= prospero::PollPadState(0, &state) == X_ERROR_SUCCESS &&
                  (uint16_t(state.gamepad.buttons) & guest_chord) == 0;
    held_clean &= driver->GetState(0, &state) == X_ERROR_SUCCESS &&
                  (uint16_t(state.gamepad.buttons) & guest_chord) == 0;
    held_clean &= driver->GetKeystroke(0, 0, &key) == X_ERROR_EMPTY;
  }
  Check(held_clean && framegen_toggles == 1,
        "held chord across raw/guest/keystroke polls never repeats or leaks keys");

  pad_buttons = prospero::kPadL3 | prospero::kPadR3;
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS &&
        (uint16_t(state.gamepad.buttons) & freecam_chord) == 0,
        "releasing shoulders first cannot activate freecam");
  Check(driver->GetKeystroke(0, 0, &key) == X_ERROR_EMPTY,
        "partial shortcut release emits no guest keystrokes");
  pad_buttons = native_chord;
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS && framegen_toggles == 1,
        "repressing shoulders before full release cannot retrigger framegen");
  pad_buttons = 0;
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS &&
        driver->GetKeystroke(0, 0, &key) == X_ERROR_EMPTY,
        "complete chord release rearms without synthetic key-up events");
  pad_buttons = native_chord;
  Check(prospero::PollPadState(0, &state) == X_ERROR_SUCCESS && framegen_toggles == 2,
        "raw UI polling and guest polling share the same framegen latch");
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS && framegen_toggles == 2,
        "guest poll after raw poll does not toggle twice");

  pad_buttons = 0; driver->GetState(0, &state);
  pad_buttons = native_chord;
  std::array<X_INPUT_STATE, 2> concurrent_states{};
  std::array<X_RESULT, 2> concurrent_results{};
  std::thread guest_a([&] { concurrent_results[0] = driver->GetState(0, &concurrent_states[0]); });
  std::thread guest_b([&] { concurrent_results[1] = driver->GetState(0, &concurrent_states[1]); });
  guest_a.join(); guest_b.join();
  Check(framegen_toggles == 3 && concurrent_results[0] == X_ERROR_SUCCESS &&
        concurrent_results[1] == X_ERROR_SUCCESS &&
        (uint16_t(concurrent_states[0].gamepad.buttons) & guest_chord) == 0 &&
        (uint16_t(concurrent_states[1].gamepad.buttons) & guest_chord) == 0,
        "concurrent GetState calls deliver one request and both suppress the chord");
  Check(framegen_unlocked_probes == 3,
        "framegen callback executes outside the pad mutex and is safe to reenter polling");

  pad_buttons = 0; driver->GetState(1, &state);
  pad_buttons = native_chord;
  Check(driver->GetState(1, &state) == X_ERROR_SUCCESS && framegen_toggles == 3 &&
        (uint16_t(state.gamepad.buttons) & guest_chord) == guest_chord,
        "second controller neither toggles framegen nor loses ordinary buttons");

  pad_buttons = 0; driver->GetState(0, &state);
  pad_buttons = prospero::kPadL3 | prospero::kPadR3;
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS && framegen_toggles == 3 &&
        (uint16_t(state.gamepad.buttons) & freecam_chord) == freecam_chord,
        "standalone L3+R3 remains available to the existing free camera");
  Check(driver->GetKeystroke(0, 0, &key) == X_ERROR_SUCCESS &&
        uint16_t(key.virtual_key) == uint16_t(ui::VirtualKey::kXInputPadLThumbPress),
        "ordinary stick-click keystrokes remain available outside framegen chord");
  pad_buttons = 0; driver->GetState(0, &state);

  pad_buttons = prospero::kPadL1 | prospero::kPadR1 | prospero::kPadOptions;
  constexpr uint16_t settings_chord = X_INPUT_GAMEPAD_LEFT_SHOULDER |
      X_INPUT_GAMEPAD_RIGHT_SHOULDER | X_INPUT_GAMEPAD_START;
  Check(prospero::PollPadState(0, &state) == X_ERROR_SUCCESS &&
        uint16_t(state.gamepad.buttons) == settings_chord && framegen_toggles == 3,
        "L1+R1+Options is unchanged for the native settings menu");
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS && uint16_t(state.gamepad.buttons) == 0,
        "settings menu shortcut still captures guest input");

  pad_buttons = 0; driver->GetState(0, &state);
  overlay_active = true; pad_buttons = native_chord;
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS && framegen_toggles == 4 &&
        uint16_t(state.gamepad.buttons) == 0,
        "framegen shortcut remains independent of overlay input capture");
  overlay_active = false;
  pad_connected = false;
  Check(driver->GetState(0, &state) == X_ERROR_DEVICE_NOT_CONNECTED,
        "disconnect while shortcut is held remains a guest disconnect");
  pad_connected = true;
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS && framegen_toggles == 4 &&
        (uint16_t(state.gamepad.buttons) & guest_chord) == 0,
        "reconnecting with shortcut held does not create a fresh toggle");
  pad_buttons = prospero::kPadL3 | prospero::kPadR3;
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS && framegen_toggles == 4 &&
        (uint16_t(state.gamepad.buttons) & freecam_chord) == 0,
        "partial release after reconnect still cannot leak the freecam chord");
  pad_buttons = 0; driver->GetState(0, &state);
  pad_buttons = native_chord | prospero::kPadCross | prospero::kPadUp;
  pad_lx = 255;
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS && framegen_toggles == 5,
        "full release after reconnect rearms the next deliberate toggle");
  Check(uint16_t(state.gamepad.buttons) == (X_INPUT_GAMEPAD_A | X_INPUT_GAMEPAD_DPAD_UP) &&
        int16_t(state.gamepad.thumb_lx) == 32767,
        "shortcut consumption preserves unrelated buttons and analog movement");
  Check(framegen_unlocked_probes == 5, "every framegen request executes once outside native pad lock");

  pad_buttons = 0; pad_lx = 128; driver->GetState(0, &state);
  explicit_pad_buttons = false;
  framegen_probe_unlocked = false;
  std::printf("%s framegen shortcut through production input driver: %u checks, %u failures\n",
              failures == began_failures ? "PASS" : "FAIL", checks - began_checks, failures - began_failures);
}

int main(int argc, char** argv) {
  const int log = argc > 1 ? open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600) : -1;
  prospero_trace_set_output_fd(log);
  using namespace rex;
  using namespace rex::input;
  using namespace rex::kernel::xam;
  InputSystem input(nullptr);
  fixture_input = &input;
  auto owned = std::make_unique<prospero::ProsperoInputDriver>(nullptr, 0);
  auto* driver = owned.get();
  Check(driver->Setup() == X_STATUS_SUCCESS, "native driver initializes");
  input.AddDriver(std::move(owned));
  X_INPUT_CAPABILITIES caps{};
  Check(XamInputGetCapabilities_entry(0, 0, &caps) == X_ERROR_SUCCESS,
        "capabilities with no filter succeeds");
  Check(last_read_user == 99 && opened_users.front() == 99,
        "Xbox user zero maps to initial PS5 user despite login-list order");
  Check(caps.type == XINPUT_DEVTYPE_GAMEPAD && uint16_t(caps.flags) == X_INPUT_CAPS_FFB_SUPPORTED &&
        uint16_t(caps.gamepad.buttons) == 0xf3ff && uint16_t(caps.vibration.left_motor_speed) == 0xffff,
        "capabilities preserve production big-endian fields and rumble support");
  Check(XamInputGetCapabilities_entry(1, XINPUT_FLAG_GAMEPAD, &caps) == X_ERROR_SUCCESS &&
        last_read_user == 44, "explicit second user keeps its native pad");
  Check(XamInputGetCapabilities_entry(1, XINPUT_FLAG_ANY_USER, &caps) == X_ERROR_SUCCESS &&
        last_read_user == 99, "XAM ANY_USER capabilities reaches the initial user's pad");
  Check(XamInputGetCapabilities_entry(3, XINPUT_FLAG_ANY_USER | XINPUT_FLAG_GAMEPAD, &caps) ==
            X_ERROR_SUCCESS && last_read_user == 99,
        "XAM ANY_USER plus GAMEPAD capabilities succeeds");
  Check(XamInputGetCapabilitiesEx_entry(0, 1, XINPUT_FLAG_ANY_USER, &caps) == X_ERROR_SUCCESS &&
        last_read_user == 99, "XAM capabilities Ex accepts ANY_USER");
  Check(XamInputGetCapabilitiesEx_entry(0, 3, XINPUT_FLAG_ANY_USER | XINPUT_FLAG_GAMEPAD, &caps) ==
            X_ERROR_SUCCESS && last_read_user == 99,
        "XAM capabilities Ex accepts ANY_USER plus GAMEPAD");
  Check(XamInputGetCapabilities_entry(0xff, XINPUT_FLAG_GAMEPAD, &caps) == X_ERROR_SUCCESS &&
        last_read_user == 99, "XAM user 0xff capabilities maps to the initial user");
  Check(XamInputGetCapabilities_entry(0, 2, &caps) == X_ERROR_DEVICE_NOT_CONNECTED,
        "non-gamepad XAM query remains rejected");
  Check(driver->GetCapabilities(0, uint32_t(1) << 29, &caps) == X_ERROR_INVALID_PARAMETER,
        "unknown high selector remains invalid at the native boundary");
  Check(driver->GetCapabilities(0, 3, &caps) == X_ERROR_INVALID_PARAMETER,
        "unknown low flag remains invalid at the native boundary");
  Check(XamInputGetCapabilitiesEx_entry(0, 0, 0, nullptr) == X_ERROR_BAD_ARGUMENTS,
        "null capabilities pointer remains rejected by XAM");
  Check(XamInputGetCapabilities_entry(3, 0, &caps) == X_ERROR_DEVICE_NOT_CONNECTED,
        "unassigned PS5 user is not fabricated as a connected pad");

  X_INPUT_STATE state{};
  Check(XamInputGetState_entry(1, 0, &state) == X_ERROR_SUCCESS && last_read_user == 44 &&
        uint16_t(state.gamepad.buttons) == X_INPUT_GAMEPAD_B,
        "explicit second-user state is distinct from initial-user state");
  Check(XamInputGetState_entry(1, XINPUT_FLAG_ANY_USER, &state) == X_ERROR_SUCCESS &&
        last_read_user == 99 && uint16_t(state.gamepad.buttons) == X_INPUT_GAMEPAD_A,
        "ANY_USER state uses the same pad as ANY_USER capabilities");
  Check(XamInputGetState_entry(0xff, 0, &state) == X_ERROR_SUCCESS && last_read_user == 99,
        "user 0xff state uses the initial pad");
  Check(XamInputGetState_entry(0xff, 0, nullptr) == X_ERROR_SUCCESS,
        "null-state query remains supported by InputSystem");
  overlay_active = true;
  Check(XamInputGetState_entry(0, 0, &state) == X_ERROR_SUCCESS &&
        uint16_t(state.gamepad.buttons) == 0, "overlay captures guest input");
  overlay_active = false;
  X_INPUT_VIBRATION vibration{};
  vibration.left_motor_speed = 0xabcd;
  vibration.right_motor_speed = 0x1234;
  Check(XamInputSetState_entry(0xff, 0, &vibration) == X_ERROR_SUCCESS &&
        last_rumble_user == 99 && last_vibration.large_motor == 0xab && last_vibration.small_motor == 0x12,
        "XAM rumble user mapping and big-endian motor values reach native pad");
  X_INPUT_KEYSTROKE key{};
  Check(XamInputGetKeystroke_entry(1, XINPUT_FLAG_ANY_USER, &key) == X_ERROR_SUCCESS &&
        key.user_index == 0 && uint16_t(key.flags) == X_INPUT_KEYSTROKE_KEYDOWN,
        "ANY_USER keystroke selects the same initial controller");
  const unsigned before_rumble = rumble_calls;
  Check(driver->SetState(0, &vibration) == X_ERROR_SUCCESS &&
        driver->SetState(0, &vibration) == X_ERROR_SUCCESS && rumble_calls == before_rumble,
        "diagnostics do not reissue an unchanged native rumble request");
  CheckFrameGenerationShortcut(driver);
  for (unsigned i = 0; i < 300; ++i) {
    pad_buttons = (i & 1) ? prospero::kPadCircle : prospero::kPadSquare;
    pad_lx = uint8_t((i % 127) + 129);
    Check(driver->GetState(0, &state) == X_ERROR_SUCCESS, "changed pad remains responsive under log budget");
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  pad_buttons = prospero::kPadTriangle;
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS, "coalesced final analog state is flushed on later poll");
  pad_connected = false;
  Check(driver->GetState(0, &state) == X_ERROR_DEVICE_NOT_CONNECTED,
        "physical disconnect remains visible to the guest");
  pad_connected = true;
  Check(driver->GetState(0, &state) == X_ERROR_SUCCESS, "physical reconnect remains visible to the guest");
  pad_read_error = int(0x80920001u);
  Check(driver->GetState(0, &state) == X_ERROR_DEVICE_NOT_CONNECTED,
        "native pad read failure remains a disconnect");
  input.Shutdown();
  prospero_trace_dump_fd(log);
  prospero_trace_set_output_fd(-1);
  if (log >= 0) close(log);
  std::printf("%s production XAM/InputSystem/Prospero chain: %u checks, %u failures\n",
              failures ? "FAIL" : "PASS", checks, failures);
  return failures ? 1 : 0;
}
