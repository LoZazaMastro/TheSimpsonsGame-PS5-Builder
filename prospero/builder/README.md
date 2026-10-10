# The Simpsons Game PS5 Builder

A Windows 11 x64 application that creates **PPSA74207** from your own Xbox 360
ISO/XISO or extracted game files.

Maintained by **LoZazaMastro**.
[Download](https://github.com/LoZazaMastro/TheSimpsonsGame-PS5-Builder/releases/latest)
· [Main README](../../README.md)
· [Donate](https://ko-fi.com/lozazamastro)

The English WPF interface can be resized, minimized and maximized. It uses
Segoe UI, yellow controls and Windows Acrylic, with a dark background when
transparency is unavailable. The EXE includes its .NET runtime and extraction
code, so players do not need to install an SDK or separate extractor.

## Create a PS5 folder

1. **Game files:** select your Xbox 360 ISO/XISO or the extracted folder
   containing **default.xex**, **movies** and the other game files.
2. Choose a parent folder for the output. The builder will create **PPSA74207**
   inside it. If that folder already exists, choose another location.
3. **Settings:** choose the graphics and frame-rate options. Experimental
   settings are marked. Native game and menu rates are limited to 30 or 60 FPS.
4. **Language:** choose a language included on the disc. **English** is selected
   by default in version 3.10.6.
5. **Build:** review the summary and start. The completed folder appears after
   extraction or copying and file verification finish.
6. Transfer the folder to **/data/homebrew/PPSA74207** on the PS5 and mount it
   with ShadowMountPlus.

The input files remain unchanged. Cancelling a build removes its unfinished
output. The builder does not replace an existing PPSA74207 folder.

Regional discs use the same **Title ID 45410809**. The builder applies this
check to both ISO files and extracted folders, without a Media ID whitelist.
Languages depend on the game files present on your disc.

## Default profile

The builder starts with 4K game video mode, 3× rendering, matching shadows,
120 Hz output, VSync off, FSR 1 with Strong sharpening, FXAA, Clean eyes and
16× anisotropic filtering. Gameplay is set to 60 FPS and menus to 30 FPS.
The game language is English and subtitles are off.

Ink outlines use Original, black and strength 0.6. Colour adjustments are
neutral, tessellation is off and the audio queue is 8 frames. Frame generation
uses a 120 FPS target and a 2× multiplier for Auto. Selecting Lossless.dll turns
frame generation on; without the file, it stays off.

The [settings catalogue](Assets/capabilities.json) contains the available
choices. **16× anisotropic filtering is saved as anisotropic_override = 5**,
which is the renderer's code for that setting.

Version 3.10.6 includes the native PS5 executable and title assets from 3.10.5.

## Frame generation

Select your own **Lossless.dll** in Settings to unlock the controls. The builder
copies it to **PPSA74207/userdata/lossless/Lossless.dll**. The file must come
from your copy of [Lossless Scaling](https://store.steampowered.com/app/993090/Lossless_Scaling/);
it is not included in the builder.

Choose a fixed **60 or 120 FPS** target, or **Auto** with a **2×, 3× or 4×**
multiplier. Frame generation works with either 30 or 60 native FPS and does not
change the simulation rate. It is experimental and may add input delay or
visual artifacts. A compatible display and connection are needed for 120 Hz.

## In-game settings

Press **L1 + R1 + Options** to open the English quick menu. Frame generation,
VSync and colour adjustments can be changed during play. Settings that need a
restart are marked, including render resolution, output refresh rate and the
frame-generation target and multiplier.

Press **L1 + R1 + L3 + R3** to toggle frame generation for the current session.
Release all four buttons before pressing again. The shortcut keeps the saved
startup setting; changing the menu switch also saves the new setting.
Frame generation pauses while the quick menu is open.

Menus use the selected render scale. A separate original resolution for menus
is not supported.

## Updates and logs

Keep **gamedata**, **userdata** and your saves when updating the title.
To apply settings from a new build to an existing installation, copy the new
**userdata/simpsons.toml** into the title's writable **userdata** folder,
keeping its other files. If you use an **.ffpfsc** image, rebuild it from the
updated folder and mount the new image.

Use **Open logs**, **Copy diagnostics** or **Export full log** in the builder.
The default log folder is
**%LOCALAPPDATA%\TheSimpsonsGame-PS5-Builder\logs**. Each session records the
version, settings, source inspection, build progress and file checks.

## Project layout

| Path | Purpose |
| --- | --- |
| Builder.Core | Disc reading, source checks, settings, copying and file verification |
| Builder.Wpf | WPF interface, window controls and diagnostics |
| Builder.Tests | ISO, extraction, settings and build tests |
| Builder.PatchTests | Tests for optional game patches |
| Builder.UiTests | Windows UI and layout tests |
| Assets/capabilities.json | PS5 settings and defaults |
| Assets/payload.json | Title file list and hashes |
| Assets/payload.zip | Embedded PS5 title files |
| prepare_payload.py | Packages the native title for the builder |
| THIRD-PARTY-NOTICES.txt | Builder credits and licenses |
| Assets/PS5-THIRD-PARTY-NOTICES.txt | PS5 runtime and dependency notices |

## Build the standalone Windows executable

Use **.NET SDK 10.0.x**. First build the native PS5 title using the
[PS5 guide](../README.md), then run these commands from the repository root:

```sh
python3 prospero/builder/prepare_payload.py \
  --title-dir dist/PPSA74207 --version 3.10

dotnet publish prospero/builder/Builder.Wpf/Builder.Wpf.csproj \
  -c Release -r win-x64 --self-contained true \
  -o prospero/builder/publish
```

The output is **prospero/builder/publish/TheSimpsonsGame-PS5-Builder.exe**.
The build embeds the prepared title, file hashes and software notices.

## Development checks

The Core tests can compare extraction against the upstream C extractor.
For example, on Linux:

```sh
cc -O2 -D__LINUX__ tools/extract-xiso/extract-xiso.c \
  -o /tmp/simpsons-extract-xiso-reference

dotnet run --project prospero/builder/Builder.Tests/Builder.Tests.csproj \
  -c Release -- \
  prospero/builder/Assets/capabilities.json \
  /tmp/simpsons-builder-tests \
  /tmp/simpsons-builder-tests.json \
  /tmp/simpsons-extract-xiso-reference \
  prospero/builder/Assets
```

The tests cover disc structures, malformed input, file checks, output
collisions, cancellation, settings and optional patches. See
[Builder.UiTests](Builder.UiTests/README.md) for Windows UI checks.

## Credits and licenses

See the [main credits](../../README.md#credits),
[builder notices](THIRD-PARTY-NOTICES.txt) and
[PS5 notices](Assets/PS5-THIRD-PARTY-NOTICES.txt). Original license texts and
source notices are included with their components.

The ISO reader uses **extract-xiso** as a format and test reference. Its
original acknowledgement is retained:

> This product includes software developed by in &lt;in@fishtank.com&gt;.

The **Donate** button below **About** opens
[ko-fi.com/lozazamastro](https://ko-fi.com/lozazamastro).
