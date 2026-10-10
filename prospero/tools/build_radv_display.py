#!/usr/bin/env python3
"""Build one PS5 VideoOut object and retain every other Sonic RADV object.

This is a source build, not a binary instruction patch. The upstream GPU
compiler, memory allocator and all other driver objects remain byte-identical.
"""
from __future__ import annotations
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import urllib.request
import uuid

MESA = '0b2d6d1a61d9bbf89cf8beb88a696144f67c61f8'
SDK = 'b5efad528e0ac1b8289f39d72a0e890ff68ff0a6'
MESA_SHA = '0ebd2b5157a88d03016c7778300c9d3a69bd627e2410338d31e03d1124e51f65'
BASE_WSI_SHA = '32c437481a24dafd31b1b587930a256293a201dce70cd6c1947f0ef301d4553b'
MEMBER = 'wsi_common_videoout.c.o'
TARGET = 'src/vulkan/wsi/libvulkan_wsi.a.p/' + MEMBER


def sha(path):
    with Path(path).open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def fetch(url, path, expected):
    if not path.exists():
        temporary = path.with_name(path.name + '.downloading')
        with urllib.request.urlopen(url, timeout=60) as source, temporary.open('wb') as target:
            shutil.copyfileobj(source, target)
        if sha(temporary) != expected:
            temporary.unlink()
            raise RuntimeError('Pinned download checksum mismatch: ' + path.name)
        temporary.replace(path)
    if sha(path) != expected:
        raise RuntimeError('Cached input checksum mismatch: ' + path.name)


