#!/usr/bin/env python3
"""Validate actual PS5 operational hooks behind explicit host boundary fixtures.

No test below is a PS5 console run. Native device calls, guest memory translation
and semaphore ownership are fixtures; the complete production audio/input/FS
drivers, PCM converter, event ring and reporting helpers are compiled unchanged.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import time

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def fields(line):
    return dict(part.split("=", 1) for part in line.split() if "=" in part)

def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--report", type=Path, default=root / "prospero/validation/operational-diagnostics-v3.8.json")
    args = parser.parse_args()
    sdk = root / "tools/rexglue-sdk"
    capture = args.report.with_suffix("")
    capture.mkdir(parents=True, exist_ok=True)
    results = []
    def run(name, command, timeout=40):
        start = time.monotonic()
        result = subprocess.run(command, cwd=root, capture_output=True, text=True, timeout=timeout)
        log = capture / (name + ".txt")
        log.write_text("COMMAND " + shlex.join(map(str, command)) + "\n" + result.stdout + result.stderr +
                       "\nEXIT " + str(result.returncode) + "\n")
        results.append({"test": name, "exit_code": result.returncode,
                        "elapsed_seconds": round(time.monotonic() - start, 3),
                        "capture": str(log.relative_to(root)), "capture_sha256": digest(log)})
        print(name, "PASS" if result.returncode == 0 else "FAIL", flush=True)
        if result.returncode:
            print(result.stdout + result.stderr, flush=True)
            raise RuntimeError(name + " failed")

    production = [
        "tools/rexglue-sdk/include/rex/platform/trace_game_prospero.h",
        "tools/rexglue-sdk/include/rex/platform/audio_metrics_prospero.h",
        "tools/rexglue-sdk/src/core/trace_prospero.cpp",
        "tools/rexglue-sdk/include/rex/platform/trace_prospero.h",
        "tools/rexglue-sdk/src/input/prospero/prospero_input_driver.cpp",
        "tools/rexglue-sdk/include/rex/input/prospero/prospero_input_driver.h",
        "tools/rexglue-sdk/src/audio/prospero/prospero_audio_driver.cpp",
        "tools/rexglue-sdk/include/rex/audio/prospero/prospero_audio_driver.h",
        "tools/rexglue-sdk/src/core/filesystem_posix.cpp",
        "tools/rexglue-sdk/src/filesystem/virtual_file_system.cpp",
        "tools/rexglue-sdk/src/filesystem/devices/host_path_file.cpp",
        "simpsons/src/frame_pacing.cpp", "simpsons/src/audio.cpp"]
    report = {"result": "FAIL", "production_source_sha256": {p: digest(root / p) for p in production},
              "tests": results, "limits": [
                  "Linux host execution with native services/guest-memory/semaphore fixtures does not validate PS5 AudioOut latency, pad firmware, permissions or title FPS.",
                  "FS/input host runners retain their controlled syscalls and XAM selection fixture; the new schema/errno/counters are exercised on production source.",
                  "File stream counts are per handle; high-volume successful opens/close lines and repeated failures are rate-limited with explicit suppression counters.",
                  "Audio empty_queue_outputs means this backend supplied silence; it is not a hardware underrun counter or proof the guest should emit sound.",
                  "Only the already existing no-recent-physics-step pacing classifier and actual DAC hook entry/return are reported; no guessed level ID or gameplay state is introduced."]}
    try:
        with tempfile.TemporaryDirectory(prefix="simpsons-operational-host-") as temporary:
            work = Path(temporary)
            for directory in ("rex", "rex/audio"):
                (work / directory).mkdir(parents=True, exist_ok=True)
            (work / "rex/kernel.h").write_text("#pragma once\n")
            (work / "rex/memory.h").write_text('''#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
namespace fixture { extern std::atomic<uint64_t> translated; }
namespace rex::memory { class Memory { public:
  alignas(16) std::array<float, 6 * 256> samples{};
  template<class T> T TranslateVirtual(uint32_t address) {
    if (address != 0x1000) std::abort();
    fixture::translated.fetch_add(1); return reinterpret_cast<T>(samples.data());
  }
  uint8_t* TranslatePhysical(uint32_t) { return reinterpret_cast<uint8_t*>(samples.data()); }
}; }
''')
            (work / "rex/thread.h").write_text('''#pragma once
#include <atomic>
#include <cstdint>
namespace fixture { extern std::atomic<bool> credit_fail; }
namespace rex::thread { class Semaphore { public:
  std::atomic<unsigned> attempts{0}, released{0};
  bool Release(uint32_t count, void*) {
    attempts.fetch_add(1); if (fixture::credit_fail.load()) return false;
    released.fetch_add(count); return true;
  }
}; }
''')
            (work / "rex/audio/flags.h").write_text('''#pragma once
#include <atomic>
#include <string>
namespace fixture { extern std::atomic<bool> audio_mute; extern std::string audio_dump_file; }
#define REXCVAR_GET(name) (::fixture::name)
''')
            (work / "rex/logging.h").write_text("#pragma once\n#define REXAPU_INFO(...)\n#define REXAPU_WARN(...)\n#define REXAPU_ERROR(...)\n")
            common = [args.cxx, "-std=c++23", "-O2", "-g", "-DNDEBUG", "-pthread", "-mssse3",
                      "-D__PROSPERO__=1", "-fsanitize=undefined", "-fno-sanitize-recover=all",
                      "-I" + str(work), "-I" + str(sdk / "include")]
            audio = work / "audio-host"
            run("compile-audio", common + [str(sdk / "src/audio/prospero/prospero_audio_driver.cpp"),
                str(root / "prospero/tests/prospero_audio_diagnostics_host.cpp"),
                str(sdk / "src/core/trace_prospero.cpp"), "-Wl,--wrap=write", "-o", str(audio)])
            audio_results = {}
            for case in ("init_fail", "open_fail", "verify_fail", "healthy", "output_fail", "credit_fail", "queue_full", "reuse"):
                log = capture / ("audio-" + case + ".log")
                run("audio-" + case, [str(audio), case, str(log)], timeout=20)
                lines = log.read_text().splitlines()
                snapshots = [fields(line) for line in lines if "OPS category=audio " in line]
                for data in snapshots:
                    if int(data["queued"]) > 64 or int(data["peak_queued"]) > 64:
                        raise RuntimeError("Audio queue snapshot exceeds real capacity")
                    if int(data["empty_queue_outputs"]) + int(data["silent_guest_outputs"]) > int(data["output_successes"]):
                        raise RuntimeError("Audio snapshot counters are mixed or inconsistent")
                if case == "healthy":
                    periodic = [x for x in snapshots if x["final"] == "0" and int(x["elapsed_ms"]) >= 5000]
                    if not periodic or any(x["submitted"] != "1" or x["rejected"] != "0" for x in periodic):
                        raise RuntimeError("Periodic real driver snapshot/counters missing")
                if case == "output_fail" and not any(x["rejected"] == "1" and x["failed"] == "1" for x in snapshots):
                    raise RuntimeError("Rejected submit after output failure missing from final report")
                if case == "queue_full" and not any(x["submitted"] == "64" and x["rejected"] == "1" for x in snapshots):
                    raise RuntimeError("Queue-overflow accounting changed")
                if case == "reuse" and len({x["instance"] for x in snapshots if x["final"] == "1"}) != 80:
                    raise RuntimeError("Distinct driver instance identities were lost during retirement/reuse")
                audio_results[case] = {"result": "PASS", "snapshot_lines": len(snapshots),
                    "log": str(log.relative_to(root)), "log_sha256": digest(log)}
            report["audio"] = audio_results
            helpers = work / "helpers-host"
            run("compile-helpers", common + [str(root / "prospero/tests/operational_helpers_host.cpp"),
                str(sdk / "src/core/trace_prospero.cpp"), "-Wl,--wrap=clock_gettime", "-o", str(helpers)])
            helper_log = capture / "helpers.log"
            run("helpers", [str(helpers), str(helper_log)])
            helper_lines = helper_log.read_text().splitlines(keepends=True)
            if not any("truncated=1" in line for line in helper_lines) or any(len(line.encode()) > 1536 for line in helper_lines):
                raise RuntimeError("Fixed line bound or explicit truncation failed")
            if not any("space%20%25%0Apretend%3Dfield" in line for line in helper_lines):
                raise RuntimeError("Path encoding permitted an unescaped field/newline")
            if not any("phase=569 " in line and "c=0x0000000000001f39" in line for line in helper_lines):
                raise RuntimeError("Exact suppression count 7993 missing from checkpoint")
            run("input", ["python3", str(root / "prospero/tests/run_prospero_input_host.py"),
                "--cxx", args.cxx, "--diagnostic-log", str(capture / "input.log")])
            run("filesystem", ["python3", str(root / "prospero/tests/run_filesystem_posix_host.py"),
                "--cxx", args.cxx, "--diagnostic-log", str(capture / "filesystem.log")])
        report["result"] = "PASS"
    except Exception as error:
        report["error"] = str(error)
    report["test_source_sha256"] = {str(p.relative_to(root)): digest(p) for p in [
        Path(__file__).resolve(), root / "prospero/tests/prospero_audio_diagnostics_host.cpp",
        root / "prospero/tests/operational_helpers_host.cpp", root / "prospero/tests/run_prospero_input_host.py",
        root / "prospero/tests/prospero_input_host.cpp", root / "prospero/tests/run_filesystem_posix_host.py",
        root / "prospero/tests/filesystem_posix_host.cpp"]}
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print("RESULT", report["result"], args.report, flush=True)
    return 0 if report["result"] == "PASS" else 1

if __name__ == "__main__":
    raise SystemExit(main())
