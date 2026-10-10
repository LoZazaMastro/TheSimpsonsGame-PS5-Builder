#!/usr/bin/env python3
"""Compile and test the real PS5 profile migration and ReXGlue TOML persistence."""
import argparse
import os
import pathlib
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='clang++')
    parser.add_argument('--baseline-ref', help='Also verify the exact v3.4 source at this full Git commit fails under lstat EPERM')
    parser.add_argument('--capture-directory', type=pathlib.Path,
                        help='Save each actual test execution output and exit status here')
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[2]
    sdk = root / 'tools/rexglue-sdk'
    if args.baseline_ref and not re.fullmatch(r'[0-9a-fA-F]{40}', args.baseline_ref):
        parser.error('--baseline-ref must be a full 40-digit commit hash')
    if args.capture_directory:
        args.capture_directory.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='simpsons-profile-host-') as directory:
        scratch = pathlib.Path(directory)
        stubs = scratch / 'include/rex'
        stubs.mkdir(parents=True)
        # The real CVar loader/serializer is linked. Only its unrelated logging
        # sink is replaced; file operations below remain real POSIX operations.
        (stubs / 'logging.h').write_text('''#pragma once
namespace profile_test { template<class... T> void Log(const T&...) {} }
#define REXLOG_DEBUG(...) ::profile_test::Log(__VA_ARGS__)
#define REXLOG_INFO(...) ::profile_test::Log(__VA_ARGS__)
#define REXLOG_WARN(...) ::profile_test::Log(__VA_ARGS__)
#define REXLOG_ERROR(...) ::profile_test::Log(__VA_ARGS__)
''')
        injector = scratch / 'profile_lstat_eperm.so'
        subprocess.run([args.cxx, '-x', 'c', '-std=c11', '-Wall', '-Wextra', '-Werror',
                        '-fPIC', '-shared', str(root / 'prospero/tests/profile_lstat_eperm.c'),
                        '-ldl', '-o', str(injector)], check=True)
        common = [args.cxx, '-std=c++23', '-Wall', '-Wextra', '-Werror', '-O1', '-DNDEBUG',
                   '-I', str(scratch / 'include'), '-I', str(root / 'simpsons/src'),
                   '-I', str(sdk / 'include'),
                   '-I', str(sdk / 'thirdparty/tomlplusplus/include'),
                   '-I', str(sdk / 'thirdparty/cli11/include'),
                   str(root / 'prospero/tests/prospero_profile_host.cpp'),
                   str(sdk / 'src/core/cvar.cpp'), '-pthread', '-ldl']
        for name in ('open', 'mkstemp', 'write', 'fsync', 'rename', 'close'):
            common.append('-Wl,--wrap=' + name)

        def compile_run(source, label):
            binary = scratch / label
            baseline_flags = ['-DPROSPERO_PROFILE_BASELINE=1'] if label == 'profile_baseline' else []
            subprocess.run(common + baseline_flags + [str(source), '-o', str(binary)], check=True)
            cases = scratch / (label + '-cases')
            cases.mkdir()
            env = os.environ.copy()
            env['LD_PRELOAD'] = str(injector)
            env['PROSPERO_PROFILE_DENY_LSTAT_ROOT'] = str(cases)
            result = subprocess.run([str(binary), str(cases), str(root / 'prospero/profile.toml')],
                                    env=env, capture_output=True, text=True)
            if args.capture_directory:
                (args.capture_directory / (label + '.txt')).write_text(
                    'exit_code=' + str(result.returncode) + '\n\n[stdout]\n' + result.stdout +
                    '\n[stderr]\n' + result.stderr)
            return result

        if args.baseline_ref:
            baseline = scratch / 'profile_v3_4.cpp'
            baseline.write_bytes(subprocess.check_output(
                ['git', 'show', args.baseline_ref + ':simpsons/src/prospero_profile.cpp'], cwd=root))
            result = compile_run(baseline, 'profile_baseline')
            expected = 'Cannot inspect PS5 configuration path: Operation not permitted'
            if result.returncode == 0 or expected not in result.stderr or 'PASS boundary:' not in result.stdout:
                raise RuntimeError('baseline did not reproduce the real lstat EPERM failure:\n' +
                                   result.stdout + result.stderr)
            print('EXPECTED BASELINE FAILURE ' + args.baseline_ref + ': ' + result.stderr.strip(), flush=True)
        result = compile_run(root / 'simpsons/src/prospero_profile.cpp', 'profile_host')
        print(result.stdout, end='', flush=True)
        if result.returncode:
            raise RuntimeError(result.stderr or 'profile test failed without diagnostic')


if __name__ == '__main__':
    main()
