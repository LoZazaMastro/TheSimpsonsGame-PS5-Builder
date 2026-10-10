#!/usr/bin/env python3
"""Decode PS5 trace logs with explicit image anchors and a matching debug ELF.

The ELF is read as data, never executed. Raw stack words are possible code
addresses, not an unwound backtrace. By default, truncated dumps are rejected;
--allow-incomplete can retain an anchored prefix while marking it incomplete.
Use --telemetry-only for running/stalled-session FPS and operational logs;
that mode needs neither an ELF nor addr2line and leaves all addresses raw.
An optional SHA-bound line map supplies verified source ranges when discarded
DWARF COMDAT units overlap real low-address helper functions.
"""
from __future__ import annotations

import argparse
import bisect
import hashlib
import json
from pathlib import Path
import re
import shlex
import shutil
import struct
import subprocess
import sys


class DecodeError(ValueError):
    pass


KINDS = {
    1: "phase", 2: "alloc_begin", 3: "alloc_end", 4: "free_begin",
    5: "free_end", 6: "realloc_begin", 7: "realloc_end", 8: "align_begin",
    9: "align_end", 10: "heap_detail", 11: "thread", 12: "fiber",
    13: "jump", 14: "heap_links", 15: "heap_check",
}
PHASES = {
    100: "DIAGNOSTICS_READY", 110: "CRT_LIBC_READY",
    111: "CRT_CONSTRUCTORS_DONE", 112: "CRT_MAIN_ENTER", 113: "CRT_MAIN_EXIT",
    200: "HOST_THREAD_START", 201: "HOST_THREAD_RETURN", 202: "HOST_THREAD_FINISH",
    210: "GUEST_THREAD_EXECUTE", 211: "FIBER_CONVERT_BEGIN", 212: "FIBER_CONVERT_END",
    213: "GUEST_CALL_BEGIN", 214: "GUEST_CALL_END", 215: "GUEST_THREAD_EXIT",
    220: "SIGNAL_THREAD_REGISTRATION_FAILED", 221: "SIGNAL_THREAD_BOUND",
    300: "INTERRUPT_SET", 301: "INTERRUPT_BEGIN", 302: "INTERRUPT_END",
    310: "GUEST_SWAP_BEGIN", 311: "GUEST_SWAP_END", 320: "PRESENT_BEGIN",
    321: "PRESENT_END", 322: "SUBMIT_ERROR", 400: "TERMINAL_SIGNAL",
    401: "EXPLICIT_ABORT",
    500: "INPUT_INIT", 501: "INPUT_USER", 502: "INPUT_CONNECTION",
    503: "INPUT_BUTTONS", 504: "INPUT_ANALOG", 505: "INPUT_CAPTURE",
    506: "INPUT_RUMBLE", 507: "INPUT_ERROR", 530: "AUDIO_INIT",
    531: "AUDIO_FIRST_SUBMIT", 532: "AUDIO_WORKER", 533: "AUDIO_STATS",
    534: "AUDIO_QUEUE", 535: "AUDIO_ERROR", 536: "AUDIO_SHUTDOWN",
    560: "FILE_OPEN", 561: "FILE_READ_ERROR", 562: "FILE_WRITE_ERROR",
    563: "FILE_SHORT", 564: "FILE_CLOSE", 565: "FILE_OTHER_ERROR",
    566: "VFS_OPEN", 567: "VFS_MOUNT", 568: "GUEST_IO_ERROR",
    569: "LOG_SUPPRESSED", 590: "PACING_MODE", 591: "DAC_MIXER",
    900: "METRICS_START", 901: "METRICS_WAIT_ERROR",
}
UINT64_LIMIT = 1 << 64
# These native PS5 helpers precede ordinary compiled code. In the linked image,
# discarded COMDAT line programs still cover their addresses. LLVM addr2line
# can combine the correct ELF symbol name with a different CU's source line,
# so even a matching name does not establish source provenance here.
PS5_HELPERS_REQUIRE_LINE_MAP = frozenset({
    "__wrap_pthread_exit", "ps5_ffs", "ps5_pathconf", "ps5_timegm",
    "_setjmp", "_longjmp", "___longjmp",
})


def unsigned(value: str, label: str) -> int:
    if not re.fullmatch(r"(?:0x[0-9a-fA-F]+|[0-9]+)", value):
        raise DecodeError(f"invalid unsigned integer for {label}: {value!r}")
    result = int(value, 16 if value.startswith("0x") else 10)
    if result >= UINT64_LIMIT:
        raise DecodeError(f"{label} exceeds 64 bits")
    return result


def fields(text: str, required=()) -> dict[str, str]:
    result = {}
    for token in text.split():
        match = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)=(\S+)", token)
        if not match or match[1] in result:
            raise DecodeError(f"malformed or duplicate field: {token!r}")
        result[match[1]] = match[2]
    missing = set(required) - result.keys()
    if missing:
        raise DecodeError("missing fields: " + ", ".join(sorted(missing)))
    return result


def numeric_fields(text: str, required=()) -> dict[str, int]:
    return {key: unsigned(value, key) for key, value in fields(text, required).items()}


