#!/usr/bin/env python3
"""Test actual target emutls changes with host pthreads and the local SDK-derived registry."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-source", type=Path, required=True,
                        help="matched PS5-PayloadSDK source checkout/export")
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--registry-source", type=Path,
                        help="Override the local cxa_prospero.c provider for a baseline comparison")
    parser.add_argument("--no-leak-check", action="store_true",
                        help="disable only LeakSanitizer when /proc thread enumeration is unavailable")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    source = args.sdk_source.resolve()
    registry = (args.registry_source.resolve() if args.registry_source else
                root / "prospero/runtime/cxa_prospero.c")
    with tempfile.TemporaryDirectory(prefix="simpsons-emutls-host-") as directory:
        work = Path(directory)
        common = ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined",
                  "-fno-plt", "-pthread", "-I", str(source / "platform/include"),
                  "-I", str(root / "tools/rexglue-sdk/include")]
        subprocess.run([args.cc, "-std=c11", "-D_GNU_SOURCE", *common,
                        "-DPROSPERO_EMUTLS_HOST_TEST=1", "-c",
                        str(root / "prospero/runtime/emutls.c"),
                        "-o", str(work / "emutls.o")], check=True)
        subprocess.run([args.cc, "-std=c11", "-D_GNU_SOURCE", *common, "-c",
                        str(registry),
                        "-o", str(work / "sdk-cxa.o")], check=True)
        subprocess.run([args.cxx, "-std=c++23", *common, "-femulated-tls",
                        str(root / "prospero/tests/emutls_host_test.cpp"),
                        str(root / "tools/rexglue-sdk/src/core/signal_thread_prospero.cpp"),
                        str(root / "tools/rexglue-sdk/src/core/trace_prospero.cpp"),
                        str(work / "emutls.o"), str(work / "sdk-cxa.o"),
                        "-o", str(work / "emutls_test")], check=True)
        leak_check = "0" if args.no_leak_check else "1"
        environment = dict(os.environ, ASAN_OPTIONS=f"detect_leaks={leak_check}:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        for order in ("emutls-first", "registry-first", "explicit-drain"):
            subprocess.run([str(work / "emutls_test"), order], env=environment,
                           timeout=20, check=True)
    print("Host ASan/UBSan checks passed; PS5 pthread ABI is checked separately by cross-build.")
    if args.no_leak_check:
        print("LeakSanitizer was explicitly disabled; this run makes no leak-check claim.")


if __name__ == "__main__":
    main()
