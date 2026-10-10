#!/usr/bin/env python3
"""Run the real guest heaps and PS5 backend against the SDK's host kernel model."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", required=True, type=Path)
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--baseline-commit", help="Reproduce the two defects using this commit's unmodified xmemory source/header")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / "tools/rexglue-sdk"
    platform = args.sdk_root.resolve() / "platform"
    includes = ["-I" + str(platform / "include"), "-I" + str(platform / "tests"),
                "-I" + str(sdk / "include"), "-I" + str(sdk / "thirdparty"),
                "-I" + str(sdk / "thirdparty/simde"),
                "-I" + str(sdk / "thirdparty/fmt/include"),
                "-I" + str(sdk / "thirdparty/spdlog/include"),
                "-I" + str(sdk / "thirdparty/cli11/include"),
                "-I" + str(sdk / "thirdparty/tomlplusplus/include")]
    with tempfile.TemporaryDirectory(prefix="simpsons-heap-host-") as directory:
        build = Path(directory)
        objects = []
        baseline_source = None
        if args.baseline_commit:
            baseline_source = build / "baseline-xmemory.cpp"
            baseline_header = build / "baseline-include/rex/system/xmemory.h"
            baseline_header.parent.mkdir(parents=True)
            for relative, target in [
                ("tools/rexglue-sdk/src/system/xmemory.cpp", baseline_source),
                ("tools/rexglue-sdk/include/rex/system/xmemory.h", baseline_header),
            ]:
                target.write_bytes(subprocess.check_output(
                    ["git", "show", args.baseline_commit + ":" + relative], cwd=root))
            includes.insert(0, "-I" + str(build / "baseline-include"))
        for source in [platform / "src/shm.c", platform / "src/placement.c",
                       platform / "tests/host_kernel.c"]:
            obj = build / (source.stem + ".o")
            cmd = [args.cc, "-std=c11", "-pthread", "-O1", *includes]
            if source.name == "host_kernel.c":
                cmd.append("-DsceKernelQueryMemoryProtection=host_unimplemented_query")
            subprocess.run([*cmd, "-c", str(source), "-o", str(obj)], check=True)
            objects.append(str(obj))
        sources = [baseline_source or sdk / "src/system/xmemory.cpp", sdk / "src/core/memory_prospero.cpp",
                   sdk / "src/core/cvar.cpp", sdk / "src/core/mutex.cpp", sdk / "src/core/math_gcc.cpp",
                   sdk / "src/core/perf/counter.cpp", Path(__file__).with_name("heap_memory_host.cpp")]
        for index, source in enumerate(sources):
            obj = build / f"source-{index}.o"
            subprocess.run([args.cxx, "-std=c++23", "-pthread", "-O1", "-g", "-msse4.1",
                            "-ffunction-sections", "-fdata-sections", "-DSPDLOG_FMT_EXTERNAL",
                            "-DFMT_HEADER_ONLY", "-D_GLIBCXX_ASSERTIONS", "-DREX_PLATFORM_PROSPERO=1", "-D__PROSPERO__=1", "-DREX_PROSPERO_HEAP_HOST_TEST=1",
                            *(["-DREX_PROSPERO_HEAP_BASELINE=1"] if args.baseline_commit else []),
                            *includes, "-c", str(source), "-o", str(obj)], check=True)
            objects.append(str(obj))
        binary = build / "heap_memory_host"
        subprocess.run([args.cxx, "-pthread", "-Wl,--gc-sections", *objects,
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