class ElfImage:
    def __init__(self, path: Path, expected_sha256: str | None):
        self.path = path.resolve()
        data = self.path.read_bytes()
        self.sha256 = hashlib.sha256(data).hexdigest()
        if expected_sha256 is not None:
            if not re.fullmatch(r"[0-9a-fA-F]{64}", expected_sha256):
                raise DecodeError("--expected-sha256 must contain exactly 64 hexadecimal digits")
            if self.sha256 != expected_sha256.lower():
                raise DecodeError(f"ELF SHA-256 mismatch: expected {expected_sha256.lower()}, got {self.sha256}")
        if len(data) < 64 or data[:6] != b"\x7fELF\x02\x01":
            raise DecodeError("--elf must be a little-endian ELF64 file")
        kind, machine = struct.unpack_from("<HH", data, 16)
        if machine != 62 or kind not in (2, 3, 0xFE10):
            raise DecodeError("--elf must be a linked x86-64 executable, not an object or another architecture")
        self.entry, phoff = struct.unpack_from("<QQ", data, 24)
        phsize, phcount = struct.unpack_from("<HH", data, 54)
        if phsize != 56 or not phcount or phoff + phsize * phcount > len(data):
            raise DecodeError("ELF program headers are missing or out of bounds")
        self.executable = []
        for index in range(phcount):
            ptype, flags, offset, address, _, stored, memory, alignment = struct.unpack_from(
                "<IIQQQQQQ", data, phoff + index * phsize)
            if offset + stored > len(data) or address + memory >= UINT64_LIMIT:
                raise DecodeError("ELF program segment is out of bounds")
            if ptype == 1 and flags & 1 and stored:
                if stored > memory:
                    raise DecodeError("ELF executable segment is larger than its memory mapping")
                self.executable.append((address, address + stored))
        if not self.contains(self.entry):
            raise DecodeError("ELF entry is outside file-backed executable PT_LOAD segments")
        self.function_symbols = self._read_function_symbols(data)
        self.symbol_starts = [item[0] for item in self.function_symbols]
        self.symbol_max_ends = []
        high = 0
        for _, end, _ in self.function_symbols:
            high = max(high, end)
            self.symbol_max_ends.append(high)
        self.symbol_ranges_by_name = {}
        for start, end, name in self.function_symbols:
            self.symbol_ranges_by_name.setdefault(name, []).append((start, end))

    def contains(self, address: int) -> bool:
        return any(low <= address < high for low, high in self.executable)

    def contains_range(self, start: int, end: int) -> bool:
        return start < end and any(low <= start < end <= high for low, high in self.executable)

    def symbols_at(self, address: int) -> list[str]:
        result = set()
        index = bisect.bisect_right(self.symbol_starts, address) - 1
        while index >= 0 and self.symbol_max_ends[index] > address:
            start, end, name = self.function_symbols[index]
            if start <= address < end:
                result.add(name)
            index -= 1
        return sorted(result)

    def _read_function_symbols(self, data: bytes) -> list[tuple[int, int, str]]:
        shoff = struct.unpack_from("<Q", data, 40)[0]
        shsize, shcount = struct.unpack_from("<HH", data, 58)
        if shoff == 0:
            if shcount:
                raise DecodeError("ELF section table is missing")
            return []
        if shsize != 64 or shoff + shsize > len(data):
            raise DecodeError("ELF section headers are out of bounds")
        if not shcount:  # ELF extended section numbering.
            shcount = struct.unpack_from("<Q", data, shoff + 32)[0]
        if not shcount or shoff + shsize * shcount > len(data):
            raise DecodeError("ELF section table is out of bounds")
        sections = [struct.unpack_from("<IIQQQQIIQQ", data, shoff + index * shsize)
                    for index in range(shcount)]
        symbols = set()
        for section in sections:
            _, kind, _, _, offset, size, linked, _, _, entry_size = section
            if kind not in (2, 11):  # SHT_SYMTAB / SHT_DYNSYM.
                continue
            if entry_size != 24 or size % entry_size or offset + size > len(data) or linked >= shcount:
                raise DecodeError("ELF symbol table is out of bounds or malformed")
            strings = sections[linked]
            string_offset, string_size = strings[4:6]
            if strings[1] != 3 or string_offset + string_size > len(data):
                raise DecodeError("ELF symbol string table is out of bounds")
            string_table = data[string_offset:string_offset + string_size]
            for position in range(offset, offset + size, entry_size):
                name_offset, info, _, index, start, length = struct.unpack_from("<IBBHQQ", data, position)
                if (info & 15) not in (2, 10) or index == 0 or not length:
                    continue  # Ignore imports, non-functions and unbounded aliases.
                end = start + length
                if end >= UINT64_LIMIT or not self.contains_range(start, end):
                    continue
                if name_offset >= string_size:
                    raise DecodeError("ELF function name is outside its string table")
                stop = string_table.find(b"\0", name_offset)
                if stop < 0:
                    raise DecodeError("ELF function name is unterminated")
                name = string_table[name_offset:stop].decode("utf-8", errors="strict")
                if name:
                    symbols.add((start, end, name))
        return sorted(symbols)


