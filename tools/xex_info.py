#!/usr/bin/env python3
"""Parse an Xbox 360 XEX2 header (big-endian) and report recomp-relevant scope."""
import struct, sys

def be32(b, o): return struct.unpack_from(">I", b, o)[0]
def be16(b, o): return struct.unpack_from(">H", b, o)[0]

OPT_KEYS = {
    0x000002FF: "RESOURCE_INFO",
    0x000003FF: "FILE_FORMAT_INFO",
    0x000004FF: "BASE_FILE_TIMESTAMP",
    0x000005FF: "DELTA_PATCH_DESCRIPTOR",
    0x00000200: "BOUNDING_PATH/?",
    0x000080FF: "BOUNDING_PATH",
    0x000100FF: "DEVICE_ID",
    0x00010100: "ENTRY_POINT_alt",
    0x00010001: "HEADER_SECTION_TABLE?",
    0x000101FF: "IMPORT_LIBRARIES",
    0x00010201: "IMAGE_BASE_ADDRESS",
    0x000103FF: "IMPORT_LIBRARIES2",
    0x00020104: "TLS_INFO",
    0x00010100 & 0xFFFFFFFF: "?",
    0x00010100: "ENTRY_POINT",
    0x00020200: "DEFAULT_STACK_SIZE",
    0x00020301: "DEFAULT_FS_CACHE_SIZE",
    0x00020401: "DEFAULT_HEAP_SIZE",
    0x00030000: "SYSTEM_FLAGS",
    0x00040006: "EXECUTION_ID",
    0x00040201: "?",
    0x000183FF: "ORIGINAL_PE_NAME",
    0x00018002: "CHECKSUM_TIMESTAMP",
    0x00018102: "CHECKSUM",
    0x0001C001: "?",
    0x00020005: "?",
    0x00040310: "?",
}

COMP = {0: "none", 1: "raw/basic", 2: "compressed(LZX)", 3: "delta-compressed"}
ENC  = {0: "not encrypted", 1: "encrypted (AES, needs key)"}

def main(path):
    b = open(path, "rb").read()
    assert b[:4] == b"XEX2", "not XEX2"
    module_flags = be32(b, 4)
    pe_offset    = be32(b, 8)
    sec_info_off = be32(b, 0x10)
    opt_count    = be32(b, 0x14)
    print(f"module_flags = 0x{module_flags:08X}")
    print(f"basefile offset (header size) = 0x{pe_offset:X}")
    print(f"security info offset = 0x{sec_info_off:X}")
    print(f"optional header count = {opt_count}")
    print("-" * 60)

    opt = {}
    for i in range(opt_count):
        k = be32(b, 0x18 + i*8)
        v = be32(b, 0x18 + i*8 + 4)
        opt[k] = v
        print(f"  opt[{i:2}] key=0x{k:08X} val=0x{v:08X}  {OPT_KEYS.get(k,'?')}")
    print("-" * 60)

    # FILE_FORMAT_INFO
    if 0x000003FF in opt:
        o = opt[0x000003FF]
        size = be32(b, o)
        enc = be16(b, o+4)
        comp = be16(b, o+6)
        print(f"FILE_FORMAT_INFO @0x{o:X}: encryption={ENC.get(enc,enc)}  compression={COMP.get(comp,comp)}")
    # base address / entry point
    if 0x00010201 in opt:
        print(f"IMAGE_BASE_ADDRESS = 0x{opt[0x00010201]:08X}")
    if 0x00010100 in opt:
        print(f"ENTRY_POINT        = 0x{opt[0x00010100]:08X}")
    if 0x00030000 in opt:
        print(f"SYSTEM_FLAGS       = 0x{opt[0x00030000]:08X}")
    if 0x000183FF in opt:
        o = opt[0x000183FF]; n = be32(b, o)
        print(f"ORIGINAL_PE_NAME   = {b[o+4:o+4+n].split(bytes([0]))[0].decode('latin1')}")

    # Security info: image size + load address + AES key blob present
    so = sec_info_off
    sec_size = be32(b, so)
    image_size = be32(b, so+4)
    print(f"\nSECURITY INFO: header_size=0x{sec_size:X} image_size=0x{image_size:X} ({image_size/1024/1024:.1f} MB loaded)")

    # IMPORT_LIBRARIES (key 0x000103FF) — list library names + import counts
    imp_key = 0x000103FF if 0x000103FF in opt else (0x000101FF if 0x000101FF in opt else None)
    if imp_key:
        o = opt[imp_key]
        total_size = be32(b, o)
        name_table_size = be32(b, o+4)
        count = be32(b, o+8)
        print(f"\nIMPORT_LIBRARIES @0x{o:X}: blob=0x{total_size:X} name_table=0x{name_table_size:X} libs={count}")
        names_blob = b[o+12: o+12+name_table_size]
        names = [s.decode('latin1') for s in names_blob.split(b'\x00') if s]
        print("  libraries:", ", ".join(names))
        # records follow the name table
        p = o + 12 + name_table_size
        for li in range(count):
            rec_size = be32(b, p)
            # at +0x24 into record: import count (uint16) then version, name index
            ic = be16(b, p + 0x24)
            name_index = be16(b, p + 0x22)
            lname = names[name_index] if name_index < len(names) else f"#{name_index}"
            print(f"    {lname:24} imports={ic}")
            p += rec_size

if __name__ == "__main__":
    main(sys.argv[1])
