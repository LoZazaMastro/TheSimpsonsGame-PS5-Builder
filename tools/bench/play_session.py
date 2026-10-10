#!/usr/bin/env python3
"""Profile a session the player drives by hand.

Launches the game from an isolated directory with the player's own display
and audio settings (only telemetry is added), then records while they play:
a `perf` sample every interval, the engine's per-frame perf CSV, and one GPU
frame trace the first time the game is clearly in a level (heavy vertex
load), with a second trace a minute later. Ends when the player quits.

    python3 tools/bench/play_session.py <name> [--interval 45] [--sample 10]
"""

import argparse
import csv
import os
import time

import autorun


def in_level(perf_csv):
    """True when the last few frames look like gameplay rather than menus."""
    try:
        with open(perf_csv) as f:
            rows = list(csv.DictReader(f))[-30:]
    except OSError:
        return False
    if len(rows) < 30:
        return False
    heavy = sum(1 for r in rows if int(r.get("vertices_processed") or 0) > 60000)
    return heavy >= 20


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--interval", type=float, default=45)
    ap.add_argument("--sample", type=float, default=10)
    ap.add_argument("--timeout", type=float, default=2700)
    ap.add_argument("--set", action="append", default=[])
    args = ap.parse_args()

    overrides = {k.strip(): v for k, v in (kv.split("=", 1) for kv in args.set)}
    base = dict(autorun.BASE_OVERRIDES)
    for k in ("fullscreen", "window_width", "window_height", "audio_mute", "mnk_mode"):
        base.pop(k, None)
    autorun.BASE_OVERRIDES = base
    run_dir, exe, user_data = autorun.prepare(args.name, overrides)
    run = autorun.Run(run_dir, exe, user_data)
    run.start()
    perf_csv = os.path.join(run_dir, "perf.csv")
    trace_request = os.path.join(run_dir, "trace.request")
    traces_fired = 0
    next_trace_at = None
    next_sample = time.time() + 30
    n = 0
    deadline = time.time() + args.timeout
    try:
        while run.alive() and time.time() < deadline:
            now = time.time()
            if traces_fired < 2 and in_level(perf_csv):
                if next_trace_at is None:
                    next_trace_at = now + 15
                elif now >= next_trace_at:
                    open(trace_request, "w").close()
                    traces_fired += 1
                    run.note(f"trace request {traces_fired}")
                    next_trace_at = now + 60
            if now >= next_sample:
                n += 1
                label = f"{n:02d}_{'level' if in_level(perf_csv) else 'menu'}"
                run.perf(args.sample, label)
                next_sample = time.time() + args.interval
            time.sleep(1)
    finally:
        run.stop()
        with open(os.path.join(run_dir, "summary.txt"), "w") as f:
            f.write("\n".join(run.notes) + "\n")


if __name__ == "__main__":
    main()