class LineMap:
    """Only explicitly bounded, ELF-identity-checked source locations are used."""
    def __init__(self, path: Path, elf: ElfImage):
        self.path = path.resolve()
        data = self.path.read_bytes()
        self.sha256 = hashlib.sha256(data).hexdigest()

        def unique_object(pairs):
            result = {}
            for key, value in pairs:
                if key in result:
                    raise DecodeError(f"line map has duplicate JSON field {key!r}")
                result[key] = value
            return result

        try:
            def invalid_constant(value):
                raise DecodeError(f"invalid line map JSON constant {value}")
            document = json.loads(data, object_pairs_hook=unique_object, parse_constant=invalid_constant)
        except (json.JSONDecodeError, UnicodeError) as error:
            raise DecodeError(f"invalid line map JSON: {error}") from error
        if not isinstance(document, dict):
            raise DecodeError("line map must be a JSON object")
        expected = document.get("elf_sha256")
        if not isinstance(expected, str) or not re.fullmatch(r"[0-9a-fA-F]{64}", expected):
            raise DecodeError("line map elf_sha256 must contain 64 hexadecimal digits")
        if expected.lower() != elf.sha256:
            raise DecodeError(f"line map ELF SHA-256 mismatch: expected {expected.lower()}, got {elf.sha256}")
        if "schema_version" in document and (type(document["schema_version"]) is not int or document["schema_version"] != 1):
            raise DecodeError("unsupported line map schema_version")
        if "status" in document and document["status"] != "PASS":
            raise DecodeError("line map producer status is not PASS")

        def text_field(value, label):
            if (not isinstance(value, str) or not value.strip() or value != value.strip() or
                    any(ord(char) < 32 or ord(char) == 127 for char in value)):
                raise DecodeError(f"invalid line map {label}")
            return value

        def bound(value, label):
            if type(value) is int:
                if 0 <= value < UINT64_LIMIT:
                    return value
            elif isinstance(value, str):
                return unsigned(value, "line map " + label)
            raise DecodeError(f"invalid line map {label}")

        def symbol_range(item):
            if not isinstance(item, dict) or not {"start", "end", "symbol"} <= item.keys():
                raise DecodeError("line map range is missing start/end/symbol")
            start, end = bound(item["start"], "start"), bound(item["end"], "end")
            symbol = text_field(item["symbol"], "symbol")
            if not elf.contains_range(start, end):
                raise DecodeError("line map range is empty, reversed or outside executable ELF bounds")
            ranges = elf.symbol_ranges_by_name.get(symbol, [])
            if not any(low <= start < end <= high for low, high in ranges):
                raise DecodeError(f"line map range does not belong to ELF function symbol {symbol!r}")
            return start, end, symbol

        provenance = document.get("source_provenance")
        if provenance is not None:
            if not isinstance(provenance, dict):
                raise DecodeError("line map source_provenance must be an object")
            text_field(provenance.get("file"), "source_provenance.file")
            for key, count in (("sha256", 64), ("md5", 32)):
                value = provenance.get(key)
                if not isinstance(value, str) or not re.fullmatch(rf"[0-9a-fA-F]{{{count}}}", value):
                    raise DecodeError(f"invalid line map source_provenance.{key}")
        declared = document.get("functions")
        if declared is not None:
            if not isinstance(declared, list) or not declared:
                raise DecodeError("line map functions must be a nonempty array")
            declared = [symbol_range(item) for item in declared]
            previous_end = 0
            for start, end, name in declared:
                if start < previous_end:
                    raise DecodeError("line map function ranges overlap or are not ordered")
                previous_end = end
                if (start, end) not in elf.symbol_ranges_by_name[name]:
                    raise DecodeError("line map function bounds differ from its ELF symbol size")
        mappings = document.get("mappings")
        if not isinstance(mappings, list) or not mappings:
            raise DecodeError("line map mappings must be a nonempty array")
        self.mappings = []
        previous_end = 0
        for item in mappings:
            start, end, symbol = symbol_range(item)
            if self.mappings and start < previous_end:
                raise DecodeError("line map ranges overlap or are not ordered")
            previous_end = end
            source = text_field(item.get("file"), "file")
            if provenance is not None and source != provenance["file"]:
                raise DecodeError("line map file differs from its source_provenance file")
            line, column = item.get("line"), item.get("column", 0)
            if type(line) is not int or not 0 <= line <= 0x7fffffff:
                raise DecodeError("line map line must be a nonnegative source line number")
            if type(column) is not int or not 0 <= column <= 0x7fffffff:
                raise DecodeError("line map column must be a nonnegative integer")
            if declared is not None and not any(name == symbol and low <= start < end <= high
                                               for low, high, name in declared):
                raise DecodeError("line map mapping is outside its declared function")
            self.mappings.append({"start": start, "end": end, "symbol": symbol,
                                  "file": source, "line": line, "column": column})
        self.starts = [item["start"] for item in self.mappings]
        self.provenance = provenance

    def lookup(self, address: int) -> dict | None:
        index = bisect.bisect_right(self.starts, address) - 1
        if index >= 0 and address < self.mappings[index]["end"]:
            item = self.mappings[index]
            return {"function": item["symbol"],
                    "location": f"{item['file']}:{item['line']}" if item["line"] else f"{item['file']}:?",
                    "inline_depth": 0, "file": item["file"], "line": item["line"] or None,
                    "column": item["column"], "line_status": "available" if item["line"] else "compiler_line_unavailable",
                    "resolution": "verified_line_map", "range_start": hex(item["start"]),
                    "range_end_exclusive": hex(item["end"])}
        return None

    def metadata(self) -> dict:
        return {"path": str(self.path), "sha256": self.sha256, "elf_sha256_verified": True,
                "range_count": len(self.mappings), "symbol_bounds_verified": True,
                "recorded_source_provenance": self.provenance,
                "source_identity_policy": "source checksum is recorded producer evidence; the decoder validates ELF identity and symbol bounds"}


