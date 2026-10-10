# Frame trace semantics - the gotchas that cost the most time

Working notes from getting the offline native renderer (`tools/native-renderer/
trace_replay.cpp`) to parity with the emulated backend, one recorded frame at a
time. Every section below is a semantic that looked optional and wasn't. If a
future tool parses `.xtr` traces or reimplements resolves, read this first.

## Memory records come AFTER the packet that reads them

The trace writer logs a packet's guest-memory reads while executing the packet,
so in the stream the `kMemoryRead` commands land after their `kPacketStart`.
Any single in-order pass that consumes memory at packet time reads stale bytes.
Two packet classes break from this:

- `PM4_LOAD_ALU_CONSTANT` - loads shader constants from a guest memory table.
  Parsed too early, whole constant ranges come out zero (or whatever the
  parser's placeholder is; the SDK's `PacketDisassembler` used `0xDEADBEEF`,
  which shows up in float dumps as `-6.25985e+18` - a useful fingerprint).
- `PM4_IM_LOAD` - shader microcode hashing. Hashing before the upload record
  applies produced wrong hashes for 67 of 186 shaders in one trace, which then
  failed to join against the AOT set.

The fix that matches the runtime exactly: defer each packet's processing until
the next `kPacketStart` arrives. By then every record belonging to it has been
applied. (A whole-trace "apply all memory first" pre-pass is close, but reads
final state - wrong for any table rewritten mid-frame.)

## Resolve destinations are tiled, swapped, and endian-flavored

A resolve (EDRAM copy, `RB_MODECONTROL & 7 == 6` draw) writes to guest texture
memory. Three attributes of the destination, all from `RB_COPY_DEST_INFO`, are
mandatory:

- **Tiling.** The destination is a texture, laid out in the Xenos 32x32 tile
  scheme. Write through `texture_util::GetTiledOffset2D` (the same helper used
  to read tiled textures). Linear writes look plausible in a viewer that also
  assumes linear, and scramble every downstream sampler. A flat buffer (shadow
  map, mostly one value) still "matches" byte-wise under wrong tiling - do not
  trust match percentages on uniform content.
- **Channel swap.** Bit 24 (`copy_dest_swap`) swaps red and blue during the
  copy. Every 2_10_10_10 resolve in this game has it set except none observed
  so far - but honor the bit, not the observation.
- **Endian.** Bits 0:2. The intermediate buffers here use endian 2 (8in32
  swap, store big-endian dwords); the final frontbuffer uses endian 0 (store
  little-endian). Writing one convention unconditionally makes exactly one of
  them channel-rotated.

The resolve rectangle is NOT the scissor. The copy draw carries three vertices
(screen-space corner positions) in the stream referenced by vertex fetch
constant slot 0 - the rectangle is their bounding box. The scissor agrees for
fullscreen resolves and silently lies for small ones: this game's "bloom"
resolves turned out to be a 64x8 auto-exposure strip.

## Depth is 20e4 with exponent bias 15

`k_24_8_FLOAT` depth (20-bit mantissa, 4-bit exponent) decodes as
`(1 + m/2^20) * 2^(e-15)` with denormals below `e=0`. A hand-rolled
encoder/decoder pair with the wrong bias is self-consistent - the roundtrip
hides the bug until something else (the reference dump, the lighting pass that
samples the depth) reads the real values. Use `xenos::Float32To20e4` /
`Float20e4To32` from the SDK, which are exported by the runtime library.

## Post-process chains need one replay convergence run per stage

Single-frame traces never contain resolve output (resolves only fence, the
pixels are GPU-produced). The replayer regenerates them into a sidecar and
applies it on the next run - but textures upload before rendering, so each run
advances the chain exactly one stage. This game's chain is scene -> glow mask
-> ink outlines (the cel-shading edge pass) -> exposure/bloom -> composite:
five runs to converge. Two runs made the ink pass look broken for a whole
debugging session when its only problem was eating a one-generation-stale mask.

## What a single frame trace cannot contain

Guest memory uploaded in earlier frames and never re-read during the traced
frame is simply absent (the shared-memory trace cache dedupes). A census that
checks each draw's vertex streams against reconstructed memory found 99 of 964
draws reading all-zero streams in this trace - the character meshes, the sky,
the lighting overlay volumes, and the exposure measurement mesh. Those draws
no-op in any consumer of the trace. The only fix is capturing a fresh trace in
a fresh game session (`tools/bench/capture_fresh_trace.sh`).

## Viewer traps

Decoding a swapped-and-tiled 2_10_10_10 buffer with a linear little-endian
viewer produces images that are wrong in ways that look like renderer bugs
(horizontal banding = tiling; hue rotations = swap/endian). Two sessions of
"why is the road purple" ended with the road being correct and the viewer
reading blue as red. Decode with the same tiling/swap/endian the hardware
uses before believing any color comparison.
