#!/bin/bash
# Pixel-exact rendering verification harness.
#
# Captures the game's actual Vulkan swapchain output at fixed frame numbers
# during the deterministic boot->menu sequence (via Mesa's screenshot layer,
# present on SteamOS -- no engine changes needed), and compares two capture
# sets numerically with ffmpeg PSNR. This is the correctness gate for
# renderer work: after any rendering-path change, `capture` + `compare`
# proves output-identical frames (or shows exactly which frames diverge and
# by how much) without a human eyeballing the screen.
#
# Launch-observe-kill only; refuses to run while a game instance is open.
#
# Usage:
#   tools/bench/shots.sh capture <name>       # boot, grab frames, store set
#   tools/bench/shots.sh compare <a> <b>      # PSNR-compare two sets
#   BENCH_SECONDS=90 ... capture <name>       # longer run (default 75)
#
# Frame policy, measured on real double-boot runs of identical code:
#   frames <= 360 (boot/static screens) are BYTE-IDENTICAL across
#   independent boots -- these are the hard exact-match gate.
#   frames > 360 (animated menu attract loop) differ by animation phase
#   between boots (PSNR 17-41dB on identical code), so they are advisory
#   context only, never a pass/fail signal.

set -u
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT/simpsons/out/build/linux-amd64-relwithdebinfo"
GAME_BIN="$BUILD_DIR/simpsons"
SHOTS_ROOT="$ROOT/tools/bench/shots"
SECONDS_TO_RUN="${BENCH_SECONDS:-60}"
# <start>-<count>-<interval>: 25 frames, 240..1680 (static gate + animated context).
FRAME_SPEC="240-25-60"
STATIC_GATE_MAX=360

mode="${1:-}"

case "$mode" in
  capture)
    name="${2:?capture name required}"
    if pgrep -x simpsons > /dev/null 2>&1; then
      echo "error: a simpsons process is already running -- refusing" >&2
      exit 1
    fi
    [ -x "$GAME_BIN" ] || { echo "error: game binary missing" >&2; exit 1; }
    CFG="$BUILD_DIR/simpsons.toml"
    [ -f "$CFG" ] || { echo "error: config missing" >&2; exit 1; }
    outdir="$SHOTS_ROOT/$name"
    rm -rf "$outdir"
    mkdir -p "$outdir"
    # Pin the window to a fixed size and windowed mode for the run: captures
    # taken on different displays/docks otherwise come out at different
    # resolutions (measured: 1280x800 vs 1920x1080 between two boots),
    # which makes pixel-identical comparison structurally impossible.
    cp "$CFG" "$CFG.shots-backup"
    restore_cfg() { mv -f "$CFG.shots-backup" "$CFG" 2>/dev/null; }
    trap restore_cfg EXIT
    sed -i 's/^fullscreen = .*/fullscreen = false/' "$CFG"
    sed -i '/^window_width = /d;/^window_height = /d' "$CFG"
    printf 'window_width = 1280\nwindow_height = 720\n' >> "$CFG"
    echo "capturing '$name': booting ${SECONDS_TO_RUN}s windowed 1280x720, frames $FRAME_SPEC ..."
    env LD_LIBRARY_PATH="$ROOT/tools/rexglue-bin/linux-amd64/lib" DISABLE_LSFG=1 \
      VK_LOADER_LAYERS_ENABLE=VK_LAYER_MESA_screenshot \
      VK_LAYER_MESA_SCREENSHOT_CONFIG="frames=${FRAME_SPEC};output_dir=${outdir}" \
      "$GAME_BIN" --game_data_root "$ROOT/gamedata" > /dev/null 2>&1 &
    GPID=$!
    sleep "$SECONDS_TO_RUN"
    kill -9 "$GPID" 2>/dev/null
    wait "$GPID" 2>/dev/null
    restore_cfg
    trap - EXIT
    n=$(ls "$outdir"/*.png 2>/dev/null | wc -l)
    # Environment manifest: first frame's dimensions define the set; compare
    # refuses sets from mismatched environments.
    first=$(ls "$outdir"/*.png 2>/dev/null | head -1)
    if [ -n "$first" ]; then
      ffprobe -v error -select_streams v:0 -show_entries stream=width,height \
        -of csv=p=0 "$first" > "$outdir/.dimensions"
    fi
    echo "captured $n frames into $outdir ($(cat "$outdir/.dimensions" 2>/dev/null))"
    [ "$n" -gt 0 ] || { echo "error: no frames captured (layer env not honored?)" >&2; exit 1; }
    ;;

  compare)
    a="${2:?set A required}"; b="${3:?set B required}"
    da="$SHOTS_ROOT/$a"; db="$SHOTS_ROOT/$b"
    [ -d "$da" ] && [ -d "$db" ] || { echo "error: missing capture set(s)" >&2; exit 1; }
    dim_a=$(cat "$da/.dimensions" 2>/dev/null); dim_b=$(cat "$db/.dimensions" 2>/dev/null)
    if [ -n "$dim_a" ] && [ "$dim_a" != "$dim_b" ]; then
      echo "error: capture environments differ ($a=$dim_a vs $b=$dim_b) -- recapture both" >&2
      exit 1
    fi
    echo "comparing '$a' vs '$b':"
    echo "  frames <= $STATIC_GATE_MAX: HARD GATE (must be pixel-identical)"
    echo "  frames  > $STATIC_GATE_MAX: advisory (animation phase differs across boots)"
    gate_fail=0; gate_pairs=0; adv_pairs=0
    for fa in "$da"/*.png; do
      fn=$(basename "$fa")
      fb="$db/$fn"
      [ -f "$fb" ] || continue
      num=${fn%.png}
      psnr=$(ffmpeg -hide_banner -i "$fa" -i "$fb" \
             -filter_complex "psnr" -f null - 2>&1 | \
             grep -o "average:[0-9.inf]*" | cut -d: -f2)
      if [ "$num" -le "$STATIC_GATE_MAX" ] 2>/dev/null; then
        gate_pairs=$((gate_pairs+1))
        if [ "$psnr" = "inf" ]; then
          printf "  GATE %-24s psnr=inf      IDENTICAL\n" "$fn"
        else
          printf "  GATE %-24s psnr=%-9s FAIL (must be identical)\n" "$fn" "${psnr:-n/a}"
          gate_fail=$((gate_fail+1))
        fi
      else
        adv_pairs=$((adv_pairs+1))
        printf "  adv  %-24s psnr=%-9s\n" "$fn" "${psnr:-n/a}"
      fi
    done
    echo "gate: $gate_pairs frames, $gate_fail failing; advisory: $adv_pairs frames"
    [ "$gate_pairs" -gt 0 ] || { echo "error: no gate frames captured" >&2; exit 1; }
    [ "$gate_fail" -eq 0 ]
    ;;

  *)
    echo "usage: $0 capture <name> | compare <a> <b>" >&2
    exit 2
    ;;
esac
