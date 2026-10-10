# PS5 output selection: preserve the Sonic driver

The working driver remains Mesa revision
`0b2d6d1a61d9bbf89cf8beb88a696144f67c61f8`, paired with PS5 SDK
`b5efad528e0ac1b8289f39d72a0e890ff68ff0a6`. That revision already implements
VideoOut at 59.94 Hz and 119.88 Hz. Its source is
[PS5_Mesa](https://github.com/mihawk-99/PS5_Mesa/blob/0b2d6d1a61d9bbf89cf8beb88a696144f67c61f8/src/vulkan/wsi/wsi_common_videoout.c).

`wsi_common_videoout.base.c` is that exact source. Its SHA-256 is
`32c437481a24dafd31b1b587930a256293a201dce70cd6c1947f0ef301d4553b`.
`wsi_common_videoout.c` contains the small adaptation for this title. The MIT
copyright notice is retained. The application/request separation follows the
approach in the supplied ProsperoEden revision
`8ed1b84ab56e435792d6ad9469b44f3d025e77ba`.

## Changes

- Package metadata declares 120 Hz capability with `attribute3 = 524352`.
- The driver requests it only when the application has set
  `SIMPSONS_VIDEOOUT_120HZ=1` before Vulkan initialization. A capable package
  therefore still starts at the working 60 Hz default.
- Unsupported output (`sceVideoOutIsOutputSupported <= 0`) cannot be mistaken
  for a successful configuration.
- The existing vblank measurement rejects an unmeasurable clock as well as an
  output that remains at 60 Hz. Failure to restore a known mode fails display
  initialization instead of publishing an unverified clock.
- A small C accessor publishes the physical output clock to the application,
  separate from both the selected Vulkan mode handle and native game FPS.

The application selects 60/120 Hz independently of the guest simulation. Only
30 and 60 native FPS are exposed. Frame generation uses the effective display
clock; requesting 120 Hz does not by itself mean the display accepted 120 Hz.

## Rebuild

`prospero/bootstrap.sh` builds the original pinned Sonic archive first, verifies
its receipt, then runs `prospero/tools/build_radv_display.py`. The latter
configures Mesa from a checksum-pinned source archive and compiles only
`wsi_common_videoout.c.o`. It copies the original driver archive and replaces
that one member. It compares every other member's name, size and SHA-256 before
publishing the new archive. The working Sonic archive is never overwritten.

The current baseline contains 703 objects; **702 remain byte-identical**.
The GPU compiler, memory management and rendering code therefore keep their
existing compiled objects. The new archive's receipt records the source,
compiler command, SDK, replacement object and archive hashes. The title's
linker refuses a stale receipt or an archive from another display source.

For an existing verified Sonic dependency tree:

```bash
python3 prospero/tools/build_radv_display.py \
  --sonic-reference /absolute/path/to/Sonic-Reference \
  --sdk /absolute/path/to/ps5-sdk \
  --output /absolute/path/to/libvulkan_radeon.ps5.a
export RADV_ARCHIVE=/absolute/path/to/libvulkan_radeon.ps5.a
```

Build-time and mocked VideoOut/Vulkan tests do not exercise the PS5's HDMI
connection, its firmware or a physical display. 120 Hz remains an experimental
choice in the title until evaluated on the user's console.
