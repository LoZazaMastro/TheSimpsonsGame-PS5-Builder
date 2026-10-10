# PS5 title artwork and Home music

The PS5 title is **The Simpsons Game**, with title ID `PPSA74207`.
Artwork and the soundtrack source were supplied by **LoZazaMastro**.
Game artwork, music and trademarks remain the property of their respective
rights holders; the source-code license does not grant rights to those assets.

## Prepared files

| Source | PS5 destination | Format |
|---|---|---|
| `Cover.png` | `sce_sys/icon0.png` | 512 x 512 PNG, RGBA |
| `Background.png` | `sce_sys/pic0.dds` | 3840 x 2160, BC7_UNORM, DDS DX10 2D, one mip level |
| `IMG Launch.png` | `sce_sys/pic1.dds` | 3840 x 2160, BC7_UNORM, DDS DX10 2D, one mip level |
| `The Simpsons Game Soundtrack - Menu Music.mp3` | `sce_sys/snd0.at9` | Stereo ATRAC9, 48 kHz, 144 kbps, full-track loop |

Original sources are preserved in `assets/source/`. The cover is resized
from 1024 x 1024 using Lanczos filtering. The two 2048 x 1152 backgrounds are
scaled uniformly to 3840 x 2160, with no cropping. BC7 is lossy compression.
Prepared PNG previews accompany the backgrounds for inspection.

## Home music level

The music uses a gain of **0.4456254690668728** relative to the original
source: the earlier 0.5 gain multiplied by `10^(-1/20)`. This is an additional
**1 dB reduction** applied during preparation of the native v3.10.5 assets.
Builder 3.10.6 includes that same audio file. In-game audio is unaffected.

FFmpeg decodes the original MP3 to stereo PCM. The gain is applied once,
before conversion to PCM16 and ATRAC9 encoding. The converter patch prevents
additional limiter or fade processing. ATRAC9 encoding is lossy, so decoded
samples need not equal the original samples multiplied by the input gain.

The useful track length is 100.217333 seconds, or 4,810,432 samples per channel.
The file is 1,804,200 bytes. The loop spans the full useful recording, with
codec-delay compensation, and retains silence or fades present in the source.

[The asset manifest](assets/manifest.json) records output hashes and format
checks. [The gain report](assets/audio-gain-verification.json) records the PCM
level verification. The native v3.10.5 audio report also records complete
LibAtrac9 decoding and an approximately -1 dB RMS change from the previous
encoded asset.

## Conversion tools and attribution

The conversion recipe uses `png_to_bc7_dds` and `wav_to_at9` from
[Nazky/UnleashedRecomp-Prospero](https://github.com/Nazky/UnleashedRecomp-Prospero/tree/a63abf2db139b04da90989024da5bc804d11323a),
revision `a63abf2db139b04da90989024da5bc804d11323a`.
The audio validation uses [LibAtrac9 by Alex Barney](https://github.com/Thealexbarney/LibAtrac9).
The recipe preserves the reference sources and builds patched tools in a
temporary directory. Original copyright notices and license files remain with
the corresponding sources; see also the
[project credits](../README.md#credits) and
[third-party notices](builder/Assets/PS5-THIRD-PARTY-NOTICES.txt).

The prepared assets are already included in `sce_sys/`; rebuilding them is
not required to compile or use the builder.

## Regenerate assets

From the repository root, with Python 3.9 or later, a native C/C++ compiler,
`patch` and FFmpeg installed:

```sh
python3 prospero/prepare_assets.py --sonic-root /path/to/UnleashedRecomp-Prospero
```

To regenerate only the Home music:

```sh
python3 prospero/prepare_assets.py --sonic-root /path/to/UnleashedRecomp-Prospero --audio-only
```

The recipe checks pinned input and converter hashes, output headers, dimensions,
PCM gain and loop metadata. The complete operation updates the prepared
backgrounds and music; `--audio-only` updates the music. Title icon and metadata
preparation are handled separately. Format and decoder checks do not replace
an on-console visual or listening check.