def new_session(path: Path, line: int, number: int, pid=None) -> dict:
    return {
        "source": str(path), "session": number, "line_start": line, "line_end": line,
        "pid": pid, "trace_begin": None, "trace_end": None, "anchors": None,
        "drops": None, "ordering": None, "events": [], "checkpoints": [],
        "telemetry": [], "raw_stacks": [], "warnings": [],
    }


def parse_log(path: Path, allow_incomplete: bool, telemetry_only: bool = False) -> list[dict]:
    sessions = []
    current = None
    stack = None
    text = path.read_text(encoding="utf-8", errors="strict")

    def start(line: int, pid=None):
        nonlocal current, stack
        current = new_session(path, line, len(sessions) + 1, pid)
        sessions.append(current)
        stack = None
        return current

    def pending(line: int, pid=None, checkpoint=None):
        nonlocal current
        if current is None:
            return start(line, pid)
        if pid is not None and current["pid"] is not None and pid != current["pid"]:
            return start(line, pid)
        if current["trace_end"] is not None:
            # The recorder freezes at its terminal dump. An explicit startup
            # checkpoint can identify a later run even if the OS reused a PID.
            if checkpoint is not None and checkpoint["phase"] in (100, 110, 111, 112):
                return start(line, pid)
            raise DecodeError("telemetry after a terminal dump has no unambiguous new-session boundary")
        if checkpoint is not None and current["checkpoints"] and checkpoint["phase"] in (100, 110, 111, 112):
            previous = current["checkpoints"][-1]
            old_time = unsigned(previous["monotonic_ns"], "monotonic_ns")
            sequence_reset = checkpoint["seq"] > 0 and previous["seq"] > 0 and checkpoint["seq"] <= previous["seq"]
            time_reset = checkpoint["monotonic_ns"] > 0 and old_time > 0 and checkpoint["monotonic_ns"] < old_time
            if sequence_reset or time_reset:
                return start(line, pid)
            if checkpoint["phase"] == 100 and any(item["phase"] == 100 for item in current["checkpoints"]):
                raise DecodeError("repeated startup for the same PID has no sequence/time reset; session boundary is ambiguous")
        if current["pid"] is None:
            current["pid"] = pid
        return current

    for number, original in enumerate(text.splitlines(), 1):
        line = original.strip()
        ps5_prefix = line.startswith("[PPSA74207] ")
        if ps5_prefix:
            line = line[len("[PPSA74207] "):]
        try:
            if line.startswith("TRACE BEGIN "):
                begin = numeric_fields(line[len("TRACE BEGIN "):],
                                       ("format", "capacity", "pid", "monotonic_ns", "clock_known", "retained", "reserved",
                                        "published", "overwritten", "incomplete", "snapshot_raced"))
                if begin["format"] != 1 or not 0 < begin["capacity"] <= 1048576:
                    raise DecodeError("unsupported TRACE format or capacity")
                if current is not None and current["trace_begin"] is not None:
                    if current["trace_end"] is None and not allow_incomplete:
                        raise DecodeError("new TRACE BEGIN before the previous TRACE END; truncated or interleaved sessions")
                    start(number, begin["pid"])
                else:
                    pending(number, begin["pid"])
                current["trace_begin"] = begin
            elif line.startswith("TRACE END "):
                if current is None or current["trace_begin"] is None or current["trace_end"] is not None:
                    raise DecodeError("TRACE END without one open TRACE BEGIN")
                current["trace_end"] = numeric_fields(line[len("TRACE END "):],
                                                      ("retained", "incomplete", "snapshot_raced"))
            elif line.startswith("TRACE image_anchor_known="):
                if current is None or current["trace_begin"] is None or current["trace_end"] is not None:
                    raise DecodeError("image anchors are outside an open TRACE block")
                if current["anchors"] is not None:
                    raise DecodeError("duplicate image anchors in one trace session")
                current["anchors"] = numeric_fields(line[len("TRACE "):],
                    ("image_anchor_known", "runtime_anchor", "linked_anchor", "load_bias"))
            elif line.startswith("TRACE dropped_"):
                if current is None or current["trace_begin"] is None or current["trace_end"] is not None or current["drops"] is not None:
                    raise DecodeError("drop counters are duplicated or outside an open TRACE block")
                current["drops"] = numeric_fields(line[len("TRACE "):],
                    ("dropped_busy", "dropped_race", "dropped_stale", "dropped_frozen", "dropped_exhausted"))
            elif line.startswith("TRACE ordering="):
                if current is None or current["trace_begin"] is None or current["trace_end"] is not None:
                    raise DecodeError("ordering descriptor is outside an open TRACE block")
                value = fields(line[len("TRACE "):], ("ordering", "thread", "addresses"))
                if value != {"ordering": "reservation_sequence", "thread": "opaque_pthread_handle", "addresses": "absolute"}:
                    raise DecodeError("unsupported trace address/ordering convention")
                if current["ordering"] is not None:
                    raise DecodeError("duplicate ordering descriptor")
                current["ordering"] = value
            elif line.startswith("T "):
                if current is None or current["trace_begin"] is None or current["trace_end"] is not None:
                    raise DecodeError("event outside an open TRACE block")
                values = fields(line[2:], ("seq", "thread", "kind", "caller", "a", "b", "c", "d"))
                kind = re.fullmatch(r"([a-z_]+)\(([0-9]+)\)", values["kind"])
                if not kind:
                    raise DecodeError("malformed event kind")
                kind_number = unsigned(kind[2], "kind")
                if kind[1] != KINDS.get(kind_number, "unknown"):
                    raise DecodeError("event kind name does not match its stable numeric code")
                event = {"line": number, "seq": unsigned(values["seq"], "seq"),
                         "kind": kind[1], "kind_id": kind_number,
                         **{key: values[key] for key in ("thread", "caller", "a", "b", "c", "d")}}
                for key in ("thread", "caller", "a", "b", "c", "d"):
                    unsigned(event[key], key)
                if event["seq"] == 0 or (current["events"] and event["seq"] <= current["events"][-1]["seq"]):
                    raise DecodeError("event reservation sequences must be positive and strictly increasing")
                if kind_number == 1:
                    event["phase"] = unsigned(event["a"], "phase")
                if kind_number == 15:
                    detail = unsigned(event["d"], "heap_check.d")
                    event["heap_check"] = {"failure_flags": detail >> 32, "source_line": detail & 0xffffffff}
                current["events"].append(event)
            elif line.startswith("trace-checkpoint "):
                values = fields(line[len("trace-checkpoint "):],
                    ("seq", "pid", "monotonic_ns", "clock_known", "thread", "phase", "caller", "b", "c", "d"))
                parsed = {key: unsigned(value, key) for key, value in values.items()}
                pending(number, parsed["pid"], parsed)
                current["checkpoints"].append({"line": number, **values, "phase": parsed["phase"], "seq": parsed["seq"]})
            elif (re.match(r"(?:METRICS|HEARTBEAT|MEMORY|AUDIO|INPUT|FILES|FILE_IO|RUNTIME)\b", line) or
                  (ps5_prefix and re.match(r"[A-Z][A-Z0-9_]* ", line) and line.partition(" ")[0] not in ("RAW", "TRACE", "T", "S"))):
                category, _, rest = line.partition(" ")
                flags, values, parse_warning = [], {}, None
                try:
                    # This tokenizes data only; no shell or command evaluation.
                    # Keep the original line even when an operational path is
                    # quoted, escaped, or a future schema cannot be tokenized.
                    for token in shlex.split(rest, posix=True):
                        if "=" not in token:
                            flags.append(token)
                            continue
                        key, value = token.split("=", 1)
                        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", key) or key in values:
                            raise ValueError("malformed or duplicate telemetry field")
                        values[key] = value
                except ValueError as error:
                    values, flags = {}, []
                    parse_warning = str(error)
                pid = unsigned(values["pid"], "pid") if "pid" in values else None
                pending(number, pid)
                current["telemetry"].append({"line": number, "category": category, "fields": values,
                    "flags": flags, "raw_line": original, "parse_warning": parse_warning})
            elif line.startswith("RAW STACK BEGIN "):
                if current is None or current["trace_begin"] is None:
                    raise DecodeError("raw stack has no preceding trace session")
                if stack is not None and stack["end"] is None:
                    raise DecodeError("nested RAW STACK BEGIN")
                values = numeric_fields(line[len("RAW STACK BEGIN "):],
                    ("thread", "pid", "registered", "rsp", "low", "high", "limit_bytes"))
                if values["pid"] != current["pid"]:
                    raise DecodeError("raw stack PID differs from its trace session")
                if values["registered"] not in (0, 1) or values["limit_bytes"] > 65536:
                    raise DecodeError("unsupported raw stack bounds")
                stack = {"line": number, "begin": values, "end": None, "rows": [],
                         "code_address_candidates": [], "interpretation": "memory words, not an unwound backtrace"}
                current["raw_stacks"].append(stack)
            elif line.startswith("S address="):
                if stack is None or stack["end"] is not None:
                    raise DecodeError("stack words outside an open RAW STACK block")
                match = re.fullmatch(r"S address=(0x[0-9a-fA-F]+)((?: 0x[0-9a-fA-F]+){1,4})", line)
                if not match:
                    raise DecodeError("malformed raw stack row")
                address = unsigned(match[1], "stack address")
                words = match[2].split()
                for word in words:
                    unsigned(word, "stack word")
                begin = stack["begin"]
                expected = ((begin["rsp"] + 7) & ~7) + sum(len(row["words"]) * 8 for row in stack["rows"])
                if (not begin["registered"] or not begin["low"] <= begin["rsp"] < begin["high"] or
                    address != expected or address + len(words) * 8 > begin["high"] or
                    address + len(words) * 8 - ((begin["rsp"] + 7) & ~7) > begin["limit_bytes"]):
                    raise DecodeError("raw stack row violates its explicit registered bounds/order")
                stack["rows"].append({"line": number, "address": match[1], "words": words})
            elif line.startswith("RAW STACK END "):
                if stack is None or stack["end"] is not None:
                    raise DecodeError("RAW STACK END without an open RAW STACK block")
                stack["end"] = numeric_fields(line[len("RAW STACK END "):], ("bytes",))
                stored = sum(len(row["words"]) * 8 for row in stack["rows"])
                if stack["end"]["bytes"] != stored:
                    raise DecodeError("raw stack byte count differs from parsed words")
            elif line.startswith(("TRACE BEGIN", "TRACE END", "RAW STACK BEGIN", "RAW STACK END")):
                raise DecodeError("truncated diagnostic delimiter")
            if current is not None:
                current["line_end"] = number
        except DecodeError as error:
            raise DecodeError(f"{path}:{number}: {error}") from error

    if not sessions:
        raise DecodeError(f"{path}: no structured TRACE session found")
    for session in sessions:
        label = f"{path}, session {session['session']}"
        begin, end = session["trace_begin"], session["trace_end"]
        if begin is None and telemetry_only:
            session["completeness"] = {
                "trace_dump_present": False, "framing_complete": False,
                "complete_event_history": False, "loss_counters": None,
                "note": "nonterminal telemetry window; no terminal ring dump or complete event history is claimed",
            }
            continue
        if begin is None:
            raise DecodeError(f"{label}: telemetry has no terminal TRACE BEGIN/image anchors; refusing guessed symbolization")
        if end is None and not allow_incomplete and not telemetry_only:
            raise DecodeError(f"{label}: truncated TRACE: TRACE END is missing (use --allow-incomplete only for an anchored prefix)")
        if not telemetry_only and (session["anchors"] is None or session["anchors"]["image_anchor_known"] != 1):
            raise DecodeError(f"{label}: explicit image anchors are missing or unknown; refusing guessed load bias")
        if (session["drops"] is None or session["ordering"] is None) and not telemetry_only:
            raise DecodeError(f"{label}: trace metadata is missing or truncated")
        count = len(session["events"])
        if count > begin["capacity"] or count > begin["retained"]:
            raise DecodeError(f"{label}: event count exceeds declared retention/capacity")
        if end is not None and (count != begin["retained"] or end["retained"] != count or
                end["incomplete"] != begin["incomplete"] or end["snapshot_raced"] != begin["snapshot_raced"]):
            raise DecodeError(f"{label}: TRACE header/footer/event counts disagree")
        if session["events"] and session["events"][-1]["seq"] > begin["reserved"]:
            raise DecodeError(f"{label}: event sequence exceeds the declared reservation counter")
        raw_complete = all(item["end"] is not None for item in session["raw_stacks"])
        if not raw_complete and not allow_incomplete and not telemetry_only:
            raise DecodeError(f"{label}: truncated RAW STACK dump")
        framing = end is not None and raw_complete and count == begin["retained"]
        losses = {key: begin[key] for key in ("overwritten", "incomplete", "snapshot_raced")}
        losses.update(session["drops"] or {})
        sequences_cover_history = count == begin["reserved"] == begin["published"] and all(
            event["seq"] == index for index, event in enumerate(session["events"], 1))
        session["completeness"] = {
            "trace_dump_present": True, "framing_complete": framing, "parsed_events": count,
            "declared_retained": begin["retained"], "loss_counters": losses,
            "complete_event_history": not telemetry_only and framing and sequences_cover_history and not any(losses.values()),
            "sequences_cover_reserved_published_history": sequences_cover_history,
            "ordering_note": "reservation sequence; not timestamps or a total order of event completion",
        }
    return sessions


