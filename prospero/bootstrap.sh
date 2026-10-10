#!/usr/bin/env bash
# Download/build the public platform dependencies; no game data is used.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
source "$root/prospero/pins.sh"
deps=${PROSPERO_DEPS:-$root/.deps/prospero}
jobs=${BUILD_JOBS:-3}
[[ $jobs =~ ^[1-9][0-9]*$ ]] || { echo "BUILD_JOBS must be positive" >&2; exit 2; }
for command in git tar rsync make wget unzip sha256sum flock cmake ninja meson pkg-config glslangValidator python3 g++; do
    command -v "$command" >/dev/null || { echo "Missing host tool: $command (see prospero/README.md)" >&2; exit 2; }
done
python3 -c 'import mako, ply, yaml' || exit 2
llvm_config=${PS5_LLVM_CONFIG:-$(command -v llvm-config-18 || command -v llvm-config || true)}
[[ -n $llvm_config ]] || { echo "LLVM 18 is required" >&2; exit 2; }
[[ $("$llvm_config" --version) == 18.* ]] || { echo "This port is validated with LLVM 18; set PS5_LLVM_CONFIG" >&2; exit 2; }
export LLVM_CONFIG="$(realpath "$llvm_config")"
export PATH="$("$llvm_config" --bindir):$PATH"
export TAR_OPTIONS="--no-same-owner${TAR_OPTIONS:+ $TAR_OPTIONS}"
mkdir -p "$deps/src" "$deps/cache" "$deps/bin"

checkout() {
    local path=$1 url=$2 revision=$3
    if [[ ! -d $path/.git ]]; then
        [[ ! -e $path ]] || { echo "Refusing to replace existing path: $path" >&2; exit 2; }
        git init -q "$path"
        git -C "$path" remote add origin "$url"
        git -C "$path" fetch --depth=1 origin "$revision"
        git -C "$path" checkout -q --detach FETCH_HEAD
    fi
    [[ $(git -C "$path" rev-parse HEAD) == "$revision" ]] || {
        echo "Wrong dependency revision at $path; expected $revision" >&2; exit 2;
    }
}

export SONIC_PS5_REFERENCE=${SONIC_PS5_REFERENCE:-$deps/src/sonic-reference}
export PS5_PAYLOAD_SDK=${PS5_PAYLOAD_SDK:-$deps/sdk}
sdk_source=${PS5_SDK_SOURCE:-$deps/src/ps5-sdk}
checkout "$SONIC_PS5_REFERENCE" https://github.com/Nazky/UnleashedRecomp-Prospero.git "$SONIC_REVISION"
checkout "$sdk_source" https://github.com/mihawk-99/PS5_PayloadSDK.git "$SDK_REVISION"
sdk_tree="$deps/sdk-export-$SDK_REVISION"
if [[ ! -f $PS5_PAYLOAD_SDK/.ps5-sdk-revision ]]; then
    mkdir -p "$sdk_tree"
    git -C "$sdk_source" archive "$SDK_REVISION" | tar -x --no-same-owner -C "$sdk_tree"
    bash "$sdk_tree/platform/tools/setup-sdk.sh" "$PS5_PAYLOAD_SDK" "$SDK_REVISION" "$deps/cache"
fi
[[ $(cat "$PS5_PAYLOAD_SDK/.ps5-sdk-revision") == "$SDK_REVISION" ]] || {
    echo "SDK headers and libps5platform must both be revision $SDK_REVISION" >&2; exit 2;
}

# The reference Mesa builder has one fixed SDK location. Never replace an
# existing unrelated dependency at that path.
reference_sdk="$SONIC_PS5_REFERENCE/.deps/native/ps5-payload-sdk"
mkdir -p "$(dirname "$reference_sdk")"
if [[ ! -e $reference_sdk ]]; then
    ln -s "$PS5_PAYLOAD_SDK" "$reference_sdk"
fi
[[ $(cat "$reference_sdk/.ps5-sdk-revision") == "$SDK_REVISION" ]] || {
    echo "The reference checkout points to a different SDK: $reference_sdk" >&2; exit 2;
}

# Bound parallel memory usage in Mesa; the reference otherwise uses Ninja's
# automatic parallelism. Shell-safe paths are written with bash printf %q.
ninja_path=$(command -v ninja)
printf '#!/usr/bin/env bash\nexec %q -j %q "$@"\n' "$ninja_path" "$jobs" > "$deps/bin/ninja-limited"
chmod +x "$deps/bin/ninja-limited"
export NINJA="$deps/bin/ninja-limited"
bash "$SONIC_PS5_REFERENCE/tools/build-radv.sh" release
export RADV_ARCHIVE="$SONIC_PS5_REFERENCE/.deps/native/radv-release/lib/libvulkan_radeon.ps5.a"
provenance="$SONIC_PS5_REFERENCE/.deps/native/radv-release/PROVENANCE.txt"
[[ -s $RADV_ARCHIVE ]] || { echo "RADV cache has no archive: $RADV_ARCHIVE" >&2; exit 2; }
python3 - "$provenance" "$MESA_REVISION" "$SDK_REVISION" "$RADV_ARCHIVE" <<'PY'
import hashlib, pathlib, sys
metadata = dict(line.split(': ', 1) for line in pathlib.Path(sys.argv[1]).read_text().splitlines() if ': ' in line)
assert metadata.get('revision') == sys.argv[2], 'RADV revision differs from pins.sh'
assert metadata.get('sdk') == sys.argv[3], 'RADV was built against a different SDK'
digest = hashlib.sha256()
with open(sys.argv[4], 'rb') as stream:
    for chunk in iter(lambda: stream.read(1024 * 1024), b''):
        digest.update(chunk)
assert metadata.get('archive sha256') == digest.hexdigest(), 'RADV archive differs from its recorded hash'
PY

# Retain the working Sonic GPU driver and rebuild only its VideoOut object.
# Title capability metadata and the user's output request are separate: 60 Hz
# stays the default even though the package can also request 120 Hz.
python3 "$root/prospero/tools/build_radv_display.py" \
    --sonic-reference "$SONIC_PS5_REFERENCE" --sdk "$PS5_PAYLOAD_SDK" \
    --work "$deps/radv-display-work" \
    --output "$deps/radv-display/libvulkan_radeon.ps5.a" --jobs "$jobs"
export RADV_ARCHIVE="$deps/radv-display/libvulkan_radeon.ps5.a"

env_file="$deps/environment.sh"
{
    printf 'export SONIC_PS5_REFERENCE=%q\n' "$SONIC_PS5_REFERENCE"
    printf 'export PS5_PAYLOAD_SDK=%q\n' "$PS5_PAYLOAD_SDK"
    printf 'export PS5_SDK_SOURCE=%q\n' "$sdk_source"
    printf 'export RADV_ARCHIVE=%q\n' "$RADV_ARCHIVE"
    printf 'export LLVM_CONFIG=%q\n' "$LLVM_CONFIG"
    printf 'export PATH=%q:"$PATH"\n' "$("$llvm_config" --bindir)"
} > "$env_file"
printf 'Dependencies ready. Run: source %q\n' "$env_file"