def members(path):
    result = []
    long_names = b''
    with path.open('rb') as file:
        if file.read(8) != b'!<arch>\n':
            raise RuntimeError('A regular non-thin archive is required')
        while header := file.read(60):
            if len(header) != 60 or header[58:] != b'`\n':
                raise RuntimeError('Invalid archive header')
            name = header[:16].decode().strip()
            size = int(header[48:58])
            data = file.read(size)
            if len(data) != size:
                raise RuntimeError('Truncated archive member')
            if size & 1:
                file.read(1)
            if name == '//':
                long_names = data
                continue
            if name in ('/', '/SYM64/') or name.startswith('__.SYMDEF'):
                continue
            if name.startswith('/'):
                start = int(name[1:])
                end = long_names.index(b'/\n', start)
                name = long_names[start:end].decode()
            else:
                name = name.rstrip('/')
            result.append({'name': name, 'bytes': len(data),
                           'sha256': hashlib.sha256(data).hexdigest()})
    return result


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sonic-reference', type=Path, required=True)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--work', type=Path, default=root / '.deps/prospero/radv-display-work')
    parser.add_argument('--output', type=Path, default=root / '.deps/prospero/radv-display/libvulkan_radeon.ps5.a')
    parser.add_argument('--jobs', type=int, default=3)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    sonic, sdk, work, output = [p.resolve() for p in
                              (args.sonic_reference, args.sdk, args.work, args.output)]
    if (sdk / '.ps5-sdk-revision').read_text().strip() != SDK:
        raise RuntimeError('The PS5 headers and platform library must use the pinned SDK')
    baseline = sonic / '.deps/native/radv-release/lib/libvulkan_radeon.ps5.a'
    original_provenance = baseline.parent.parent / 'PROVENANCE.txt'
    metadata = dict(line.split(': ', 1) for line in original_provenance.read_text().splitlines()
                    if ': ' in line)
    if metadata.get('revision') != MESA or metadata.get('sdk') != SDK or metadata.get('archive sha256') != sha(baseline):
        raise RuntimeError('The Sonic RADV archive does not match its pinned provenance')
    if output == baseline:
        raise RuntimeError('The working Sonic baseline archive must not be replaced')
    source = root / 'prospero/radv/wsi_common_videoout.c'
    original_source = source.with_name('wsi_common_videoout.base.c')
    if sha(original_source) != BASE_WSI_SHA:
        raise RuntimeError('The recorded Sonic WSI source has changed')
    inputs = {'baseline_archive': sha(baseline), 'wsi_source': sha(source),
              'base_wsi_source': BASE_WSI_SHA, 'recipe': sha(Path(__file__)),
              'cross_file': sha(sonic / 'tooling/radv/ps5-cross.ini'),
              'clc_cache': sha(sonic / 'tooling/radv/clc-cache.tar.xz')}
    receipt = output.with_suffix(output.suffix + '.build.json')
    if receipt.exists() and output.exists():
        old = json.loads(receipt.read_text())
        if old.get('inputs') == inputs and old.get('archive_sha256') == sha(output):
            print('PS5 display archive is current:', output)
            return
    work.mkdir(parents=True, exist_ok=True)
    archive = work / 'mesa-0b2d6d1.tar.gz'
    fetch(f'https://codeload.github.com/mihawk-99/PS5_Mesa/tar.gz/{MESA}', archive, MESA_SHA)
    mesa = work / 'mesa-src'
    if not mesa.exists():
        mesa.mkdir()
        with tarfile.open(archive) as tar:
            for info in tar:
                parts = Path(info.name).parts[1:]
                if not parts:
                    continue
                info.name = str(Path(*parts))
                tar.extract(info, mesa, filter='data')
    destination = mesa / 'src/vulkan/wsi/wsi_common_videoout.c'
    if sha(destination) not in (BASE_WSI_SHA, inputs['wsi_source']):
        raise RuntimeError('Refusing to replace an unrelated WSI source in the build tree')
    shutil.copyfile(source, destination)
    # Use the same cached OpenCL inputs as Sonic. Building this one WSI object
    # only needs generated headers, but Mesa configuration checks these tools.
    cache, binaries = work / 'clc-cache', work / 'clc-bin'
    cache.mkdir(exist_ok=True)
    binaries.mkdir(exist_ok=True)
    with tarfile.open(sonic / 'tooling/radv/clc-cache.tar.xz') as tar:
        tar.extractall(cache, filter='data')
    clc = ('#!/usr/bin/env python3\nimport sys,pathlib,shutil\n'
           f'cache=pathlib.Path({str(cache)!r})\n'
           "args=sys.argv[1:]\noutput=args[args.index('-o')+1]\n"
           "shutil.copyfile(cache/pathlib.Path(output).name,output)\n"
           "if '--depfile' in args: pathlib.Path(args[args.index('--depfile')+1]).write_text(output+':\\n')\n")
    bindgen = ('#!/usr/bin/env python3\nimport sys,pathlib,shutil\n'
               f'cache=pathlib.Path({str(cache)!r})\n'
               'for output in sys.argv[2:4]: shutil.copyfile(cache/pathlib.Path(output).name,output)\n')
    for name, code in [('mesa_clc', clc), ('vtn_bindgen2', bindgen)]:
        (binaries / name).write_text(code)
        (binaries / name).chmod(0o755)
    constants, cross = work / 'cross-constants.ini', work / 'ps5-cross.ini'
    constants.write_text(f'[constants]\nsdk = {str(sdk)!r}\n')
    shutil.copyfile(sonic / 'tooling/radv/ps5-cross.ini', cross)
    packagecache = mesa / 'subprojects/packagecache'
    packagecache.mkdir(exist_ok=True)
    fetch('https://zlib.net/fossils/zlib-1.3.1.tar.gz', packagecache / 'zlib-1.3.1.tar.gz',
          '9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23')
    build = work / 'mesa-build'
    env = os.environ.copy()
    env['PATH'] = str(binaries) + os.pathsep + env['PATH']
    runs = []

    def run(name, command):
        capture = work / (name + '.log')
        with capture.open('w') as log:
            status = subprocess.run(list(map(str, command)), env=env, stdout=log,
                                    stderr=subprocess.STDOUT).returncode
        runs.append({'command': list(map(str, command)), 'exit_code': status,
                     'capture': str(capture), 'capture_sha256': sha(capture)})
        if status:
            raise RuntimeError(f'{name} failed; see {capture}')

    if not (build / 'build.ninja').exists():
        run('configure', ['meson', 'setup', build, mesa, '--cross-file', constants,
            '--cross-file', cross, '-Dvulkan-drivers=amd', '-Dgallium-drivers=', '-Dplatforms=',
            '-Dradv-winsys=ps5', '-Dllvm=disabled', '-Damd-use-llvm=false', '-Dvideo-codecs=',
            '-Dbuildtype=debugoptimized', '-Db_ndebug=true', '-Dglx=disabled', '-Degl=disabled',
            '-Dgbm=disabled', '-Dopengl=false', '-Dgles1=disabled', '-Dgles2=disabled',
            '-Dvalgrind=disabled', '-Dlibunwind=disabled', '-Dzstd=disabled', '-Dzlib=enabled',
            '--force-fallback-for=zlib', '-Dexpat=disabled', '-Dxmlconfig=disabled',
            '-Dshader-cache=enabled', '-Dbuild-tests=false', '-Dvulkan-layers=', '-Dtools=',
            '-Dmesa-clc=system', '-Dradv-build-id=' + MESA])
    run('build-display-object', ['ninja', '-C', build, '-j', str(args.jobs), TARGET])
    object_path = build / TARGET
    entries = json.loads((build / 'compile_commands.json').read_text())
    compile_entry = next(e for e in entries if e['file'].endswith('/wsi_common_videoout.c'))
    if str(sdk / 'bin/prospero-clang') not in compile_entry['command']:
        raise RuntimeError('The cached WSI build uses a different PS5 compiler')
    before = members(baseline)
    if sum(m['name'] == MEMBER for m in before) != 1:
        raise RuntimeError('Expected exactly one VideoOut object in the Sonic archive')
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + '.staging-' + uuid.uuid4().hex)
    try:
        shutil.copyfile(baseline, temporary)
        run('replace-display-object', [sdk / 'bin/prospero-ar', 'rD', temporary, object_path])
        after = members(temporary)
        if len(before) != len(after):
            raise RuntimeError('Archive member count changed')
        for old, new in zip(before, after):
            if old['name'] == MEMBER:
                if new['name'] != MEMBER or new['sha256'] != sha(object_path):
                    raise RuntimeError('The new WSI object is not present exactly once')
            elif old != new:
                raise RuntimeError('A non-display driver object changed: ' + old['name'])
        if sha(source) != inputs['wsi_source']:
            raise RuntimeError('Display source changed during its build')
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)
    record = {'status': 'PASS', 'created_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'mesa_revision': MESA, 'sdk_revision': SDK, 'mesa_archive_sha256': MESA_SHA,
              'inputs': inputs, 'archive_path': str(output), 'archive_sha256': sha(output),
              'baseline_archive_path': str(baseline), 'replacement_object': str(object_path),
              'replacement_object_sha256': sha(object_path), 'compile_entry': compile_entry,
              'runs': runs, 'member_count': len(after),
              'all_other_members_byte_identical': True, 'unchanged_member_count': len(after) - 1,
              'limits': ['The archive is built for PS5 and compared byte for byte. No display or PS5 GPU is executed here.']}
    receipt.write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps({'status': 'PASS', 'archive': str(output), 'sha256': record['archive_sha256'],
                      'unchanged_members': record['unchanged_member_count']}))


if __name__ == '__main__':
    main()
