#!/usr/bin/env python3
"""Reproduce and verify the real PS5 ReX thread adapter on host pthreads."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import time

BASELINE = "8a93bd1ca0da475716f953b39e1a19155b52af84"
SOURCE = "tools/rexglue-sdk/src/core/threading_posix.cpp"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", type=Path, required=True)
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--baseline-commit", default=BASELINE)
    parser.add_argument("--compile-commands", type=Path)
    parser.add_argument("--no-leak-check", action="store_true",
                        help="Disable only LeakSanitizer when this runner cannot enumerate /proc tasks")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / "tools/rexglue-sdk"
    report_path = root / "prospero/validation/thread-lifecycle-full-audit.json"
    captures = root / "prospero/validation/thread-lifecycle-full-audit-captures"
    captures.mkdir(parents=True, exist_ok=True)
    hashes = {p: sha(root / p) for p in [SOURCE,
        "tools/rexglue-sdk/src/core/threading.cpp", "tools/rexglue-sdk/include/rex/thread.h",
        "prospero/tests/thread_lifecycle_host.cpp", "prospero/tests/run_thread_lifecycle_host.py"]}
    results = []

    def run(name, command, timeout=90, expected=0):
        started = time.monotonic()
        environment = dict(os.environ)
        if "asan" in name:
            environment["ASAN_OPTIONS"] = ("detect_leaks=0:" if args.no_leak_check else "detect_leaks=1:") + "halt_on_error=1"
            environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
        result = subprocess.run(command, cwd=root, capture_output=True, text=True, timeout=timeout,
                                env=environment)
        capture = captures / (name + ".txt")
        capture.write_text(result.stdout + result.stderr + f"\nEXIT_CODE={result.returncode}\n")
        results.append({"name": name, "command": command, "exit_code": result.returncode,
                        "expected_exit_code": expected,
                        "elapsed_seconds": round(time.monotonic() - started, 3),
                        "capture": str(capture.relative_to(root)), "capture_sha256": sha(capture)})
        print(f"{name}: exit {result.returncode} expected {expected}", flush=True)
        if result.returncode != expected:
            print((result.stdout + result.stderr)[-6000:], flush=True)
            raise RuntimeError(f"{name} failed")
        return result.stdout + result.stderr

    with tempfile.TemporaryDirectory(prefix="simpsons-thread-lifecycle-") as directory:
        work = Path(directory)
        compat = work / "compat"
        compat.mkdir()
        # Host declarations use different *_name_np spellings and pthread_t.
        # Replace only those adapters; compile the production PS5 branch intact.
        (compat / "preinclude.h").write_text("""#pragma once
