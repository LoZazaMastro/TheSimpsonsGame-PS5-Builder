#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
source "$root/prospero/pins.sh"
deps=${PROSPERO_DEPS:-$root/.deps/prospero}
[[ ! -f $deps/environment.sh ]] || source "$deps/environment.sh"
: "${PS5_PAYLOAD_SDK:?Run prospero/bootstrap.sh first, then source its environment.sh}"
: "${SONIC_PS5_REFERENCE:?Run prospero/bootstrap.sh first}"
[[ $(cat "$PS5_PAYLOAD_SDK/.ps5-sdk-revision") == "$SDK_REVISION" ]] || {
    echo "Mismatched PS5 SDK. Run the pinned bootstrap." >&2; exit 2;
}
export PS5_PAYLOAD_SDK SONIC_PS5_REFERENCE
export RADV_ARCHIVE=${RADV_ARCHIVE:-$deps/radv-display/libvulkan_radeon.ps5.a}
[[ -s $RADV_ARCHIVE && -s $RADV_ARCHIVE.build.json ]] || {
    echo "Build the PS5 display archive with prospero/bootstrap.sh first" >&2; exit 2;
}
build=${PROSPERO_BUILD_DIR:-$root/out/prospero}
jobs=${BUILD_JOBS:-3}
cmake -S "$root/simpsons" -B "$build" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$root/prospero/ps5-toolchain.cmake" \
    -DPS5_PAYLOAD_SDK="$PS5_PAYLOAD_SDK" \
    -DSONIC_PS5_REFERENCE="$SONIC_PS5_REFERENCE" \
    -DREXSDK_DIR="$root/tools/rexglue-sdk" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build "$build" --target simpsons --parallel "$jobs"
bash "$root/prospero/package.sh" "$build/simpsons.elf"
