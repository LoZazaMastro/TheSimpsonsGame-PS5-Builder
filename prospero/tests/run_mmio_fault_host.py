#!/usr/bin/env python3
"""Test the real PS5 MMIO callback with controlled native-query results."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="c++")
    parser.add_argument("--mmio-source", type=Path,
                        help="Optional baseline source for the regression demonstration")
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[2]
    runtime = project / "tools/rexglue-sdk"
    includes = [runtime / "include", runtime / "thirdparty/fmt/include",
                runtime / "thirdparty/spdlog/include", runtime / "thirdparty/simde",
                runtime / "thirdparty/tomlplusplus/include"]
    with tempfile.TemporaryDirectory(prefix="simpsons-mmio-fault-") as temporary:
        executable = Path(temporary) / "mmio-fault-host"
        command = [args.cxx, "-std=c++23", "-pthread", "-O1", "-DNDEBUG",
                   "-D__PROSPERO__=1", "-DFMT_HEADER_ONLY", "-DSPDLOG_FMT_EXTERNAL",
                   "-msse4.1", "-Wall", "-Wextra",
                   *("-I" + str(path) for path in includes),
                   str(args.mmio_source or runtime / "src/system/mmio_handler.cpp"),
                   str(runtime / "src/core/mutex.cpp"),
                   str(Path(__file__).with_name("mmio_fault_host.cpp")),
                   "-o", str(executable)]
        subprocess.run(command, check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
