#!/usr/bin/env python3
"""Exercise the actual PS5 cooperative fiber backend against the matched SDK model."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import signal
import subprocess
import tempfile
import time


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", type=Path, default=repo.parent / "PS5-PayloadSDK")
    parser.add_argument("--target-sdk", type=Path, default=repo.parent / "ps5-sdk-current")
    parser.add_argument("--clang", default=shutil.which("clang"))
    parser.add_argument("--clangxx", default=shutil.which("clang++"))
    parser.add_argument("--report", type=Path, default=repo / "prospero/validation/fiber-prospero-host-v3.8.json")
    args = parser.parse_args()
    report_path = args.report.resolve()
    captures = report_path.with_suffix("").with_name(report_path.stem + "-captures")
    captures.mkdir(parents=True, exist_ok=True)
    sdk = args.sdk_root.resolve()
    platform = sdk / "platform"
    source = repo / "tools/rexglue-sdk/src/core/fiber_prospero.cpp"
    header = repo / "tools/rexglue-sdk/include/rex/thread/fiber.h"
    test = repo / "prospero/tests/fiber_prospero_host.cpp"
    baseline = repo / "prospero/tests/fiber_prospero_baseline"
    old_test = repo / "prospero/tests/fiber_posix_refused_host.cpp"
    callers = [repo / "tools/rexglue-sdk/src/system/xthread.cpp",
               repo / "tools/rexglue-sdk/src/kernel/crt/threading.cpp"]
    sdk_sources = [platform / x for x in ("src/shm.c", "src/placement.c", "tests/host_kernel.c")]
    production = [source, header, repo / "tools/rexglue-sdk/src/core/fiber_posix.cpp",
                  repo / "tools/rexglue-sdk/src/core/CMakeLists.txt", *sdk_sources,
                  platform / "include/ps5platform/kernel.h", platform / "include/ps5platform/shm.h",
                  *callers]
    inputs = production + [test, old_test, baseline / "fiber_posix.cpp", baseline / "rex/thread/fiber.h", Path(__file__).resolve()]
    key = lambda path: str(path.relative_to(repo)) if path.is_relative_to(repo) else str(path)
    hashes = {key(path): sha(path) for path in inputs}
    records = []
    report = {
        "result": "FAIL", "console_execution": False,
        "scope": "Actual PS5 Fiber source and explicit context assembly, with actual matched SDK virtual-range code; only console kernel calls are modeled on Linux.",
        "source_sha256": hashes, "captures": records,
        "limits": [
            "The old forced-EPERM test proves the ignored-return/current-fiber defect, not that PS5 returned EPERM in the supplied session.",
            "The old source was not executed against a PS5 native getcontext implementation; its write extent remains unmeasured.",
            "The 256 KiB stack capacity mismatch is established from source/API semantics; no supplied crash log identifies a created-fiber stack overflow.",
            "The guarded 16 MiB stack policy uses direct memory eagerly and must be included in on-console memory/performance measurements.",
            "The switch is cooperative at a SysV call boundary. It preserves six callee-saved integer registers, stack/instruction pointers, all MXCSR bits and x87 control. Caller-saved x87 data/status and XMM/YMM registers are governed by the compiler's normal call ABI.",
            "ASan and TSan fiber-switch annotations are not part of this target; this suite uses UBSan, live canaries, explicit register probes and real guard faults on the host.",
        ],
    }

    def run(name: str, command: list[str], cwd: Path = repo, timeout: int = 50) -> dict:
        began = time.monotonic()
        try:
            result = subprocess.run(command, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    timeout=timeout)
            code, output = result.returncode, result.stdout
            timed_out = False
        except subprocess.TimeoutExpired as error:
            code, output, timed_out = None, error.stdout or b"", True
        path = captures / (name + ".txt")
        path.write_bytes(("COMMAND " + shlex.join(command) + "\n").encode() + output +
                         f"\nEXIT {code}; timeout={timed_out}\n".encode())
        record = {"name": name, "exit_code": code, "timeout": timed_out,
                  "seconds": round(time.monotonic() - began, 3), "capture": key(path),
                  "sha256": sha(path)}
        records.append(record)
        print(f"{name}: exit={code}; timeout={timed_out}", flush=True)
        if code not in (0, -signal.SIGABRT, -signal.SIGSEGV):
            print(output.decode(errors="replace")[-3000:], flush=True)
        return {**record, "output": output.decode(errors="replace")}

    okay = True
    try:
        pin = subprocess.check_output(["git", "-C", str(sdk), "rev-parse", "HEAD"], text=True).strip()
        report["sdk_commit"] = pin
        if pin != "b5efad528e0ac1b8289f39d72a0e890ff68ff0a6":
            raise RuntimeError("Matched SDK pin changed")
        if not args.clang or not args.clangxx:
            raise RuntimeError("Clang and clang++ are required for the target and UBSan checks")
        with tempfile.TemporaryDirectory(prefix="simpsons-fiber-host-") as temporary:
            work = Path(temporary)
            variants = [("gcc-release", "gcc", "g++", []),
                        ("clang-release", args.clang, args.clangxx, ["-ffp-model=strict"]),
                        ("clang-ubsan", args.clang, args.clangxx,
                         ["-ffp-model=strict", "-fsanitize=undefined", "-fno-sanitize-recover=all"])]
            for name, cc, cxx, flags in variants:
                common = ["-O2", "-g", "-fno-omit-frame-pointer", "-fno-strict-aliasing",
                          "-ffunction-sections", "-fdata-sections", "-pthread",
                          "-I" + str(platform / "include"), "-I" + str(platform / "src"),
                          "-I" + str(platform / "tests")]
                objects = []
                for sdk_source in sdk_sources:
                    obj = work / (name + "-" + sdk_source.stem + ".o")
                    cflags = [flag for flag in flags if flag != "-ffp-model=strict"]
                    result = run(name + "-compile-" + sdk_source.stem,
                                 [cc, "-std=gnu11", *common, *cflags, "-c", str(sdk_source), "-o", str(obj)])
                    if result["exit_code"]:
                        raise RuntimeError("SDK host-model compilation failed")
                    objects.append(str(obj))
                binary = work / name
                command = [cxx, "-std=c++23", *common, *flags, "-D__PROSPERO__=1",
                           "-I" + str(repo / "tools/rexglue-sdk/include"), str(source), str(test),
                           *objects, "-Wl,--gc-sections", "-Wl,--wrap=ps5_vrange_reserve", "-o", str(binary)]
                result = run(name + "-link", command)
                if result["exit_code"]:
                    raise RuntimeError("Fiber host build failed")
                result = run(name + "-suite", [str(binary)])
                okay = okay and result["exit_code"] == 0 and "PASS complete PS5 fiber host suite" in result["output"]
                if name == "clang-release":
                    for case, expected in (("null-switch", -signal.SIGABRT),
                                           ("return", -signal.SIGABRT),
                                           ("destroy-current", -signal.SIGABRT),
                                           ("lower-guard", -signal.SIGSEGV),
                                           ("upper-guard", -signal.SIGSEGV)):
                        result = run(name + "-" + case, [str(binary), case])
                        okay = okay and result["exit_code"] == expected

            old_binary = work / "old-ignored-return"
            result = run("baseline-build", ["g++", "-std=c++23", "-O2", "-D__PROSPERO__=1",
                         "-I" + str(baseline), "-I" + str(repo / "tools/rexglue-sdk/include"),
                         str(old_test), str(baseline / "fiber_posix.cpp"), "-Wl,--wrap=swapcontext",
                         "-o", str(old_binary)])
            if result["exit_code"]:
                raise RuntimeError("Prior production source reproduction failed to build")
            result = run("baseline-refused-swap", [str(old_binary)])
            reproduced = result["exit_code"] == 1 and "target entered=0; Current remains source=0" in result["output"]
            okay = okay and reproduced
            report["baseline_ignored_swap_failure_reproduced"] = reproduced

            target_object = work / "fiber-prospero-target.o"
            database = repo / "out/prospero-current/compile_commands.json"
            target_entry = None
            if database.is_file():
                target_entry = next((entry for entry in json.loads(database.read_text())
                                     if entry["file"].endswith(("/src/core/fiber_posix.cpp", "/src/core/fiber_prospero.cpp"))), None)
            if target_entry:
                original = target_entry.get("arguments") or shlex.split(target_entry["command"])
                command, skip = [], False
                for item in original:
                    if skip:
                        skip = False
                        continue
                    if item in ("-o", "-MF", "-MT", "-MQ"):
                        skip = True
                        continue
                    if item in ("-MD", "-MMD", "-MP"):
                        continue
                    command.append(str(source) if item == target_entry["file"] else item)
                command += ["-o", str(target_object)]
                target_cwd = Path(target_entry["directory"])
                report["target_compile_origin"] = "Actual core target entry from compile_commands.json, output redirected and source replaced with dedicated PS5 backend."
            else:
                target_sdk = args.target_sdk.resolve()
                command = [args.clangxx, "--target=x86_64-sie-ps5", "-std=c++23", "-O2", "-fno-plt",
                           "-femulated-tls", "-fvisibility=hidden", "-fno-visibility-from-dllstorageclass",
                           "-fno-stack-protector", "-D__PROSPERO__=1", "-I" + str(repo / "tools/rexglue-sdk/include"),
                           "-I" + str(target_sdk / "target/include/c++/v1"),
                           "-I" + str(target_sdk / "target/include"), "-I" + str(platform / "include"),
                           "-c", str(source), "-o", str(target_object)]
                target_cwd = repo
                report["target_compile_origin"] = "Explicit PS5 Clang18 target with the matched native SDK headers."
            result = run("ps5-target-compile", command, target_cwd)
            if result["exit_code"]:
                raise RuntimeError("Native target object failed to compile")
            result = run("ps5-target-undefined", ["nm", "-u", str(target_object)])
            forbidden = {"getcontext", "makecontext", "swapcontext", "setcontext"}
            referenced = {line.split()[-1] for line in result["output"].splitlines() if line.split()}
            report["native_context_imports_absent"] = not forbidden.intersection(referenced)
            okay = okay and result["exit_code"] == 0 and report["native_context_imports_absent"]
            result = run("ps5-target-switch-disassembly", ["objdump", "-d", "--disassemble=prospero_fiber_switch", str(target_object)])
            okay = okay and result["exit_code"] == 0
            report["target_object_sha256"] = sha(target_object)
            if not database.is_file():
                raise RuntimeError("Actual compile database is required to compile the two caller failure paths")
            entries = json.loads(database.read_text())
            for caller in callers:
                entry = next((entry for entry in entries if Path(entry["file"]).resolve() == caller), None)
                if entry is None:
                    raise RuntimeError("No target compile command for " + key(caller))
                original = entry.get("arguments") or shlex.split(entry["command"])
                command, skip = [], False
                for item in original:
                    if skip:
                        skip = False
                        continue
                    if item in ("-o", "-MF", "-MT", "-MQ"):
                        skip = True
                        continue
                    if item in ("-MD", "-MMD", "-MP"):
                        continue
                    command.append(item)
                command += ["-o", str(work / (caller.stem + "-caller-target.o"))]
                result = run("ps5-target-caller-" + caller.stem, command, Path(entry["directory"]))
                if result["exit_code"]:
                    raise RuntimeError("Caller failure propagation failed to compile: " + key(caller))
            report["caller_failure_paths_validation"] = "Both full production translation units compile using their actual PS5 target commands. Caller failure branches were reviewed for guest allocation cleanup and no invalid registration; they were not executed on a PS5."
        report["sources_unchanged_during_tests"] = all(sha(path) == hashes[key(path)] for path in inputs)
        okay = okay and report["sources_unchanged_during_tests"]
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        report["error"] = str(error)
        okay = False
    report["result"] = "PASS" if okay else "FAIL"
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    print("RESULT", report["result"], report_path, flush=True)
    return 0 if okay else 1


if __name__ == "__main__":
    raise SystemExit(main())
