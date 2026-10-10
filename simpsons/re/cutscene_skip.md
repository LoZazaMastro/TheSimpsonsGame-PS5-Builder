# Cutscene-skip patch — investigation notes

**Status: partial / best-effort (as scoped).** Goal was a patch letting the player skip a story
cutscene (`.vp6` video) even on first viewing — a global skip, no per-save "seen" tracking. This
file records what static analysis established, the wall it hit, and the exact next step, so the
work can be resumed without re-deriving it. No patch shipped this pass: the guest function that
would need hooking is reachable only past a function-pointer-dispatch wall that static analysis
cannot cross, and pinning it down requires a step deliberately not taken this session (see
"Next step").

## What cutscenes are, confirmed

Story cutscenes are VP6-encoded videos at `gamedata/movies/en/*.vp6`. They are **decoded by
recompiled guest code**, not by the engine runtime — the SDK's only FFmpeg use is XMA *audio*
(`tools/rexglue-sdk/src/audio/xma_context.cpp`); there is no video demux/playback anywhere in
`tools/rexglue-sdk`. (Distinct from the four boot-logo `.vp6`s, which the launcher skips by
renaming the files on disk — irrelevant here.)

## The declaration layer (plaintext, fully readable)

`gamedata/simpsons_gameflow.lua` + `simpsons_gameflow_helpers.lua` declare every cutscene:

```lua
-- helpers.lua:76
function NewMovie( movieName, stream, movieType, localizedKey, movieVolume )
    local object = MoviePkg:New( movieName, stream, movieType )   -- tolua-bound native class
    ...
-- gameflow.lua:69  (stream name == the .vp6 basename)
mode:AddEntry( NewMovie( "Homer's Dream", "loc_igc01", "INTRO", "FE_FMV_01", 0.85 ) )
```

`MoviePkg` (and `New`/`SetMovieVolume`/`SetLocalizedKey`) is a **tolua-bound native class** — its
methods are C strings registered in the guest binary at runtime. `stream` (`"loc_igc01"`) is the
`.vp6` basename. So the native "play this movie" path is reached when a `MoviePkg` is triggered
by the gameflow — but that trigger is native, unnamed guest code.

## VP6 decode layer — located

Eight functions in `simpsons/generated/default/simpsons_recomp.10.cpp` contain the VP6
motion-compensation instructions (`vpkuwus`/`vpkuhus`; see the codegen note at
`tools/rexglue-sdk/src/codegen/builders/vector.cpp:1095-1140` about the green-chroma bug in this
exact filter):

```
sub_8237C878  sub_82382848  sub_82382BC0  sub_82385C38
sub_82385CB8  sub_82385D78  sub_82385E30  sub_82385EF8
```

These are the per-block decode/motion-comp leaves — the bottom of the movie module (roughly the
`0x8237xxxx`–`0x8238xxxx` address range).

## The wall: function-pointer dispatch

Walking `recomp_index.py callers` **up** from those 8 leaves does not converge on a single
"play movie" driver — each ancestor covers only 1 of the 8 leaves. That non-convergence is the
diagnosis, not a dead end: the movie module dispatches internally through **function pointers**,
which `recomp_index.py`'s literal-`sub_XXXX()`-call graph structurally cannot see. Confirmed by
`analysis/switch_tables.toml`: there are real jump tables inside the module at `0x8238E8E0` and
`0x8238E94C` (address lists of `0x8238Exxx` targets) — indirect dispatch, exactly what blocks the
static walk.

Separately, the generated C++ contains **no** `.vp6`/`movies` string literals — guest string
constants live in the xex data section and are loaded by address, so the path-construction /
file-open site can't be found by grepping the generated code either.

## Next step (deliberately not taken this session)

Two ways to pin the play-driver function; both were held back this pass (the first was
interrupted; the second pops a game window on the user's screen, which caused disruption earlier):

1. **Static — xex data-section scan.** Find the address of the `movies\en\...%s...vp6` format
   string in `analysis/default.xex`'s rodata, then grep the generated code for a `sub_` that
   loads that address into a register before an `NtCreateFile`/`NtOpenFile` call. That caller is
   at/near the movie-open path; walk its callers to the play driver. No game execution needed.
2. **Dynamic — GDB breakpoint.** Because this is a *static* recompile, guest `bl` became real
   native calls, so the host call stack mirrors the guest one. A conditional breakpoint on
   `NtCreateFile_entry` (`tools/rexglue-sdk/src/kernel/xboxkrnl/xboxkrnl_io.cpp:106`) with
   `target_path` containing `.vp6`, then `bt`, names the calling `sub_XXXX` directly. The main-
   menu **attract loop** already plays a `.vp6` unattended (the existing `bench.sh` boot-to-menu
   harness reaches it), so this needs no manual gameplay — just a live boot.

## The skip mechanism, once the driver is found

Same weak-alias override idiom as `simpsons/src/draw_telemetry.cpp`, behind a new opt-in CMake
flag mirroring `SIMPSONS_DRAW_TELEMETRY`. Poll `XamInputGetState_entry`
(`tools/rexglue-sdk/src/kernel/xam/xam_input.cpp:95`) each decode iteration via `rex::CallFrame`
(so the side call doesn't clobber the wrapped body's guest registers) and short-circuit the
decode loop on a skip press. Fallback if a clean early-return corrupts audio-sync/subtitle/
cleanup state: "fast-forward" the loop (keep decoding, stop presenting/waiting) instead of a hard
abort — which is needed can't be known until the driver's structure is seen.

**Honest gap:** validating a skip against a real *story* cutscene (vs. the attract loop) needs
in-level content the boot-to-menu harness can't reach — one manual checkpoint. This is the single
place in the whole effort where "no manual gameplay" can't be fully honored.
