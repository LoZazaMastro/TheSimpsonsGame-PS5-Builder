# codegraph — fast code navigation for this repo

Two tools, for the two very different halves of this codebase.

## 1. `clangd_query.py` — the hand-written engine (`tools/rexglue-sdk`)

Real semantic navigation (respects overloads, macros, includes) via the
clangd bundled at `tools/clang20/bin/clangd`, driven over raw LSP.

```
cd tools/codegraph
R=/home/deck/simpsons-recomp/tools/rexglue-sdk
B=$R/build   # wherever that project's compile_commands.json lives

python3 clangd_query.py --root "$R" --compile-commands-dir "$B" symbol <name>
python3 clangd_query.py --root "$R" --compile-commands-dir "$B" outline <file>
python3 clangd_query.py --root "$R" --compile-commands-dir "$B" def <file> <line> <col>
python3 clangd_query.py --root "$R" --compile-commands-dir "$B" refs <file> <line> <col>
python3 clangd_query.py --root "$R" --compile-commands-dir "$B" callers <file> <line> <col>
python3 clangd_query.py --root "$R" --compile-commands-dir "$B" callees <file> <line> <col>
```

`symbol` is fuzzy and project-wide (find a function/class by name without
knowing what file it's in). The rest need a precise line/col — get those
from `outline` or from `symbol`'s own output first.

Each invocation starts a fresh clangd, so `symbol`/`def`/`refs`/`callers`/
`callees` all pay a real one-time cost (~10-15s) to load the on-disk index
cache (`build/.cache/clangd/index/`, safe to keep across sessions — delete
it if results look stale after a big refactor). `outline` is near-instant
since it only needs the one open file, no cross-file index.

If `compile_commands.json` is missing or stale (only a handful of thirdparty
entries, no real project files), the project wasn't configured with
`CMAKE_EXPORT_COMPILE_COMMANDS=ON`. Fix: `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .`
in the build directory, no rebuild needed, Ninja regenerates it instantly.

**Only point this at `tools/rexglue-sdk`.** Do not try to background-index
`simpsons/generated/default` with it — 203MB of near-identical mechanically
generated functions is exactly the wrong shape for clangd; use tool #2.

## 2. `recomp_index.py` — the generated recompiled game code

`simpsons/generated/default/*.cpp`: ~82k `DEFINE_REX_FUNC(sub_XXXXXXXX)`
blocks, mechanically translated PPC. No types/overloads/templates worth
understanding semantically — what's actually needed is address/name lookup
and an exact call graph (exact because this code has no virtual dispatch:
every call is a literal `sub_XXXXXXXX(ctx, base)`, so a regex-built graph
is not a heuristic, it's ground truth).

```
cd tools/codegraph
python3 recomp_index.py build              # ~19s one-time (SQLite output alongside this file)
python3 recomp_index.py name sub_8297AEB0   # -> address + file:line, or "not found"
python3 recomp_index.py addr 0x8297AEB0     # -> reverse lookup, same info
python3 recomp_index.py gap 0x8297AEB0      # -> nearest registered functions above/below
                                             #    + gap size; THIS is the missed-function-
                                             #    hunting workflow from the crash sessions,
                                             #    now one command instead of manual grep+python
python3 recomp_index.py callers sub_8297AEB0   # every sub_XXXXXXXX that calls this
python3 recomp_index.py callees sub_8297AEB0   # every sub_XXXXXXXX this one calls
```

Rebuild (`build --force`) after any mega-pass / hand-patch session that adds
functions to `simpsons/generated/default` — the DB is otherwise stale and
`gap`/`callers`/`callees` will lie about what's actually registered.
