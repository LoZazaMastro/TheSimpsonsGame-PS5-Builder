#!/usr/bin/env python3
"""Compile and run the production portable-path helper against host POSIX I/O."""
import argparse
import pathlib
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="clang++")
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[2]
    sdk = root / "tools/rexglue-sdk"
    # A source-boundary guard complements the executable helper test: otherwise
    # adding the rejected call back to SetupEnvironment would not be caught by
    # a helper-only host harness. This is not an end-to-end PS5 runtime test.
    source = (sdk / "src/ui/rex_app.cpp").read_text()
    setup = source.split("bool ReXApp::SetupEnvironment() {", 1)[1].split(
        "bool ReXApp::ConstructRuntime(", 1)[0]
    if re.search(r"\b(?:current_path|chdir)\s*\(", setup):
        raise RuntimeError("SetupEnvironment must prepare explicit paths without changing CWD")
    with tempfile.TemporaryDirectory(prefix="simpsons-paths-host-") as work:
        binary = pathlib.Path(work) / "prospero_paths_host"
        subprocess.run([
            args.cxx, "-std=c++23", "-Wall", "-Wextra", "-Werror", "-O1",
            "-DPROSPERO_HOST_CHDIR_EPERM=1",
            "-I", str(sdk / "include"),
            str(root / "prospero/tests/prospero_paths_host.cpp"),
            str(sdk / "src/ui/prospero_paths.cpp"), "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
