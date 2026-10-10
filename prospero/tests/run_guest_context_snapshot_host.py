#!/usr/bin/env python3
"""Check an actual generated setjmp caller with the real pinned SDK jump code."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root', type=Path, required=True)
    parser.add_argument('--sdk-prefix', type=Path, required=True)
    parser.add_argument('--cxx', default='clang++')
    parser.add_argument('--capture-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / 'tools/rexglue-sdk'
    captures = args.capture_dir.resolve()
    captures.mkdir(parents=True, exist_ok=True)
    generated = root / 'simpsons/generated/default/simpsons_recomp.50.cpp'
    original = generated.read_text()
    begin = original.index('DEFINE_REX_FUNC(sub_82844398) {')
    end = original.index('\nDEFINE_REX_FUNC(', begin + 1)
    function = original[begin:end]
    (captures / 'generated-function.txt').write_text(function)
    records = []

    def run(label, command, timeout=60):
        result = subprocess.run(command, cwd=root, capture_output=True, text=True, timeout=timeout)
        capture = captures / (label + '.txt')
        capture.write_text(result.stdout + result.stderr + '\nEXIT_CODE=' + str(result.returncode) + '\n')
        records.append({'case': label, 'command': command, 'exit_code': result.returncode,
                        'capture': str(capture.relative_to(root)), 'capture_sha256': sha(capture)})
        if result.returncode:
            print(capture.read_text(), flush=True)
            raise SystemExit('Failed: ' + label)
        return result.stdout

    with tempfile.TemporaryDirectory(prefix='simpsons-context-snapshot-') as directory:
        work = Path(directory)
        archive = args.sdk_prefix.resolve() / 'target/lib/libc.a'
        original_object = work / 'sdk-original.o'
        original_object.write_bytes(subprocess.check_output(['ar', 'p', str(archive), '_setjmp.o']))
        renamed = work / 'sdk-isolated.o'
        run('isolate-symbols', ['objcopy', '--redefine-sym', '_setjmp=tested_sdk_setjmp',
             '--redefine-sym', '___longjmp=tested_sdk_longjmp',
             '--redefine-sym', '_longjmp=tested_sdk_weaklongjmp', str(original_object), str(renamed)])
        include = work / 'include'
        include.mkdir()
        (include / 'csetjmp').write_text('#pragma once\n#include "' + str(args.sdk_root.resolve() / 'include/freebsd/x86/setjmp.h') + '"\n'
            'extern "C" int _setjmp(jmp_buf) asm("tested_sdk_setjmp") __attribute__((returns_twice));\n'
            'extern "C" void ___longjmp(jmp_buf, int) asm("tested_sdk_longjmp") __attribute__((noreturn));\n')
        extracted = work / 'generated.cpp'
        extracted.write_text('#include "simpsons_init.h"\n' + function)
        includes = [include, root / 'simpsons/src', root / 'simpsons/generated/default',
                    sdk / 'include', sdk / 'thirdparty/simde', sdk / 'thirdparty/fmt/include',
                    sdk / 'thirdparty/spdlog/include']
        for label, flags in [('optimized', ['-O3', '-fomit-frame-pointer']),
                              ('frame-pointer', ['-O2', '-fno-omit-frame-pointer'])]:
            executable = work / label
            common = [args.cxx, '-std=c++23', *flags, '-DNDEBUG', '-D__PROSPERO__=1',
                      '-march=x86-64-v3', '-ffp-model=strict', '-fno-strict-aliasing',
                      '-femulated-tls', '-ffunction-sections', '-fdata-sections', '-pthread',
                      *['-I' + str(p) for p in includes]]
            run(label + '-compile', [*common, str(extracted),
                str(root / 'prospero/tests/guest_context_snapshot_host.cpp'), str(renamed),
                '-Wl,--gc-sections', '-o', str(executable)])
            print(run(label + '-run', [str(executable)]), end='', flush=True)
            ir = captures / (label + '-function.ll')
            run(label + '-ir', [*common, '-S', '-emit-llvm', str(extracted), '-o', str(ir)])
            run(label + '-disassembly', ['objdump', '-d', '--disassemble=__imp__sub_82844398', str(executable)])
        object_hash = sha(original_object)
    result = {'status': 'PASS', 'ps5_executed': False,
              'scope': 'Exact unmodified generated sub_82844398, real PPCContext and indirect dispatch macro, matched SDK jump machine code with isolated symbols; mock guest memory and callee only.',
              'limits': 'This exercises one of eleven guest setjmp call sites. It does not prove all paths or make a PS5 crash-cause claim.',
              'sdk_object_sha256': object_hash,
              'sources': {str(p.relative_to(root)): sha(p) for p in [generated,
                  root / 'simpsons/src/guest_jump.h', root / 'simpsons/generated/default/simpsons_init.h',
                  root / 'prospero/tests/guest_context_snapshot_host.cpp', Path(__file__).resolve()]},
              'cases': records}
    (captures / 'results.json').write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__':
    main()
