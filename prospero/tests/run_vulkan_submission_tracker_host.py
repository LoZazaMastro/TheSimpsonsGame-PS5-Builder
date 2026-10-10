#!/usr/bin/env python3
"""Run the production submission tracker against deterministic Vulkan API fakes.

Optional --baseline-ref compiles the identical harness against that Git revision's
tracker .cpp and requires it to fail, proving this test catches the old defect.
Only Vulkan dependencies are stubbed. The real class, RAII lifecycle and complete
production implementation are compiled; no copied comparison function is tested.
"""
import argparse
import hashlib
import json
import pathlib
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline-ref')
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[2]
    sdk = root / 'tools/rexglue-sdk'
    source = sdk / 'src/ui/vulkan/vulkan_submission_tracker.cpp'
    header = sdk / 'include/rex/ui/vulkan/submission_tracker.h'
    harness = root / 'prospero/tests/vulkan_submission_tracker_host.cpp'
    stubs = root / 'prospero/tests/vulkan_submission_tracker_stubs'
    captures = root / 'prospero/validation/vulkan-submission-tracker-captures'
    captures.mkdir(parents=True, exist_ok=True)
    report_path = root / 'prospero/validation/vulkan-submission-tracker-regression.json'
    inputs = [source, header, harness, pathlib.Path(__file__).resolve(), *sorted(stubs.rglob('*.h'))]
    report = {
        'status': 'RUNNING',
        'scope': 'Actual production tracker class and complete cpp compiled separately; Vulkan calls simulated',
        'hardware_validated': False,
        'not_tested_here': 'Real GPU/driver fence completion, image output, or physical PS5 display',
        'source_sha256': {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
        'runs': [],
    }
    report_path.write_text(json.dumps(report, indent=2) + '\n')

    def run(name, implementation, expected_success):
        binary = captures / name
        command = ['g++', '-std=c++23', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                   '-I' + str(stubs), '-I' + str(sdk / 'include'),
                   '-I' + str(sdk / 'thirdparty/vulkan-headers/include'),
                   str(harness), str(implementation), '-o', str(binary)]
        compiled = subprocess.run(command, text=True, capture_output=True, timeout=60)
        (captures / (name + '-compile.txt')).write_text(compiled.stdout + compiled.stderr)
        if compiled.returncode:
            raise RuntimeError(f'{name} compilation failed:\n{compiled.stderr}')
        result = subprocess.run([str(binary)], text=True, capture_output=True, timeout=30)
        (captures / (name + '-runtime.txt')).write_text(result.stdout + result.stderr)
        lines = result.stdout.strip().splitlines()
        item = {'name': name, 'source_sha256': hashlib.sha256(implementation.read_bytes()).hexdigest(),
                'exit_code': result.returncode, 'passed': sum(s.startswith('PASS ') for s in lines),
                'failed': sum(s.startswith('FAIL ') for s in lines), 'checks': lines}
        report['runs'].append(item)
        if expected_success:
            if result.returncode or item['passed'] != 14 or item['failed']:
                raise RuntimeError(f'{name} failed:\n{result.stdout}{result.stderr}')
        elif result.returncode != 1 or not item['failed']:
            raise RuntimeError(f'{name} failed to reject baseline defect:\n{result.stdout}{result.stderr}')

    try:
        run('production', source, True)
        if args.baseline_ref:
            baseline_source = captures / 'baseline.cpp'
            relative = str(source.relative_to(root))
            baseline = subprocess.run(['git', 'show', f'{args.baseline_ref}:{relative}'], cwd=root,
                                      check=True, capture_output=True)
            baseline_source.write_bytes(baseline.stdout)
            report['baseline_git_ref'] = args.baseline_ref
            run('baseline', baseline_source, False)
        report['status'] = 'PASS'
    except Exception as error:
        report['status'] = 'FAIL'
        report['error'] = str(error)
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'status': report['status'], 'runs': [
        {key: item[key] for key in ('name', 'passed', 'failed')} for item in report['runs']],
        'report': str(report_path.relative_to(root))}))


if __name__ == '__main__':
    main()
