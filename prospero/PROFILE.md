# PS5 configuration

The builder creates `PPSA74207/userdata/simpsons.toml` from the selected
settings. The file uses flat TOML keys; the headings shown in the interface
are not TOML sections. The complete settings and allowed values are defined
in [the capability catalog](builder/Assets/capabilities.json).

## Recommended builder settings

Builder 3.10.6 selects English for a new configuration and when **Restore
recommended settings** is used. Other languages remain selectable, provided
they are included in the game data.

| Setting | Default |
|---|---|
| Game language | English, `user_language = 1` |
| Renderer | Vulkan with native render targets |
| Output | 4K, 120 Hz, VSync off |
| Render scale | 3, corresponding to 3840 x 2160 from a 1280 x 720 render |
| Shadows | Match render scale |
| Upscaling | FSR 1, sharpness reduction 0.0 |
| Anti-aliasing | FXAA |
| Eyes | Clean |
| Anisotropic filtering | 16x, stored as `anisotropic_override = 5` |
| Outlines | Original, black |
| Game frame rate | 60 FPS |
| Menu frame rate | 30 FPS |
| Frame generation | Enabled when Lossless.dll is supplied; 120 Hz target, 2x multiplier |

These are configuration targets, not guaranteed performance measurements.
Native game frame rates are limited to 30 or 60 FPS. Frame generation controls
additional presentation frames without raising the game's native frame rate.

The source fallback in [profile.toml](profile.toml) is a separate, minimal
startup profile. The builder writes its full selected configuration explicitly.
Builder 3.10.6 includes the existing native v3.10.5 executable, whose embedded
fallback remains unchanged. That fallback is used only when neither an existing
configuration nor a configuration supplied with the title is available.

## Configuration location and updates

The runtime records its actual configuration path in `logs/startup.log`.

- With a writable title folder, the path is normally
  `/app0/userdata/simpsons.toml`, inside the mounted title folder.
- With a read-only image, writable state uses
  `/data/homebrew/PPSA74207/userdata/simpsons.toml`.
- A new writable state folder can be seeded from the title's
  `/app0/userdata/simpsons.toml`.

Updating `eboot.bin` does not reset a saved language or other current settings.
To apply a newly selected builder configuration to an existing installation,
close the game, back up its current TOML, and copy the generated TOML to the
actual writable configuration path.

Keep `prospero_profile_version = 34`. Older configurations are migrated once,
with their original bytes backed up to `simpsons.toml.before-v3.4.bak` or a
numbered variant. Migration preserves language, input, audio, paths and other
settings outside the graphics migration. A current or newer profile is left
unchanged. Invalid TOML, inaccessible files or failed writes are reported in
the startup log rather than silently resetting the configuration.

The final TOML must be a regular file, not a symbolic link. Edit it only while
the game is closed. Saving through the quick menu may omit values equal to
the runtime defaults; fewer lines alone do not indicate lost settings.

## Quick settings

Open the English quick menu with **L1 + R1 + Options**.

- Frame generation, colour adjustments and VSync apply during play.
- **L1 + R1 + L3 + R3** toggles frame generation for the current session.
  Release all four buttons before toggling again. The quick-menu switch also
  saves the selection for later launches.
- Render resolution, output refresh rate, frame-generation target and
  multiplier require a restart. The menu labels settings that need one.

Changing VSync does not change the display's output refresh rate. Frame
generation requires its DLL to be available before startup. The shortcut
cannot recover from a complete process or GPU lockup.

A separate resolution for menus is not implemented; menus use the selected
render scale. `menu_frame_rate` also covers pause and loading states.

For installation paths, see [ShadowMountPlus setup](SHADOWMOUNT.md). For
configuration and rendering errors, see [diagnostics](FAULT_DIAGNOSTICS.md).
