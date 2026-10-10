#!/usr/bin/env python3
"""Run the real telemetry worker with public SDK counter fixtures."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--version", choices=("v3.8", "v3.9"), default="v3.9")
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
sdk_source = Path(os.environ.get("PS5_SDK_SOURCE", root.parent / "PS5-PayloadSDK"))
work = root / ("out/frame-metrics-" + args.version); work.mkdir(parents=True, exist_ok=True)
capture = root / ("prospero/validation/frame-metrics-" + args.version + "-captures"); capture.mkdir(parents=True, exist_ok=True)
files = [root / "prospero/tests/frame_metrics_host.cpp", root / "tools/rexglue-sdk/src/core/metrics_prospero.cpp"]
results = []
for name in ("g++", os.environ.get("HOST_CLANGXX", "clang++")):
    compiler = shutil.which(name)
    if not compiler: raise SystemExit(f"Compiler unavailable: {name}")
    label = Path(compiler).name
    command = [compiler, "-std=c++23", "-O2", "-pthread", "-I", str(root / "tools/rexglue-sdk/include"),
               "-I", str(sdk_source / "platform/include"), *map(str, files), "-o", str(work / label)]
    built = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (capture / f"{label}-build.txt").write_text(built.stdout); built.check_returncode()
    env = dict(os.environ, METRICS_CAPTURE=str(capture / f"{label}-telemetry.txt"))
    ran = subprocess.run([str(work / label)], env=env, text=True, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, timeout=20)
    (capture / f"{label}-run.txt").write_text(ran.stdout); ran.check_returncode(); print(ran.stdout, end="")
    results.append({"command": command, "exit_code": ran.returncode, "stdout": ran.stdout})
for name in ("frame_stats_prospero.h", "metrics_prospero.h", "audio_metrics_prospero.h", "trace_phases_prospero.h"):
    files.append(root / "tools/rexglue-sdk/include/rex/platform" / name)
(root / ("prospero/validation/frame-metrics-" + args.version + ".json")).write_text(json.dumps({
    "status": "PASS", "scope": "Synthetic math and real host telemetry worker with mocked SDK counters and an observed audio-flush scheduling seam; real audio snapshot tests are separate",
    "ps5_performance_measured": False, "runs": results,
    "sources": {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest() for path in files},
}, indent=2) + "\n")
