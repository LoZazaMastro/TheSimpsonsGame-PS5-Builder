# Present / swap path — where a frame reaches the screen

Notes toward the README's "Phase 3, take over presentation first" step. The short version: the
present boundary is the `VdSwap` kernel call, not any single guest function, and the obvious
"call the native swap directly" shortcut is wrong for a subtle reason. Recording it so the next
person doesn't re-derive it or repeat the mistake below.

## The correct boundary is VdSwap

The guest asks for a frame flip through the `VdSwap` kernel export (confirmed in
`analysis/imports_real.txt`). Exactly one guest function calls it: `sub_824544F0`
(`simpsons/generated/default/simpsons_recomp.16.cpp`, the `__imp__VdSwap(ctx, base)` call is in
its body). That function calls `VdSwap` once per invocation and has only a handful of direct
callers, which is what you'd expect of a per-frame present driver.

Do **not** treat `sub_824544F0` as the thing to override, though. Its callee list (via
`tools/codegraph/recomp_index.py callees sub_824544F0`) includes ~18 other end-of-frame handlers
(D3D method-table entries 2, 5, 65, and more). It's an "end the frame and then present" driver,
not a bare swap. Overriding it wholesale would skip all that end-of-frame work.

## Why the naive native takeover breaks rendering

The engine already implements `VdSwap` in `tools/rexglue-sdk/src/kernel/xboxkrnl/xboxkrnl_video.cpp`
(`VdSwap_entry`, ~line 428). It does not present immediately. It writes a small PM4 packet
sequence into the primary ring buffer: a type-0 packet carrying the front-buffer texture fetch,
then a `PM4_XE_SWAP` type-3 packet with the frame's front-buffer address / width / height, then
NOP-fills the reserved 64-word slot. The command processor consumes that packet later, in ring
order, at `ExecutePacketType3_XE_SWAP` (`command_processor.cpp:1068`), and only then calls
`IssueSwap`.

That round trip is not gratuitous emulation overhead. The SWAP packet sits in the ring buffer
*interleaved with the frame's draw packets*, and the command processor runs the ring in order. So
the swap is guaranteed to happen after the frame's draws are consumed. If you "took over present"
by calling `IssueSwap` directly from `VdSwap_entry`, you'd swap before the frame's draws had been
processed and show an incomplete or previous frame. The packet is the ordering mechanism.

## Conclusion for the roadmap

- The disproven earlier guess (D3D method-table entry 72, `sub_824675C8`) is not the present
  handler. Confirmed empirically: it was never called across a full boot-to-menu run despite
  thousands of draws. Ignore it for this purpose.
- Present is correctly identified now (`VdSwap` -> ring packet -> `IssueSwap`), but there's no
  cheap native win here. The per-frame cost is a handful of dwords once per frame. "Take over
  present first" is only worth doing as a small, provable proof-of-concept, and even then it has
  to preserve draw/swap ordering (e.g. by keeping the swap on the ring, or by flushing all
  pending draws before a direct `IssueSwap`), not just shortcut the packet.
- The real performance target remains elsewhere: the UI/2D draw batching (the menu's ~3,700
  draws/frame), and CPU-side hot paths. Present/swap is not where the frame time goes.
