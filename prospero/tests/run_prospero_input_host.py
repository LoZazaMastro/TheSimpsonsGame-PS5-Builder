#!/usr/bin/env python3
"""Run the complete Prospero driver behind production XAM/InputSystem methods.

Native pad/user services and inactive UI/injection are fixtures. Only unused
kernel/window umbrella headers and logging are replaced; production driver,
input class declarations, endian fields and native struct layouts are compiled.
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
    parser.add_argument("--cxx", default="c++")
    parser.add_argument("--baseline-commit")
    parser.add_argument("--diagnostic-log", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / "tools/rexglue-sdk"
    driver_relative = "tools/rexglue-sdk/src/input/prospero/prospero_input_driver.cpp"
    def read(relative):
        if args.baseline_commit:
            return subprocess.check_output(["git", "show", args.baseline_commit + ":" + relative], cwd=root)
        return (root / relative).read_bytes()
    driver = read(driver_relative)
    xam = read("tools/rexglue-sdk/src/kernel/xam/xam_input.cpp").decode()
    system = read("tools/rexglue-sdk/src/input/input_system.cpp").decode()
    report = {
        "status": "RUNNING", "hardware_validated": False,
        "scope": "Complete production Prospero driver and selected production XAM/InputSystem methods; native pad services and atomic framegen request callback simulated",
        "not_tested_here": "Physical DualSense polling or actual renderer response to a session toggle",
        "baseline_commit": args.baseline_commit,
        "source_sha256": {
            driver_relative: hashlib.sha256(driver).hexdigest(),
            "tools/rexglue-sdk/src/kernel/xam/xam_input.cpp": hashlib.sha256(xam.encode()).hexdigest(),
            "tools/rexglue-sdk/src/input/input_system.cpp": hashlib.sha256(system.encode()).hexdigest(),
        },
    }
    for path in [Path(__file__), Path(__file__).with_name("prospero_input_host.cpp"),
                 sdk / "include/rex/input/prospero/prospero_input_driver.h",
                 sdk / "include/rex/input/prospero/pad_mapping.h",
                 sdk / "include/rex/input/prospero/pad_abi.h",
                 sdk / "include/rex/platform/framegen_prospero.h"]:
        report["source_sha256"][str(path.relative_to(root))] = hashlib.sha256(path.read_bytes()).hexdigest()
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    for label, contents in [("driver", driver), ("XAM", xam.encode()), ("InputSystem", system.encode())]:
        print(label + " SHA256: " + hashlib.sha256(contents).hexdigest(), flush=True)
    methods = ["InputSystem::InputSystem(", "X_STATUS InputSystem::Setup()",
               "void InputSystem::Shutdown()", "void InputSystem::AddDriver(",
               "X_RESULT InputSystem::GetCapabilities(", "X_RESULT InputSystem::GetState(",
               "X_RESULT InputSystem::SetState(", "X_RESULT InputSystem::GetKeystroke("]
    system_methods = "InputSystem::~InputSystem() = default;\n" + "\n".join(function(system, s) for s in methods)
    xam_methods = xam[xam.index("constexpr uint32_t XINPUT_FLAG_GAMEPAD"):
                      xam.index("rex::input::InputSystem* input_system()")]
    xam_methods += "\n".join(function(xam, s) for s in [
        "u32 XamInputGetCapabilities_entry(", "u32 XamInputGetCapabilitiesEx_entry(",
        "u32 XamInputGetState_entry(", "u32 XamInputSetState_entry(", "u32 XamInputGetKeystroke_entry("])
    with tempfile.TemporaryDirectory(prefix="simpsons-prospero-input-host-") as temporary:
        work = Path(temporary)
        (work / "rex/ui").mkdir(parents=True)
        (work / "rex/kernel.h").write_text("#pragma once\n#include <rex/system/xtypes.h>\n")
        (work / "rex/ui/window.h").write_text("#pragma once\nnamespace rex::ui { class Window; }\n")
        (work / "rex/logging.h").write_text("#pragma once\n#define REXLOG_WARN(...)\n#define REXLOG_ERROR(...)\n#define REXLOG_INFO(...)\n")
        (work / "driver.cpp").write_bytes(driver)
        (work / "input_system_methods.inc").write_text(system_methods)
        (work / "xam_input_methods.inc").write_text(xam_methods)
        binary = work / "prospero_input_host"
        subprocess.run([args.cxx, "-std=c++23", "-O2", "-DNDEBUG", "-g", "-pthread",
                        "-fsanitize=undefined", "-fno-sanitize-recover=all", "-DREX_PLATFORM_PROSPERO=1",
                        "-D__PROSPERO__=1",
                        "-I" + str(work), "-I" + str(sdk / "include"),
                        str(work / "driver.cpp"), str(Path(__file__).with_name("prospero_input_host.cpp")),
                        str(sdk / "src/core/trace_prospero.cpp"),
                        "-o", str(binary)], check=True)
        log = args.diagnostic_log or work / "input.log"
        log.parent.mkdir(parents=True, exist_ok=True)
        result = subprocess.run([str(binary), str(log)], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        report["exit_code"] = result.returncode
        report["output"] = output.splitlines()
        print(output, end="", flush=True)
        if args.baseline_commit:
            failures = ["XAM ANY_USER capabilities reaches the initial user's pad",
                        "XAM ANY_USER plus GAMEPAD capabilities succeeds",
                        "XAM capabilities Ex accepts ANY_USER",
                        "XAM capabilities Ex accepts ANY_USER plus GAMEPAD"]
            if result.returncode != 1 or any("FAIL " + message not in output for message in failures):
                raise RuntimeError("baseline did not reproduce all four ANY_USER capability failures")
            print("PASS baseline: four capability selector failures reproduced", flush=True)
        elif result.returncode:
            raise RuntimeError("production input chain failed: " + str(result.returncode))
        elif not args.baseline_commit:
            content = log.read_text()
            phases = {int(line.split("phase=", 1)[1].split()[0], 0)
                      for line in content.splitlines() if "trace-checkpoint" in line and "phase=" in line}
            expected = {500, 501, 502, 503, 504, 505, 506, 507, 569}
            if not expected.issubset(phases):
                raise RuntimeError("Missing native input diagnostic phases: " + str(expected - phases))
            analog = [line for line in content.splitlines() if "trace-checkpoint" in line and "phase=504 " in line]
            print("PASS native input operational diagnostics: " + json.dumps({"phases": sorted(phases), "analog_lines": len(analog)}), flush=True)
            report["diagnostic_phases"] = sorted(phases)
            report["diagnostic_log_sha256"] = hashlib.sha256(log.read_bytes()).hexdigest()
            report["checks"] = {name: {"checks": int(total), "failures": int(failed)}
                for name, total, failed in re.findall(r"PASS ([^\n]+): (\d+) checks, (\d+) failures", output)}
        report["status"] = "PASS"
        if args.report:
            args.report.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
