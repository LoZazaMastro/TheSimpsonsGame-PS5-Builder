#!/usr/bin/env python3
"""Prove reporter shutdown precedes constructor-registered dtors on main return."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline-commit', required=True)
    parser.add_argument('--cxx', default='clang++')
    parser.add_argument('--capture-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    captures = args.capture_dir.resolve()
    captures.mkdir(parents=True, exist_ok=True)
    records = []
    with tempfile.TemporaryDirectory(prefix='simpsons-crt-exit-') as directory:
        work = Path(directory)
        for label in ('baseline', 'current'):
            text = (subprocess.check_output(['git', 'show', args.baseline_commit + ':prospero/app_crt.cpp'], cwd=root).decode()
                    if label == 'baseline' else (root / 'prospero/app_crt.cpp').read_text())
            # Isolate host main without changing the CRT implementation or
            # moving any start/stop, init, exit or atexit operations.
            assert text.count('__asm__("main")') == 1
            text = text.replace('__asm__("main")', '__asm__("fixture_application_main")')
            source = work / (label + '.cpp')
            source.write_text(text)
            obj = work / (label + '.o')
            renames = {'_start': 'fixture_run_start', '_init': 'fixture_run_init',
                       '_fini': 'fixture_run_fini', 'atexit': 'host_atexit', 'exit': 'host_exit'}
            for section in ('preinit', 'init', 'fini'):
                for edge in ('start', 'end'):
                    renames['__' + section + '_array_' + edge] = 'fixture_' + section + '_' + edge
            executable = work / label
            commands = [(label + '-compile', [args.cxx, '-std=c++23', '-O2', '-fno-builtin',
                *['-D' + k + '=' + v for k, v in renames.items()], '-I' + str(root / 'prospero'),
                '-c', str(source), '-o', str(obj)], 0),
                (label + '-link', [args.cxx, '-std=c++23', '-O2', '-no-pie',
                '-I' + str(root / 'tools/rexglue-sdk/include'),
                str(root / 'prospero/tests/crt_metrics_order_host.cpp'), str(obj), '-o', str(executable)], 0)]
            commands.extend((label + '-return-' + str(status), [str(executable), str(status)],
                             42 if label == 'baseline' else status) for status in (0, 7))
            for case, command, expected in commands:
                result = subprocess.run(command, cwd=root, capture_output=True, text=True, timeout=30)
                capture = captures / (case + '.txt')
                capture.write_text(result.stdout + result.stderr + '\nEXIT_CODE=' + str(result.returncode) + '\n')
                records.append({'case': case, 'command': command, 'exit_code': result.returncode,
                                'expected_exit_code': expected, 'capture': str(capture.relative_to(root)),
                                'capture_sha256': sha(capture)})
                if result.returncode != expected:
                    print(capture.read_text(), flush=True)
                    raise SystemExit('Unexpected result: ' + case)
                if '-return-' in case: print(case + ': ' + result.stdout, end='', flush=True)
    (captures / 'results.json').write_text(json.dumps({'status': 'PASS', 'ps5_executed': False,
        'baseline_commit': args.baseline_commit,
        'scope': 'Real CRT entry function and real init/fini/atexit registration order; controlled native calls, main body, reporter stop and LIFO exit adapter.',
        'limits': 'Tests ordering on normal return and error return from main. Actual reporter pthread joining is covered separately. Explicit exit before main returns still uses the later _fini fallback; this is not proof of a historical crash cause.',
        'cases': records,
        'sources': {str(p.relative_to(root)): sha(p) for p in [root / 'prospero/app_crt.cpp',
            root / 'prospero/tests/crt_metrics_order_host.cpp', Path(__file__).resolve()]},
    }, indent=2) + '\n')


if __name__ == '__main__':
    main()
