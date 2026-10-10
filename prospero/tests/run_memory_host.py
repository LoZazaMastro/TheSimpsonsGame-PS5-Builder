#!/usr/bin/env python3
"""Compile the real backend against the payload SDK's Linux host-kernel model."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", required=True, type=Path, help="PS5-PayloadSDK checkout root")
    parser.add_argument("--cc", default="cc")
    parser.add_argument("--cxx", default="c++")
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[2]
    platform = args.sdk_root.resolve() / "platform"
    runtime = project / "tools/rexglue-sdk"
    includes = ["-I" + str(platform / "include"), "-I" + str(platform / "tests")]
    with tempfile.TemporaryDirectory(prefix="simpsons-memory-host-") as directory:
        build = Path(directory)
        objects = []
        for source in (platform / "src/shm.c", platform / "src/placement.c",
                       platform / "tests/host_kernel.c"):
            target = build / (source.stem + ".o")
            command = [args.cc, "-std=c11", "-pthread", "-O1", *includes]
            if source.name == "host_kernel.c":
                command.append("-DsceKernelQueryMemoryProtection=host_unimplemented_query")
            subprocess.run([*command, "-c", str(source), "-o", str(target)], check=True)
            objects.append(str(target))
        executable = build / "memory_host_test"
        subprocess.run([
            args.cxx, "-std=c++23", "-pthread", "-O1", "-Wall", "-Wextra", "-Werror",
            "-D__PROSPERO__=1", "-I" + str(runtime / "include"), *includes,
            str(runtime / "src/core/memory_prospero.cpp"),
            str(Path(__file__).with_name("memory_prospero_host.cpp")),
            *objects, "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
