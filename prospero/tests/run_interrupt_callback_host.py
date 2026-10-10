#!/usr/bin/env python3
"""Verify coherent callback publication using the production header."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[2]
capture = root / "prospero/validation/interrupt-callback-v3.8-captures"
capture.mkdir(parents=True, exist_ok=True)
work = root / "out/interrupt-callback-v3.8"
work.mkdir(parents=True, exist_ok=True)
source = root / "prospero/tests/interrupt_callback_host.cpp"
header = root / "tools/rexglue-sdk/include/rex/graphics/interrupt_callback.h"
results = []
for name in ("g++", os.environ.get("HOST_CLANGXX", "clang++")):
    compiler = shutil.which(name)
    if not compiler:
        raise SystemExit(f"Required host compiler unavailable: {name}")
    label = Path(compiler).name
    executable = work / label
    command = [compiler, "-std=c++20", "-O3", "-pthread", "-I",
               str(root / "tools/rexglue-sdk/include"), str(source), "-o", str(executable)]
    built = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (capture / f"{label}-build.txt").write_text(built.stdout)
    built.check_returncode()
    ran = subprocess.run([str(executable)], text=True, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, timeout=30)
    (capture / f"{label}-run.txt").write_text(ran.stdout)
    ran.check_returncode()
    print(ran.stdout, end="")
    results.append({"compiler": compiler, "command": command, "exit_code": ran.returncode,
                    "stdout": ran.stdout})
(root / "prospero/validation/interrupt-callback-v3.8.json").write_text(json.dumps({
    "status": "PASS", "scope": "Host concurrency contract test, not console execution",
    "header_sha256": hashlib.sha256(header.read_bytes()).hexdigest(),
    "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(), "runs": results,
    "observed_console_crash_origin_proved": False,
    "save_format": "GraphicsSystem keeps two uint32 words in the same order; one coherent snapshot",
}, indent=2) + "\n")