#define pthread_getname_np host_pthread_getname_np
#define pthread_setname_np host_pthread_setname_np
#include <pthread.h>
#undef pthread_getname_np
#undef pthread_setname_np
""")
        (compat / "pthread_np.h").write_text("""#pragma once
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
static inline void pthread_get_name_np(pthread_t, char* out, size_t bytes) {
  if (bytes) out[0] = 0;
}
static inline void pthread_set_name_np(pthread_t, const char*) {}
static inline int pthread_getthreadid_np() { return int(syscall(SYS_gettid)); }
""")
        baseline = work / "baseline-threading.cpp"
        baseline.write_bytes(subprocess.check_output(
            ["git", "show", args.baseline_commit + ":" + SOURCE], cwd=root))
        baseline_sha = sha(baseline)
        includes = [compat, args.sdk_root.resolve() / "platform/include", sdk / "include",
                    sdk / "thirdparty", *[sdk / "thirdparty" / p for p in [
                        "fmt/include", "spdlog/include", "cli11/include", "tomlplusplus/include", "simde"]]]
        flags = ["-std=c++23", "-O1", "-g", "-pthread", "-msse4.1", "-fno-omit-frame-pointer",
                 "-ffunction-sections", "-fdata-sections", "-DNDEBUG", "-D__PROSPERO__=1",
                 "-DSPDLOG_FMT_EXTERNAL", "-DFMT_HEADER_ONLY", "-include", str(compat / "preinclude.h"),
                 *["-I" + str(p) for p in includes]]
        wraps = ["sem_init", "sem_wait", "sem_post", "sem_destroy", "pthread_join", "pthread_kill",
                 "pthread_create", "pthread_getschedparam", "pthread_setschedparam", "pthread_attr_setschedparam"]
        support = [str(sdk / "src/core/threading.cpp"),
                   str(root / "prospero/tests/thread_lifecycle_host.cpp")]
        for variant, source in [("baseline", baseline), ("baseline-debug", baseline),
                                ("fixed", root / SOURCE), ("fixed-asan", root / SOURCE),
                                ("fixed-debug", root / SOURCE)]:
            sanitizer = ["-fsanitize=address,undefined"] if variant == "fixed-asan" else []
            binary = work / variant
            variant_flags = [flag for flag in flags if not (variant.endswith("debug") and flag == "-DNDEBUG")]
            run("compile-" + variant, [args.cxx, *variant_flags, *sanitizer, str(source), *support,
                "-Wl,--gc-sections", *["-Wl,--wrap=" + name for name in wraps], "-o", str(binary)])
            if variant.endswith("debug"):
                cases = ["reused-identity"]
            else:
                cases = ["lifecycle", "scheduling"] if variant == "baseline" else [
                    "lifecycle", "nested", "concurrent", "scheduling", "failures", "exit",
                    "terminate-suspended", "reused-identity"]
            for case in cases:
                output = run(variant + "-" + case, [str(binary), case, *(["baseline"] if variant == "baseline" else [])],
                             15, -6 if variant == "baseline-debug" else 0)
                if variant == "baseline-debug" and not all(marker in output for marker in [
                        "OBSERVED actual pthread handle reuse", "Terminate", "Assertion"]):
                    raise RuntimeError("Baseline debug did not identify the reused-handle self-detection defect")

        if args.compile_commands:
            commands = json.loads(args.compile_commands.read_text())
            entry = next(x for x in commands if x["file"] == str(root / SOURCE))
            command = shlex.split(entry["command"])
            output_index = command.index("-o") + 1
            command[output_index] = str(work / "threading-prospero-target.o")
            run("target-cross-compile", command)
            target_sha = sha(work / "threading-prospero-target.o")
        else:
            target_sha = None

    unchanged = all(sha(root / p) == h for p, h in hashes.items())
    report = {
        "result": "PASS" if unchanged else "SOURCE_CHANGED_DURING_TEST",
        "scope": "Real production PS5 branch with Linux pthreads and explicit name/affinity/scheduling adapters",
        "baseline_commit": args.baseline_commit, "baseline_source_sha256": baseline_sha,
        "reviewed_source_sha256": hashes, "sources_unchanged_during_test": unchanged,
        "reproduced_baseline_defects": [
            "Initial resume deposits a semaphore token while startup waits on a different condition; next suspend returns early",
            "Repeated waits join the same native pthread and destroy its semaphore again",
            "Desktop priority values are outside the SDK-probed PS5 range and raw native priority leaks into logical query",
            "With a truly recycled native handle, Debug Terminate mistakes a finished ReX object for the current thread and asserts; Release failure was not reproduced"],
        "fixed_checks": ["Initial gate and explicit suspension", "Nested suspend counts", "Interrupted semaphore retry",
                         "128 rapid suspend/resume pairs", "Rejected signal rollback",
                         "Eight simultaneous waiters making sixteen waits with one native teardown",
                         "Finished-thread suspend/resume/APC/affinity guards", "Inherited native scheduling",
                         "Logical priority preservation", "Semaphore/thread-creation failure ownership",
                         "Explicit Thread::Exit followed by repeated waits",
                         "External Terminate in initial gate and signal suspension followed by repeated waits",
                         "Actual pthread handle reuse does not replace logical current-thread identity"],
        "asan_ubsan": "PASS for all eight fixed cases",
        "leaksan": "DISABLED explicitly because /proc task enumeration fails in this runner" if args.no_leak_check else "ENABLED",
        "target_object_sha256": target_sha, "captures": results,
        "limits": ["No v3.6 or changed build has run on Andrea's PS5",
                   "Host tests do not reproduce the console allocator abort or establish its writer",
                   "The PS5 scheduling range is modeled from the SDK-linked public port evidence; it is not measured by this harness",
                   "Asynchronous cancellation of arbitrary running guest threads remains an inherited runtime risk",
                   "No guarantee of deterministic real-time signal delivery or 60 FPS follows from these tests"]
    }
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    print(report["result"], str(report_path), flush=True)
    if not unchanged:
        raise RuntimeError("Sources changed during test")


if __name__ == "__main__":
    main()
