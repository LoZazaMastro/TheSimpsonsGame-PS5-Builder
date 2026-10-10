#!/usr/bin/env python3
"""Build actual local/SDK allocators and run bounded, source-hashed host checks."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import shutil
import signal
import subprocess
import tempfile
import time

SDK_PIN = "b5efad528e0ac1b8289f39d72a0e890ff68ff0a6"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    repo = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", type=Path, default=repo.parent / "PS5-PayloadSDK")
    parser.add_argument("--cc", default=shutil.which("clang") or "cc")
    parser.add_argument("--cxx", default=shutil.which("clang++") or "c++")
    parser.add_argument("--report", type=Path,
                        default=repo / "prospero/validation/heap-allocator-host-v3.8.json")
    args = parser.parse_args()
    sdk = args.sdk_root.resolve()
    report_path = args.report.resolve()
    capture_dir = report_path.with_suffix("")
    capture_dir.mkdir(parents=True, exist_ok=True)
    captures: list[dict] = []

    def run(name: str, command: list[str], timeout: int = 40) -> dict:
        started = time.monotonic()
        try:
            result = subprocess.run(command, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, timeout=timeout)
            code, output, timed_out = result.returncode, result.stdout, False
        except subprocess.TimeoutExpired as exc:
            code, output, timed_out = None, exc.stdout or b"", True
        capture = capture_dir / (name + ".log")
        capture.write_bytes(("COMMAND " + shlex.join(command) + "\n").encode()
                            + output + (f"\nEXIT {code}; timeout={timed_out}\n").encode())
        record = {"test": name, "exit_code": code,
                  "capture": str(capture.relative_to(repo)) if capture.is_relative_to(repo) else str(capture),
                  "capture_sha256": digest(capture), "timed_out": timed_out,
                  "elapsed_seconds": round(time.monotonic() - started, 3)}
        captures.append(record)
        print(f"{name}: exit={code} timeout={timed_out}", flush=True)
        if output:
            print(output.decode(errors="replace").rstrip(), flush=True)
        return {**record, "output": output.decode(errors="replace")}

    platform = sdk / "platform"
    local = repo / "prospero/runtime/dlmalloc"
    harness = repo / "prospero/tests/heap_allocator_host.c"
    traced_wrapper = repo / "prospero/runtime/heap_wrap_prospero.c"
    traced_heap = repo / "prospero/runtime/heap_prospero.c"
    trace_source = repo / "tools/rexglue-sdk/src/core/trace_prospero.cpp"
    trace_header = repo / "tools/rexglue-sdk/include/rex/platform/trace_prospero.h"
    common = [platform / path for path in (
        "src/placement.c",
        "tests/host_kernel.c", "tests/host_libc.c")]
    sdk_sources = common + [platform / path for path in (
        "src/dlmalloc/mspace.c", "src/dlmalloc/malloc.c", "src/placement.h",
        "include/ps5platform/heap.h", "include/ps5platform/kernel.h",
        "tests/host_kernel.h", "src/heap_wrap.c", "src/heap.c")]
    production = [local / name for name in ("mspace.c", "malloc.c", "allocator_lock.h", "SOURCE")] + sdk_sources + [traced_wrapper, traced_heap, trace_source, trace_header]
    source_hashes = {str(p.relative_to(repo)) if p.is_relative_to(repo) else str(p): digest(p)
                     for p in production}
    test_hashes = {str(p.relative_to(repo)): digest(p) for p in (harness, Path(__file__).resolve())}
    pin = subprocess.check_output(["git", "-C", str(sdk), "rev-parse", "HEAD"], text=True).strip()
    report: dict = {
        "result": "FAIL", "sdk_pin": pin, "required_sdk_pin": SDK_PIN,
        "production_source_sha256": source_hashes, "test_source_sha256": test_hashes,
        "captures": captures, "process_timeout_seconds": 40,
        "scope": "Actual dlmalloc mspace plus matched SDK heap.c and placement.c, original or instrumented matched wrappers, and real production event recorder; SDK host kernel models only kernel/direct-memory operations.",
        "limits": [
            "Host Linux x86-64 execution does not reproduce PS5 firmware, libc pthread ABI, graphics workload, or title memory pressure.",
            "Both healthy variants may pass; an SDK baseline PASS does not explain or reproduce the console failure.",
            "The real SDK __wrap_* functions are called directly, following its host-test strategy. Global native-title ELF --wrap linkage is not validated here.",
            "The deliberate corruption child tests dispatch to a host observer and SIGABRT with the actual production event ring dump; production fault signal/report integration is validated separately.",
            "The exact local lock header is tested under forced contention and injected nanosleep EINTR; the baseline pthread lock has no such custom backoff contract."
        ]
    }
    # Reverse exactly the documented diagnostic-only patch. Do not describe
    # this patched vendor file as unmodified or approve unrelated drift.
    restored_allocator = (local / "malloc.c").read_text()
    restored_allocator = restored_allocator.replace(
        "/* Optional diagnostics only after the existing first-smallbin check fails.\n"
        "   The hook receives values already evaluated by that check. */\n"
        "#ifndef DLMALLOC_UNLINK_FIRST_FAILURE\n"
        "#define DLMALLOC_UNLINK_FIRST_FAILURE(M, B, P, F, I, BACK, ADDR_OK) ((void)0)\n"
        "#endif\n\n", "", 1)
    restored_allocator = restored_allocator.replace("  mchunkptr F_back = 0;\\\n  int F_address_ok = 0;\\\n", "", 1)
    restored_allocator = restored_allocator.replace(
        "RTCHECK((F_address_ok = ok_address(M, F)) && (F_back = F->bk) == P)",
        "RTCHECK(ok_address(M, F) && F->bk == P)", 1)
    restored_allocator = restored_allocator.replace(
        "    DLMALLOC_UNLINK_FIRST_FAILURE(M, B, P, F, I, F_back, F_address_ok);\\\n", "", 1)
    allocator_patch_only = restored_allocator.encode() == (platform / "src/dlmalloc/malloc.c").read_bytes()
    wrapper_body = traced_wrapper.read_text().split("/* BEGIN MATCHED SDK IMPLEMENTATION */\n", 1)[1].split("/* END MATCHED SDK IMPLEMENTATION */", 1)[0]
    restored_wrapper = re.sub(r"(?m)^static (void \*|void|size_t|int|ssize_t)\nprospero_impl_(\w+)\(",
                              r"\1\n__wrap_\2(", wrapper_body)
    wrapper_body_identical = restored_wrapper.encode() == (platform / "src/heap_wrap.c").read_bytes()
    restored_heap = traced_heap.read_text().replace("#include <stdatomic.h>\n", "", 1)
    restored_heap = restored_heap.replace(
        "/* Atomic reads without the segment lock: approximate independent fields. */",
        "/* Read without the segment lock: a report, not a decision. */", 1)
    restored_heap = restored_heap.replace(
        "/* Local diagnostic adaptation: the independent telemetry worker samples\n"
        " * these counters without segment_lock. Atomic objects preserve the existing\n"
        " * segment-lock algorithm while avoiding concurrent plain reads and writes.\n"
        " * Each sampled field is approximate; this is not a transactional snapshot.\n"
        " */\n"
        "static _Atomic unsigned segment_count;\n"
        "static _Atomic size_t mapped_bytes, peak_bytes;\n"
        "_Static_assert(ATOMIC_INT_LOCK_FREE == 2 && ATOMIC_LONG_LOCK_FREE == 2,\n"
        "               \"Heap telemetry counters must be always lock-free on PS5\");\n",
        "static unsigned segment_count;\nstatic size_t mapped_bytes, peak_bytes;\n", 1)
    heap_patch_only = restored_heap.encode() == (platform / "src/heap.c").read_bytes()
    okay = pin == SDK_PIN and allocator_patch_only and wrapper_body_identical and heap_patch_only
    report["heap_has_only_documented_atomic_counter_patch"] = heap_patch_only
    report["allocator_has_only_documented_failure_hook_patch"] = allocator_patch_only
    report["wrapper_implementation_matches_sdk_after_name_linkage_restore"] = wrapper_body_identical
    try:
        if not okay:
            raise RuntimeError("SDK pin or documented vendor patch/implementation identity mismatch")
        version = run("compiler-version", [args.cc, "--version"])
        if version["exit_code"] != 0:
            raise RuntimeError("Host compiler is unavailable")
        with tempfile.TemporaryDirectory(prefix="simpsons-heap-host-") as directory:
            work = Path(directory)
            flags = [args.cc, "-std=gnu11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                     "-pthread", "-I" + str(platform / "include"),
                     "-I" + str(platform / "src"), "-I" + str(platform / "tests"),
                     "-I" + str(local), "-I" + str(repo / "tools/rexglue-sdk/include")]
            objects = []
            for index, source in enumerate(common):
                obj = work / f"common-{index}.o"
                check = run(f"compile-{source.stem}", flags + ["-c", str(source), "-o", str(obj)])
                if check["exit_code"] != 0:
                    raise RuntimeError(f"Compilation failed: {source}")
                objects.append(str(obj))
            trace_obj = work / "production-trace.o"
            check = run("compile-production-trace", [args.cxx, "-std=c++23", "-O2", "-g",
                        "-D__PROSPERO__=1", "-pthread", "-I" + str(repo / "tools/rexglue-sdk/include"),
                        "-c", str(trace_source), "-o", str(trace_obj)])
            if check["exit_code"] != 0:
                raise RuntimeError("Production event recorder failed to compile")
            outcomes = {}
            for variant, provider, wrapper, heap in (
                ("sdk-pthread-baseline", platform / "src/dlmalloc/mspace.c", platform / "src/heap_wrap.c", platform / "src/heap.c"),
                ("local-atomic-lock", local / "mspace.c", traced_wrapper, traced_heap)):
                heap_obj = work / (variant + "-heap.o")
                check = run("compile-heap-" + variant, flags + ["-c", str(heap), "-o", str(heap_obj)])
                if check["exit_code"] != 0:
                    raise RuntimeError("Production heap provider failed to compile")
                mspace_obj = work / (variant + "-mspace.o")
                check = run("compile-" + variant, flags + ["-w", "-c", str(provider), "-o", str(mspace_obj)])
                if check["exit_code"] != 0:
                    raise RuntimeError(f"Allocator compilation failed: {variant}")
                binary = work / variant
                variant_flags = ["-DHEAP_LOCAL_LOCK_TEST=1", "-DHEAP_TRACE_TEST=1"] if variant == "local-atomic-lock" else []
                wrapper_obj = work / (variant + "-wrapper.o")
                check = run("compile-wrapper-" + variant, flags + ["-c", str(wrapper), "-o", str(wrapper_obj)])
                if check["exit_code"] != 0:
                    raise RuntimeError("Wrapper compilation failed")
                harness_obj = work / (variant + "-harness.o")
                check = run("compile-harness-" + variant, flags + variant_flags + ["-c", str(harness), "-o", str(harness_obj)])
                if check["exit_code"] != 0:
                    raise RuntimeError("Allocator harness compilation failed")
                wraps = ["-Wl,--wrap=" + name for name in (
                    "__real_malloc", "__real_realloc", "__real_free", "nanosleep",
                    "sceKernelAllocateDirectMemory", "localeconv")]
                trace_objects = [str(trace_obj)] if variant == "local-atomic-lock" else []
                check = run("link-" + variant, [args.cxx, "-pthread", "-no-pie", str(harness_obj), str(mspace_obj), str(wrapper_obj)] +
                            objects + [str(heap_obj)] + trace_objects + wraps + ["-o", str(binary)])
                if check["exit_code"] != 0:
                    raise RuntimeError(f"Harness link failed: {variant}")
                healthy = run(variant + "-healthy", [str(binary), "suite"])
                corrupt = run(variant + "-corruption-child", [str(binary), "corrupt"])
                healthy_pass = healthy["exit_code"] == 0 and "PASS complete healthy allocator suite" in healthy["output"]
                corruption_pass = (corrupt["exit_code"] == -signal.SIGABRT and
                                   "CORRUPT fixture armed" in corrupt["output"])
                if variant == "local-atomic-lock":
                    corruption_pass = corruption_pass and ("OBSERVER prospero_heap_failure kind=corruption operation=mspace_malloc" in corrupt["output"])
                    event_pattern = re.compile(
                        r"T seq=(\d+) thread=(0x[0-9a-fA-F]+) kind=\w+\((\d+)\) "
                        r"caller=(0x[0-9a-fA-F]+) a=(0x[0-9a-fA-F]+) b=(0x[0-9a-fA-F]+) "
                        r"c=(0x[0-9a-fA-F]+) d=(0x[0-9a-fA-F]+)")
                    hook_site = next(i for i, line in enumerate((local / "malloc.c").read_text().splitlines(), 1)
                                     if line.strip() == "unlink_first_small_chunk(ms, b, p, idx);")
                    detail_results = []
                    for fixture, failure_flag in (("backlink", 2), ("address", 1)):
                        result = run("trace-corrupt-" + fixture, [str(binary), "trace-corrupt-" + fixture])
                        events = []
                        for match in event_pattern.finditer(result["output"]):
                            values = match.groups()
                            events.append(dict(zip(("seq", "thread", "kind", "caller", "a", "b", "c", "d"),
                                                   (int(x, 10) if i in (0, 2) else int(x, 16) for i, x in enumerate(values)))))
                        links = next((e for e in events if e["kind"] == 14), None)
                        check_event = next((e for e in events if e["kind"] == 15), None)
                        fixture_info = re.search(r"TRACE fixture freed=(0x[0-9a-fA-F]+) bad_address=(\d+)", result["output"])
                        if not links or not check_event or not fixture_info:
                            raise RuntimeError("Actual production ring did not contain the smallbin diagnostics")
                        freed = int(fixture_info.group(1), 16)
                        before_failure = [e for e in events if e["seq"] < links["seq"]]
                        original_call = next((e for e in reversed(before_failure) if e["kind"] == 2 and e["d"] == 1), None)
                        last_free = next((e for e in reversed(before_failure) if e["kind"] == 4 and e["a"] == freed), None)
                        if not original_call or not last_free:
                            raise RuntimeError("Allocator/free history missing from actual ring")
                        expected_next = freed if fixture == "backlink" else 1
                        valid = (result["exit_code"] == -signal.SIGABRT and links["c"] == freed - 16 and
                                 links["d"] == expected_next and links["a"] != 0 and links["b"] != 0 and
                                 check_event["a"] == 12 and check_event["b"] == 0 and check_event["c"] == 64 and
                                 check_event["d"] >> 32 == failure_flag and check_event["d"] & 0xffffffff == hook_site and
                                 links["caller"] == check_event["caller"] != 0 and
                                 links["thread"] == check_event["thread"] == original_call["thread"] == last_free["thread"] and
                                 original_call["b"] == 64 and original_call["c"] == 1 and
                                 last_free["seq"] < original_call["seq"] and
                                 not any(e["kind"] == 3 and e["seq"] > original_call["seq"] for e in events))
                        callsite = run("trace-" + fixture + "-allocate-caller", ["addr2line", "-f", "-C", "-e", str(binary), hex(original_call["caller"])])
                        freesite = run("trace-" + fixture + "-free-caller", ["addr2line", "-f", "-C", "-e", str(binary), hex(last_free["caller"])])
                        valid = valid and "trace_fixture_allocate" in callsite["output"] and "trace_fixture_free" in freesite["output"]
                        okay = okay and valid
                        detail_results.append({"fixture": fixture, "result": "PASS" if valid else "FAIL",
                                               "expected_failure_flag": failure_flag, "expected_hook_line": hook_site,
                                               "heap_links": links, "heap_check": check_event,
                                               "original_allocation_begin": original_call, "preceding_free_begin": last_free,
                                               "captured_event_count": len(events),
                                               "note": "Address fixture uses F=1; SIGABRT confirms the logger does not dereference an address rejected by the original predicate."})
                    report["actual_trace_corruption_fixtures"] = detail_results
                outcomes[variant] = {"healthy_suite": "PASS" if healthy_pass else "FAIL",
                                     "deliberate_corruption": "PASS" if corruption_pass else "FAIL",
                                     "corruption_child_exit_code": corrupt["exit_code"],
                                     "failure_observer_expected": variant == "local-atomic-lock"}
                okay = okay and healthy_pass and corruption_pass
            report["variants"] = outcomes
            report["baseline_conclusion"] = (
                "The real SDK pthread baseline passed the healthy host suite; the console allocator failure was not reproduced here."
                if outcomes["sdk-pthread-baseline"]["healthy_suite"] == "PASS" else
                "The SDK baseline failed the host suite; consult its captured output before attributing any cause.")
            database = repo / "out/prospero-current/compile_commands.json"
            if not database.is_file():
                raise RuntimeError("Actual PS5 compile database is needed to validate native allocator providers")
            entries = json.loads(database.read_text())
            template = next((entry for entry in entries if entry["file"].endswith("/prospero/runtime/dlmalloc/mspace.c")), None)
            if template is None:
                raise RuntimeError("No actual target command for local mspace provider")
            native_objects = []
            for target_source in (local / "mspace.c", traced_wrapper, traced_heap):
                entry = next((entry for entry in entries if Path(entry["file"]).resolve() == target_source), template)
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
                    command.append(str(target_source) if item == entry["file"] else item)
                native_object = work / (target_source.stem + "-native.o")
                command += ["-o", str(native_object)]
                old_directory = Path.cwd()
                try:
                    import os
                    os.chdir(entry["directory"])
                    check = run("native-compile-" + target_source.stem, command)
                finally:
                    os.chdir(old_directory)
                if check["exit_code"] != 0:
                    raise RuntimeError("Native allocator provider did not compile")
                symbols = run("native-symbols-" + target_source.stem,
                              ["nm", "--defined-only", "--extern-only", str(native_object)])
                if target_source == traced_wrapper:
                    actual = {line.split()[-1] for line in symbols["output"].splitlines() if line.split() and line.split()[-1].startswith("__wrap_")}
                    expected = {"__wrap_" + name for name in ("malloc", "calloc", "free", "malloc_usable_size", "realloc", "reallocf", "reallocarray", "posix_memalign", "aligned_alloc", "memalign", "getdelim", "getline")}
                    if actual != expected:
                        raise RuntimeError("Explicit wrapper provider does not define exactly the12 matched API wrappers")
                native_objects.append({"source": str(target_source.relative_to(repo)), "object_sha256": digest(native_object),
                                       "compile_origin": "Actual CMake target command; source substituted only when new provider is not yet regenerated."})
            report["native_target_objects"] = native_objects
        report["sources_unchanged_during_test"] = all(
            digest(repo / p if not p.startswith("/") else Path(p)) == sha for p, sha in source_hashes.items())
        okay = okay and report["sources_unchanged_during_test"]
        report["result"] = "PASS" if okay else "FAIL"
    except (OSError, RuntimeError, subprocess.SubprocessError) as exc:
        report["error"] = str(exc)
        okay = False
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    print(f"RESULT {report['result']}: {report_path}", flush=True)
    return 0 if okay else 1


if __name__ == "__main__":
    raise SystemExit(main())
