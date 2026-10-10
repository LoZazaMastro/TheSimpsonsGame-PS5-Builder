#!/bin/bash
# Deterministic performance benchmark for engine changes.
#
# Boots the game, lets it sit through the boot->main-menu sequence (which is
# deterministic and, at ~3.7k draws/frame, the heaviest reliably-reproducible
# scene we have), records the engine's own per-frame CSV (fps, frame time,
# draw calls, stalls, cache hits), then kills the game and prints summary
# stats -- optionally diffed against a saved baseline.
#
# This intentionally injects NO input and does not touch saves: it only
# launches, observes, and kills. Do not run while another instance of the
# game is already running.
#
# Usage:
#   tools/bench/bench.sh                       # run, print stats
#   tools/bench/bench.sh --save-baseline NAME  # run, save stats as baseline
#   tools/bench/bench.sh --against NAME        # run, diff vs saved baseline
#   BENCH_SECONDS=90 tools/bench/bench.sh      # longer capture (default 75)

set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT/simpsons/out/build/linux-amd64-relwithdebinfo"
GAME_BIN="$BUILD_DIR/simpsons"
CFG="$BUILD_DIR/simpsons.toml"
BENCH_DIR="$ROOT/tools/bench"
BASELINES="$BENCH_DIR/baselines"
SECONDS_TO_RUN="${BENCH_SECONDS:-75}"

mode="report"
name=""
case "${1:-}" in
  --save-baseline) mode="save"; name="${2:?baseline name required}" ;;
  --against)       mode="diff"; name="${2:?baseline name required}" ;;
  "") ;;
  *) echo "unknown arg: $1" >&2; exit 2 ;;
esac

if pgrep -x simpsons > /dev/null 2>&1; then
  echo "error: a simpsons process is already running -- refusing to interfere" >&2
  exit 1
fi
[ -x "$GAME_BIN" ] || { echo "error: game binary not found at $GAME_BIN" >&2; exit 1; }
[ -f "$CFG" ] || { echo "error: config not found at $CFG" >&2; exit 1; }

mkdir -p "$BASELINES"
CSV="$(mktemp -u "$BENCH_DIR/run-XXXXXX.csv")"

# Enable per-frame CSV logging for this run only; restore config afterward.
cp "$CFG" "$CFG.bench-backup"
restore_cfg() { mv -f "$CFG.bench-backup" "$CFG" 2>/dev/null; }
trap restore_cfg EXIT
sed -i '/^perf_log_csv = /d' "$CFG"
echo "perf_log_csv = \"$CSV\"" >> "$CFG"

echo "benchmark: booting game for ${SECONDS_TO_RUN}s (boot -> menu attract loop)..."
env LD_LIBRARY_PATH="$ROOT/tools/rexglue-bin/linux-amd64/lib" DISABLE_LSFG=1 \
  "$GAME_BIN" --game_data_root "$ROOT/gamedata" > /dev/null 2>&1 &
GPID=$!
sleep "$SECONDS_TO_RUN"
kill -9 "$GPID" 2>/dev/null
wait "$GPID" 2>/dev/null
restore_cfg
trap - EXIT

[ -s "$CSV" ] || { echo "error: no CSV produced" >&2; exit 1; }

# Menu-phase stats: rows where the draw storm is active (draw_calls > 3000)
# so light boot/loading frames don't dilute the numbers. Falls back to all
# rows past warmup if the scene never reaches that draw volume.
stats=$(awk -F, '
  NR > 1 && $3 > 3000 { n++; fps += $2; ft += $1; dc += $3;
    if ($2 < minf || minf == "") minf = $2; if ($2 > maxf) maxf = $2 }
  END {
    if (n < 100) exit 42
    printf "%d %.2f %d %d %.3f %.1f", n, fps/n, minf, maxf, ft/n/1000, dc/n
  }' "$CSV")
if [ $? -eq 42 ] || [ -z "$stats" ]; then
  stats=$(awk -F, '
    NR > 60 { n++; fps += $2; ft += $1; dc += $3;
      if ($2 < minf || minf == "") minf = $2; if ($2 > maxf) maxf = $2 }
    END { printf "%d %.2f %d %d %.3f %.1f", n, fps/n, minf, maxf, ft/n/1000, dc/n }' "$CSV")
  echo "note: menu draw storm not reached; stats cover all post-warmup frames"
fi
read -r frames avg_fps min_fps max_fps avg_ms avg_draws <<< "$stats"

echo "frames=$frames avg_fps=$avg_fps min=$min_fps max=$max_fps avg_frame_ms=$avg_ms avg_draws=$avg_draws"

case "$mode" in
  save)
    echo "$stats" > "$BASELINES/$name"
    echo "saved baseline '$name'"
    ;;
  diff)
    [ -f "$BASELINES/$name" ] || { echo "error: no baseline '$name'" >&2; exit 1; }
    read -r b_frames b_avg_fps b_min b_max b_ms b_draws < "$BASELINES/$name"
    awk -v a="$avg_fps" -v b="$b_avg_fps" -v am="$avg_ms" -v bm="$b_ms" 'BEGIN {
      printf "vs baseline: avg_fps %.2f -> %.2f (%+.1f%%), frame_ms %.3f -> %.3f (%+.1f%%)\n",
        b, a, (a-b)/b*100, bm, am, (am-bm)/bm*100 }'
    ;;
esac

rm -f "$CSV"
