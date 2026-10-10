# The Simpsons Game PS5 port

This directory contains the native PS5 build, runtime integration and title
packaging. The port uses Vulkan/RADV for graphics, scePad for controller input
and sceAudioOut for audio. Its PS5 title ID is **PPSA74207**.

For normal installation, use the [Windows builder](builder/README.md). The
instructions below are for building the native title from source.

## Requirements

Use Linux x86-64 or WSL2 with Ubuntu 24.04. In WSL, keep the checkout in the
Linux filesystem. The bootstrap needs network access and storage for the SDK,
Mesa and compiled objects.

The build requires LLVM/Clang 18, CMake 3.25 or newer, Meson 1.4 or newer and
glslangValidator 12.2 or newer. Install the tools on Ubuntu 24.04 with:

```sh
sudo apt-get update
sudo apt-get install build-essential clang-18 llvm-18 lld-18 libclang-rt-18-dev \
  cmake ninja-build pkg-config git patch rsync wget curl unzip xz-utils \
  python3 python3-venv python3-pip bison flex zlib1g-dev glslang-tools

python3 -m venv .venv-prospero-host
source .venv-prospero-host/bin/activate
python3 -m pip install 'meson==1.12.1' mako ply PyYAML
export PS5_LLVM_CONFIG=/usr/bin/llvm-config-18
```

The build uses the public PS5 Payload SDK. An official Sony SDK is not required.

### Dependency revisions

[pins.sh](pins.sh) defines the source revisions used by the port.

| Source | Revision |
| --- | --- |
| [TheSimpsonsGameRecomp](https://github.com/YesterMester/TheSimpsonsGameRecomp) | 4c34a3ac5a8856e92b31e704fe425a81c4e65003 |
| [UnleashedRecomp-Prospero](https://github.com/Nazky/UnleashedRecomp-Prospero) | 66d49e3062c3f4bc0a1dc8f1eb3ae438acc89323 |
| [PS5_PayloadSDK](https://github.com/mihawk-99/PS5_PayloadSDK) | b5efad528e0ac1b8289f39d72a0e890ff68ff0a6 |
| [PS5_Mesa](https://github.com/mihawk-99/PS5_Mesa) | 0b2d6d1a61d9bbf89cf8beb88a696144f67c61f8 |

Keep the SDK headers and platform library at the same revision. The bootstrap
prepares the SDK and RADV dependencies and writes the environment file used
by the build. See [radv/README.md](radv/README.md) for the display integration.

## Build the title

Run from the repository root:

```sh
BUILD_JOBS=3 bash prospero/bootstrap.sh
source .deps/prospero/environment.sh
BUILD_JOBS=3 bash prospero/build.sh
```

Reduce **BUILD_JOBS** if the compiler runs out of memory. The build uses the
recompiled source already included in the repository; it does not need the game
ISO or run the recompiler again. Game data is added separately by the builder.

| Output | Default path |
| --- | --- |
| Game ELF | out/prospero/simpsons.elf |
| Compilation database | out/prospero/compile_commands.json |
| Native ELF | out/prospero-package/eboot.elf |
| PS5 title folder | dist/PPSA74207/ |

The title folder contains **eboot.bin**, **sce_module/libc.prx**, the Home
presentation files in **sce_sys**, and directories for settings, logs and
cache. [package.sh](package.sh) checks the title ID and assets before finishing
the folder.

To package an ELF that has already been built:

```sh
source .deps/prospero/environment.sh
bash prospero/package.sh out/prospero/simpsons.elf
```

Set **PROSPERO_DEPS** and **PROSPERO_BUILD_DIR** to use different dependency
and build locations. Keep PROSPERO_DEPS consistent between bootstrap and build,
and load the environment file from that location.

Home artwork and music are already prepared. See [ASSETS.md](ASSETS.md) for
formats, source credits and conversion details.

## Add your game files

The [Windows builder](builder/README.md) can extract your Xbox 360 ISO/XISO or
copy an existing game folder into the title. To copy the data manually, place
the complete extracted game in **PPSA74207/gamedata**.

**default.xex** and **movies** belong directly inside **gamedata**, alongside
the other game files. Avoid creating an extra **gamedata/gamedata** directory.
You can check the basic folder structure with:

```sh
python3 prospero/check_gamedata.py '/path/to/gamedata' --json
```

## Transfer to PS5

Transfer **PPSA74207** to **/data/homebrew/PPSA74207** and mount it with
[ShadowMountPlus](https://github.com/drakmor/shadowMountPlus). The PS5 must have
a homebrew environment that supports native titles.

Keep **userdata**, **cache** and **logs** writable by the game. The package uses
0777 for these directories; check that your transfer tool preserves the
permissions. The game sees the mounted title as **/app0**.

For an **.ffpfsc** installation, create the image from the completed folder.
When updating, rebuild the image and mount the new copy. See
[SHADOWMOUNT.md](SHADOWMOUNT.md) for the mounting workflow.

## Settings, saves and logs

| Contents | Writable title path |
| --- | --- |
| Settings | userdata/simpsons.toml |
| Optional frame-generation DLL | userdata/lossless/Lossless.dll |
| Saves and user state | userdata/ |
| Startup log | logs/startup.log |
| Runtime logs | logs/ |
| Runtime and shader cache | cache/ |

A read-only title keeps writable settings, saves, cache and logs under
**/data/homebrew/PPSA74207**. Preserve that folder's user data when updating.
The builder writes your selected settings into **userdata/simpsons.toml**.

Press **L1 + R1 + Options** for the English quick menu. Press
**L1 + R1 + L3 + R3** to toggle frame generation for the session. The menu
identifies settings that need a restart. See the
[main README](../README.md#settings-while-playing) for available controls.

## Credits

The port builds on **YesterMester's TheSimpsonsGameRecomp**, **ReXGlue**,
**Xenia**, **Nazky's UnleashedRecomp-Prospero**, **hedge-dev**, the public
**PS5 Payload SDK**, **Mesa/RADV** and the frame-generation work in
**ProsperoEden**, **Eden** and **lsfg-vk**.

Full author credits and license links are in the
[main README](../README.md#credits). Original notices remain with the source
components and in the [PS5 notices](builder/Assets/PS5-THIRD-PARTY-NOTICES.txt).
