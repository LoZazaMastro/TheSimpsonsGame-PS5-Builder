#!/usr/bin/env python3
"""Test actual SharedMemory with a modeled guest-memory provider at 4/16 KiB.

The baseline is taken from the specified git revision, never reconstructed by
undoing a patch. TSAN must detect its real plain bitmap access race; the current
source must complete the same workload with no warning. Models do not emulate
PS5 mprotect, GPU scheduling, rendering or physical memory.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import time


STUBS = {
    "rex/memory.h": r'''#pragma once
#include <cstdint>
#include <cstddef>
#include <utility>
#include <vector>
namespace rex::memory {
inline size_t test_page_size = 16384;
inline size_t page_size() { return test_page_size; }
class Memory {
 public:
  using PhysicalMemoryInvalidationCallback = std::pair<uint32_t,uint32_t>(*)(void*,uint32_t,uint32_t,bool);
  std::vector<uint8_t> bytes = std::vector<uint8_t>(2 * 1024 * 1024);
  uint8_t* TranslatePhysical(uint32_t address) const { return const_cast<uint8_t*>(bytes.data()) + address; }
  void* RegisterPhysicalMemoryInvalidationCallback(PhysicalMemoryInvalidationCallback, void* context) { return context; }
  void UnregisterPhysicalMemoryInvalidationCallback(void*) {}
  void EnablePhysicalMemoryAccessCallbacks(uint32_t, uint32_t, bool, bool) {}
};
}
''',
    "rex/cvar.h": r'''#pragma once
namespace rex::cvar {
enum class Lifecycle { kHotReload, kRequiresRestart };
struct Registration { Registration lifecycle(Lifecycle) const { return {}; } };
}
#define REXCVAR_DEFINE_BOOL(name, val, category, description) bool& FLAGS_##name##_storage_() { static bool storage = val; return storage; } [[maybe_unused]] static auto registration_##name = rex::cvar::Registration{}
#define REXCVAR_GET(name) FLAGS_##name##_storage_()
''',
    "rex/perf/counter.h": r'''#pragma once
namespace rex::perf {
enum class CounterId { kUploadLockWaitUs, kCpUploadUs, kUploadScanUs, kUploadCopyUs };
struct ScopedCounterTimer { explicit ScopedCounterTimer(CounterId) {} void Stop() {} };
}
#define PERF_counter_add(...)
''',
    "rex/dbg.h": r'''#pragma once
#define COUNT_profile_set(...)
#define SCOPE_profile_cpu_f(...)
''',
    "rex/logging.h": "#pragma once\n#define REXGPU_ERROR(...) ((void)0)\n",
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="g++")
    parser.add_argument("--baseline", default="8a93bd1ca0da475716f953b39e1a19155b52af84")
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    args.output_dir = args.output_dir.resolve()
    root = Path(__file__).resolve().parents[2]
    sdk = root / "tools/rexglue-sdk"
    args.output_dir.mkdir(parents=True, exist_ok=True)
    source = "tools/rexglue-sdk/src/graphics/shared_memory.cpp"
    header = "tools/rexglue-sdk/include/rex/graphics/shared_memory.h"
    results = []
    source_hashes = {name: digest(root / name) for name in (source, header)}
    with tempfile.TemporaryDirectory(prefix="simpsons-shared-memory-host-") as directory:
        work = Path(directory)
        stubs = work / "stubs"
        for name, text in STUBS.items():
            target = stubs / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text)
        for variant in ("baseline", "current"):
            variant_dir = work / variant
            variant_dir.mkdir()
            actual_source = sdk / "src/graphics/shared_memory.cpp"
            extra_include = []
            if variant == "baseline":
                actual_source = variant_dir / "shared_memory.cpp"
                actual_source.write_bytes(subprocess.check_output(
                    ["git", "show", f"{args.baseline}:{source}"], cwd=root))
                baseline_header = variant_dir / "rex/graphics/shared_memory.h"
                baseline_header.parent.mkdir(parents=True)
                baseline_header.write_bytes(subprocess.check_output(
                    ["git", "show", f"{args.baseline}:{header}"], cwd=root))
                extra_include = ["-I", str(variant_dir)]
            for sanitizer in ("none", "thread"):
                binary = variant_dir / ("test_" + sanitizer)
                command = [args.cxx, "-std=c++23", "-g", "-O1", "-DNDEBUG", "-pthread", "-D__PROSPERO__=1",
                           "-I", str(stubs), *extra_include,
                           "-I", str(sdk / "include"), "-I", str(sdk / "thirdparty/simde"),
                           str(root / "prospero/tests/shared_memory_host.cpp"),
                           str(actual_source), str(sdk / "src/core/math_gcc.cpp"),
                           "-o", str(binary)]
                if sanitizer == "thread":
                    command += ["-fsanitize=thread", "-fno-omit-frame-pointer"]
                compiled = subprocess.run(command, capture_output=True, text=True, timeout=60)
                compile_capture = args.output_dir / f"{variant}-{sanitizer}-compile.txt"
                compile_capture.write_text(shlex.join(command) + "\n" + compiled.stdout + compiled.stderr)
                if compiled.returncode:
                    raise RuntimeError(f"compile failed: {compile_capture}")
                suites = ("functional", "concurrent") if sanitizer == "none" else ("concurrent",)
                for page in (4096, 16384):
                    for suite in suites:
                        name = f"{variant}-{sanitizer}-{page}-{suite}"
                        env = dict(os.environ, TSAN_OPTIONS="halt_on_error=1:exitcode=66")
                        started = time.monotonic()
                        run = subprocess.run([str(binary), str(page), suite], env=env,
                                             capture_output=True, text=True, timeout=45)
                        capture = args.output_dir / (name + ".txt")
                        combined = run.stdout + run.stderr
                        capture.write_text(combined)
                        baseline_detection = variant == "baseline" and sanitizer == "thread"
                        ok = (run.returncode == 66 and "WARNING: ThreadSanitizer: data race" in combined
                              and "shared_memory.cpp" in combined) if baseline_detection else (
                                  run.returncode == 0 and "ThreadSanitizer:" not in combined)
                        results.append({"name": name, "exit_code": run.returncode,
                                        "expected_baseline_race": baseline_detection,
                                        "pass": ok, "elapsed_seconds": round(time.monotonic() - started, 3),
                                        "capture": str(capture.relative_to(root)), "sha256": digest(capture)})
                        print(f"{'PASS' if ok else 'FAIL'} {name}: exit={run.returncode}", flush=True)
    report = {
        "result": "PASS" if all(r["pass"] for r in results) else "FAIL",
        "baseline_revision": args.baseline,
        "production_source_sha256": source_hashes,
        "sources_unchanged_during_test": all(digest(root / n) == h for n, h in source_hashes.items()),
        "tests": results,
        "scope": "Actual SharedMemory.cpp and class with __PROSPERO__=1 (compiler atomic builtins), actual bit/math/thread-lock code; model guest memory, CVars/logging/profiling only.",
        "limits": ["No PS5 execution", "No GPU rendering", "No PS5 fault/protection calls", "Does not reproduce or identify the allocator corruption from console logs"],
    }
    report_path = args.output_dir.parent / "shared-memory-host-full-audit.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    return 0 if report["result"] == "PASS" and report["sources_unchanged_during_test"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
