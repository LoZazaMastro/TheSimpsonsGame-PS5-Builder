#!/usr/bin/env python3
"""Verify ReX thread ownership against the actual local direct-stack wrapper.

The SDK host kernel implements mapping/allocation. Linux supplies pthreads;
name/affinity adapters do not establish PS5 signal, cancellation or kernel ABI.
Each runtime case is a fresh process. The exact delivered v3.8 source is also
compiled, to demonstrate the defects rather than infer them from a fixed PASS.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import resource
import shlex
import signal
import subprocess
import tempfile
import time

BASELINE = "4ae0aebca81d34233f4cffce8720dc51922f0baf"
SOURCE = "tools/rexglue-sdk/src/core/threading_posix.cpp"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", type=Path, required=True)
    parser.add_argument("--cc", default="cc")
    parser.add_argument("--cxx", default="c++")
    parser.add_argument("--baseline-commit", default=BASELINE)
    parser.add_argument("--compile-commands", type=Path)
    parser.add_argument("--no-leak-check", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = args.sdk_root.resolve() / "platform"
    rex = root / "tools/rexglue-sdk"
    captures = root / "prospero/validation/thread-ownership-v3.9-captures"
    report_path = root / "prospero/validation/thread-ownership-v3.9.json"
    captures.mkdir(parents=True, exist_ok=True)
    inputs = [SOURCE, "tools/rexglue-sdk/src/core/threading.cpp",
              "tools/rexglue-sdk/include/rex/thread.h",
              "tools/rexglue-sdk/src/core/signal_thread_prospero.cpp",
              "tools/rexglue-sdk/include/rex/platform/signal_thread_prospero.h",
              "tools/rexglue-sdk/src/core/trace_prospero.cpp",
              "tools/rexglue-sdk/include/rex/platform/trace_phases_prospero.h",
              "prospero/runtime/threads_prospero.c", "prospero/runtime/cxa_prospero.c",
              "prospero/tests/thread_ownership_host.cpp",
              "prospero/tests/thread_lifecycle_host.cpp",
              "prospero/tests/run_thread_ownership_host.py"]
    hashes = {p: sha(root / p) for p in inputs}
    sdk_inputs = ["tests/host_kernel.c", "tests/host_kernel.h", "src/placement.c",
                  "src/placement.h", "include/ps5platform/kernel.h",
                  "include/ps5platform/libc.h"]
    sdk_hashes = {p: sha(sdk / p) for p in sdk_inputs}
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    results = []
    report = {
        "result": "IN_PROGRESS", "hardware": "NOT_RUN",
        "baseline_commit": args.baseline_commit,
        "reviewed_source_sha256": hashes, "sdk_source_sha256": sdk_hashes,
        "scope": "Actual ReX PS5 adapter + local direct-stack/cxa runtime + SDK host mapping model + Linux pthreads",
        "adapters": ["Linux spelling of pthread name/id functions", "PS5 affinity calls return a modeled CPU mask",
                     "A wrapper blocks/fails real sigaction for controlled publication interleavings",
                     "A wrapper counts/fails native pthread_create, otherwise calls real Linux pthread_create",
                     "A one-shot wrapper returns EINVAL from the native pthread_join to verify the wait boundary fails closed",
                     "The prior lifecycle fixture models native scheduling and selected error paths"],
        "limits": ["No console execution or GPU/gameplay performance is demonstrated",
                   "These reproductions do not identify the writer of the user's allocator abort",
                   "Cancellation in an arbitrary guest/driver critical section remains an inherited runtime risk",
                   "Pre-existing native GetCurrentThread facades retain the historical allocation policy"],
        "captures": results,
        "cleanup_variants": ["C++ exceptions enabled: Linux pthread cleanup RAII",
                             "-fno-exceptions: native C pthread cleanup registration, without C++ stack unwinding",
                             "Exact target compile: PS5 native cleanup imports; no target execution"],
    }

    def save():
        report_path.write_text(json.dumps(report, indent=2) + "\n")

    def run(name, command, timeout=90, expected=0, cwd=None):
        started = time.monotonic()
        environment = dict(os.environ)
        if "asan" in name:
            environment["ASAN_OPTIONS"] = ("detect_leaks=0:" if args.no_leak_check else "detect_leaks=1:") + "halt_on_error=1"
            environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
        try:
            result = subprocess.run([str(x) for x in command], cwd=cwd or root,
                                    capture_output=True, text=True, timeout=timeout,
                                    env=environment)
            output = result.stdout + result.stderr
            code = result.returncode
        except subprocess.TimeoutExpired as error:
            output = (error.stdout or b"") + (error.stderr or b"")
            if isinstance(output, bytes):
                output = output.decode(errors="replace")
            code = "TIMEOUT"
        capture = captures / (name + ".txt")
        capture.write_text(output + f"\nEXIT_CODE={code}\n")
        results.append({"name": name, "command": [str(x) for x in command],
                        "exit_code": code, "expected_exit_code": expected,
                        "elapsed_seconds": round(time.monotonic() - started, 3),
                        "capture": str(capture.relative_to(root)), "capture_sha256": sha(capture)})
        save()
        print(f"{name}: exit {code} expected {expected}", flush=True)
        if code != expected:
            print(output[-7000:], flush=True)
            raise RuntimeError(name + " failed")
        return output

    try:
        with tempfile.TemporaryDirectory(prefix="simpsons-thread-ownership-") as temporary:
            work = Path(temporary)
            compat = work / "compat"
            compat.mkdir()
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
static inline void pthread_get_name_np(pthread_t, char* out, size_t bytes) { if (bytes) out[0] = 0; }
static inline void pthread_set_name_np(pthread_t, const char*) {}
static inline int pthread_getthreadid_np() { return int(syscall(SYS_gettid)); }
""")
            baseline = work / "baseline-threading.cpp"
            baseline.write_bytes(subprocess.check_output(
                ["git", "show", args.baseline_commit + ":" + SOURCE], cwd=root))
            report["baseline_source_sha256"] = sha(baseline)
            includes = [compat, sdk / "include", sdk / "src", sdk / "tests", rex / "include", rex / "thirdparty",
                        *[rex / "thirdparty" / p for p in ["fmt/include", "spdlog/include",
                          "cli11/include", "tomlplusplus/include", "simde"]]]
            cflags = ["-std=c11", "-pthread", "-O1", "-g", "-ffunction-sections", "-fdata-sections",
                      *["-I" + str(p) for p in includes]]
            flags = ["-std=c++23", "-O1", "-g", "-pthread", "-msse4.1", "-fno-omit-frame-pointer",
                     "-ffunction-sections", "-fdata-sections", "-DNDEBUG", "-D__PROSPERO__=1",
                     "-DSPDLOG_FMT_EXTERNAL", "-DFMT_HEADER_ONLY", "-include", str(compat / "preinclude.h"),
                     *["-I" + str(p) for p in includes]]
            support = [rex / "src/core/threading.cpp", rex / "src/core/signal_thread_prospero.cpp",
                       rex / "src/core/trace_prospero.cpp"]
            fixed_cases = ["install", "install-signal", "install-failure", "close-finished", "close-running",
                           "self-close", "close-exit", "terminate-close", "terminate-wait",
                           "create-failure", "native-fallback", "terminate-locked-state", "join-failure"]
            report["fixed_cases"] = fixed_cases
            for variant in ["baseline", "fixed", "fixed-asan", "fixed-native-cleanup"]:
                sanitizer = ["-fsanitize=address,undefined"] if "asan" in variant else []
                cleanup_flags = ["-fno-exceptions", "-DSPDLOG_NO_EXCEPTIONS"] if "native-cleanup" in variant else []
                objects = []
                sources = [sdk / "tests/host_kernel.c", sdk / "src/placement.c",
                           root / "prospero/runtime/cxa_prospero.c", root / "prospero/runtime/threads_prospero.c"]
                for source in sources:
                    obj = work / (variant + "-" + source.stem + ".o")
                    rename = ["-D__real_pthread_create=ownership_native_create",
                              "-D__real_pthread_join=ownership_native_join"] if source.name == "threads_prospero.c" else []
                    if source.name == "host_kernel.c":
                        rename = ["-DscePthreadGetaffinity=unused_host_getaffinity",
                                  "-DscePthreadSetaffinity=unused_host_setaffinity"]
                    run("compile-" + variant + "-" + source.stem,
                        [args.cc, *cflags, *sanitizer, *rename, "-c", source, "-o", obj])
                    objects.append(obj)
                binary = work / variant
                run("compile-" + variant,
                    [args.cxx, *flags, *sanitizer, *cleanup_flags, baseline if variant == "baseline" else root / SOURCE,
                     *support, root / "prospero/tests/thread_ownership_host.cpp", *objects,
                     "-Wl,--gc-sections,--wrap=pthread_create,--wrap=pthread_join,--wrap=pthread_detach",
                     "-Wl,--wrap=sigaction,--wrap=sem_destroy,--wrap=pthread_cancel,--wrap=pthread_mutex_lock",
                     "-Wl,--wrap=pthread_mutex_unlock", "-o", binary], timeout=180)
                cases = ["install", "install-signal", "close-finished", "join-failure"] if variant == "baseline" else fixed_cases
                for case in cases:
                    expected = -signal.SIGRTMIN if variant == "baseline" and case == "install-signal" else 0
                    if variant != "baseline" and case == "join-failure": expected = -signal.SIGABRT
                    output = run(variant + "-" + case,
                        [binary, case, *(["baseline"] if variant == "baseline" else [])], timeout=25,
                        expected=expected)
                    if case == "join-failure":
                        marker = "failed_join_reported_success=1" if variant == "baseline" else "PS5 native thread join failed: error 22"
                        if marker not in output:
                            raise RuntimeError("Native join failure lacked the expected boundary diagnostic")

            # Re-run the old meaningful lifecycle coverage against this exact
            # new adapter and signal registry, without modifying its old report.
            lifecycle_wraps = ["sem_init", "sem_wait", "sem_post", "sem_destroy", "pthread_join", "pthread_kill",
                               "pthread_create", "pthread_getschedparam", "pthread_setschedparam", "pthread_attr_setschedparam"]
            lifecycle_cases = ["lifecycle", "nested", "concurrent", "scheduling", "failures", "exit",
                               "terminate-suspended", "reused-identity"]
            report["prior_lifecycle_cases_rerun"] = lifecycle_cases
            for variant in ["lifecycle", "lifecycle-asan", "lifecycle-native-cleanup"]:
                sanitizer = ["-fsanitize=address,undefined"] if "asan" in variant else []
                cleanup_flags = ["-fno-exceptions", "-DSPDLOG_NO_EXCEPTIONS"] if "native-cleanup" in variant else []
                cxa = work / (variant + "-cxa.o")
                run("compile-" + variant + "-cxa", [args.cc, *cflags, *sanitizer, "-c",
                    root / "prospero/runtime/cxa_prospero.c", "-o", cxa])
                binary = work / variant
                run("compile-" + variant, [args.cxx, *flags, *sanitizer, *cleanup_flags, root / SOURCE,
                    *support, root / "prospero/tests/thread_lifecycle_host.cpp", cxa,
                    "-Wl,--gc-sections", *["-Wl,--wrap=" + name for name in lifecycle_wraps],
                    "-o", binary], timeout=180)
                for case in lifecycle_cases:
                    run(variant + "-" + case, [binary, case], timeout=25)

            if args.compile_commands:
                commands = json.loads(args.compile_commands.read_text())
                entry = next(x for x in commands if Path(x["file"]).resolve() == root / SOURCE)
                command = entry.get("arguments") or shlex.split(entry["command"])
                target = work / "threading-prospero-target.o"
                command[command.index("-o") + 1] = str(target)
                run("target-cross-compile", command, timeout=180, cwd=entry["directory"])
                report["target_object_sha256"] = sha(target)

        unchanged = all(sha(root / p) == h for p, h in hashes.items())
        report.update(result="PASS" if unchanged else "SOURCE_CHANGED_DURING_TEST",
                      sources_unchanged_during_test=unchanged,
                      asan_ubsan="PASS", leak_sanitizer="DISABLED" if args.no_leak_check else "ENABLED")
        save()
        if not unchanged:
            raise RuntimeError("Sources changed during test; rerun after those inputs are stable")
    except Exception as error:
        report["result"] = "FAIL"
        report["error"] = str(error)
        save()
        raise
    print("PASS " + str(report_path), flush=True)


if __name__ == "__main__":
    main()
