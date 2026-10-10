#!/usr/bin/env python3
"""Reproduce malformed-cache and worker-lifetime bugs using production blocks.

The baseline and current parser source is extracted without algorithm rewrites.
The actual packed record layouts, XXH3, bounded reader and POSIX file operations
are used. Fixtures replace shader creation and Vulkan driver calls only. This
does not execute Vulkan, the console allocator, or a commercial game image.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

from run_pipeline_cache_host import function


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--baseline", default="8a93bd1ca0da475716f953b39e1a19155b52af84")
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / "tools/rexglue-sdk"
    relative = "tools/rexglue-sdk/src/graphics/vulkan/pipeline_cache.cpp"
    current = (root / relative).read_text()
    baseline = subprocess.check_output(["git", "show", args.baseline + ":" + relative], cwd=root).decode()
    cache_header = (sdk / "include/rex/graphics/vulkan/pipeline_cache.h").read_text()
    render_header = (sdk / "include/rex/graphics/vulkan/render_target_cache.h").read_text()
    translator_header = (sdk / "include/rex/graphics/pipeline/shader/spirv_translator.h").read_text()
    if not re.search(r"static constexpr uint32_t kVersion = 12;", translator_header):
        raise RuntimeError("Update fixture's shader modification header version")
    types = cache_header[cache_header.index("  REXPACKEDSTRUCT(ShaderStoredHeader, {"):
                         cache_header.index("\n  struct Pipeline {")]
    render_key = function(render_header, "  union RenderPassKey {") + ";"
    fs_source = (sdk / "src/core/filesystem_posix.cpp").read_text()
    fileops = "\n".join(function(fs_source, signature) for signature in [
        "bool Seek(FILE* file,", "int64_t Tell(FILE* file)", "bool TruncateStdioFile(FILE* file,"])
    fileops = fileops.replace("bool TruncateStdioFile(", "bool ActualTruncateStdioFile(", 1)
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    report = {
        "baseline_commit": args.baseline,
        "mode": "C++23 Release -O2 -DNDEBUG; undefined-behavior sanitizer",
        "boundary": "Exact source parsing/teardown blocks and record layouts, real stdio/XXH3; mocked shader/Vulkan sinks; no PS5 execution",
        "sources": {}, "cases": [],
    }
    for path in [root / relative, sdk / "include/rex/graphics/vulkan/cache_file_reader.h",
                 sdk / "include/rex/graphics/vulkan/pipeline_cache.h",
                 root / "prospero/tests/cache_input_host.cpp", Path(__file__)]:
        report["sources"][str(path.relative_to(root))] = hashlib.sha256(path.read_bytes()).hexdigest()
    report["baseline_source_sha256"] = hashlib.sha256(baseline.encode()).hexdigest()
    common_cases = ["shader-cold", "shader-valid", "shader-huge-count", "shader-short-record",
                    "shader-bad-hash", "shader-trailing-bytes", "shader-truncate-error",
                    "shader-write-error", "shader-oversize-file", "shader-io-error",
                    "pipeline-mixed", "pipeline-all-unsupported", "pipeline-corrupt-tail",
                    "pipeline-short-tail", "aot-valid", "aot-unaligned", "aot-oversize",
                    "shutdown-cold", "shutdown-warm"]
    expected_failures = {"shader-huge-count", "shader-truncate-error", "shader-write-error",
                         "shader-oversize-file", "shader-io-error", "pipeline-mixed",
                         "pipeline-all-unsupported", "pipeline-corrupt-tail", "pipeline-short-tail",
                         "aot-oversize", "shutdown-cold", "shutdown-warm"}
    with tempfile.TemporaryDirectory(prefix="simpsons-cache-input-") as temporary:
        work = Path(temporary)
        for label, source in [("baseline", baseline), ("current", current)]:
            storage = function(source, "void VulkanPipelineCache::InitializeShaderStorage(")
            shader_start = storage.index("  struct {", storage.index("  ++shader_storage_index_;"))
            shader_read = storage[shader_start:storage.index("  // A storage made from the executable")]
            extent_start = storage.find("  uint64_t shader_storage_extent;")
            shader_extent = (storage[extent_start:storage.index("  ++shader_storage_index_;", extent_start)]
                             if extent_start >= 0 else "")
            pipeline_start = storage.find("  uint64_t pipeline_storage_extent;")
            if pipeline_start < 0:
                pipeline_start = storage.index("  pipeline_storage_file_flush_needed_ = false;")
            pipeline_read = storage[pipeline_start:storage.index("  // Initialize the Xenos shader storage stream.")]
            pipeline_truncate = storage[storage.index("  // If any pipeline descriptions were corrupted"):
                                        storage.index("  shader_storage_cache_root_ = cache_root;")]
            aot = function(source, "bool VulkanPipelineCache::TryLoadAotTranslation(")
            aot_start = aot.index("  if (!aot_file) {", aot.index("  FILE* aot_file ="))
            aot_read = aot[aot_start:aot.index("  // The binding sidecar is mandatory:")]
            shutdown = function(source, "void VulkanPipelineCache::Shutdown()")
            shutdown = shutdown[shutdown.index("  WritePipelineInventory();") + len("  WritePipelineInventory();"):
                                shutdown.index("  ShutdownShaderStorage();")]
            blocks = {"cache_types": types, "render_pass_key": render_key, "fileops": fileops,
                      "shader_extent": shader_extent, "shader_read": shader_read,
                      "pipeline_read": pipeline_read, "pipeline_truncate": pipeline_truncate,
                      "aot_read": aot_read, "cache_shutdown": shutdown,
                      # The helper is new: baseline runs only its old inline paths.
                      "driver_read": function(current, "std::vector<uint8_t> ReadDriverCacheFile(")}
            for name, block in blocks.items():
                (work / (name + ".inc")).write_text(block + "\n")
            binary = work / ("cache_input_" + label)
            compile_command = [args.cxx, "-std=c++23", "-O2", "-DNDEBUG", "-pthread", "-g",
                               "-fsanitize=undefined", "-fno-sanitize-recover=all", "-Wall", "-Wextra",
                               "-Werror", "-Wno-unused-function", "-Wno-unused-private-field", "-Wno-unused-parameter",
                               "-I", str(sdk / "include"), "-I", str(sdk / "thirdparty/simde"),
                               "-I", str(sdk / "thirdparty/vulkan-headers/include"),
                               "-I", str(sdk / "thirdparty/xxHash"), "-I", str(work),
                               str(root / "prospero/tests/cache_input_host.cpp"), "-o", str(binary)]
            compiled = subprocess.run(compile_command, capture_output=True, text=True)
            (output / f"{label}-compile.txt").write_text(compiled.stdout + compiled.stderr)
            if compiled.returncode:
                raise RuntimeError(f"{label} compile failed\n{compiled.stderr}")
            cases = common_cases + (["read-helpers", "driver"] if label == "current" else [])
            for case in cases:
                result = subprocess.run([str(binary), case], capture_output=True, text=True, timeout=20)
                expected = 1 if label == "baseline" and case in expected_failures else 0
                capture = output / f"{label}-{case}.txt"
                capture.write_text(result.stdout + result.stderr)
                entry = {"revision": label, "case": case, "expected_exit": expected,
                         "exit": result.returncode, "capture": str(capture.relative_to(root))}
                report["cases"].append(entry)
                if result.returncode != expected:
                    raise RuntimeError(f"{label}/{case}: expected {expected}, got {result.returncode}\n"
                                       + result.stdout + result.stderr)
                if expected and "FAIL:" not in result.stderr:
                    raise RuntimeError("Baseline failure did not originate in a concrete behavioral assertion")
                print(f"PASS {label}/{case}" + (" (baseline defect reproduced)" if expected else ""), flush=True)
    report["status"] = "pass"
    report["counts"] = {"cases": len(report["cases"]), "baseline_defects_reproduced": len(expected_failures),
                        "current_cases_passed": len(common_cases) + 2}
    destination = root / "prospero/validation/cache-input-full-audit.json"
    destination.write_text(json.dumps(report, indent=2) + "\n")
    print("Report: " + str(destination))


if __name__ == "__main__":
    main()
