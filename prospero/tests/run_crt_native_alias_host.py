#!/usr/bin/env python3
"""Check real native and generated copies across the runtime's guest aliases."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

from run_pipeline_cache_host import block_end


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='clang++')
    parser.add_argument('--baseline-commit', required=True)
    parser.add_argument('--capture-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / 'tools/rexglue-sdk'
    generated = root / 'simpsons/generated/default/simpsons_recomp.65.cpp'
    original = generated.read_text()
    start = original.index('DEFINE_REX_FUNC(sub_82A3D1D8)')
    original = original[start:block_end(original, start)]
    captures = args.capture_dir.resolve()
    captures.mkdir(parents=True, exist_ok=True)
    records = []
    with tempfile.TemporaryDirectory(prefix='simpsons-native-crt-') as directory:
        temporary = Path(directory)
        legacy = temporary / 'generated_copy.cpp'
        legacy.write_text('#include "simpsons_init.h"\n' + original + '\n')
        relative = 'simpsons/src/crt_native.cpp'
        for variant in ('baseline', 'current'):
            native = temporary / (variant + '.cpp')
            native.write_bytes(subprocess.check_output(
                ['git', 'show', args.baseline_commit + ':' + relative], cwd=root)
                if variant == 'baseline' else (root / relative).read_bytes())
            executable = temporary / variant
            includes = [root / 'simpsons/src', root / 'simpsons/generated/default',
                        sdk / 'include', sdk / 'thirdparty/simde',
                        sdk / 'thirdparty/fmt/include', sdk / 'thirdparty/spdlog/include']
            command = [args.cxx, '-std=c++23', '-O2', '-DNDEBUG', '-D__PROSPERO__=1',
                       '-march=x86-64-v3', '-ffp-model=strict', '-fno-strict-aliasing',
                       '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                       '-fsanitize=undefined', '-fno-sanitize-recover=all',
                       '-DNATIVE_CRT_SOURCE="' + str(native) + '"',
                       *(['-DEXPECT_BASELINE=1'] if variant == 'baseline' else []),
                       *['-I' + str(p) for p in includes], str(legacy),
                       str(root / 'prospero/tests/crt_native_alias_host.cpp'), '-o', str(executable)]
            for stage, argv, expected in [('compile', command, 0),
                                         ('run', [str(executable)], 2 if variant == 'baseline' else 0)]:
                result = subprocess.run(argv, cwd=root, capture_output=True, text=True, timeout=60)
                output = result.stdout + result.stderr + '\nEXIT_CODE=' + str(result.returncode) + '\n'
                capture = captures / (variant + '-' + stage + '.txt')
                capture.write_text(output)
                records.append({'case': variant + '-' + stage, 'command': argv,
                                'exit_code': result.returncode, 'expected_exit_code': expected,
                                'capture': str(capture.relative_to(root)),
                                'capture_sha256': hashlib.sha256(capture.read_bytes()).hexdigest()})
                print(output, end='', flush=True)
                if result.returncode != expected:
                    raise SystemExit('Unexpected ' + variant + '-' + stage + ' result')
    sources = ['simpsons/src/crt_native.cpp', 'simpsons/src/guest_memory.h',
               'simpsons/generated/default/simpsons_recomp.65.cpp',
               'simpsons/generated/default/simpsons_init.h',
               'tools/rexglue-sdk/src/system/xmemory.cpp',
               'prospero/tests/crt_native_alias_host.cpp', 'prospero/tests/run_crt_native_alias_host.py']
    (captures / 'results.json').write_text(json.dumps({
        'result': 'PASS', 'baseline_commit': args.baseline_commit,
        'boundary': 'Actual native hook and exact generated sub_82A3D1D8 body, real shared '
                    'host mmap aliases, Release/UBSan. This proves copy semantics only; '
                    'it does not establish that the game used such a copy during a console crash.',
        'cases': records,
        'source_sha256': {p: hashlib.sha256((root / p).read_bytes()).hexdigest() for p in sources},
    }, indent=2) + '\n')


if __name__ == '__main__':
    main()
