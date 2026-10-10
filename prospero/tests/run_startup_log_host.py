#!/usr/bin/env python3
"""Exercise the real pre-libc logger/selector with controlled native API returns.

This validates code flow and errors, not console import safety or mount access.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="clang++")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    with tempfile.TemporaryDirectory(prefix="simpsons-startup-host-") as directory:
        work = Path(directory)
        flags = ["-std=c++23", "-O1", "-Wall", "-Wextra", "-Werror",
                 "-ffunction-sections", "-fdata-sections", "-fno-builtin"]
        subprocess.run([args.cxx, *flags, "-D_start=host_unused_start",
                        "-D_init=host_unused_init", "-D_fini=host_unused_fini",
                        "-c", str(root / "prospero/app_crt.cpp"),
                        "-o", str(work / "app_crt.o")], check=True)
        subprocess.run([args.cxx, *flags,
                        str(root / "prospero/tests/startup_log_host_test.cpp"),
                        str(work / "app_crt.o"), "-Wl,--gc-sections",
                        "-o", str(work / "startup_test")], check=True)
        for mode in ("writable", "read-only", "missing-logs", "denied", "other-error", "fallback-denied"):
            subprocess.run([str(work / "startup_test"), mode], timeout=10, check=True)
    print("Host startup checks passed. Native kernel calls were controlled; PS5 execution remains untested.")


if __name__ == "__main__":
    main()
