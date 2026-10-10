#!/usr/bin/env python3
"""Build the real POSIX filesystem backend and exercise save/asset I/O on Linux.

This test does not emulate PS5 file permissions, FFPFSC or kernel ABI. It covers
the shared access-mask and enumeration logic, with controlled syscall failures.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--source", type=Path, help="Optional unmodified baseline source")
    parser.add_argument("--diagnostic-log", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / "tools/rexglue-sdk"
    source = args.source or sdk / "src/core/filesystem_posix.cpp"
    with tempfile.TemporaryDirectory(prefix="simpsons-filesystem-host-") as work:
        binary = Path(work) / "filesystem_posix_host"
        subprocess.run([
            args.cxx, "-std=c++23", "-O1", "-g", "-Wall", "-Wextra",
            "-D__PROSPERO__=1", "-ffunction-sections", "-fdata-sections",
            "-I" + str(sdk / "include"),
            "-I" + str(sdk / "thirdparty/fmt/include"),
            "-I" + str(sdk / "thirdparty/spdlog/include"),
            str(source), str(Path(__file__).with_name("filesystem_posix_host.cpp")),
            str(sdk / "src/core/trace_prospero.cpp"), "-pthread",
            "-Wl,--gc-sections", "-Wl,--wrap=pread", "-Wl,--wrap=pwrite",
            "-Wl,--wrap=write", "-Wl,--wrap=readdir", "-Wl,--wrap=stat",
            "-o", str(binary),
        ], check=True)
        log = args.diagnostic_log or Path(work) / "filesystem.log"
        log.parent.mkdir(parents=True, exist_ok=True)
        result = subprocess.run([str(binary), str(log)], check=False, timeout=30)
        if result.returncode == 0 and not args.source:
            content = log.read_text()
            for category in ("file_open", "file_error", "file_close", "file_short"):
                if "OPS category=" + category not in content:
                    raise RuntimeError("Missing production filesystem diagnostic: " + category)
            if "space%20%25%20file" not in content or "errno=5" not in content:
                raise RuntimeError("Escaped path or injected EIO context missing")
            print("PASS filesystem operational path/error/counter diagnostics", flush=True)
        return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
