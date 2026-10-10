#!/usr/bin/env python3
"""Run game pointer/endian regressions on a sparse Linux host mapping.

Compiles the actual game header with and without its PS5 platform branch.
No payload SDK kernel behavior or console execution is tested here.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="c++", help="native host C++23 compiler")
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[2]
    with tempfile.TemporaryDirectory(prefix="simpsons-guest-memory-host-") as directory:
        for branch in ("desktop", "prospero"):
            executable = Path(directory) / branch
            command = [args.cxx, "-std=c++23", "-O1", "-Wall", "-Wextra", "-Werror",
                       "-I" + str(project / "simpsons/src"),
                       "-I" + str(project / "tools/rexglue-sdk/include")]
            if branch == "prospero":
                command.append("-D__PROSPERO__=1")
            subprocess.run([*command, str(Path(__file__).with_name("guest_memory_host.cpp")),
                            "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
