# Changelog

## 3.10.6 - First public release

- Set **English** as the default game language. Other languages on the disc,
  including Italian, remain available.
- Added the project README with screenshots, setup instructions and credits.
- Includes the PS5 executable and Home assets from 3.10.5.

## 3.10.5 - Quick settings and live graphics controls

- Made the **L1 + R1 + Options** menu available in English, independently of
  the game language.
- Added live controls for frame generation, VSync, saturation, vibrance,
  contrast and brightness.
- Added the **L1 + R1 + L3 + R3** frame-generation reminder in the builder
  and quick menu.
- Kept shortcut changes temporary. Changing the frame-generation switch in
  the menu saves the setting for the next launch.
- Updated the preset to 120 Hz output, VSync off and Strong FSR sharpening.
  The preset kept 4K, 3× rendering, 60 FPS gameplay, 30 FPS menus, FXAA,
  Clean eyes, 16× anisotropic filtering and Italian game language.
- Enabled frame generation by default when **Lossless.dll** is selected.
  Without the file, it stays off and its controls are disabled.
- Reduced the PS5 Home music by another **1 dB**.
- Added **LoZazaMastro** to the credits and a **Donate** button below **About**,
  linked to [Ko-fi](https://ko-fi.com/lozazamastro).
- Removed **Mute audio** from the builder.

## 3.10.4 - Frame generation and controller toggle

- Fixed frame-generation synchronization and improved fallback handling.
- Added **L1 + R1 + L3 + R3** to toggle frame generation for the current session.
- Updated the 4K/3× profile with 60 FPS gameplay and 30 FPS menus.
- Corrected anisotropic filtering configuration: **5** in the TOML means **16×**.

## 3.10.2 - Disc and folder recognition

- Fixed the Xbox 360 title check to use **45410809**.
- Applied the same recognition to ISO/XISO files and extracted game folders.
- Removed incorrect region and Media ID restrictions.
- Improved source identification in the builder logs.

## 3.10.1 - WPF builder

- Added the dark WPF interface with Segoe UI, yellow controls and Acrylic.
- Added window resizing, maximize and restore.
- Packaged the builder with its .NET runtime.
- Added detailed logs for extraction, copying, cancellation and file checks.
