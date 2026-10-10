# Install with ShadowMountPlus

**Title:** The Simpsons Game  
**Title ID:** `PPSA74207`

The builder creates a complete `PPSA74207` folder containing the native PS5
program, title assets, selected configuration and copied game data.
[ShadowMountPlus](https://github.com/drakmor/shadowMountPlus) is an external
mounting tool maintained by drakmor; it is not included in this project.

## Folder installation

1. Build the title from an Xbox 360 ISO/XISO or an existing `gamedata` folder.
2. Copy the complete output folder to `/data/homebrew/PPSA74207`, or to a
   supported location scanned by your ShadowMountPlus installation.
3. Refresh or rescan the library, then register and mount the title using
   ShadowMountPlus.
4. Launch **The Simpsons Game** from the PS5 Home screen.

The folder must retain its structure, including `eboot.bin`, `sce_module`,
`sce_sys`, `gamedata` and `userdata`. Keep only one active source for
`PPSA74207` in the scanned locations. Preserve the generated title metadata,
including `downloadDataSize = 256`; it is not a field to replace with the
sum of the files' byte sizes.

## Image installation

An `.ffpfsc` image must be created from the complete built folder, including
its game data. The builder itself produces the folder, not an FF-PFSC image.
Use a compatible image creation tool and place the resulting image in a
location supported by your ShadowMountPlus installation.

The image is mounted at `/app0`. Since the image is read-only, runtime logs,
configuration, caches and saves use the writable state folder described below.
The initial configuration inside the image can seed that writable state.

## Update the source that is actually mounted

Close the game before replacing files. Check the title's source in
ShadowMountPlus details, especially if both a folder and an older image exist.

- **Folder:** replace the native program and title assets supplied by the
  update. Preserve `gamedata`, `userdata`, `cache` and `logs` unless the release
  instructions explicitly identify an additional change.
- **Image:** unmount the title, update the complete source folder, rebuild the
  image and replace the image that ShadowMountPlus uses.

Replacing a loose `eboot.bin` does not update the copy inside an image.
Loader-managed `mount.lnk` and `mount_img.lnk` can help identify the active
source; do not edit them manually. Check the executable hash or the build
marker in `startup.log` to identify the program actually running.

## Game data and writable state

| Content | Writable folder | Read-only image |
|---|---|---|
| Game data | `/app0/gamedata` | `/app0/gamedata` inside the image |
| Configuration | `/app0/userdata/simpsons.toml` | `/data/homebrew/PPSA74207/userdata/simpsons.toml` |
| Saves | `/app0/userdata` | `/data/homebrew/PPSA74207/userdata` |
| Cache | `/app0/cache` | `/data/homebrew/PPSA74207/cache` |
| Logs | `/app0/logs` | `/data/homebrew/PPSA74207/logs` |

For a folder installed at the recommended location, `/app0` refers to
`/data/homebrew/PPSA74207`. `startup.log` records the actual state and
configuration paths. The title does not use `/data/TheSimpsonsGame`.

The state folder must be writable. The runtime selects the separate state
location for a read-only mount; it does not treat every permission or I/O
error as a reason to silently change locations.

## If launch fails

Preserve `startup.log`, `fault.log` and the matching `simpsons_*.log` files.
The first application marker is `CRT: _start entered`. If no application
log exists, inspect ShadowMountPlus's own log and its kernel-log viewer for
the same launch attempt. A successful mount alone does not prove that the
application process started, and an unmount after a failed launch may be
cleanup rather than the cause.

Use the [diagnostics guide](FAULT_DIAGNOSTICS.md) to interpret application
logs. Configuration location and preservation rules are described in
[PS5 configuration](PROFILE.md).
