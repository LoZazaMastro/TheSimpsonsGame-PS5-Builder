#!/usr/bin/env python3
"""Run the actual PS5 abort wrapper against a native host abort signal."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--without-wrapper", action="store_true",
                        help="negative control: run the same checks without abort interception")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / "tools/rexglue-sdk"
    with tempfile.TemporaryDirectory(prefix="simpsons-abort-host-") as directory:
        binary = Path(directory) / "abort_test"
        command = [args.cxx, "-std=c++23", "-O2", "-DNDEBUG", "-Wall", "-Wextra", "-Werror", "-pthread",
                   "-I", str(sdk / "include"),
                   str(root / "prospero/tests/abort_prospero_host.cpp"),
                   str(sdk / "src/core/fault_prospero.cpp"),
                   str(sdk / "src/core/trace_prospero.cpp")]
        if not args.without_wrapper:
            command += [str(sdk / "src/core/abort_prospero.cpp"), "-Wl,--wrap=abort"]
        subprocess.run([*command, "-o", str(binary)], check=True)
        return subprocess.run([str(binary)], timeout=15).returncode


if __name__ == "__main__":
    raise SystemExit(main())
