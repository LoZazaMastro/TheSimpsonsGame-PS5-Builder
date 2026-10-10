#!/usr/bin/env python3
"""Exercise real trace recorder concurrency, partial writers and bounded dumps."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import resource
import shlex
import subprocess
import tempfile


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='clang++')
    parser.add_argument('--capture-dir', required=True, type=Path)
    parser.add_argument('--target-commands', type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / 'tools/rexglue-sdk'
    captures = args.capture_dir.resolve()
    captures.mkdir(parents=True, exist_ok=True)
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    records = []

    def run(label, command, cwd=root):
        result = subprocess.run(command, cwd=cwd, text=True, capture_output=True, timeout=45)
        path = captures / (label + '.txt')
        path.write_text(result.stdout + result.stderr + '\nEXIT_CODE=' + str(result.returncode) + '\n')
        records.append({'name': label, 'command': command, 'exit_code': result.returncode,
                        'capture': str(path.relative_to(root)), 'sha256': digest(path)})
        if result.returncode:
            raise SystemExit(label + ': ' + result.stdout + result.stderr)
        return result.stdout

    with tempfile.TemporaryDirectory(prefix='simpsons-trace-') as temporary:
        build = Path(temporary)
        binary = build / 'trace_test'
        common = [args.cxx, '-std=c++23', '-O2', '-pthread', '-Wall', '-Wextra', '-Werror',
                  '-I' + str(sdk / 'include')]
        run('compile', [*common, '-DPROSPERO_TRACE_TESTING=1',
            str(root / 'prospero/tests/trace_prospero_host.cpp'), str(sdk / 'src/core/trace_prospero.cpp'),
            '-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=write,--wrap=clock_gettime',
            '-o', str(binary)])
        modes = ['basic', 'concurrent', 'abandoned', 'interrupted', 'checkpoint', 'eintr',
                 'iofail', 'clockfail', 'raw-full', 'raw-end', 'raw-unknown',
                 'raw-outside', 'raw-unregistered']
        for mode in modes:
            capture = captures / (mode + '.log')
            print(run(mode, [str(binary), mode, str(capture)]), end='', flush=True)
            text = capture.read_text()
            records.append({'name': mode + '-log', 'capture': str(capture.relative_to(root)),
                            'sha256': digest(capture), 'bytes': capture.stat().st_size})
            if mode == 'iofail':
                assert not text
                continue
            if mode.startswith('raw'):
                rows = re.findall(r'^S address=(0x[0-9a-f]+)((?: 0x[0-9a-f]+)+)$', text, re.M)
                words = sum(len(values.split()) for _, values in rows)
                expected = 8192 if mode == 'raw-full' else 1 if mode == 'raw-end' else 0
                assert words == expected, (mode, words)
                assert f'RAW STACK END bytes={expected * 8}' in text
                if not expected:
                    assert 'no memory read' in text
                continue
            assert text.count('TRACE BEGIN') == 1 and text.count('TRACE END') == 1
            assert 'load_bias=0x0000000000400000' in text
            assert re.search(r'pid=[1-9][0-9]* monotonic_ns=', text)
            rows = re.findall(r'^T seq=(\d+) thread=(0x[0-9a-f]+) kind=[a-z_]+\((\d+)\) '
                              r'caller=(0x[0-9a-f]+) a=(0x[0-9a-f]+) b=(0x[0-9a-f]+) '
                              r'c=(0x[0-9a-f]+) d=(0x[0-9a-f]+)$', text, re.M)
            seqs = [int(row[0]) for row in rows]
            assert seqs == sorted(set(seqs)) and 0 < len(rows) <= 16384
            for seq, thread, kind, caller, a, b, c, d in rows:
                assert int(thread, 16)
                if int(kind) == 2:
                    value = int(a, 16)
                    assert int(caller, 16) == 0xfedcba9876543210
                    assert int(b, 16) == (~value & ((1 << 64) - 1))
                    assert int(c, 16) == value ^ 0x55aa55aa55aa55aa
                    assert int(d, 16) == value + 1
            begin = next(line for line in text.splitlines() if 'TRACE BEGIN' in line)
            header = dict(re.findall(r'(\w+)=(\d+)', begin))
            assert int(header['retained']) == len(rows)
            if mode == 'concurrent':
                assert int(header['reserved']) == 96000
                assert int(header['published']) == len(rows) + int(header['overwritten'])
                assert len({row[1] for row in rows}) > 1
            if mode in ('abandoned', 'interrupted'):
                assert int(header['incomplete']) == 1
                assert re.search(r'dropped_busy=[1-9][0-9]*', text)
                assert 1 not in seqs
            if mode in ('checkpoint', 'clockfail'):
                assert 'trace-checkpoint seq=1001' in text and 'phase=110' in text
                assert 'METRICS test=1\n' in text and 'MUST_NOT_WRITE' not in text and 'phase=999' not in text
            if mode == 'clockfail':
                assert 'monotonic_ns=0 clock_known=0' in text
        # Build without test hooks and inspect undefined symbols/initializers.
        obj = build / 'trace-production.o'
        run('production-object', [*common, '-c', str(sdk / 'src/core/trace_prospero.cpp'), '-o', str(obj)])
        symbols = run('production-symbols', ['nm', str(obj)])
        imports = run('production-imports', ['nm', '-u', str(obj)])
        sections = run('production-sections', ['readelf', '-SW', str(obj)])
        assert not re.search(r'_GLOBAL__sub_I|__cxx_global_var_init|__emutls|__cxa_atexit', symbols)
        assert not re.search(r'malloc|calloc|realloc|operator new|_Zn|printf|pthread_mutex', imports)
        assert '.init_array' not in sections
        if args.target_commands:
            entries = json.loads(args.target_commands.resolve().read_text())
            entry = next(e for e in entries if e['file'].endswith('/fault_prospero.cpp'))
            for name in ['trace_prospero', 'fault_prospero']:
                command = shlex.split(entry['command'])
                command[command.index('-o') + 1] = str(build / (name + '-ps5.o'))
                command[-1] = str(sdk / ('src/core/' + name + '.cpp'))
                run(name + '-target-compile', command, Path(entry['directory']))
                target_symbols = run(name + '-target-symbols', ['nm', str(build / (name + '-ps5.o'))])
                target_sections = run(name + '-target-sections', ['readelf', '-SW', str(build / (name + '-ps5.o'))])
                assert not re.search(r'_GLOBAL__sub_I|__cxx_global_var_init|__emutls|__cxa_atexit', target_symbols)
                assert '.init_array' not in target_sections
    sources = ['tools/rexglue-sdk/src/core/trace_prospero.cpp',
               'tools/rexglue-sdk/include/rex/platform/trace_prospero.h',
               'tools/rexglue-sdk/src/core/fault_prospero.cpp',
               'tools/rexglue-sdk/include/rex/platform/fault_prospero.h',
               'prospero/tests/trace_prospero_host.cpp', 'prospero/tests/run_trace_prospero_host.py']
    (captures / 'results.json').write_text(json.dumps({
        'result': 'PASS', 'ps5_executed': False,
        'boundary': 'Real recorder code on Linux; PS5 objects compile without global initializers. '
                    'Partial writers, real thread concurrency, allocation refusal, guarded mapped memory, '
                    'EINTR/partial/failed writes and clock failure are exercised. No native PS5 execution.',
        'source_sha256': {p: digest(root / p) for p in sources}, 'captures': records,
    }, indent=2) + '\n')
    print('All trace tests passed; target execution remains a separate hardware gate.')


if __name__ == '__main__':
    main()
