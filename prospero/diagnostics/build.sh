#!/usr/bin/env bash
# Build only the optional SDK payload; never launch or deploy it.
set -euo pipefail

task_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
task_root=$(cd -- "$task_dir/../.." && pwd)
task_sdk=${PS5_PAYLOAD_SDK:-}
task_output=${1:-$task_root/out/prospero-diagnostics}

if [[ $# -gt 1 || -z $task_sdk || ! -x $task_sdk/bin/prospero-clang ]]; then
    echo "Usage: PS5_PAYLOAD_SDK=/path/to/sdk bash prospero/diagnostics/build.sh [output-directory]" >&2
    echo "The script only compiles the optional payload; it does not build or send the game." >&2
    exit 2
fi

mkdir -p -- "$task_output"
task_elf="$task_output/launch-probe-PPSA74207.elf"
"$task_sdk/bin/prospero-clang" -std=c11 -O2 -Wall -Wextra -Werror \
    -lSceSystemService -lSceUserService \
    -o "$task_elf" "$task_dir/launch_probe.c"
printf '%s\n' "$task_elf"
echo "Compiled only. No PS5 execution or console validation has been performed."
