#!/usr/bin/env python3
"""Turn a diagnostic capture into the three Stage 1 answers.

Run the game once with the diagnostic settings enabled (launcher: Support tab),
then point this at the resulting files:

    tools/codegraph/analyze_capture.py --log <simpsons.log> --perf <perf.csv> \
                                       --shaders <shader_inventory.csv>

Q1  Which guest function issues the draw storm, and who calls it.
Q2  How many shaders and how many specialization variants (is AOT tractable).
Q3  Where in-level frame time goes, menu vs gameplay.

Every input is optional; whatever is supplied gets analysed.
"""

import argparse
import csv
import re
import statistics
import sys
from collections import defaultdict
from pathlib import Path

# [draw-telemetry] frame=1234 total=3702 82463E60=3700 callers: 8245A1B0x3700 ...
TELEMETRY_RE = re.compile(
    r"\[draw-telemetry\] frame=(\d+) total=(\d+) (INIT-TIME )?([0-9A-F]{8})=(\d+) callers:(.*)"
)
CALLER_RE = re.compile(r"([0-9A-F]{8})x(\d+)")


def load_func_table(repo_root):
    """Map guest addresses to function names via the generated mapping table."""
    table = {}
    init = repo_root / "simpsons" / "generated" / "default" / "simpsons_init.cpp"
    if not init.is_file():
        return table
    # Entries look like: { 0x82463E60, sub_82463E60 },
    pattern = re.compile(r"\{\s*0x([0-9A-Fa-f]{8})\s*,\s*(\w+)\s*\}")
    with init.open(errors="ignore") as handle:
        for line in handle:
            m = pattern.search(line)
            if m:
                table[int(m.group(1), 16)] = m.group(2)
    return table


def resolve(addr, func_table):
    """Name the function containing addr (the mapping table is start-addresses)."""
    if not func_table:
        return ""
    if addr in func_table:
        return func_table[addr]
    best = None
    for start in func_table:
        if start <= addr and (best is None or start > best):
            best = start
    # A return address far past any known start is not meaningfully attributed.
    if best is None or addr - best > 0x4000:
        return ""
    return f"{func_table[best]}+0x{addr - best:X}"


def analyse_draws(log_path, func_table):
    print("=" * 78)
    print("Q1  DRAW STORM ATTRIBUTION")
    print("=" * 78)

    per_frame = defaultdict(int)
    hook_calls = defaultdict(int)
    hook_frames = defaultdict(set)
    caller_calls = defaultdict(int)
    init_time_hits = defaultdict(int)

    with Path(log_path).open(errors="ignore") as handle:
        for line in handle:
            m = TELEMETRY_RE.search(line)
            if not m:
                continue
            frame, total, init_time, hook, calls, callers = m.groups()
            frame, total, calls = int(frame), int(total), int(calls)
            per_frame[frame] = total
            hook_calls[hook] += calls
            hook_frames[hook].add(frame)
            if init_time:
                init_time_hits[hook] += calls
            for caddr, ccount in CALLER_RE.findall(callers):
                caller_calls[(hook, caddr)] += int(ccount)

    if not per_frame:
        print("  No telemetry found. Was `draw_telemetry` enabled for the capture?")
        print("  (Check the log for lines beginning '[draw-telemetry]'.)")
        return

    totals = sorted(per_frame.values())
    print(f"  Frames with draws : {len(per_frame)}")
    print(f"  Draws per frame   : median {statistics.median(totals):.0f}, "
          f"peak {max(totals)}, min {min(totals)}")
    print()
    print("  By emitting function (the draw routine itself):")
    for hook, calls in sorted(hook_calls.items(), key=lambda kv: -kv[1]):
        frames = len(hook_frames[hook])
        share = 100.0 * calls / max(1, sum(hook_calls.values()))
        name = resolve(int(hook, 16), func_table) or f"sub_{hook}"
        flag = "  <-- INIT-TIME, should be ~0 in steady state" if hook in init_time_hits else ""
        print(f"    {name:<28} {calls:>10} calls  {calls/frames:>8.0f}/frame  "
              f"{share:>5.1f}%{flag}")

    print()
    print("  By CALLER (who asked for the draws -- this is the batching target):")
    print("  Most emitters are reached through the D3D vtable, so the caller is")
    print("  the function worth hooking, not the emitter.")
    ranked = sorted(caller_calls.items(), key=lambda kv: -kv[1])[:15]
    for (hook, caddr), calls in ranked:
        name = resolve(int(caddr, 16), func_table) or f"0x{caddr}"
        emitter = resolve(int(hook, 16), func_table) or f"sub_{hook}"
        print(f"    {name:<34} {calls:>10} calls   via {emitter}")
    if not ranked:
        print("    (none recorded)")