def read_phase_names() -> dict[int, str]:
    result = dict(PHASES)
    include = Path(__file__).resolve().parents[2] / "tools/rexglue-sdk/include/rex/platform"
    for filename in ("trace_phases_prospero.h", "trace_game_prospero.h"):
        header = include / filename
        if header.is_file():
            for name, value in re.findall(r"\bPROSPERO_(?:PHASE|GAME)_([A-Z0-9_]+)\s*=\s*([0-9]+)", header.read_text()):
                result[int(value)] = name
    return result


def symbolize_addresses(elf: ElfImage, addresses: set[int], executable: str,
                        line_map: LineMap | None = None) -> dict[int, dict]:
    def resolve(batch, demangle):
        command = [executable, "-a", "-f", "-i", *( ["-C"] if demangle else []), "-e", str(elf.path)]
        output = subprocess.run(command, input="".join(f"{address:#x}\n" for address in batch),
                                text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
        if output.returncode:
            raise DecodeError(f"addr2line failed ({output.returncode}): {output.stderr.strip()}")
        groups = {}
        current = None
        for line in output.stdout.splitlines():
            if re.fullmatch(r"0x[0-9a-fA-F]+", line):
                current = int(line, 16)
                if current in groups:
                    raise DecodeError("addr2line returned a duplicate address group")
                groups[current] = []
            elif current is None:
                raise DecodeError("addr2line returned output before its address marker")
            else:
                groups[current].append(line)
        if set(groups) != set(batch):
            raise DecodeError("addr2line output does not match requested code addresses")
        frames = {}
        for address, lines in groups.items():
            if len(lines) % 2:
                raise DecodeError("addr2line returned an incomplete function/location pair")
            frames[address] = [{"function": lines[index], "location": lines[index + 1],
                                "inline_depth": index // 2} for index in range(0, len(lines), 2)]
        return frames

    result, pending = {}, []
    for address in sorted(addresses):
        mapped = line_map.lookup(address) if line_map is not None else None
        if mapped is not None:
            result[address] = {"frames": [mapped], "resolution": "verified_line_map",
                               "elf_symbols": elf.symbols_at(address), "symbol_bounds_verified": True}
        elif PS5_HELPERS_REQUIRE_LINE_MAP.intersection(elf.symbols_at(address)):
            result[address] = {
                "frames": [], "resolution": "ambiguous_dwarf", "elf_symbols": elf.symbols_at(address),
                "warning": "PS5 native helper source provenance requires a verified line map; matching addr2line symbol names alone cannot reject discarded COMDAT lines"}
        else:
            pending.append(address)
    for offset in range(0, len(pending), 256):
        batch = pending[offset:offset + 256]
        raw = resolve(batch, False)
        accepted = []
        for address in batch:
            names = elf.symbols_at(address)
            outer = raw[address][-1]["function"] if raw[address] else None
            if outer not in names:
                # A discarded COMDAT's DWARF may still claim an interval at
                # zero. Executable-segment membership alone does not validate
                # the file/line returned by a generic addr2line lookup.
                result[address] = {
                    "frames": [], "resolution": "ambiguous_dwarf", "elf_symbols": names,
                    "warning": "addr2line outer function does not match a bounded ELF function symbol; source locations withheld",
                    "rejected_outer_function": outer}
            else:
                accepted.append(address)
        if accepted:
            demangled = resolve(accepted, True)
            for address in accepted:
                if ([item["location"] for item in raw[address]] !=
                        [item["location"] for item in demangled[address]]):
                    raise DecodeError("addr2line changed source locations while demangling the same address")
                result[address] = {"frames": demangled[address], "resolution": "symbol_checked_addr2line",
                                   "elf_symbols": elf.symbols_at(address),
                                   "symbol_consistency": "outer_function_matches_ELF"}
    return result


def annotate(sessions: list[dict], elf: ElfImage, addr2line: str, line_map: LineMap | None = None) -> None:
    requested = set()
    candidates = []
    phase_names = read_phase_names()
    for session in sessions:
        anchor = session["anchors"]
        runtime, linked, bias = anchor["runtime_anchor"], anchor["linked_anchor"], anchor["load_bias"]
        if linked != elf.entry:
            raise DecodeError(f"{session['source']}, session {session['session']}: ELF entry {elf.entry:#x} does not match logged linked_anchor {linked:#x}; wrong debug ELF")
        if runtime == 0 or runtime < linked or runtime - linked != bias:
            raise DecodeError(f"{session['source']}, session {session['session']}: inconsistent explicit anchor/load_bias values")
        session["image_validation"] = {
            "entry_matches_linked_anchor": True, "runtime_anchor": hex(runtime),
            "linked_anchor": hex(linked), "load_bias": hex(bias),
            "elf_sha256": elf.sha256,
            "identity_limit": "entry match validates the anchor; exact build identity additionally requires a trusted expected ELF SHA-256",
        }

        def address_info(raw: str) -> dict:
            absolute = unsigned(raw, "absolute PC")
            info = {"absolute": raw, "load_bias": hex(bias), "linked_address": None,
                    "status": "below_load_bias", "frames": []}
            if absolute >= bias:
                adjusted = absolute - bias
                info["linked_address"] = hex(adjusted)
                if elf.contains(adjusted):
                    info["status"] = "in_elf_executable_segment"
                    requested.add(adjusted)
                    candidates.append((info, adjusted))
                else:
                    info["status"] = "outside_elf_executable_segments"
            return info

        for event in session["events"] + session["checkpoints"]:
            event["caller_symbol"] = address_info(event["caller"])
            if "phase" in event:
                event["phase_name"] = phase_names.get(event["phase"], f"UNKNOWN_PHASE_{event['phase']}")
        for raw_stack in session["raw_stacks"]:
            for row in raw_stack["rows"]:
                for index, word in enumerate(row["words"]):
                    absolute = unsigned(word, "raw stack word")
                    if absolute >= bias and elf.contains(absolute - bias):
                        raw_stack["code_address_candidates"].append({
                            "stack_word_address": hex(unsigned(row["address"], "stack address") + index * 8),
                            "value": word, "classification": "CANDIDATE_ONLY_UNWIND_NOT_PERFORMED",
                            "symbol": address_info(word),
                        })
    symbols = symbolize_addresses(elf, requested, addr2line, line_map)
    for info, address in candidates:
        info.update(symbols[address])


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logs", nargs="+", type=Path, help="fault/trace logs; appended sessions supported")
    parser.add_argument("--elf", type=Path, help="matching linked debug ELF, required for symbolization (read only; never executed)")
    parser.add_argument("--telemetry-only", action="store_true", help="decode running/stalled-session telemetry without an ELF, addr2line, or address inference")
    parser.add_argument("--expected-sha256", help="trusted debug ELF SHA-256, recommended for exact build matching")
    parser.add_argument("--line-map", type=Path, help="verified source-range JSON bound to this ELF SHA-256; defaults to ELF + '.line-map.json' when present")
    parser.add_argument("--addr2line", default="addr2line", help="GNU or LLVM addr2line executable")
    parser.add_argument("--allow-incomplete", action="store_true", help="decode an explicitly anchored truncated prefix and mark it incomplete")
    parser.add_argument("--output", type=Path, help="write JSON here; default is stdout")
    args = parser.parse_args(argv)
    try:
        elf, executable, line_map = None, None, None
        if args.telemetry_only:
            if args.elf is not None or args.expected_sha256 is not None or args.line_map is not None:
                raise DecodeError("--telemetry-only does not accept an ELF, ELF digest or line map; addresses remain raw")
        else:
            if args.elf is None:
                raise DecodeError("--elf is required for symbolization; use --telemetry-only for a running/stalled-session log")
            elf = ElfImage(args.elf, args.expected_sha256)
            map_path = args.line_map
            if map_path is None:
                adjacent = Path(str(args.elf) + ".line-map.json")
                if adjacent.exists():
                    map_path = adjacent
            if map_path is not None:
                line_map = LineMap(map_path, elf)
            executable = shutil.which(args.addr2line)
            if not executable:
                raise DecodeError(f"addr2line executable not found: {args.addr2line}")
        sessions = []
        inputs = []
        for source in args.logs:
            source = source.resolve()
            inputs.append({"path": str(source), "sha256": hashlib.sha256(source.read_bytes()).hexdigest()})
            sessions.extend(parse_log(source, args.allow_incomplete, args.telemetry_only))
        if args.telemetry_only:
            names = read_phase_names()
            for session in sessions:
                for event in session["events"] + session["checkpoints"]:
                    if "phase" in event:
                        event["phase_name"] = names.get(event["phase"], f"UNKNOWN_PHASE_{event['phase']}")
        else:
            annotate(sessions, elf, executable, line_map)
        result = {
            "format": "prospero-symbolized-trace-1",
            "status": "TELEMETRY_ONLY" if args.telemetry_only else "COMPLETE" if all(s["completeness"]["framing_complete"] for s in sessions) else "PARTIAL",
            "elf": None if elf is None else {"path": str(elf.path), "sha256": elf.sha256, "expected_sha256_verified": args.expected_sha256 is not None,
                    "entry": hex(elf.entry), "executable_file_ranges": [{"start": hex(a), "end_exclusive": hex(b)} for a, b in elf.executable]},
            "inputs": inputs, "sessions": sessions,
            "line_map": None if line_map is None else line_map.metadata(),
            "address_policy": "raw recorded addresses only; no ELF read, addr2line, symbolization or inferred bias" if args.telemetry_only else "only explicit per-session anchor bias, exact recorded PCs, executable PT_LOAD filtering; no guessed bases or return-address decrement",
            "stack_policy": "raw stack matches are possible code addresses, not proof of a call chain or a backtrace",
            "source_policy": "not applied in telemetry-only mode" if args.telemetry_only else "line-map ranges require ELF SHA-256 and symbol bounds; ambiguous PS5 native helpers require that map; other generic DWARF locations require an outer function matching a containing ELF symbol, which checks symbol association rather than full CU provenance",
            "execution_policy": "telemetry-only mode reads log text and invokes no address-resolution program" if args.telemetry_only else "the supplied ELF is read as data and passed to addr2line; it is never executed",
        }
        rendered = json.dumps(result, indent=2) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(rendered)
        else:
            sys.stdout.write(rendered)
        return 0
    except (DecodeError, OSError, UnicodeError, struct.error, subprocess.SubprocessError) as error:
        print(f"trace decoding refused: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
