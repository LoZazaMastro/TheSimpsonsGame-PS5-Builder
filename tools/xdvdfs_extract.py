#!/usr/bin/env python3
"""Minimal XDVDFS reader: extract a single named file from an Xbox 360 (XGD2) ISO."""
import struct, sys

SECTOR = 2048
MAGIC = b"MICROSOFT*XBOX*MEDIA"

def find_partition_base(f):
    # Volume descriptor sits 32 sectors into the XDVDFS partition.
    # Scan candidate base offsets used by XGD1/2/3 images.
    for base in (0x00000000, 0x0000FD90 * SECTOR, 0x0FD90000, 0x02080000, 0x18310000):
        f.seek(base + 32 * SECTOR)
        if f.read(len(MAGIC)) == MAGIC:
            return base
    # Fallback: brute scan first 600 MB on sector boundaries.
    f.seek(0)
    pos = 0
    chunk = 1 << 20
    while pos < 600 * (1 << 20):
        f.seek(pos)
        data = f.read(chunk)
        if not data:
            break
        i = data.find(MAGIC)
        if i != -1 and (pos + i) % SECTOR == 0:
            return (pos + i) - 32 * SECTOR
        pos += chunk - len(MAGIC)
    raise RuntimeError("XDVDFS magic not found")

def read_dir(f, base, sector, size):
    f.seek(base + sector * SECTOR)
    data = f.read(size)
    entries = []
    # Walk the directory by scanning 4-byte-aligned records (AVL tree, but a
    # linear sweep over the allocated region recovers every live entry).
    pos = 0
    while pos + 14 <= len(data):
        left, right, start, fsize, attr, nlen = struct.unpack_from("<HHIIBB", data, pos)
        if nlen == 0 or nlen == 0xFF or left == 0xFFFF and right == 0xFFFF and start == 0 and fsize == 0:
            pos += 4
            continue
        name = data[pos + 14: pos + 14 + nlen]
        try:
            name_s = name.decode("ascii")
        except UnicodeDecodeError:
            pos += 4
            continue
        if all(32 <= b < 127 for b in name):
            entries.append((name_s, start, fsize, attr))
        pos += 4
    return entries

def main(iso, target, out):
    with open(iso, "rb") as f:
        base = find_partition_base(f)
        f.seek(base + 32 * SECTOR + 20)
        root_sector, root_size = struct.unpack("<II", f.read(8))
        sys.stderr.write(f"partition base=0x{base:X} root_sector={root_sector} root_size={root_size}\n")
        entries = read_dir(f, base, root_sector, root_size)
        match = next((e for e in entries if e[0].lower() == target.lower()), None)
        if not match:
            sys.stderr.write("root entries: " + ", ".join(e[0] for e in entries[:40]) + "\n")
            raise RuntimeError(f"{target} not found in root")
        name, start, fsize, attr = match
        sys.stderr.write(f"{name}: start_sector={start} size={fsize}\n")
        f.seek(base + start * SECTOR)
        remaining = fsize
        with open(out, "wb") as o:
            while remaining:
                buf = f.read(min(1 << 20, remaining))
                if not buf:
                    raise RuntimeError("unexpected EOF")
                o.write(buf)
                remaining -= len(buf)
        sys.stderr.write(f"wrote {out} ({fsize} bytes)\n")

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], sys.argv[3])
