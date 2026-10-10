#!/bin/bash
# Rank the game's translated shaders by real GPU cost on this machine.
#
# RADV (the Mesa Vulkan driver) prints ACO compilation statistics for every
# pipeline it compiles when RADV_DEBUG=shaderstats is set: VGPR/SGPR register
# pressure, instruction counts, theoretical occupancy (maxwaves), spills.
# Register pressure and occupancy are the numbers that decide how fast a
# shader actually runs on the GPU -- a pixel shader that drops occupancy to
# a handful of waves stalls the whole APU in heavy scenes.
#
# This boots the game cold with the engine's on-disk pipeline cache moved
# aside (so every pipeline recompiles and gets stats printed), captures the
# driver output through the boot->menu sequence, restores the cache, and
# prints the worst shaders by register pressure / occupancy. The engine's
# own log tags each pipeline creation with its guest shader hashes, so a
# flagged shader can be traced back to the exact guest microcode for
# targeted translator work.
#
# No input injection; launch-observe-kill only. Do not run while the game
# is already running.
#
# Usage: tools/bench/shader_stats.sh   (BENCH_SECONDS=90 to extend)

set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT/simpsons/out/build/linux-amd64-relwithdebinfo"
GAME_BIN="$BUILD_DIR/simpsons"
SECONDS_TO_RUN="${BENCH_SECONDS:-75}"
OUT_DIR="$ROOT/tools/bench"
RAW="$OUT_DIR/shaderstats-raw.txt"

if pgrep -x simpsons > /dev/null 2>&1; then
  echo "error: a simpsons process is already running -- refusing to interfere" >&2
  exit 1
fi
[ -x "$GAME_BIN" ] || { echo "error: game binary not found" >&2; exit 1; }

# Force full pipeline recompilation by hiding the engine's pipeline storage
# for this run (restored afterward, so normal launches stay fast).
CACHE_ROOT="$HOME/.var/app/com.visualstudio.code/data/simpsons/cache"
moved_cache=""
if [ -d "$CACHE_ROOT" ]; then
  moved_cache="$CACHE_ROOT.shaderstats-aside"
  mv "$CACHE_ROOT" "$moved_cache"
fi
restore_cache() {
  if [ -n "$moved_cache" ] && [ -d "$moved_cache" ]; then
    rm -rf "$CACHE_ROOT"
    mv "$moved_cache" "$CACHE_ROOT"
  fi
}
trap restore_cache EXIT

echo "shader-stats: booting cold for ${SECONDS_TO_RUN}s with RADV_DEBUG=shaderstats..."
env LD_LIBRARY_PATH="$ROOT/tools/rexglue-bin/linux-amd64/lib" DISABLE_LSFG=1 \
  RADV_DEBUG=shaderstats \
  "$GAME_BIN" --game_data_root "$ROOT/gamedata" > "$RAW" 2>&1 &
GPID=$!
sleep "$SECONDS_TO_RUN"
kill -9 "$GPID" 2>/dev/null
wait "$GPID" 2>/dev/null
restore_cache
trap - EXIT

[ -s "$RAW" ] || { echo "error: no driver output captured" >&2; exit 1; }

echo ""
echo "=== worst shaders by VGPR pressure (higher = fewer waves = slower) ==="
# ACO stat blocks look like: "Fragment Shader: ... VGPRs: 84 ... MaxWaves: 5 ..."
awk '
  /Shader:/ { stage = $1 }
  /VGPRs:/ {
    for (i = 1; i <= NF; ++i) {
      if ($i == "VGPRs:") vgpr = $(i + 1)
      if ($i == "MaxWaves:") waves = $(i + 1)
      if ($i == "Instrs:") instrs = $(i + 1)
    }
    printf "%5d vgpr  %2d waves  %6d instrs  %s\n", vgpr, waves, instrs, stage
  }' "$RAW" | sort -rn | head -20

echo ""
echo "raw driver output kept at: $RAW"
echo "cross-reference flagged pipelines with the engine log lines"
echo "  \"Creating graphics pipeline state with VS <hash>, PS <hash>\""
echo "to map them back to guest shader microcode."
