#!/usr/bin/env python3
"""Run the actual SDK jump object against the PS5 guest macro on x86-64 Linux.

Only object symbol names and the system-header include boundary are adapted.
The SDK machine code and real target jmp_buf layout are used unchanged. This
does not execute Sony setjmp and makes no claim about its unverified ABI.
"""
import argparse
import hashlib
import json
from pathlib import Path
import resource
import subprocess
import tempfile


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root', type=Path, required=True)
    parser.add_argument('--sdk-prefix', type=Path, required=True)
    parser.add_argument('--cxx', default='clang++')
    parser.add_argument('--objcopy', default='objcopy')
    parser.add_argument('--capture-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = args.sdk_root.resolve()
    archive = args.sdk_prefix.resolve() / 'target/lib/libc.a'
    captures = args.capture_dir.resolve()
    captures.mkdir(parents=True, exist_ok=True)
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    records = []

    def run(label, command, timeout=30):
        result = subprocess.run(command, cwd=root, capture_output=True, text=True, timeout=timeout)
        output = result.stdout + result.stderr + '\nEXIT_CODE=' + str(result.returncode) + '\n'
        path = captures / (label + '.txt')
        path.write_text(output)
        records.append({'test': label, 'command': command, 'exit_code': result.returncode,
                        'capture': str(path.relative_to(root)), 'sha256': sha(path)})
        if result.returncode:
            print(output, flush=True)
            raise SystemExit('Failed: ' + label)
        return result.stdout

    with tempfile.TemporaryDirectory(prefix='simpsons-sdk-jump-') as temporary:
        build = Path(temporary)
        original = build / 'sdk-setjmp-original.o'
        original.write_bytes(subprocess.run(['ar', 'p', str(archive), '_setjmp.o'],
                                             capture_output=True, check=True).stdout)
        renamed = build / 'sdk-setjmp-renamed.o'
        run('rename', [args.objcopy,
            '--redefine-sym', '_setjmp=tested_sdk_setjmp',
            '--redefine-sym', '___longjmp=tested_sdk_longjmp',
            '--redefine-sym', '_longjmp=tested_sdk_weaklongjmp', str(original), str(renamed)])
        machine_hashes = []
        for label, obj in [('original', original), ('renamed', renamed)]:
            binary = build / (label + '.text')
            run(label + '-text', [args.objcopy, '--dump-section', '.text=' + str(binary),
                                  str(obj), str(build / (label + '-copy.o'))])
            machine_hashes.append(sha(binary))
        if machine_hashes[0] != machine_hashes[1]:
            raise SystemExit('Symbol isolation changed SDK machine code')
        run('sdk-disassembly', ['objdump', '-d', str(original)])
        # Read the actual SDK machine header through an absolute include. Its
        # typedef needs only sys/cdefs.h, which the Linux host supplies. Avoid
        # glibc's different jmp_buf while retaining the real guest header.
        include_dir = build / 'include'
        include_dir.mkdir()
        shim = include_dir / 'csetjmp'
        shim.write_text('#pragma once\n#include "' + str(sdk / 'include/freebsd/x86/setjmp.h') + '"\n'
                        'extern "C" int _setjmp(jmp_buf) asm("tested_sdk_setjmp") '
                        '__attribute__((returns_twice));\n'
                        'extern "C" void ___longjmp(jmp_buf, int) asm("tested_sdk_longjmp") '
                        '__attribute__((noreturn));\n')
        (captures / 'host-header-boundary.txt').write_text(shim.read_text())
        for variant, flags in [('optimized', ['-O3', '-fomit-frame-pointer']),
                               ('frame-pointer', ['-O2', '-fno-omit-frame-pointer'])]:
            executable = build / variant
            command = [args.cxx, '-std=c++23', *flags, '-march=x86-64-v3',
                       '-ffp-model=strict', '-fno-strict-aliasing', '-femulated-tls',
                       '-fno-optimize-sibling-calls', '-D__PROSPERO__=1', '-pthread',
                       '-I' + str(include_dir), '-I' + str(root / 'simpsons/src'),
                       str(root / 'prospero/tests/guest_jump_sdk_host.cpp'), str(renamed),
                       '-o', str(executable)]
            run(variant + '-compile', command, 60)
            symbols = run(variant + '-symbols', ['nm', str(executable)])
            for expected in (' T tested_sdk_setjmp', ' T tested_sdk_longjmp'):
                if expected not in symbols:
                    raise SystemExit('Missing strong SDK test symbol: ' + expected)
            dynamic = run(variant + '-dynamic', ['nm', '-D', str(executable)])
            if any('setjmp' in row or 'longjmp' in row for row in dynamic.splitlines()):
                raise SystemExit('Host system jump implementation leaked into the test')
            print(run(variant + '-run', [str(executable)]), end='', flush=True)
        object_hash = sha(original)

    source_paths = [root / 'simpsons/src/guest_jump.h',
                    root / 'prospero/tests/guest_jump_sdk_host.cpp', Path(__file__).resolve(),
                    sdk / 'include/freebsd/x86/setjmp.h', sdk / 'libc/_setjmp.S']
    result = {'result': 'PASS', 'ps5_executed': False,
              'boundary': 'The actual matched SDK archive member runs on x86-64 Linux. '
                          'Only symbol names are isolated; the .text SHA is unchanged. '
                          'The real PS5 guest macro and SDK jmp_buf typedef are compiled. '
                          'Sony setjmp/longjmp extent and the console crash writer remain unproven.',
              'sdk_archive_sha256': sha(archive), 'sdk_object_sha256': object_hash,
              'sdk_machine_code_sha256': machine_hashes[0],
              'source_sha256': {str(p): sha(p) for p in source_paths}, 'captures': records}
    (captures / 'results.json').write_text(json.dumps(result, indent=2) + '\n')
    print('Host SDK jump tests passed; this is not a PS5 execution result.')


if __name__ == '__main__':
    main()
