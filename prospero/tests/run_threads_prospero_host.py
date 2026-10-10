#!/usr/bin/env python3
"""Run the actual local/SDK thread wrappers with Linux pthreads and the SDK kernel model.

No PS5 execution or native ABI validation is claimed. Each case is a fresh process.
"""
import argparse
from pathlib import Path
import resource
import signal
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", required=True, type=Path)
    parser.add_argument("--cc", default="cc")
    parser.add_argument("--cxx", default="c++")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = args.sdk_root.resolve() / "platform"
    flags = ["-std=c11", "-pthread", "-O1", "-g", "-Wall", "-Wextra",
             "-I" + str(sdk / "include"), "-I" + str(sdk / "tests"),
             "-I" + str(root / "tools/rexglue-sdk/include")]
    redirects = {
        "__real_pthread_create": "observed_pthread_create",
        "__real_pthread_join": "observed_pthread_join",
        "__real_pthread_detach": "observed_pthread_detach",
        "pthread_key_create": "observed_key_create",
        "pthread_setspecific": "observed_setspecific",
        "pthread_attr_setstack": "observed_attr_setstack",
        "sceKernelAllocateDirectMemory": "observed_allocate",
        "sceKernelMapDirectMemory": "observed_map",
        "sceKernelReleaseDirectMemory": "observed_release",
    }
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    with tempfile.TemporaryDirectory(prefix="simpsons-threads-host-") as temporary:
        build = Path(temporary)
        # Include the real declarations before replacing these two calls. A
        # command-line rename would also rename glibc's inline pthread_equal
        # implementation (and its const attribute), defeating ID injection.
        identity_observation = build / "identity_observation.h"
        identity_observation.write_text(
            "#define _GNU_SOURCE 1\n"
            "#include <pthread.h>\n#include <stdlib.h>\n"
            "int observed_pthread_equal(pthread_t, pthread_t);\n"
            "void observed_free(void *);\n"
            "#define pthread_equal observed_pthread_equal\n"
            "#define free observed_free\n")
        objects = []
        for source in (sdk / "tests/host_kernel.c", sdk / "src/placement.c", sdk / "src/cxa.c",
                       root / "prospero/tests/threads_prospero_host.c"):
            target = build / (source.stem + ".o")
            subprocess.run([args.cc, *flags, "-c", str(source), "-o", str(target)], check=True)
            objects.append(target)
        trace_object = build / "production-trace.o"
        subprocess.run([args.cxx, "-std=c++23", "-O2", "-g", "-pthread", "-D__PROSPERO__=1",
                        "-I" + str(root / "tools/rexglue-sdk/include"),
                        "-c", str(root / "tools/rexglue-sdk/src/core/trace_prospero.cpp"),
                        "-o", str(trace_object)], check=True)
        binaries = {}
        for name, source in (("baseline", sdk / "src/threads.c"),
                             ("current", root / "prospero/runtime/threads_prospero.c")):
            obj = build / (name + ".o")
            subprocess.run([args.cc, *flags, "-Werror",
                            *(f"-D{a}={b}" for a, b in redirects.items()),
                            "-include", str(identity_observation),
                            "-c", str(source), "-o", str(obj)], check=True)
            executable = build / name
            subprocess.run([args.cxx, "-pthread", *(str(p) for p in objects), str(obj),
                            *([str(trace_object)] if name == "current" else []),
                            "-Wl,--wrap=pthread_create,--wrap=pthread_join,--wrap=pthread_detach",
                            "-o", str(executable)], check=True)
            binaries[name] = executable
        subprocess.run([str(binaries["baseline"]), "baseline"], check=True, timeout=15)
        reused_id = subprocess.run([str(binaries["baseline"]), "id-reuse"],
                                   capture_output=True, text=True, timeout=15)
        if reused_id.returncode != 1 or "freed_reused_record" not in reused_id.stderr:
            raise SystemExit("SDK baseline did not reproduce the injected pthread_t reuse bug: " +
                             reused_id.stdout + reused_id.stderr)
        print("PASS baseline-id-reuse   expected exit 1: the SDK freed the new thread's record")
        print(reused_id.stderr.strip())
        modes = ("sizes", "reuse", "cache-count", "cache-bytes", "detached", "explicit-stack",
                 "attributes", "alloc-fail", "map-fail", "reserve-fail", "create-fail",
                 "attr-fail", "key-fail", "fallback-fail", "reaper-fail", "detach-fail",
                 "reaper-join-fail", "overflow", "id-reuse")
        for mode in modes:
            subprocess.run([str(binaries["current"]), mode], check=True, timeout=15)
        for mode, expected in (("guard-fault", signal.SIGSEGV), ("specific-fail", signal.SIGABRT)):
            result = subprocess.run([str(binaries["current"]), mode], timeout=15)
            if result.returncode != -expected:
                raise SystemExit(f"{mode}: expected signal {expected}, got {result.returncode}")
            print(f"PASS {mode:<19} terminated with real signal {expected}")
    print("Host thread tests passed; console stack allocation/execution remains untested.")


if __name__ == "__main__":
    main()
