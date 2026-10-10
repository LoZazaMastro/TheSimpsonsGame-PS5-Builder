#!/usr/bin/env python3
"""Run the complete production PS5 TimerQueue using host threads and clocks."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--baseline-commit')
    parser.add_argument('--expect-default', choices=['true', 'false'], required=True)
    parser.add_argument('--capture-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / 'tools/rexglue-sdk'
    rel = 'tools/rexglue-sdk/src/core/timer_queue.cpp'
    data = (subprocess.check_output(['git', 'show', args.baseline_commit + ':' + rel], cwd=root)
            if args.baseline_commit else (root / rel).read_bytes())
    captures = args.capture_dir.resolve()
    captures.mkdir(parents=True, exist_ok=True)
    variant = 'baseline' if args.baseline_commit else 'current'
    records = []
    with tempfile.TemporaryDirectory(prefix='simpsons-timer-audit-') as temporary:
        work = Path(temporary)
        source = work / 'timer_queue.cpp'
        source.write_bytes(data)
        binary = work / 'timer_queue_host'
        command = [args.cxx, '-std=c++23', '-O2', '-DNDEBUG', '-pthread', '-g',
                   '-fsanitize=undefined', '-fno-sanitize-recover=all',
                   '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                   '-D__PROSPERO__=1', '-I' + str(sdk / 'include'),
                   '-I' + str(sdk / 'thirdparty/disruptorplus/include'),
                   '-DTIMER_QUEUE_SOURCE="' + str(source) + '"',
                   str(root / 'prospero/tests/timer_queue_host.cpp'), '-o', str(binary)]
        for label, argv in [('compile', command), ('blocking', [str(binary), 'true', args.expect_default]),
                            ('spin', [str(binary), 'false', args.expect_default])]:
            result = subprocess.run(argv, cwd=root, capture_output=True, text=True, timeout=60)
            text = result.stdout + result.stderr + '\nEXIT_CODE=' + str(result.returncode) + '\n'
            capture = captures / (variant + '-' + label + '.txt')
            capture.write_text(text)
            records.append({'test': variant + '-' + label, 'exit_code': result.returncode,
                            'capture': str(capture.relative_to(root)),
                            'capture_sha256': hashlib.sha256(capture.read_bytes()).hexdigest()})
            print(text, end='', flush=True)
            if result.returncode:
                raise SystemExit(result.returncode)
    (captures / (variant + '-results.json')).write_text(json.dumps({
        'result': 'PASS', 'variant': variant, 'source_sha256': hashlib.sha256(data).hexdigest(),
        'baseline_commit': args.baseline_commit, 'expected_default': args.expect_default,
        'boundary': 'Complete TimerQueue source, actual host std::thread and disruptorplus; '
                    'registry storage and thread naming fixture. Not a PS5 timing measurement.',
        'captures': records,
    }, indent=2) + '\n')


if __name__ == '__main__':
    main()
