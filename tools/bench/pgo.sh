#!/bin/bash
# Profile-guided optimization pipeline for the recompiled game.
#
# PGO replaces the compiler's guesses about hot paths with measurements from
# a real play session: better inlining choices, hot/cold code layout across
# the 82k translated functions (instruction-cache locality), and real branch
# ordering. It changes optimization decisions only -- never semantics; a
# stale or missing profile produces a slower binary, never a broken one.
#
# Three steps:
#   tools/bench/pgo.sh instrument   # build + deploy the profiling binary
#   <play one normal session -- menus, Springfield, a level; then quit>
#   tools/bench/pgo.sh finalize     # merge profile, rebuild optimized, A/B
#
#   tools/bench/pgo.sh off          # rebuild without PGO (back to stock)
#
# The instrumented build runs noticeably slower (counters everywhere) --
# that's expected and only lasts the one profiling session.

set -eu
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT/simpsons/out/build/linux-amd64-relwithdebinfo"
PROFDIR="$ROOT/tools/bench/pgo-profiles"
PROFDATA="$PROFDIR/game.profdata"
LAUNCHER_ENV="$ROOT/launcher/launcher-env.json"
export PATH="$ROOT/tools/clang20/bin:$PATH"
export LD_LIBRARY_PATH="/home/.steamos/offload/nix/store/ah4525ca553drv47jhvgpl9sl87i7a1d-libxml2-2.13.8/lib:${LD_LIBRARY_PATH:-}"

rebuild_with() {
  local flags="$1"
  ( cd "$BUILD_DIR" &&
    cmake . -DCMAKE_CXX_FLAGS="$flags" -DCMAKE_C_FLAGS="$flags" > /dev/null &&
    ninja )
}

case "${1:-}" in
  instrument)
    if pgrep -x simpsons > /dev/null 2>&1; then
      echo "close the game first" >&2; exit 1
    fi
    mkdir -p "$PROFDIR"
    rm -f "$PROFDIR"/*.profraw
    echo "building instrumented binary (this is a full rebuild)..."
    rebuild_with "-fprofile-generate=$PROFDIR"
    # Route the profile path through the launcher's extra-env file so a
    # normal Play press profiles transparently.
    python3 - "$LAUNCHER_ENV" "$PROFDIR" << 'EOF'
import json, sys
path, profdir = sys.argv[1], sys.argv[2]
try:
    env = json.load(open(path))
except Exception:
    env = {}
env["LLVM_PROFILE_FILE"] = f"{profdir}/game-%m.profraw"
json.dump(env, open(path, "w"), indent=2)
EOF
    echo ""
    echo "instrumented build deployed."
    echo "NOW: play one normal session (menus + the laggy spots + a level),"
    echo "quit the game, then run: tools/bench/pgo.sh finalize"
    ;;

  finalize)
    if pgrep -x simpsons > /dev/null 2>&1; then
      echo "close the game first" >&2; exit 1
    fi
    ls "$PROFDIR"/*.profraw > /dev/null 2>&1 || {
      echo "no .profraw files in $PROFDIR -- play a session on the instrumented build first" >&2
      exit 1
    }
    echo "merging profiles..."
    llvm-profdata merge -output="$PROFDATA" "$PROFDIR"/*.profraw
    # Stop profiling future launches.
    python3 - "$LAUNCHER_ENV" << 'EOF'
import json, sys
path = sys.argv[1]
try:
    env = json.load(open(path))
    env.pop("LLVM_PROFILE_FILE", None)
    json.dump(env, open(path, "w"), indent=2)
except Exception:
    pass
EOF
    echo "building optimized binary with the profile..."
    rebuild_with "-fprofile-use=$PROFDATA -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date"
    echo ""
    echo "PGO build done. Measure it:"
    echo "  tools/bench/bench.sh --against post-phase0-fixes"
    ;;

  off)
    if pgrep -x simpsons > /dev/null 2>&1; then
      echo "close the game first" >&2; exit 1
    fi
    python3 - "$LAUNCHER_ENV" << 'EOF'
import json, sys
path = sys.argv[1]
try:
    env = json.load(open(path))
    env.pop("LLVM_PROFILE_FILE", None)
    json.dump(env, open(path, "w"), indent=2)
except Exception:
    pass
EOF
    echo "rebuilding without PGO..."
    rebuild_with ""
    echo "back to stock flags."
    ;;

  *)
    echo "usage: $0 instrument | finalize | off" >&2
    exit 2
    ;;
esac
