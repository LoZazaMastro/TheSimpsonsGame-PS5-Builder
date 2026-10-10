# The game's `.str` asset container format ("SToc")

Static analysis of real `.str` files from an installed copy — no parser exists anywhere in
this repo or publicly as far as we know, so this is a from-scratch investigation, following the
same "extract what's verifiable, mark the rest as hypothesis" methodology as
[`d3d_method_table.md`](d3d_method_table.md). **This is intentionally partial.** The header
layout below is confirmed against real files; the full per-entry record layout and the embedded
payload sub-formats are not — those are flagged explicitly, not silently assumed. No repacking
capability exists yet and isn't attempted here; writing back requires real format mastery that
reading alone doesn't produce in one pass.

`.str` is not a level-specific or cutscene-specific format: it's a generic asset-chunk container
used across the engine. The two real files decoded below hold a Trinity-engine cutscene
sequencer chunk and a Havok physics collision mesh respectively — same header shape, unrelated
payload types. Build-path strings recovered from inside these files (see below) show the
original internal codename for this platform was **`XEN`** (as in `\build\XEN\ntsc_en\...`),
useful context for anyone extending this investigation.

## Confirmed header layout (all fields verified against real files, big-endian)

| Offset | Size | Field | Value(s) observed | Notes |
|---|---|---|---|---|
| `0x00` | 4 | magic | `"SToc"` (`53 54 6F 63`) | Constant across every file checked. |
| `0x04` | 4 | format version | `0x00000007` | Constant across every file checked. |
| `0x08` | 1 | entry count | `0x00`, `0x01`, `0x02` observed | See "Entry count" below — a single byte, not the full 4-byte word. |
| `0x09` | 1 | sub-version/flag | `0x04` | Constant across every file checked, independent of entry count. |
| `0x0A` | 4 | reserved | `0x00000000` | Constant/zero in every file checked. |
| `0x10` | 4 | entry record stride | `0x00000014` (20) for cutscene/collision-style files; `0x00000080` (128) for `simpsons_chars.str` | **Per-entry record size in bytes, not a fixed constant** — the earlier assumption (from a 2-file sample) that this is always 20 was wrong; it varies by content type. This is the key correction this investigation makes. |
| `0x14` onward | stride × count | entry table | see below | Zero-filled when entry count is 0. |
| `0x800` (sector 2) | — | payload area | filenames + binary blobs | Always starts exactly at the second 2048-byte sector, confirmed across every non-empty file checked regardless of size or content type. |

**Sector alignment**: every file's header occupies exactly one 2048-byte sector (matching Xbox
360 GDF/DVD sector size), zero-padded even when the entry table doesn't fill it. A file with 0
entries is exactly 2048 bytes and nothing else — a header-only stub.

**Entry count**: offset `0x08` is a single byte, not a 4-byte word — `0x09` (`0x04`, always) and
the reserved zeros at `0x0A-0x0D` are separate fields that happen to sit adjacent to it. Verified
by cross-referencing byte value against total file size and payload content across a dozen-plus
samples: `0x00` → empty/header-only 2048-byte file; `0x01` → the common single-content-chunk
case (6144–14336 bytes observed, size varies with payload size, not with this byte); `0x02`
→ `rhymes/zone05.str` and `zone06.str` specifically (10240 bytes each).

## Confirmed by direct evidence: payload starts at the entry stride's own offset

For every non-empty file checked, the first embedded string appears at exactly
`0x800 + stride` — i.e. one entry-table record's worth of bytes into sector 2, then the actual
filename begins immediately. Two independent examples, different content types, same offset
arithmetic:

```
gamedata/bargainbin/.../igc_11_folderstream.str  (stride=0x14, 1 entry)
  0x0800-0x0813: [entry record, 20 bytes -- exact field layout not decoded]
  0x0814: "bin_igc11_shot01.xml"
  0x0840: "TRINITY_SEQ_MASTER"
  0x0857: "Xx:\build\XEN\ntsc_en\assets\igcs\source\bin\bin_igc11\shots\bin_igc11_shot01.xml.XEN"
  0x08bf: "xseqm"   0x0cd0: "seqb"   0x0d00: "trSeq_bin_igc11_shot01"
  0x0f99: "start_igc_11"   0x0fa6: "end_igc_11"

gamedata/rhymes/rhymes/zone06.str  (stride=0x14, 2 entries)
  0x0800-0x0813: [entry record #1, 20 bytes -- not decoded]
  0x0814: "collisionmodel1.hkt"
  0x0843: "dx:\build\XEN\ntsc_en\assets\environs\rhymes\rhymes\zone06\export\collision\collisionmodel1.hkt.XEN"
  0x08f8: "Havok-4.1.0-r1"
  0x0910: "__classnames__"  0x0940: "__data__"  0x0970: "__types__"
  (standard Havok hkClass/hkClassMember/... reflection tags follow)
```

This is strong, repeatable evidence that the entry table's per-record stride and the payload
start offset are directly related (`payload_start = 0x800 + N * stride` is the working
hypothesis for entry N, unverified beyond N=0 since no 2-entry file's second record boundary was
traced this pass).

## A recurring 4-byte value across unrelated files (open question)

The bytes `53 93 ac 01` appear at the same relative header position in multiple *unrelated*
cutscene/folderstream files across different levels (confirmed in `bargainbin`, `neverquest`,
`cheater` samples), while `rhymes/zone06.str` (a collision/geometry file, not a cutscene) shows
a *different* recurring value (`9d 68 70 bc`) instead. This looks like a shared dependency or
content-category hash rather than random per-file data, but which is genuinely unresolved --
flagged here rather than guessed at.

## Explicitly out of scope this pass

- **The exact per-entry record layout** (the 20 or 128 bytes between `0x14` and the payload
  area) — not decoded. Likely contains at minimum a payload offset/size and the recurring hash
  noted above, based on adjacent byte patterns, but this is not verified.
- **Payload sub-formats** — Trinity sequencer chunk internals (`xseqm`/`seqb`/`SimG` tags) and
  Havok `.hkt` internals are each their own format; out of scope here.
- **`.snu` audio files** — confirmed to use a *completely different, non-`"SToc"` header*
  (`01 00 00 00 ...` observed, no magic match) — a separate format, not investigated further
  this pass.
- **Repacking/writing** — not attempted. Needs real format mastery this pass didn't produce;
  don't build a writer against the hypotheses above without further verification.

## Why this doesn't block modding

The mod-loader foundation (see the VFS overlay support added to `HostPathDevice`) works at the
whole-file level — a mod replaces an entire `.str` (or any other file) by relative path. None of
the above needs to be solved for that to work. It only becomes necessary for a *typed* extractor/
repacker that edits inside a `.str` without replacing it wholesale, which remains future work.
