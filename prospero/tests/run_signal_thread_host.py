#!/usr/bin/env python3
"""Test production signal state and deterministically reproduce old emutls reentry."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-source", type=Path, required=True)
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--no-leak-check", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    rex = root / "tools/rexglue-sdk"
    source = args.sdk_source.resolve()
    for required in ("platform/src/cxa.c", "platform/include/ps5platform/libc.h"):
        if not (source / required).is_file():
            parser.error(f"Missing pinned SDK source: {source / required}")
    common = ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined", "-pthread",
              "-I", str(rex / "include"), "-I", str(source / "platform/include")]
    registry = [str(rex / "src/core/signal_thread_prospero.cpp"),
                str(rex / "src/core/trace_prospero.cpp")]
    environment = dict(os.environ)
    environment["ASAN_OPTIONS"] = f"detect_leaks={0 if args.no_leak_check else 1}:abort_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    with tempfile.TemporaryDirectory(prefix="simpsons-signal-host-") as directory:
        work = Path(directory)
        test = work / "signal_thread_host"
        wrappers = ["pthread_self", "pthread_key_create", "pthread_setspecific", "pthread_getspecific", "pthread_once"]
        subprocess.run([args.cc, "-std=c11", "-D_GNU_SOURCE", "-fno-exceptions", *common,
                        "-c", str(root / "prospero/tests/signal_thread_cleanup_host.c"),
                        "-o", str(work / "native-cleanup.o")], check=True)
        subprocess.run([args.cxx, "-std=c++23", "-Wall", "-Wextra", "-Werror", *common,
                        str(root / "prospero/tests/signal_thread_host.cpp"), *registry, str(work / "native-cleanup.o"),
                        *(f"-Wl,--wrap={name}" for name in wrappers), "-o", str(test)], check=True)
        for case in ("basics", "publication", "key-create-error", "key-set-error", "capacity", "concurrent", "cancel"):
            subprocess.run([str(test), case], check=True, env=environment, timeout=30)

        subprocess.run([args.cc, "-std=c11", "-D_GNU_SOURCE", *common,
                        "-DPROSPERO_EMUTLS_HOST_TEST=1", "-Drealloc=signal_test_realloc", "-c",
                        str(root / "prospero/runtime/emutls.c"), "-o", str(work / "emutls.o")], check=True)
        subprocess.run([args.cc, "-std=c11", "-D_GNU_SOURCE", *common,
                        "-c", str(source / "platform/src/cxa.c"), "-o", str(work / "sdk-cxa.o")], check=True)
        for name, define in (("old-handler", ["-DV38_EMUTLS_SIGNAL_HANDLER=1"]), ("fixed-handler", [])):
            subprocess.run([args.cc, "-std=c11", *common, *define, "-c",
                            str(root / "prospero/tests/signal_emutls_reentry_host.c"),
                            "-o", str(work / f"{name}.o")], check=True)
            binary = work / name
            subprocess.run([args.cxx, "-std=c++23", *common, *registry, str(work / f"{name}.o"),
                            str(work / "emutls.o"), str(work / "sdk-cxa.o"), "-o", str(binary)], check=True)
            if name == "old-handler":
                subprocess.run([str(binary)], check=True, env=environment, timeout=30)
                result = subprocess.run([str(binary), "inject"], capture_output=True, text=True,
                                        env=environment, timeout=30)
                if result.returncode == 0 or "heap-use-after-free" not in result.stderr or "emutls.c" not in result.stderr:
                    raise RuntimeError(f"Old handler failed to reproduce the expected real emutls UAF:\n{result.stdout}\n{result.stderr}")
                print("PASS negative control: old handler during real moving realloc reproduces ASan heap-use-after-free in emutls.c", flush=True)
            else:
                subprocess.run([str(binary), "inject"], check=True, env=environment, timeout=30)
    print("Signal registry and real emutls reentry regression passed. Native PS5 signal/unwind execution is not claimed.")
    if args.no_leak_check:
        print("LeakSanitizer was explicitly disabled; this run makes no leak-check claim.")


if __name__ == "__main__":
    main()
