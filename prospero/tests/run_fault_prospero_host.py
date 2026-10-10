#!/usr/bin/env python3
"""Test the production PS5 fault writer/classifier with host POSIX signal I/O."""
import argparse
import pathlib
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="clang++")
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[2]
    sdk = root / "tools/rexglue-sdk"
    with tempfile.TemporaryDirectory(prefix="simpsons-fault-host-") as directory:
        binary = pathlib.Path(directory) / "fault_prospero_host"
        subprocess.run([
            args.cxx, "-std=c++23", "-Wall", "-Wextra", "-Werror", "-O1", "-pthread",
            "-I", str(sdk / "include"),
            str(root / "prospero/tests/fault_prospero_host.cpp"),
            str(sdk / "src/core/fault_prospero.cpp"),
            str(sdk / "src/core/trace_prospero.cpp"), "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary)], check=True)
        seh_binary = pathlib.Path(directory) / "seh_prospero_host"
        subprocess.run([
            args.cxx, "-std=c++23", "-Wall", "-Wextra", "-Werror", "-O1",
            "-DNDEBUG", "-D__PROSPERO__=1", "-I", str(sdk / "include"),
            str(root / "prospero/tests/seh_prospero_host.cpp"),
            str(sdk / "src/core/seh_posix.cpp"),
            str(sdk / "src/core/signal_thread_prospero.cpp"),
            str(sdk / "src/core/trace_prospero.cpp"), "-pthread", "-Wl,--wrap=sigaction",
            "-o", str(seh_binary),
        ], check=True)
        subprocess.run([str(seh_binary)], check=True)


if __name__ == "__main__":
    main()
