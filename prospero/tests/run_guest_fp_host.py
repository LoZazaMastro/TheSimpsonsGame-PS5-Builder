#!/usr/bin/env python3
"""Compare the old guest compiler policy and Sonic/ReX's strict FP policy."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='clang++')
    parser.add_argument('--capture-dir', required=True, type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / 'tools/rexglue-sdk'
    captures = args.capture_dir.resolve()
    captures.mkdir(parents=True, exist_ok=True)
    records = []
    with tempfile.TemporaryDirectory(prefix='simpsons-fp-policy-') as temporary:
        for variant, policy in [('baseline', []), ('strict', ['-fno-strict-aliasing', '-ffp-model=strict'])]:
            target = Path(temporary) / variant
            command = [args.cxx, '-std=c++23', '-O3', '-march=x86-64-v3',
                       '-fdenormal-fp-math=ieee', '-D__PROSPERO__=1',
                       '-I' + str(sdk / 'include'), '-I' + str(sdk / 'thirdparty/simde'),
                       *policy, str(root / 'prospero/tests/guest_fp_host.cpp'), '-o', str(target)]
            for label, argv, expected in [('compile', command, 0),
                                          ('run', [str(target)], 2 if variant == 'baseline' else 0)]:
                result = subprocess.run(argv, cwd=root, capture_output=True, text=True, timeout=60)
                text = result.stdout + result.stderr + '\nEXIT_CODE=' + str(result.returncode) + '\n'
                path = captures / (variant + '-' + label + '.txt')
                path.write_text(text)
                records.append({'test': variant + '-' + label, 'exit_code': result.returncode,
                                'expected_exit_code': expected, 'command': argv,
                                'capture': str(path.relative_to(root)),
                                'capture_sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
                print(text, end='', flush=True)
                if result.returncode != expected:
                    raise SystemExit('Unexpected result in ' + variant + '-' + label)
    sources = ['prospero/tests/guest_fp_host.cpp', 'prospero/tests/run_guest_fp_host.py',
               'tools/rexglue-sdk/include/rex/ppc/context.h',
               'tools/rexglue-sdk/include/rex/platform/fpscr.h']
    (captures / 'results.json').write_text(json.dumps({
        'result': 'PASS',
        'boundary': 'Real FPSCRRegister and host SSE arithmetic compiled with Clang; '
                    'four guest rounding modes, positive/negative half-ULP adds and conversion. '
                    'No claim that this reproduces the console allocator crash.',
        'source_sha256': {p: hashlib.sha256((root / p).read_bytes()).hexdigest() for p in sources},
        'captures': records,
    }, indent=2) + '\n')


if __name__ == '__main__':
    main()
