#!/usr/bin/env python3
"""Exercise the real PS5 heap-failure reporter and abort path on the host."""
import argparse
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="clang++")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / "tools/rexglue-sdk"
    with tempfile.TemporaryDirectory(prefix="simpsons-heap-failure-host-") as directory:
        binary = Path(directory) / "heap_failure_test"
        command = [args.cxx, "-std=c++23", "-O2", "-DNDEBUG", "-Wall", "-Wextra", "-Werror", "-pthread",
                   "-I", str(sdk / "include"), str(root / "prospero/tests/heap_failure_host.cpp"),
                   str(sdk / "src/core/fault_prospero.cpp"), str(sdk / "src/core/abort_prospero.cpp"),
                   str(sdk / "src/core/trace_prospero.cpp"),
                   "-Wl,--wrap=abort", "-Wl,--wrap=malloc", "-Wl,--wrap=calloc", "-Wl,--wrap=realloc",
                   "-o", str(binary)]
        print(shlex.join(command), flush=True)
        subprocess.run(command, check=True, timeout=30)
        return subprocess.run([str(binary)], timeout=15).returncode


if __name__ == "__main__":
    raise SystemExit(main())
