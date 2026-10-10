#!/usr/bin/env bash
# Stage a native homebrew title directory, using the reference's ELF/SELF tools.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
source "$root/prospero/pins.sh"
[[ $# == 1 ]] || { echo "Usage: prospero/package.sh path/to/simpsons.elf" >&2; exit 2; }
input=$(realpath "$1")
[[ -s $input ]] || { echo "Missing linked PS5 ELF: $input" >&2; exit 2; }
: "${SONIC_PS5_REFERENCE:?Set SONIC_PS5_REFERENCE}"
: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK}"
work="$root/out/prospero-package"
host="$root/out/prospero-host"
mkdir -p "$work" "$host"
tool="$host/ps5-native-tool"
python3 "$root/prospero/tools/build_native_tool.py" \
    --reference "$SONIC_PS5_REFERENCE" --output "$tool" --compiler "${HOST_CXX:-g++}"
stubs=${PROSPERO_STUB_DIR:-$input.link/stubs}
for stub in libSceAgc.so libSceAgcDriver.so; do
    [[ -s $stubs/$stub ]] || { echo "Missing link stub: $stubs/$stub" >&2; exit 2; }
done
"$tool" link --in "$input" --out "$work/eboot.elf" \
    --stub-dir "$PS5_PAYLOAD_SDK/target/lib" \
    --stub "$stubs/libSceAgc.so" --stub "$stubs/libSceAgcDriver.so" \
    --module-sdk 0x02000009 --companion-sdk 0x08050001 --file-name eboot.elf
python3 "$root/prospero/validate_native.py" "$work/eboot.elf"

stage=$(mktemp -d "$work/title.XXXXXX")
trap 'rm -rf -- "$stage"' EXIT
mkdir -p "$stage/sce_sys" "$stage/sce_module"
mkdir -p "$stage/logs" "$stage/cache" "$stage/userdata"
# mktemp creates the title root as 0700. ShadowMount's nullfs mount preserves
# source permissions; normalize these before the directory becomes the title.
chmod 0755 "$stage" "$stage/sce_sys" "$stage/sce_module"
chmod 0777 "$stage/logs" "$stage/cache" "$stage/userdata"
"$tool" self --sign --in "$work/eboot.elf" --out "$stage/eboot.bin" --magic 0x1D3D154F
for asset in param.json icon0.png pic0.dds pic1.dds snd0.at9; do
    cp "$root/prospero/sce_sys/$asset" "$stage/sce_sys/$asset"
done
bash "$SONIC_PS5_REFERENCE/tools/validate-assets.sh" "$stage/sce_sys"
cp "$SONIC_PS5_REFERENCE/runtime/libc.prx" "$stage/sce_module/libc.prx"
chmod 0755 "$stage/eboot.bin" "$stage/sce_module/libc.prx"
chmod 0644 "$stage/sce_sys/"*
python3 - "$stage" "$PROSPERO_TITLE_ID" <<'PY'
import json, pathlib, re, stat, sys
root = pathlib.Path(sys.argv[1])
for directory in (root, root / 'sce_sys', root / 'sce_module'):
    assert stat.S_IMODE(directory.stat().st_mode) == 0o755, directory
for directory in (root / 'logs', root / 'cache', root / 'userdata'):
    assert stat.S_IMODE(directory.stat().st_mode) == 0o777, directory
for executable in (root / 'eboot.bin', root / 'sce_module/libc.prx'):
    assert stat.S_IMODE(executable.stat().st_mode) == 0o755, executable
# Mutable files are created by the game inside these directories. Shipping
# a live config or log would overwrite preferences/history on an update.
assert not (root / 'userdata/simpsons.toml').exists()
assert not (root / 'logs/startup.log').exists()
path = root / 'sce_sys/param.json'
p = json.loads(path.read_text())
assert p['titleId'] == sys.argv[2]
assert sys.argv[2] in p['contentId']
assert re.fullmatch(r'[A-Z]{2}[0-9]{4}-PPSA[0-9]{5}_00-[A-Z0-9]{16}', p['contentId'])
# This is an explicit system metadata value, not the title's file-byte count.
# Use this project's known reference value (Sonic's published title uses 256).
# The v3 file-byte value 131364743 was rejected by ConvertDownloadSize with
# 0x80a40087 before app0/CRT startup. Do not rewrite even the JSON formatting.
download_data_size = p.get('downloadDataSize')
if type(download_data_size) is not int or download_data_size != 256:
    raise SystemExit('downloadDataSize must be the explicit project value 256; '
                     f'got {download_data_size!r}. It is not the package byte count.')
# Size accounting is informational and includes the unmodified metadata file.
total_bytes = sum(f.stat().st_size for f in root.rglob('*') if f.is_file())
print(json.dumps({'title_total_bytes': total_bytes,
                  'downloadDataSize': download_data_size,
                  'download_data_size_source': 'explicit_metadata'}))
PY
mkdir -p "$root/dist"
dest="$root/dist/$PROSPERO_TITLE_ID"
# Keep the previous successful build until the entire replacement has passed.
if [[ -e $dest ]]; then
    backup="$root/dist/$PROSPERO_TITLE_ID.previous.$(date -u +%Y%m%dT%H%M%SZ)"
    mv "$dest" "$backup"
fi
mv "$stage" "$dest"
trap - EXIT
printf 'Native PS5 title staged: %s\n' "$dest"
