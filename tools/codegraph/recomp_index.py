#!/usr/bin/env python3
"""
recomp_index.py -- fast structural index for the machine-generated PPC->C++
recompiled game code (simpsons/generated/default/*.cpp, ~80k functions).

This code is mechanically regular (DEFINE_REX_FUNC(sub_XXXXXXXX) { ... }) and
far too large/repetitive for clangd to usefully index (203MB, no interesting
types/overloads/templates). What's actually needed, over and over, is:
  - given an address, find the function that contains/starts at it
  - given an address with no registered function, find the nearest
    registered neighbors (the exact "gap-finding" workflow used all night
    to hand-translate analyzer-missed functions)
  - given a function name, find its definition (file:line) and its callers
    and callees within the generated tree (via literal sub_XXXXXXXX(ctx,
    base) call patterns -- this code has no virtual dispatch or overloads,
    so a regex-built call graph is exact, not a heuristic)

Builds once into a SQLite DB (default: tools/codegraph/recomp_index.sqlite),
rebuilds are incremental-ish (keyed by directory mtime; pass --force to
rebuild regardless).

Usage:
  python3 recomp_index.py build [--generated-dir DIR] [--db PATH] [--force]
  python3 recomp_index.py addr 0x8297AEB0
  python3 recomp_index.py gap 0x8297AEB0          # nearest registered neighbors
  python3 recomp_index.py name sub_8297AEB0
  python3 recomp_index.py callers sub_8297AEB0
  python3 recomp_index.py callees sub_8297AEB0
"""
import argparse
import os
import re
import sqlite3
import sys
import time

DEFAULT_GENERATED_DIR = "/home/deck/simpsons-recomp/simpsons/generated/default"
DEFAULT_DB = os.path.join(os.path.dirname(os.path.abspath(__file__)), "recomp_index.sqlite")

FUNC_DEF_RE = re.compile(r"^(?:DEFINE_REX_FUNC|PPC_FUNC_IMPL)\((?:__imp__)?(sub_[0-9A-Fa-f]{8})\)\s*\{")
CALL_RE = re.compile(r"\b(sub_[0-9A-Fa-f]{8})\s*\(")


def build(generated_dir, db_path, force=False):
    if os.path.exists(db_path) and not force:
        db_mtime = os.path.getmtime(db_path)
        newest_src = max((os.path.getmtime(os.path.join(generated_dir, f))
                          for f in os.listdir(generated_dir) if f.endswith(".cpp")),
                         default=0)
        if db_mtime > newest_src:
            print(f"index is up to date ({db_path}); pass --force to rebuild")
            return

    t0 = time.time()
    if os.path.exists(db_path):
        os.remove(db_path)
    conn = sqlite3.connect(db_path)
    conn.execute("CREATE TABLE functions (name TEXT PRIMARY KEY, addr INTEGER, file TEXT, line INTEGER)")
    conn.execute("CREATE INDEX idx_addr ON functions(addr)")
    conn.execute("CREATE TABLE calls (caller TEXT, callee TEXT)")
    conn.execute("CREATE INDEX idx_caller ON calls(caller)")
    conn.execute("CREATE INDEX idx_callee ON calls(callee)")

    files = sorted(f for f in os.listdir(generated_dir) if f.endswith(".cpp"))
    n_funcs = 0
    n_calls = 0
    for fi, fname in enumerate(files):
        path = os.path.join(generated_dir, fname)
        with open(path, errors="ignore") as fh:
            lines = fh.readlines()

        current_name = None
        current_body = []
        for lineno, line in enumerate(lines, 1):
            m = FUNC_DEF_RE.match(line)
            if m:
                if current_name:
                    _flush(conn, current_name, current_body)
                    n_calls += len(current_body)
                current_name = m.group(1)
                addr = int(current_name[4:], 16)
                conn.execute("INSERT OR REPLACE INTO functions VALUES (?, ?, ?, ?)",
                             (current_name, addr, fname, lineno))
                n_funcs += 1
                current_body = []
            elif current_name is not None:
                if line.rstrip() == "}":
                    _flush(conn, current_name, current_body)
                    n_calls += len(current_body)
                    current_name = None
                    current_body = []
                else:
                    current_body.extend(m.group(1) for m in CALL_RE.finditer(line))
        if current_name:
            _flush(conn, current_name, current_body)
            n_calls += len(current_body)

    conn.commit()
    conn.close()
    dt = time.time() - t0
    print(f"indexed {n_funcs} functions, {n_calls} call edges from {len(files)} files in {dt:.1f}s -> {db_path}")