def analyse_shaders(path):
    print()
    print("=" * 78)
    print("Q2  SHADER / SPECIALIZATION INVENTORY")
    print("=" * 78)

    vs_variants = defaultdict(set)
    ps_variants = defaultdict(set)
    pipelines = 0
    with Path(path).open(newline="") as handle:
        for row in csv.DictReader(handle):
            pipelines += 1
            vs_variants[row["vs_hash"]].add(row["vs_modification"])
            if row["ps_hash"] != "0" * 16:
                ps_variants[row["ps_hash"]].add(row["ps_modification"])

    def report(label, variants):
        if not variants:
            print(f"  {label}: none recorded")
            return
        counts = [len(v) for v in variants.values()]
        total = sum(counts)
        print(f"  {label}: {len(variants)} unique shaders -> {total} variants "
              f"(mean {total/len(variants):.1f}, worst {max(counts)})")
        worst = sorted(variants.items(), key=lambda kv: -len(kv[1]))[:5]
        for h, v in worst:
            print(f"      {h}  {len(v)} variants")

    print(f"  Pipeline combinations instantiated: {pipelines}")
    report("Vertex", vs_variants)
    report("Pixel ", ps_variants)

    total_variants = sum(len(v) for v in vs_variants.values()) + \
        sum(len(v) for v in ps_variants.values())
    print()
    if total_variants == 0:
        print("  No data -- was `shader_inventory_csv` set for the capture?")
    elif total_variants < 1500:
        print(f"  VERDICT: {total_variants} variants is tractable to convert ahead of time.")
    else:
        print(f"  VERDICT: {total_variants} variants is a lot. Ahead-of-time conversion would")
        print("  need a runtime fallback for unseen specializations regardless, so plan for")
        print("  the translator staying in the build.")
    print("  NOTE: this reflects only what THIS session touched. Broader play = more variants.")


def analyse_perf(path):
    print()
    print("=" * 78)
    print("Q3  FRAME TIME")
    print("=" * 78)

    rows = []
    with Path(path).open(newline="") as handle:
        for row in csv.DictReader(handle):
            try:
                rows.append({k: int(v) for k, v in row.items() if v != ""})
            except ValueError:
                continue
    if not rows:
        print("  No rows. Was `perf_log_csv` set, and did the game exit cleanly?")
        return

    def col(name):
        return [r[name] for r in rows if name in r and r[name] > 0]

    ft = col("frame_time_us")
    if ft:
        ft_sorted = sorted(ft)
        p95 = ft_sorted[int(len(ft_sorted) * 0.95)]
        print(f"  Frames: {len(rows)}")
        print(f"  Frame time: median {statistics.median(ft)/1000:.2f} ms "
              f"({1e6/statistics.median(ft):.1f} fps), p95 {p95/1000:.2f} ms "
              f"({1e6/p95:.1f} fps)")

    for name, label in (
        ("draw_calls", "Draw calls/frame"),
        ("vertices_processed", "Vertices/frame"),
        ("command_buffer_stalls", "GPU sync stalls/frame"),
        ("functions_dispatched", "Guest calls/frame (CPU work proxy)"),
        ("texture_cache_misses", "Texture cache misses/frame"),
        ("pipeline_cache_misses", "Pipeline cache misses/frame"),
    ):
        vals = col(name)
        if vals:
            print(f"  {label:<38} median {statistics.median(vals):>10.0f}  peak {max(vals):>10}")

    # Heaviest frames are where the answer lives.
    if ft and len(rows) > 20:
        print()
        print("  Slowest 5 frames (what was happening when it hurt):")
        worst = sorted((r for r in rows if r.get("frame_time_us", 0) > 0),
                       key=lambda r: -r["frame_time_us"])[:5]
        for r in worst:
            print(f"    guest_frame={r.get('guest_frame', '?'):<8} "
                  f"{r['frame_time_us']/1000:>7.1f} ms  "
                  f"draws={r.get('draw_calls', 0):<6} "
                  f"stalls={r.get('command_buffer_stalls', 0):<4} "
                  f"pipe_miss={r.get('pipeline_cache_misses', 0):<4} "
                  f"tex_miss={r.get('texture_cache_misses', 0)}")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--log", help="engine log containing [draw-telemetry] lines")
    ap.add_argument("--perf", help="perf_log_csv output")
    ap.add_argument("--shaders", help="shader_inventory_csv output")
    ap.add_argument("--repo", default=str(Path(__file__).resolve().parents[2]),
                    help="repo root, for resolving guest addresses to function names")
    args = ap.parse_args()

    if not any((args.log, args.perf, args.shaders)):
        ap.error("supply at least one of --log / --perf / --shaders")

    if args.log:
        analyse_draws(args.log, load_func_table(Path(args.repo)))
    if args.shaders:
        analyse_shaders(args.shaders)
    if args.perf:
        analyse_perf(args.perf)
    return 0


if __name__ == "__main__":
    sys.exit(main())
