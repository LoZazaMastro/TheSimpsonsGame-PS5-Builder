#!/usr/bin/env python3
"""Exercise the production Vulkan cache's thread/error branches in Release.

Only the GPU-independent blocks are extracted, byte for byte, from the real
pipeline_cache.cpp. The fixture supplies actual host threads and FILEs, with a
failing Thread::Create factory. This is not a Vulkan or PS5 execution test.
"""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile


def block_end(source, start):
    """Find a C++ compound statement's closing brace, ignoring comments/strings."""
    depth = 0
    index = source.index("{", start)
    while index < len(source):
        if source.startswith("//", index):
            index = source.index("\n", index) + 1
            continue
        if source.startswith("/*", index):
            index = source.index("*/", index + 2) + 2
            continue
        char = source[index]
        if char in "\"'":
            quote = char
            index += 1
            while source[index] != quote:
                index += 2 if source[index] == "\\" else 1
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if not depth:
                return index + 1
        index += 1
    raise RuntimeError("Unbalanced production block")


def function(source, signature):
    start = source.index(signature)
    return source[start:block_end(source, start)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="c++")
    parser.add_argument("--baseline-commit", help="Expect Release null dereferences in this unmodified revision")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / "tools/rexglue-sdk"
    relative = "tools/rexglue-sdk/src/graphics/vulkan/pipeline_cache.cpp"
    if args.baseline_commit:
        source_bytes = subprocess.check_output(
            ["git", "show", args.baseline_commit + ":" + relative], cwd=root)
    else:
        source_bytes = (root / relative).read_bytes()
    source = source_bytes.decode()
    print("Production pipeline source SHA256: " + hashlib.sha256(source_bytes).hexdigest(), flush=True)
    print("Mode: Release (-O2 -DNDEBUG), undefined-behavior sanitizer", flush=True)
    print("Boundary: real source blocks; host thread/FILE fixture, no Vulkan/console execution", flush=True)

    pool_start = source.index("    for (size_t i = 0; i < creation_thread_count; ++i)")
    pool = source[pool_start:block_end(source, pool_start)]
    preload_start = source.index("    if (creation_thread_count <= 1)")
    preload_end = block_end(source, preload_start)
    if not source[preload_end:].lstrip().startswith("else {"):
        raise RuntimeError("Preload source boundary changed")
    preload = source[preload_start:block_end(source, preload_end)]
    storage = function(source, "void VulkanPipelineCache::InitializeShaderStorage(")
    writer = storage[storage.index("  // Start the storage writing thread."):storage.rfind("}")]
    truncate = storage[storage.index("  // If any pipeline descriptions were corrupted"):
                       storage.index("  shader_storage_cache_root_ = cache_root;")]
    shutdown = function(source, "void VulkanPipelineCache::ShutdownShaderStorage()")
    shutdown = shutdown.replace("VulkanPipelineCache::", "ShaderStorageFixture::", 1)
    marker_signature = "void LogStorageStartupStage(const char* message)"
    markers = (function(source, marker_signature) if marker_signature in source else
               "void LogStorageStartupStage(const char*) {}")
    filesystem_source = (sdk / "src/core/filesystem_posix.cpp").read_text()
    fileops = "\n".join(function(filesystem_source, signature) for signature in [
        "bool Seek(FILE* file,", "int64_t Tell(FILE* file)",
        "bool TruncateStdioFile(FILE* file,"])
    fileops = fileops.replace("bool TruncateStdioFile(", "bool ActualTruncateStdioFile(", 1)

    # The isolated test must still cover each real factory and the real cleanup,
    # not silently keep testing an obsolete helper after production is refactored.
    if source.count("rex::thread::Thread::Create(") != 3:
        raise RuntimeError("Update this test for the changed set of cache thread factories")
    for label, block in [("pool", pool), ("preload", preload), ("writer", writer)]:
        if block.count("rex::thread::Thread::Create(") != 1:
            raise RuntimeError(label + " no longer contains its production factory")
    if not args.baseline_commit:
        if re.search(r"assert_not_null\((?:creation_thread|storage_write_thread_)\)",
                     pool + preload + writer):
            raise RuntimeError("Release-only assertion reintroduced at a cache thread factory")
        if "ShutdownShaderStorage();" not in writer:
            raise RuntimeError("Writer creation failure no longer cleans up production streams")

    successful_cases = ["pool-success", "preload-success", "preload-serial",
                        "writer-success", "truncate-cold", "truncate-warm"]
    failure_cases = ["pool-first-failure", "pool-partial-failure",
                     "preload-first-failure", "preload-partial-failure",
                     "writer-failure"]
    with tempfile.TemporaryDirectory(prefix="simpsons-pipeline-host-") as directory:
        work = Path(directory)
        for name, block in [("pool", pool), ("preload", preload), ("writer", writer),
                            ("truncate", truncate), ("shutdown", shutdown),
                            ("fileops", fileops), ("markers", markers)]:
            (work / (name + ".inc")).write_text(block + "\n")
        binary = work / "pipeline_cache_host"
        subprocess.run([
            args.cxx, "-std=c++23", "-pthread", "-O2", "-DNDEBUG", "-g",
            "-fsanitize=undefined", "-fno-sanitize-recover=all",
            "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
            "-DREX_PLATFORM_PROSPERO=1", "-I", str(sdk / "include"),
            "-I", str(work), str(root / "prospero/tests/pipeline_cache_host.cpp"),
            "-o", str(binary),
        ], check=True)
        cases = successful_cases + failure_cases
        if not args.baseline_commit:
            cases += ["truncate-first-failure", "truncate-reset-failure",
                      "truncate-write-failure"]
        for case in cases:
            result = subprocess.run([str(binary), case], capture_output=True, text=True, timeout=20)
            if args.baseline_commit and case in failure_cases:
                if result.returncode == 0 or not re.search(r"(?:null pointer|address 0x0)", result.stderr):
                    raise RuntimeError(f"Baseline did not reproduce the null dereference in {case}: "
                                       f"{result.returncode}\n{result.stdout}\n{result.stderr}")
                diagnostic = next(line.strip() for line in result.stderr.splitlines()
                                  if "runtime error:" in line)
                # The temporary source path is not a durable artifact.
                diagnostic = diagnostic.replace(str(work) + "/", "[production-block]/")
                print(f"PASS baseline reproduction {case}: exit={result.returncode}; {diagnostic}", flush=True)
            elif result.returncode:
                raise RuntimeError(f"{case}: exit={result.returncode}\n{result.stdout}\n{result.stderr}")
            else:
                print(result.stdout.strip(), flush=True)
        print(f"PASS {len(cases)} production-control-flow scenarios" +
              (" (5 expected baseline failures reproduced)" if args.baseline_commit else ""), flush=True)


if __name__ == "__main__":
    main()