def _flush(conn, name, calls):
    for callee in calls:
        if callee != name:
            conn.execute("INSERT INTO calls VALUES (?, ?)", (name, callee))


def _open(db_path):
    if not os.path.exists(db_path):
        print(f"no index at {db_path} -- run: python3 recomp_index.py build", file=sys.stderr)
        sys.exit(1)
    return sqlite3.connect(db_path)


def cmd_addr(db_path, addr_str):
    addr = int(addr_str, 16)
    conn = _open(db_path)
    row = conn.execute("SELECT name, file, line FROM functions WHERE addr = ?", (addr,)).fetchone()
    if row:
        print(f"{row[0]}: {row[1]}:{row[2]}")
    else:
        print(f"no registered function starts exactly at {addr_str} -- try 'gap'")


def cmd_gap(db_path, addr_str):
    addr = int(addr_str, 16)
    conn = _open(db_path)
    below = conn.execute(
        "SELECT name, addr, file, line FROM functions WHERE addr <= ? ORDER BY addr DESC LIMIT 1", (addr,)
    ).fetchone()
    above = conn.execute(
        "SELECT name, addr, file, line FROM functions WHERE addr > ? ORDER BY addr ASC LIMIT 1", (addr,)
    ).fetchone()
    if below:
        print(f"nearest below: {below[0]} (0x{below[1]:08X}) {below[2]}:{below[3]}")
    if above:
        print(f"nearest above: {above[0]} (0x{above[1]:08X}) {above[2]}:{above[3]}"
              f"  [gap = 0x{above[1] - (below[1] if below else above[1]):X}]")


def cmd_name(db_path, name):
    conn = _open(db_path)
    row = conn.execute("SELECT addr, file, line FROM functions WHERE name = ?", (name,)).fetchone()
    if row:
        print(f"0x{row[0]:08X}: {row[1]}:{row[2]}")
    else:
        print(f"'{name}' not found in the index (not a defined/registered function)")


def cmd_callers(db_path, name):
    conn = _open(db_path)
    rows = conn.execute("SELECT DISTINCT caller FROM calls WHERE callee = ? ORDER BY caller", (name,)).fetchall()
    if not rows:
        print("(no callers found in generated code -- likely reached only via a function-pointer/vtable)")
    for (caller,) in rows:
        loc = conn.execute("SELECT file, line FROM functions WHERE name = ?", (caller,)).fetchone()
        print(f"{caller}" + (f"  {loc[0]}:{loc[1]}" if loc else ""))


def cmd_callees(db_path, name):
    conn = _open(db_path)
    rows = conn.execute("SELECT DISTINCT callee FROM calls WHERE caller = ? ORDER BY callee", (name,)).fetchall()
    if not rows:
        print("(no direct sub_XXXXXXXX calls found in this function's body)")
    for (callee,) in rows:
        loc = conn.execute("SELECT file, line FROM functions WHERE name = ?", (callee,)).fetchone()
        print(f"{callee}" + (f"  {loc[0]}:{loc[1]}" if loc else "  (unregistered/external)"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--generated-dir", default=DEFAULT_GENERATED_DIR)
    ap.add_argument("--db", default=DEFAULT_DB)
    ap.add_argument("--force", action="store_true")
    ap.add_argument("command", choices=["build", "addr", "gap", "name", "callers", "callees"])
    ap.add_argument("arg", nargs="?")
    a = ap.parse_args()

    if a.command == "build":
        build(a.generated_dir, a.db, force=a.force)
        return
    if not a.arg:
        ap.error(f"'{a.command}' requires an argument")
    {"addr": cmd_addr, "gap": cmd_gap, "name": cmd_name,
     "callers": cmd_callers, "callees": cmd_callees}[a.command](a.db, a.arg)


if __name__ == "__main__":
    main()
