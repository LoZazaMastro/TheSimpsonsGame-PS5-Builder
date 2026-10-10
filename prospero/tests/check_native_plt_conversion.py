#!/usr/bin/env python3
"""Compare real PIE jump slots with the converted native ELF (no execution)."""
import argparse
import importlib.util
import json
from pathlib import Path
import struct


class Elf:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        phoff = struct.unpack_from("<Q", self.data, 32)[0]
        phsize, count = struct.unpack_from("<HH", self.data, 54)
        self.headers = [struct.unpack_from("<IIQQQQQQ", self.data, phoff + phsize * i)
                        for i in range(count)]
        self.dynamic = next(header for header in self.headers if header[0] == 2)
        self.tags = {}
        for at in range(self.dynamic[2], self.dynamic[2] + self.dynamic[5], 16):
            tag, value = struct.unpack_from("<QQ", self.data, at)
            if not tag:
                break
            self.tags[tag] = value

    def at(self, address, size):
        for ptype, _, offset, base, _, stored, _, _ in self.headers:
            if ptype == 1 and base <= address and address + size <= base + stored:
                return self.data[offset + address - base:offset + address - base + size]
        raise AssertionError(f"address not backed by file: {address:#x}")

    def plt(self):
        return self.at(self.tags[23], self.tags[2])

    def symbol(self, index):
        symbol = self.at(self.tags[6] + index * 24, 24)
        name = struct.unpack_from("<I", symbol)[0]
        strings = self.at(self.tags[5], self.tags[10])
        return strings[name:strings.index(0, name)].decode("ascii")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--native", required=True, type=Path)
    parser.add_argument("--previous-native", type=Path)
    parser.add_argument("--expect-slots", type=int, default=9)
    args = parser.parse_args()
    validator_path = Path(__file__).resolve().parents[1] / "validate_native.py"
    spec = importlib.util.spec_from_file_location("validate_native", validator_path)
    validator = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(validator)
    original, native = Elf(args.input), Elf(args.native)
    original_plt = original.plt()
    assert len(original_plt) == args.expect_slots * 24
    assert native.plt() == original_plt, "jump slots, symbol indexes or addends changed"
    assert original.tags[3] == native.tags[3], "source DT_PLTGOT was not retained"
    assert struct.unpack("<3Q", original.at(original.tags[3], 24)) == (
        original.dynamic[3], 0, 0)
    assert struct.unpack("<3Q", native.at(native.tags[3], 24)) == (
        native.dynamic[3], 0, 0)
    assert original.at(original.tags[3] + 24, args.expect_slots * 8) == native.at(
        native.tags[3] + 24, args.expect_slots * 8), "pre-bind jump slot contents changed"
    # The converter keeps code addresses; it must not patch the call thunks.
    code = next(header for header in original.headers if header[0] == 1 and header[1] & 1)
    assert original.at(code[3], code[5]) == native.at(code[3], code[5]), "code changed"
    result = validator.inspect_native(native.data)
    assert result["plt_entries"] == args.expect_slots
    if args.previous_native:
        previous = Elf(args.previous_native)
        assert previous.tags[3] != original.tags[3], "fixture does not contain the original bug"
        try:
            validator.inspect_native(previous.data, enforce_project_import_policy=False)
        except ValueError as error:
            assert "DT_PLTGOT" in str(error), str(error)
        else:
            raise AssertionError("validator accepted the wrong original PLT GOT")
    symbols = [original.symbol(struct.unpack_from("<Q", original_plt, at + 8)[0] >> 32)
               for at in range(0, len(original_plt), 24)]
    print(json.dumps({"status": "PASS (host structure only, not console execution)",
                      "jump_slots": args.expect_slots, "symbols": symbols,
                      "source_and_native_pltgot": hex(native.tags[3]),
                      "source_dynamic": hex(original.dynamic[3]),
                      "native_dynamic_and_gotplt0": hex(native.dynamic[3]),
                      "code_and_jump_relocations_unchanged": True,
                      "previous_bad_pltgot_rejected": bool(args.previous_native)}, indent=2))


if __name__ == "__main__":
    main()
