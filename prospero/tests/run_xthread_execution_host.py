#!/usr/bin/env python3
"""Prove PS5 XThread execution ownership without relying on automatic-object unwind."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import resource
import shlex
import subprocess
import tempfile


def extract(source, start, suffix=""):
    begin = source.index(start)
    opening = source.index("{", begin)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[begin:end] + suffix


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-source", type=Path, required=True)
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--baseline-commit", default="4ae0aebca81d34233f4cffce8720dc51922f0baf")
    parser.add_argument("--compile-commands", type=Path,
                        help="Also compile and inspect the exact configured PS5 XThread branches")
    parser.add_argument("--no-leak-check", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    rex = root / "tools/rexglue-sdk"
    sdk = args.sdk_source.resolve() / "platform"
    path = "tools/rexglue-sdk/src/system/xthread.cpp"
    current = (root / path).read_text()
    baseline = subprocess.check_output(["git", "show", args.baseline_commit + ":" + path], cwd=root, text=True)
    template = (root / "prospero/tests/xthread_execution_host.cpp.in").read_text()
    thread_state_header = (rex / "include/rex/system/thread_state.h").read_text().replace(
        "#include <rex/memory.h>", "namespace rex::memory { class Memory; }")
    object_header = (rex / "include/rex/system/xobject.h").read_text()
    object_ref = object_header[object_header.index("template <typename T>\nclass object_ref {"):
                               object_header.index("template <typename T>\nobject_ref<T> XObject::GetNativeObject")]
    template = template.replace("@THREAD_STATE_HEADER@", thread_state_header).replace("@OBJECT_REF@", object_ref)
    captures = root / "prospero/validation/xthread-execution-v3.9-captures"
    captures.mkdir(exist_ok=True)
    report_path = root / "prospero/validation/xthread-execution-v3.9.json"
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    results = []
    hashed = [path, "tools/rexglue-sdk/include/rex/system/xthread.h", "tools/rexglue-sdk/src/core/threading_posix.cpp",
              "tools/rexglue-sdk/include/rex/system/xobject.h", "tools/rexglue-sdk/src/system/thread_state.cpp",
              "tools/rexglue-sdk/include/rex/system/thread_state.h", "prospero/runtime/threads_prospero.c",
              "tools/rexglue-sdk/include/rex/ppc/context.h", "prospero/runtime/emutls.c",
              "prospero/runtime/cxa_prospero.c", "prospero/tests/xthread_execution_host.cpp.in",
              "prospero/tests/run_xthread_execution_host.py"]
    hashes = {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in hashed}
    environment = dict(os.environ)
    environment["ASAN_OPTIONS"] = f"detect_leaks={0 if args.no_leak_check else 1}:abort_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    report = {"result": "IN_PROGRESS", "hardware": "NOT_RUN", "sources_sha256": hashes,
              "baseline_commit": args.baseline_commit,
              "baseline_source_sha256": hashlib.sha256(baseline.encode()).hexdigest(),
              "tests": results,
              "scope": "Verbatim production execution token/XThread destructor/Exit/Terminate; real ReX thread cleanup and ThreadState/PPCContext; modeled guest table/refcounts and unused kernel operations",
              "limits": ["No native PS5 cancellation/unwind execution is established",
                         "Create/Restore owner wiring is source/target-compile checked; full guest allocation and save-state restoration are not executed",
                         "The syscall seam delays cancellation to prove ownership before its acknowledgment",
                         "Native C pthread cleanup (-fno-exceptions) is deliberate, eliminating a Linux C++ RAII-only assumption",
                         "ThreadState declaration and object_ref templates are extracted unchanged except an unused Memory include is forward declared; real ThreadState implementation is a separate TU",
                         "ps5_pthread_exit is extracted verbatim from the local runtime and linked to Linux pthread_exit"]}

    def run(label, command, timeout=60):
        result = subprocess.run([str(arg) for arg in command], cwd=root, env=environment,
                                capture_output=True, text=True, timeout=timeout)
        output = result.stdout + result.stderr
        capture = captures / (label + ".txt")
        capture.write_text(output + f"\nEXIT_CODE={result.returncode}\n")
        results.append({"case": label, "command": [str(arg) for arg in command], "exit_code": result.returncode,
                        "capture": str(capture.relative_to(root)), "capture_sha256": hashlib.sha256(capture.read_bytes()).hexdigest()})
        report_path.write_text(json.dumps(report, indent=2) + "\n")
        print(label, result.returncode, flush=True)
        if result.returncode:
            raise RuntimeError(output[-10000:])

    try:
        with tempfile.TemporaryDirectory(prefix="simpsons-xthread-execution-") as directory:
            work = Path(directory)
            compat = work / "compat"
            compat.mkdir()
            (compat / "preinclude.h").write_text("#pragma once\n#define pthread_getname_np host_pthread_getname_np\n#define pthread_setname_np host_pthread_setname_np\n#include <pthread.h>\n#undef pthread_getname_np\n#undef pthread_setname_np\n")
            (compat / "pthread_np.h").write_text("#pragma once\n#include <pthread.h>\n#include <sys/syscall.h>\n#include <unistd.h>\nstatic inline void pthread_get_name_np(pthread_t, char* p, size_t n) {if(n) p[0]=0;}\nstatic inline void pthread_set_name_np(pthread_t, const char*) {}\nstatic inline int pthread_getthreadid_np() { return int(syscall(SYS_gettid)); }\n")
            includes = [compat, sdk / "include", rex / "include", rex / "thirdparty",
                        *[rex / "thirdparty" / sub for sub in ["fmt/include", "spdlog/include", "tomlplusplus/include", "simde", "cli11/include"]]]
            common = ["-O1", "-g", "-pthread", "-ffunction-sections", "-fdata-sections", "-fno-omit-frame-pointer",
                      "-fsanitize=address,undefined", *["-I" + str(p) for p in includes]]
            flags = ["-std=c++23", *common, "-msse4.1", "-fno-exceptions", "-femulated-tls", "-DNDEBUG", "-D__PROSPERO__=1",
                     "-DSPDLOG_NO_EXCEPTIONS", "-DSPDLOG_FMT_EXTERNAL", "-DFMT_HEADER_ONLY", "-include", str(compat / "preinclude.h")]
            objects = []
            for source, extra in [(root / "prospero/runtime/emutls.c", ["-DPROSPERO_EMUTLS_HOST_TEST=1"]),
                                  (root / "prospero/runtime/cxa_prospero.c", [])]:
                obj = work / (source.stem + ".o")
                run("compile-" + source.stem, [args.cc, "-std=c11", "-D_GNU_SOURCE", *common, *extra, "-c", source, "-o", obj])
                objects.append(obj)
            exit_source = work / "ps5-exit.c"
            exit_source.write_text("#include <pthread.h>\nvoid ps5p_run_thread_destructors(void);\n" +
                                  extract((root / "prospero/runtime/threads_prospero.c").read_text(), "void\nps5_pthread_exit("))
            exit_object = work / "ps5-exit.o"
            run("compile-ps5-exit", [args.cc, "-std=c11", *common, "-c", exit_source, "-o", exit_object])
            objects.append(exit_object)
            thread_state_object = work / "thread-state.o"
            run("compile-thread-state", [args.cxx, *[flag for flag in flags if flag != "-fno-exceptions"],
                  "-c", rex / "src/system/thread_state.cpp", "-o", thread_state_object])
            objects.append(thread_state_object)
            for label, source in [("baseline", baseline), ("fixed", current)]:
                cpp = template.replace("@EXECUTION_OWNER@", "" if label == "baseline" else extract(source, "class XThreadExecutionOwner", ";"))
                for marker, start in [("@DESTRUCTOR@", "XThread::~XThread()"), ("@EXIT_METHOD@", "X_STATUS XThread::Exit("),
                                      ("@TERMINATE_METHOD@", "X_STATUS XThread::Terminate(")]:
                    cpp = cpp.replace(marker, extract(source, start))
                fixture = work / (label + ".cpp")
                fixture.write_text(cpp)
                (captures / (label + "-extracted.cpp")).write_text(cpp)
                binary = work / label
                run("compile-" + label, [args.cxx, *flags, *( ["-DXTHREAD_BASELINE=1"] if label == "baseline" else []),
                     fixture, rex / "src/core/threading_posix.cpp", rex / "src/core/threading.cpp",
                     rex / "src/core/signal_thread_prospero.cpp", rex / "src/core/trace_prospero.cpp",
                     *objects,
                     "-Wl,--gc-sections,--wrap=pthread_create,--wrap=pthread_cancel", "-o", binary], timeout=120)
                cases = ("self-exit", "self-terminate", "external-terminate") if label == "baseline" else (
                    "return", "self-exit", "self-terminate", "external-terminate", "cancel-before-start", "create-failure")
                for case in cases:
                    run(label + "-" + case, [binary, case], timeout=20)
            if args.compile_commands:
                entries = json.loads(args.compile_commands.resolve().read_text())
                entry = next(e for e in entries if e["file"].endswith("/src/system/xthread.cpp"))
                target_args = shlex.split(entry["command"])
                target_object = work / "xthread-target.o"
                target_args[target_args.index("-o") + 1] = str(target_object)
                # All configured include and input paths are absolute. Execute
                # from the project root just like the recorded host commands.
                run("compile-ps5-target", target_args, timeout=120)
                target_preprocessed = work / "xthread-target.ii"
                target_args[target_args.index("-c")] = "-E"
                target_args[target_args.index("-o") + 1] = str(target_preprocessed)
                run("preprocess-ps5-target", target_args, timeout=120)
                ps5_source = target_preprocessed.read_text()
                wiring = {}
                for start in ["X_STATUS XThread::Create()", "object_ref<XThread> XThread::Restore("]:
                    body = extract(ps5_source, start)
                    assert body.count("std::make_shared<XThreadExecutionOwner>") == 1, start
                    assert "ReleaseHandle(" not in body and "RetainHandle(" not in body, start
                    wiring[start] = {"execution_tokens": 1, "manual_handle_operations": 0}
                for start in ["X_STATUS XThread::Exit(", "X_STATUS XThread::Terminate("]:
                    body = extract(ps5_source, start)
                    assert "ReleaseHandle(" not in body and "retain_object(this)" not in body, start
                    wiring[start] = {"manual_handle_releases": 0, "automatic_self_retain": False}
                assert "std::atomic<bool> running_{false};" in ps5_source
                report["target_check"] = {
                    "result": "PASS", "hardware": "NOT_RUN",
                    "object_sha256": hashlib.sha256(target_object.read_bytes()).hexdigest(),
                    "ps5_running_is_atomic_bool": True, "preprocessed_wiring": wiring}
        report["result"] = "PASS"
        report["sources_unchanged"] = all(hashlib.sha256((root / name).read_bytes()).hexdigest() == value for name, value in hashes.items())
        if not report["sources_unchanged"]:
            report["result"] = "SOURCE_CHANGED_DURING_TEST"
            raise RuntimeError("Source changed during execution; rerun after freeze")
    except Exception as error:
        report["error"] = str(error)
        report["result"] = "FAIL"
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS " + str(report_path))


if __name__ == "__main__":
    main()
