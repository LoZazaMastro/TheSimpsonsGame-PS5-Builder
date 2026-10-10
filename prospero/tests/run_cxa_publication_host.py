#!/usr/bin/env python3
"""Reproduce SDK readiness race, verify local provider, TLS coupling and PS5 ABI."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import time


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", type=Path, required=True)
    parser.add_argument("--cc", default="cc", help="Host C compiler with working TSAN runtime")
    parser.add_argument("--clang", default="clang", help="C compiler for the emutls integration test")
    parser.add_argument("--clangxx", default="clang++")
    parser.add_argument("--compile-commands", type=Path, required=True)
    parser.add_argument("--no-leak-check", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = args.sdk_root.resolve()
    original = sdk / "platform/src/cxa.c"
    local = root / "prospero/runtime/cxa_prospero.c"
    fixture = root / "prospero/tests/cxa_publication_host.c"
    files = [original, sdk / "platform/include/ps5platform/libc.h", local, fixture,
             root / "prospero/runtime/emutls.c", root / "prospero/tests/emutls_host_test.cpp",
             root / "prospero/tests/run_emutls_host.py", Path(__file__).resolve()]
    frozen = {str(p): sha(p) for p in files}
    captures = root / "prospero/validation/cxa-publication-full-audit-captures"
    captures.mkdir(parents=True, exist_ok=True)
    results = []

    def run(name, command, expected=0, timeout=60, tsan=False):
        environment = dict(os.environ)
        if tsan:
            environment["TSAN_OPTIONS"] = "halt_on_error=1:exitcode=66"
        start = time.monotonic()
        result = subprocess.run(command, cwd=root, text=True, capture_output=True,
                                env=environment, timeout=timeout)
        output = result.stdout + result.stderr
        capture = captures / (name + ".txt")
        capture.write_text(shlex.join(command) + "\n" + output + f"\nEXIT_CODE={result.returncode}\n")
        results.append({"name": name, "command": command, "exit_code": result.returncode,
                        "expected_exit_code": expected, "capture": str(capture.relative_to(root)),
                        "capture_sha256": sha(capture),
                        "elapsed_seconds": round(time.monotonic() - start, 3)})
        print(f"{name}: exit {result.returncode} expected {expected}", flush=True)
        if result.returncode != expected:
            print(output[-6000:], flush=True)
            raise RuntimeError(name + " failed")
        return output

    with tempfile.TemporaryDirectory(prefix="simpsons-cxa-publication-") as directory:
        work = Path(directory)
        flags = ["-std=c11", "-O1", "-g", "-pthread", "-fsanitize=thread",
                 "-fno-omit-frame-pointer", "-D_GNU_SOURCE", "-I", str(sdk / "platform/include")]
        for variant, source, expected in [("sdk-baseline", original, 66), ("local-atomic", local, 0)]:
            binary = work / variant
            run("compile-" + variant, [args.cc, *flags, str(source), str(fixture), "-o", str(binary)])
            output = run("run-" + variant, [str(binary)], expected, tsan=True)
            if variant == "sdk-baseline":
                for marker in ["ThreadSanitizer: data race", "destructors_ready", "setup_destructors", "ps5p_run_thread_destructors"]:
                    if marker not in output:
                        raise RuntimeError("Baseline did not identify expected SDK publication race: " + marker)
            elif "PASS first-use publication" not in output:
                raise RuntimeError("Local registry functionality did not complete")

        emutls_command = ["python3", str(root / "prospero/tests/run_emutls_host.py"),
                          "--sdk-source", str(sdk), "--registry-source", str(local),
                          "--cc", args.clang, "--cxx", args.clangxx]
        if args.no_leak_check:
            emutls_command.append("--no-leak-check")
        run("emutls-local-provider-asan-ubsan", emutls_command, timeout=180)

        commands = json.loads(args.compile_commands.read_text())
        entry = next(x for x in commands if x["file"] == str(root / "prospero/runtime/emutls.c"))
        base_command = shlex.split(entry["command"])
        native_objects = {}
        for variant, source in [("sdk-baseline", original), ("local-atomic", local)]:
            command = list(base_command)
            command[command.index(entry["file"])] = str(source)
            native_object = work / (variant + "-target.o")
            command[command.index("-o") + 1] = str(native_object)
            run("target-compile-" + variant, command)
            nm = str(Path(command[0]).with_name("llvm-nm"))
            symbols = run("target-symbols-" + variant, [nm, "-g", str(native_object)])
            if "__atomic_" in symbols or "__sync_" in symbols:
                raise RuntimeError("Unexpected out-of-line target atomic dependency")
            native_objects[variant] = {"sha256": sha(native_object), "global_symbols": symbols}
        def defined_symbols(text):
            return sorted(line.split()[-1] for line in text.splitlines()
                          if len(line.split()) >= 3 and line.split()[-2] != "U")
        if defined_symbols(native_objects["sdk-baseline"]["global_symbols"]) != defined_symbols(native_objects["local-atomic"]["global_symbols"]):
            raise RuntimeError("Local registry changed exported C provider symbols")

    unchanged = all(sha(Path(path)) == value for path, value in frozen.items())
    report = {
        "schema_version": 1,
        "result": "PASS" if unchanged else "SOURCE_CHANGED_DURING_TEST",
        "scope": "Actual SDK baseline and local C provider with real Linux pthread_once/keys under TSAN; actual emutls integration under ASan/UBSan; actual PS5 object compilation. No console execution.",
        "sdk_revision": subprocess.check_output(["git", "-C", str(sdk), "rev-parse", "HEAD"], text=True).strip(),
        "source_sha256": frozen,
        "sources_unchanged_during_test": unchanged,
        "baseline_observation": "TSAN reports SDK setup_destructors write of plain destructors_ready racing with ps5p_run_thread_destructors read on a thread that never registered a destructor.",
        "fix": "One static atomic_bool; release publication after key/atexit initialization; acquire loads in the registry readiness checks. Static assertion requires always-lock-free bool. C ABI, callbacks, allocation routing and destructor algorithm preserved.",
        "functional_checks": ["First registration concurrent with unregistered-thread drain", "16 parallel threads with 16 nested registrations each", "Repeated explicit drain", "Native pthread-key exit", "Main-thread atexit ordering"],
        "emutls_coupling": "Existing emutls-first, registry-first and explicit-drain ASan/UBSan scenarios run against local cxa provider, including TLS array growth and destructors which register more work.",
        "leaksan": "DISABLED explicitly" if args.no_leak_check else "ENABLED",
        "target_objects": native_objects,
        "global_C_symbols_match_baseline": True,
        "no_out_of_line_target_atomic_dependency": True,
        "captures": results,
        "integration_owner": "Parent owns CMake and final ELF provider uniqueness checks; object validation does not claim final ELF linkage.",
        "limits": ["The race is real in the source and reproduced on host; no claim that it caused the console allocator abort.", "PS5 single-pass key ordering is covered by the existing emutls adaptation, but this harness executes native Linux pthread keys, not console firmware.", "No 4K/60FPS or game startup result follows from this test."]
    }
    output_path = root / "prospero/validation/cxa-publication-full-audit.json"
    output_path.write_text(json.dumps(report, indent=2) + "\n")
    print(report["result"], str(output_path), flush=True)
    if not unchanged:
        raise RuntimeError("Source changed during test")


if __name__ == "__main__":
    main()
