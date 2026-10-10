# PS5 runtime diagnostics

Diagnostics are included in the title and start when the game launches.
No additional ELF payload or TOML switch is needed for normal collection.

## Files to collect

Read the files from the same session together:

| File | Contents |
|---|---|
| `startup.log` | Executable build marker, early startup, actual paths, loaded settings, initialization and exit status |
| `fault.log` | Runtime checkpoints, periodic metrics, operations and captured terminal faults |
| `simpsons_*.log` | Game loading, Vulkan, shaders, pipelines, audio, input and runtime errors |

The log directory is normally `/data/homebrew/PPSA74207/logs`, visible to a
writable mounted title as `/app0/logs`. A read-only image also uses the writable
state location. Follow the path recorded in `startup.log`; an explicit
`log_file` setting can change the ordinary runtime log destination.

Logs can contain several appended sessions. Use PID, monotonic timestamps,
sequence numbers and the CRT build marker to identify the relevant run.
Builder 3.10.6 includes the native executable identified by:

```text
CRT: build v3.10.5-ui-menu; updated default profile, English quick settings and framegen hints
```

Component messages can retain older version numbers. The CRT marker and the
matching executable hash identify the native build.

## Read a slowdown or stalled session

Periodic summaries distinguish `guest_swap`, the game's swap cadence, from
`present_ok`, successful Vulkan presentation calls. These measure software
events; they do not prove when a display showed each image.

| Field | Meaning |
|---|---|
| `fps_avg` | Completed intervals divided by their total duration |
| `fps_min`, `fps_max` | Rates derived from the longest and shortest individual intervals |
| `fps_1pct_low` | Rate derived from the mean duration of the slowest 1% of retained intervals |
| `frame_ms_p50/p95/p99` | Percentiles of retained frame intervals |
| `since_last_ms` | Time since the last accepted event |
| `distribution_dropped` | Intervals omitted from the bounded percentile sample |

An unfinished stall has no completed frame interval yet. Inspect
`since_last_ms` and `HEARTBEAT` records alongside averages. A window without
valid intervals reports `timing=unavailable`. `queue_present_call` measures
time inside the host presentation call, not GPU execution time.

`MEMORY` records describe sampled allocator and virtual-memory counters.
They are approximate and must not be summed as physical RAM use. `OPS`
records cover audio, controller, filesystem and pacing activity; suppression
counters identify events omitted by logging limits. Lines beginning with
`framegen:` explain frame-generation state and failures.

## Read a crash

A captured terminal fault can include its signal, code, fault address,
registers, native instruction pointer, image anchors and a bounded trace.
`abort caller` identifies the caller that requested abort.
`heap failure:` identifies the integrity check that detected a problem;
neither necessarily identifies the earlier operation that caused it.

`fault.log` can contain normal runtime telemetry without a crash. Conversely,
a missing report does not prove that no crash occurred: an early failure,
stack exhaustion, forced termination, filesystem failure or complete system
stall can prevent logging. Fault reporting preserves fatal behavior and does
not make invalid memory accesses safe to continue.

## Decode metrics and native addresses

Decode telemetry without a debug executable:

```sh
python3 prospero/tools/symbolize_trace.py fault.log --telemetry-only --output telemetry.json
```

For native addresses, use the debug ELF from the exact executable build:

```sh
python3 prospero/tools/symbolize_trace.py fault.log --elf /path/to/simpsons.elf --expected-sha256 SHA256_OF_MATCHING_DEBUG_ELF --line-map /path/to/native-helper-lines.json --output decoded.json
```

The optional line map must match that ELF and supplies verified source ranges
for native helpers. Raw stack words are candidate addresses, not an unwound
backtrace. Addresses belonging to a system library need that library's matching
symbols. Truncated traces are rejected by default; use `--allow-incomplete`
only to retain an explicitly marked incomplete result.

When reporting a problem, include the builder version, native build marker,
settings, actions leading to the issue and the relevant logs. Review logs
before posting them publicly because they can contain file paths. Game data
and the ISO are not needed to inspect the diagnostic records.
