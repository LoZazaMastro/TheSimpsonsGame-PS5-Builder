#!/usr/bin/env python3
"""Verify offline decoding against real recorder/allocator logs and bad inputs."""
from __future__ import annotations
import argparse
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import signal
import struct
import subprocess


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", type=Path, default=root.parent / "PS5-PayloadSDK")
    parser.add_argument("--version", default="v3.10", help="Version suffix for new captures; existing release reports are preserved")
    parser.add_argument("--target-elf", type=Path, help="Additionally verify source mapping on this real PS5 debug ELF (never executed)")
    parser.add_argument("--line-map", type=Path, help="Verified line-map JSON for --target-elf")
    args = parser.parse_args()
    if not re.fullmatch(r"v[0-9]+\.[0-9]+", args.version):
        parser.error("--version must have the form v3.10")
    if (args.target_elf is None) != (args.line_map is None):
        parser.error("--target-elf and --line-map must be supplied together")
    sdk = args.sdk_root.resolve()
    work = root / ("out/trace-symbolizer-host-" + args.version)
    capture = root / ("prospero/validation/trace-symbolizer-" + args.version + "-captures")
    work.mkdir(parents=True, exist_ok=True)
    capture.mkdir(parents=True, exist_ok=True)
    report_path = root / ("prospero/validation/trace-symbolizer-" + args.version + ".json")
    report_path.write_text(json.dumps({"status": "IN_PROGRESS", "hardware": "NOT_RUN"}) + "\n")
    cc = shutil.which(os.environ.get("HOST_CC", "clang")) or shutil.which("cc")
    cxx = shutil.which(os.environ.get("HOST_CLANGXX", "clang++")) or shutil.which("c++")
    if not cc or not cxx:
        raise RuntimeError("host C/C++ compilers are required")
    runs, sources = [], []

    def run(name, command, expected=0, timeout=60):
        result = subprocess.run(list(map(str, command)), text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=timeout)
        target = capture / (name + ".txt")
        target.write_text(result.stdout)
        runs.append({"name": name, "command": list(map(str, command)), "exit_code": result.returncode,
                     "capture": str(target.relative_to(root)), "capture_sha256": digest(target)})
        print(f"{name}: exit {result.returncode}", flush=True)
        if result.returncode != expected:
            raise RuntimeError(f"{name}: expected exit {expected}, got {result.returncode}: {result.stdout[-4000:]}")
        return result.stdout

    platform = sdk / "platform"
    flags = ["-std=gnu11", "-O1", "-g", "-pthread", "-I" + str(platform / "include"),
             "-I" + str(platform / "src"), "-I" + str(platform / "tests"),
             "-I" + str(root / "tools/rexglue-sdk/include"),
             "-I" + str(root / "prospero/runtime/dlmalloc")]
    objects = []
    files = [platform / path for path in (
        "src/heap.c", "src/placement.c", "tests/host_kernel.c", "tests/host_libc.c")]
    files += [root / "prospero/runtime/dlmalloc/mspace.c",
              root / "prospero/runtime/heap_wrap_prospero.c",
              root / "prospero/tests/heap_allocator_host.c",
              root / "prospero/tests/trace_symbolizer_host.c"]
    for index, source in enumerate(files):
        output = work / f"fixture-{index}.o"
        extra = []
        if source.name == "heap_allocator_host.c":
            extra = ["-DHEAP_LOCAL_LOCK_TEST=1", "-DHEAP_TRACE_TEST=1", "-Dmain=heap_fixture_main"]
        run("compile-" + source.stem, [cc, *flags, *extra, "-w", "-c", source, "-o", output])
        objects.append(output)
        sources.append(source)
    source = root / "tools/rexglue-sdk/src/core/trace_prospero.cpp"
    trace = work / "production-trace.o"
    run("compile-production-trace", [cxx, "-std=c++23", "-O2", "-g", "-pthread",
        "-I" + str(root / "tools/rexglue-sdk/include"), "-c", source, "-o", trace])
    sources.append(source)
    binary = work / "real-recorder-allocator-fixture"
    wraps = ["-Wl,--wrap=" + name for name in (
        "__real_malloc", "__real_realloc", "__real_free", "nanosleep",
        "sceKernelAllocateDirectMemory", "localeconv")]
    run("link-fixture", [cxx, "-pthread", "-no-pie", *objects, trace, *wraps,
        "-Wl,--defsym=__decoder_linked_entry=ABSOLUTE(_start)", "-o", binary])
    sha = digest(binary)
    tool = root / "prospero/tools/symbolize_trace.py"
    cli = ["python3", tool, "--elf", binary, "--expected-sha256", sha]
    decoded = {}
    logs = {}
    for mode, code in (("trace-corrupt-backlink", -signal.SIGABRT),
                       ("trace-corrupt-address", -signal.SIGABRT), ("decoder-stack", 0)):
        logs[mode] = run(mode, [binary, mode], expected=code)
        path = work / (mode + ".log")
        path.write_text(logs[mode])
        decoded[mode] = json.loads(run("decode-" + mode, [*cli, path]))
        session = decoded[mode]["sessions"][0]
        assert session["completeness"]["framing_complete"]
        assert session["image_validation"]["entry_matches_linked_anchor"]
        assert [item["category"] for item in session["telemetry"]] == ["METRICS", "HEARTBEAT"]
        if mode.startswith("trace-corrupt"):
            functions = [frame["function"] for event in session["events"]
                         for frame in event["caller_symbol"]["frames"]]
            assert any("trace_fixture_allocate" in name for name in functions), functions
            assert any("trace_fixture_free" in name for name in functions), functions
            assert any(event["kind_id"] == 14 for event in session["events"])
            checks = [event["heap_check"] for event in session["events"] if event["kind_id"] == 15]
            assert checks and checks[0]["failure_flags"] == (2 if mode.endswith("backlink") else 1)
        else:
            stack = session["raw_stacks"][0]
            assert len(stack["code_address_candidates"]) == 2
            assert all(item["classification"] == "CANDIDATE_ONLY_UNWIND_NOT_PERFORMED" for item in stack["code_address_candidates"])
            assert any("decoder_candidate_marker" in frame["function"]
                       for item in stack["code_address_candidates"] for frame in item["symbol"]["frames"])

    def check_text(name, text, expected=0, extra=()):
        path = work / (name + ".log")
        path.write_text(text)
        return run(name, [*cli, *extra, path], expected=expected)

    original = logs["trace-corrupt-backlink"]
    live_text = original[:original.index("[PPSA74207] TRACE BEGIN")]
    pid = re.search(r"trace-checkpoint .*?pid=(\d+)", live_text)[1]
    live_text += f'[PPSA74207] FILE_IO pid={pid} path="/data/homebrew/PPSA74207/gamedata/Level One.bin" result=0\n'
    live_text += f'[PPSA74207] OPS category=file_open pid={pid} monotonic_ns=2 path_pct=/data/homebrew/PPSA74207/gamedata/Level%20One.bin result=0 truncated=0\n'
    checkpoint = next(line for line in live_text.splitlines() if "trace-checkpoint " in line)
    checkpoint = re.sub(r"\bseq=\d+", "seq=2", checkpoint)
    checkpoint = re.sub(r"\bphase=\d+", "phase=500", checkpoint)
    live_text += checkpoint + "\n"
    live_path = work / "healthy-live.log"
    live_path.write_text(live_text)
    live_cli = ["python3", tool, "--telemetry-only", "--addr2line", "definitely-no-such-addr2line"]
    live = json.loads(run("healthy-live-no-elf", [*live_cli, live_path]))
    assert live["status"] == "TELEMETRY_ONLY" and live["elf"] is None
    assert not live["sessions"][0]["completeness"]["complete_event_history"]
    assert live["sessions"][0]["checkpoints"][0]["phase_name"] == "DIAGNOSTICS_READY"
    assert "caller_symbol" not in live["sessions"][0]["checkpoints"][0]
    assert live["sessions"][0]["telemetry"][-2]["fields"]["path"].endswith("Level One.bin")
    assert live["sessions"][0]["telemetry"][-1]["category"] == "OPS"
    assert live["sessions"][0]["telemetry"][-1]["fields"]["path_pct"].endswith("Level%20One.bin")
    assert live["sessions"][0]["checkpoints"][-1]["phase_name"] == "INPUT_INIT"
    assert live["sessions"][0]["telemetry"][0]["fields"]["fps_avg"] == "60.000"
    live_path.write_text(live_text + live_text)
    live_reused = json.loads(run("live-reused-pid-startup-reset", [*live_cli, live_path]))
    assert len(live_reused["sessions"]) == 2
    ambiguous = re.sub(r"trace-checkpoint seq=\d+", "trace-checkpoint seq=999999", live_text, count=1)
    live_path.write_text(live_text + ambiguous)
    assert "ambiguous" in run("live-ambiguous-same-pid", [*live_cli, live_path], 2)
    raw_path_text = live_text + f'[PPSA74207] FILE_IO pid={pid} path="truncated path\n'
    live_path.write_text(raw_path_text)
    raw_path = json.loads(run("live-truncated-operation-kept-raw", [*live_cli, live_path]))
    assert raw_path["sessions"][0]["telemetry"][-1]["parse_warning"]
    assert raw_path["sessions"][0]["telemetry"][-1]["raw_line"].endswith('path="truncated path')
    assert "does not accept an ELF" in run("telemetry-rejects-elf-argument", [*cli, "--telemetry-only", live_path], 2)
    combined = json.loads(check_text("appended-sessions", original + logs["trace-corrupt-address"]))
    assert len(combined["sessions"]) == 2
    reused = json.loads(check_text("reused-pid-sessions", original + original))
    assert len(reused["sessions"]) == 2
    bias = 0x400000
    shifted = re.sub(r"\bcaller=(0x[0-9a-f]+)",
                     lambda m: "caller=" + hex(int(m[1], 16) + bias) if int(m[1], 16) else m[0], original)
    shifted = re.sub(r"\bruntime_anchor=(0x[0-9a-f]+)", lambda m: "runtime_anchor=" + hex(int(m[1], 16) + bias), shifted)
    shifted = re.sub(r"\bload_bias=0x[0-9a-f]+", "load_bias=" + hex(bias), shifted)
    moved = json.loads(check_text("explicit-relocated-session", original + shifted))
    assert len(moved["sessions"]) == 2
    first, second = moved["sessions"]
    assert first["image_validation"]["load_bias"] == "0x0"
    assert second["image_validation"]["load_bias"] == hex(bias)
    assert [event["caller_symbol"]["linked_address"] for event in first["events"]] == [event["caller_symbol"]["linked_address"] for event in second["events"]]
    assert "missing or unknown" in check_text("unknown-anchor", original.replace("image_anchor_known=1", "image_anchor_known=0"), 2)
    assert "inconsistent" in check_text("inconsistent-anchor", re.sub(r"load_bias=0x[0-9a-f]+", "load_bias=0x1", original), 2)
    assert "invalid unsigned" in check_text("malformed-event", re.sub(r"T seq=\d+", "T seq=oops", original, count=1), 2)
    truncated = original[:original.index("[PPSA74207] TRACE END")]
    assert "TRACE END is missing" in check_text("truncated-default-refused", truncated, 2)
    partial = json.loads(check_text("anchored-prefix-explicit", truncated, extra=("--allow-incomplete",)))
    assert partial["status"] == "PARTIAL"
    without_anchor = re.sub(r"TRACE image_anchor_known=.*\n", "", original)
    assert "missing or unknown" in check_text("missing-anchor", without_anchor, 2)
    wrong_elf = work / "wrong-entry.elf"
    data = bytearray(binary.read_bytes())
    struct.pack_into("<Q", data, 24, struct.unpack_from("<Q", data, 24)[0] + 1)
    wrong_elf.write_bytes(data)
    log = work / "trace-corrupt-backlink.log"
    assert "does not match logged linked_anchor" in run("wrong-elf-entry", ["python3", tool, "--elf", wrong_elf, log], 2)
    assert "SHA-256 mismatch" in run("wrong-elf-sha", ["python3", tool, "--elf", binary, "--expected-sha256", "0" * 64, log], 2)
    bad_stack = re.sub(r"S address=0x[0-9a-f]+", "S address=0x1", logs["decoder-stack"], count=1)
    assert "violates" in check_text("stack-outside-bounds", bad_stack, 2)
    assert "no structured TRACE" in check_text("unstructured-log", "ordinary log text\n", 2)
    native_checks = None
    if args.target_elf is not None:
        target_elf, line_map_path = args.target_elf.resolve(), args.line_map.resolve()
        spec = importlib.util.spec_from_file_location("tested_trace_decoder", tool)
        decoder = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(decoder)
        image = decoder.ElfImage(target_elf, None)
        document = json.loads(line_map_path.read_text())
        mapping = decoder.LineMap(line_map_path, image)
        # The matching PS5 debug image may still be ET_DYN; its SCE type is
        # applied by the later native conversion, not by the debug linker.
        expected_helpers = {"__wrap_pthread_exit", "ps5_ffs", "ps5_pathconf", "ps5_timegm"}
        assert {item["symbol"] for item in mapping.mappings} == expected_helpers
        addresses = {value for item in mapping.mappings for value in (item["start"], item["end"] - 1)}
        gap = int(document["functions"][0]["end"], 16)
        assert mapping.lookup(gap) is None and image.contains(gap)
        addresses.add(gap)
        # Include the SDK assembly helper without a guessed C/C++ source line.
        jump_start = image.symbol_ranges_by_name["_setjmp"][0][0]
        addresses.add(jump_start)
        bias = 0x700000000
        native_log = work / "native-helper-boundaries.log"
        count = len(addresses)
        lines = [f"TRACE BEGIN format=1 capacity=16384 pid=74207 monotonic_ns=1 clock_known=1 retained={count} reserved={count} published={count} overwritten=0 incomplete=0 snapshot_raced=0",
                 "TRACE dropped_busy=0 dropped_race=0 dropped_stale=0 dropped_frozen=0 dropped_exhausted=0",
                 f"TRACE image_anchor_known=1 runtime_anchor={image.entry + bias:#x} linked_anchor={image.entry:#x} load_bias={bias:#x}",
                 "TRACE ordering=reservation_sequence thread=opaque_pthread_handle addresses=absolute"]
        for index, address in enumerate(sorted(addresses), 1):
            lines.append(f"T seq={index} thread=0x1 kind=phase(1) caller={address + bias:#x} a=213 b=0 c=0 d=0")
        lines.append(f"TRACE END retained={count} incomplete=0 snapshot_raced=0")
        native_log.write_text("\n".join(lines) + "\n")
        native_tool = shutil.which("llvm-addr2line") or shutil.which("addr2line")
        assert native_tool

        def elf_alias(name):
            path = work / name
            if path.is_symlink():
                path.unlink()
            assert not path.exists(), path
            path.symlink_to(target_elf)
            return path

        unmapped_elf = elf_alias("native-unmapped.elf")
        unmapped_sidecar = Path(str(unmapped_elf) + ".line-map.json")
        if unmapped_sidecar.exists():
            unmapped_sidecar.unlink()
        native_cli = ["python3", tool, "--elf", unmapped_elf, "--expected-sha256", image.sha256,
                      "--addr2line", native_tool]
        generic = json.loads(run("native-without-map-no-invented-lines", [*native_cli, native_log]))
        assert generic["line_map"] is None
        assert all(event["caller_symbol"]["resolution"] == "ambiguous_dwarf" and
                   event["caller_symbol"]["frames"] == [] for event in generic["sessions"][0]["events"])

        def check_native(result):
            assert result["line_map"]["elf_sha256_verified"]
            assert result["line_map"]["symbol_bounds_verified"]
            seen_helpers, unknown_lines = set(), 0
            for event in result["sessions"][0]["events"]:
                symbol = event["caller_symbol"]
                address = int(symbol["linked_address"], 16)
                expected = mapping.lookup(address)
                if expected is None:
                    assert symbol["resolution"] == "ambiguous_dwarf" and not symbol["frames"]
                    continue
                assert symbol["resolution"] == "verified_line_map"
                frame = symbol["frames"][0]
                assert frame == expected
                assert frame["file"] == "prospero/link_support.c"
                seen_helpers.add(frame["function"])
                if frame["line"] is None:
                    assert frame["location"].endswith(":?") and frame["line_status"] == "compiler_line_unavailable"
                    unknown_lines += 1
            assert seen_helpers == expected_helpers and unknown_lines > 0
            wrapper = next(event["caller_symbol"]["frames"][0] for event in result["sessions"][0]["events"]
                           if int(event["caller_symbol"]["linked_address"], 16) == 0x10)
            assert wrapper["line"] == 17 and wrapper["function"] == "__wrap_pthread_exit"

        explicit = json.loads(run("native-explicit-line-map", [*native_cli, "--line-map", line_map_path, native_log]))
        check_native(explicit)
        adjacent = Path(str(unmapped_elf) + ".line-map.json")
        adjacent.write_bytes(line_map_path.read_bytes())
        automatic = json.loads(run("native-adjacent-line-map", [*native_cli, native_log]))
        check_native(automatic)
        bad_hash = copy.deepcopy(document)
        bad_hash["elf_sha256"] = "0" * 64
        adjacent.write_text(json.dumps(bad_hash))
        assert "line map ELF SHA-256 mismatch" in run("native-adjacent-wrong-hash-refused", [*native_cli, native_log], 2)
        override = json.loads(run("native-explicit-overrides-adjacent", [*native_cli, "--line-map", line_map_path, native_log]))
        check_native(override)
        adjacent.unlink()

        validation_checks = []
        def rejected_map(name, modified, diagnostic):
            path = work / (name + ".json")
            path.write_text(modified if isinstance(modified, str) else json.dumps(modified))
            try:
                decoder.LineMap(path, image)
            except decoder.DecodeError as error:
                assert diagnostic in str(error), (name, str(error))
                log = capture / (name + ".txt")
                log.write_text(f"PASS rejected invalid mapping: {error}\n")
                validation_checks.append({"name": name, "status": "PASS", "operation": "LineMap construction with the actual PS5 ELF",
                                          "capture": str(log.relative_to(root)), "capture_sha256": digest(log)})
            else:
                raise AssertionError("invalid mapping accepted: " + name)
            return path

        rejected_map("map-wrong-elf-hash", bad_hash, "SHA-256 mismatch")
        missing_hash = copy.deepcopy(document); del missing_hash["elf_sha256"]
        rejected_map("map-missing-elf-hash", missing_hash, "elf_sha256")
        for name, field, value, diagnostic in [
            ("map-reversed-range", "end", "0x0", "bounds"),
            ("map-empty-range", "end", "0x10", "bounds"),
            ("map-range-over-64bits", "end", "0x10000000000000000", "64 bits"),
            ("map-range-outside-symbol", "end", "0x2b", "ELF function symbol"),
            ("map-wrong-symbol", "symbol", "ps5_ffs", "ELF function symbol"),
            ("map-boolean-start", "start", True, "invalid line map start"),
            ("map-negative-line", "line", -1, "line must"),
            ("map-boolean-line", "line", True, "line must"),
            ("map-negative-column", "column", -1, "column must"),
            ("map-empty-file", "file", "", "invalid line map file"),
            ("map-provenance-file-mismatch", "file", "sdk/stdexcept", "source_provenance"),
        ]:
            invalid = copy.deepcopy(document); invalid["mappings"][0][field] = value
            rejected_map(name, invalid, diagnostic)
        outside = copy.deepcopy(document)
        outside["mappings"][0]["start"] = "0xffffffffffffff00"
        outside["mappings"][0]["end"] = "0xffffffffffffff01"
        outside_path = rejected_map("map-outside-executable-range", outside, "bounds")
        assert "bounds" in run("native-invalid-range-cli-refused", [*native_cli, "--line-map", outside_path, native_log], 2)
        overlap = copy.deepcopy(document); overlap["mappings"].insert(1, copy.deepcopy(overlap["mappings"][0]))
        rejected_map("map-overlap", overlap, "overlap")
        unordered = copy.deepcopy(document); unordered["mappings"][0], unordered["mappings"][1] = unordered["mappings"][1], unordered["mappings"][0]
        rejected_map("map-unordered", unordered, "ordered")
        invalid = copy.deepcopy(document); invalid["functions"][0]["end"] = "0x29"
        rejected_map("map-false-function-size", invalid, "ELF symbol size")
        invalid = copy.deepcopy(document); invalid["schema_version"] = True
        rejected_map("map-invalid-schema", invalid, "schema_version")
        invalid = copy.deepcopy(document); invalid["source_provenance"]["sha256"] = "not-a-hash"
        rejected_map("map-invalid-source-digest", invalid, "source_provenance.sha256")
        duplicate = json.dumps(document).replace('"elf_sha256":', '"elf_sha256":"' + image.sha256 + '","elf_sha256":', 1)
        rejected_map("map-duplicate-json-field", duplicate, "duplicate JSON field")
        rejected_map("map-malformed-json", "{", "invalid line map JSON")
        assert "does not accept an ELF" in run("telemetry-rejects-line-map", [*live_cli, "--line-map", line_map_path, live_path], 2)
        sources.append(line_map_path)
        native_checks = {"status": "PASS", "elf_sha256": image.sha256, "elf_path": str(target_elf),
                         "elf_execution": "NOT_RUN", "line_map_sha256": digest(line_map_path),
                         "mapping_ranges": len(mapping.mappings), "sampled_addresses": count,
                         "function_names": sorted(expected_helpers), "invalid_mapping_checks": validation_checks,
                         "generic_lookup_policy": "ambiguous native helper and unbounded padding source locations withheld even when LLVM supplies a matching symbol name"}
    sources += [tool, root / "prospero/tests/trace_symbolizer_host.c", Path(__file__).resolve()]
    report = {"status": "PASS", "scope": "Actual production recorder and matched SDK allocator host fixtures; no PS5 execution",
              "fixture_elf": str(binary), "fixture_elf_sha256": sha,
              "native_helper_line_map": native_checks,
              "source_sha256": {str(path.relative_to(root)) if path.is_relative_to(root) else str(path): digest(path) for path in sources},
              "runs": runs, "checks": ["real backlink/address corruption traces recover allocation/free source functions",
              "explicit runtime/linked entry anchors and expected ELF digest", "raw memory code matches are candidates only",
              "appended sessions and reused PID", "distinct explicit load biases", "malformed/truncated/unanchored/mismatched input refusal",
              "optional explicitly anchored incomplete prefix", "stack bounds and non-code filtering",
              "healthy nonterminal FPS/heartbeat mode without ELF or addr2line", "explicit nonterminal session resets and ambiguous same-PID refusal",
              "quoted space-containing operational path and malformed operation preserved raw"]}
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    print(f"PASS: {len(runs)} build/run/decoder checks; real heap fixture callers resolved; raw stack candidates labeled")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
