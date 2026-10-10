#!/usr/bin/env python3
"""Check native ELF structure and this project's optional import policy."""
import argparse
import json
from pathlib import Path
import struct


# These are intentionally avoided by this project's SDK wrappers, not
# universally forbidden to titles. Sonic's real release imports them.
PROJECT_EXCLUDED_MODULES = {
    "libkernel_sys.prx", "libkernel_sys.sprx",
    "libkernel_web.prx", "libkernel_web.sprx",
    "libScePosixForWebKit.prx", "libScePosixForWebKit.sprx",
}


def inspect_native(data: bytes, *, enforce_project_import_policy: bool = True) -> dict:
    if len(data) < 64 or data[:9] != b"\x7fELF\x02\x01\x01\x09\x02":
        raise ValueError("expected ELF64 little-endian FreeBSD ABI 2")
    kind, machine = struct.unpack_from("<HH", data, 16)
    if (kind, machine) != (0xFE10, 62):
        raise ValueError("expected PS5 dynamic executable type 0xFE10, AMD64")
    entry, phoff = struct.unpack_from("<QQ", data, 24)
    phsize, count = struct.unpack_from("<HH", data, 54)
    if phsize != 56 or not count or phoff + count * phsize > len(data):
        raise ValueError("invalid program header table")
    headers = []
    for index in range(count):
        ptype, flags, offset, address, _, size, memory, alignment = struct.unpack_from(
            "<IIQQQQQQ", data, phoff + index * phsize)
        if ptype == 3:
            raise ValueError("a native title must not have PT_INTERP")
        if offset + size > len(data):
            raise ValueError("program segment extends beyond the ELF")
        if ptype == 1:
            if alignment != 0x4000 or offset % alignment != address % alignment:
                raise ValueError("PT_LOAD does not preserve 16 KiB page alignment")
            if size > memory:
                raise ValueError("PT_LOAD file size exceeds memory size")
        headers.append((ptype, flags, offset, address, size, memory))
    if not any(ptype == 1 and flags & 1 and address <= entry < address + size
               for ptype, flags, _, address, size, _ in headers):
        raise ValueError("entry point is not backed by an executable load segment")

    dynamic = [item for item in headers if item[0] == 2]
    if len(dynamic) != 1 or dynamic[0][4] % 16:
        raise ValueError("expected exactly one complete dynamic table")
    _, _, offset, _, size, _ = dynamic[0]
    entries = []
    for at in range(offset, offset + size, 16):
        tag, value = struct.unpack_from("<qQ", data, at)
        if tag == 0:
            break
        entries.append((tag, value))
    else:
        raise ValueError("unterminated dynamic table")

    def virtual_file_range(address: int, size: int) -> int:
        ranges = [offset + address - start
                  for ptype, _, offset, start, stored, _ in headers
                  if ptype == 1 and start <= address and address + size <= start + stored]
        if len(ranges) != 1:
            raise ValueError("dynamic data are not backed by one load segment")
        return ranges[0]

    tags = dict(entries)
    plt_size = tags.get(2, 0)
    if plt_size:
        if plt_size % 24 or tags.get(20) != 7 or 23 not in tags or 3 not in tags:
            raise ValueError("invalid PLT relocation descriptors")
        at = virtual_file_range(tags[23], plt_size)
        relocations = [struct.unpack_from("<QQq", data, at + index)
                       for index in range(0, plt_size, 24)]
        slots = sorted(offset for offset, _, _ in relocations)
        if any(info & 0xffffffff != 7 for _, info, _ in relocations):
            raise ValueError("expected R_X86_64_JUMP_SLOT PLT relocations")
        expected_slots = list(range(tags[3] + 24, tags[3] + 24 + len(slots) * 8, 8))
        if slots != expected_slots:
            raise ValueError("PLT jump slots do not follow the DT_PLTGOT resolver header")
        got = virtual_file_range(tags[3], 24 + len(slots) * 8)
        if struct.unpack_from("<3Q", data, got) != (dynamic[0][3], 0, 0):
            raise ValueError("GOTPLT[0] must name native PT_DYNAMIC, with empty resolver slots")
    string_tables = [value for tag, value in entries if tag == 5]
    string_sizes = [value for tag, value in entries if tag == 10]
    if len(string_tables) != 1 or len(string_sizes) != 1:
        raise ValueError("invalid dynamic string table descriptors")
    string_address, string_size = string_tables[0], string_sizes[0]
    ranges = [(offset + string_address - address, string_size)
              for ptype, _, offset, address, size, _ in headers
              if ptype == 1 and address <= string_address
              and string_address + string_size <= address + size]
    if len(ranges) != 1:
        raise ValueError("dynamic strings are not backed by one load segment")
    at, size = ranges[0]
    strings = data[at:at + size]
    imports = []
    for tag, value in entries:
        if tag != 1:
            continue
        end = strings.find(b"\0", value)
        if value >= len(strings) or end < 0:
            raise ValueError("invalid DT_NEEDED string")
        name = strings[value:end].decode("ascii")
        if not name.endswith(".prx") or "/" in name:
            raise ValueError(f"non-native import module: {name}")
        imports.append(name)
    excluded = PROJECT_EXCLUDED_MODULES.intersection(imports)
    if excluded and enforce_project_import_policy:
        raise ValueError("project import policy excludes these optional modules: "
                         + ", ".join(sorted(excluded))
                         + " (their presence alone is not a proven loader incompatibility)")
    if "libkernel.prx" not in imports or "libSceLibcInternal.prx" not in imports:
        raise ValueError("missing title kernel/libc imports")
    return {"entry": hex(entry), "program_headers": count, "imports": imports,
            "private_system_imports": bool(excluded), "entry_executable": True,
            "project_import_policy_enforced": enforce_project_import_policy,
            "plt_entries": plt_size // 24, "plt_metadata_valid": True}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path)
    parser.add_argument("--allow-reference-imports", action="store_true",
                        help="inspect reference titles without applying this project's module policy")
    args = parser.parse_args()
    try:
        result = inspect_native(args.elf.read_bytes(),
                                enforce_project_import_policy=not args.allow_reference_imports)
    except (OSError, ValueError, struct.error) as error:
        parser.exit(1, f"PS5 native validation failed: {error}\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
