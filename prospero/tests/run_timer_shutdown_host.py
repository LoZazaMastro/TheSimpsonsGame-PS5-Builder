#!/usr/bin/env python3
"""Gate the real TimerQueue scheduler to reproduce a lost shutdown wakeup."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

from run_pipeline_cache_host import function


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--baseline-commit')
    parser.add_argument('--capture-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / 'tools/rexglue-sdk'
    relative = 'tools/rexglue-sdk/src/core/timer_queue.cpp'
    data = (subprocess.check_output(['git', 'show', args.baseline_commit + ':' + relative], cwd=root)
            if args.baseline_commit else (root / relative).read_bytes())
    source = data.decode()
    destructor = function(source, '  ~TimerQueue()')
    publication = ('    claim_strategy_.publish(shutdown_sequence);' if 'shutdown_sequence' in destructor
                   else '    QueueTimer(std::move(wait_item));')
    if destructor.count(publication) != 1:
        raise RuntimeError('Production shutdown publication boundary changed')
    source = source.replace(destructor, destructor.replace(publication, publication +
        '\n    timer_shutdown_test::AfterShutdownPublish(this);'), 1)
    wait = '        dp::sequence_t available = claim_strategy_.wait_until_published('
    if source.count(wait) != 1:
        raise RuntimeError('Production dispatcher wait boundary changed')
    source = source.replace(wait,
        '        timer_shutdown_test::BeforeWait(this, next_sequence, wait_queue_.empty());\n' + wait)
    end = '\n  }\n\n  std::weak_ptr<WaitItem> QueueTimer('
    if source.count(end) != 1:
        raise RuntimeError('Production dispatcher return boundary changed')
    source = source.replace(end, '\n    timer_shutdown_test::Exited(this);' + end)
    source, claims = re.subn(r'^(\s*)((?:const )?auto \w+ = claim_strategy_\.claim_one\(\);)',
        r'\1timer_shutdown_test::BeforeClaim(this, stop_requested_.load(std::memory_order_acquire));\n\1\2',
        source, flags=re.MULTILINE)
    if claims != (2 if 'shutdown_sequence' in destructor else 1):
        raise RuntimeError('Production claim sites changed')
    output = args.capture_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    variant = 'baseline' if args.baseline_commit else 'current'
    results = []
    with tempfile.TemporaryDirectory(prefix='simpsons-timer-shutdown-') as temporary:
        work = Path(temporary)
        instrumented = work / 'timer_queue.cpp'
        instrumented.write_text(source)
        binary = work / 'timer_shutdown_host'
        command = [args.cxx, '-std=c++23', '-O2', '-DNDEBUG', '-pthread', '-g',
                   '-fsanitize=undefined', '-fno-sanitize-recover=all', '-Wall', '-Wextra', '-Werror',
                   '-Wno-unused-parameter', '-D__PROSPERO__=1', '-I' + str(sdk / 'include'),
                   '-I' + str(sdk / 'thirdparty/disruptorplus/include'),
                   '-DTIMER_QUEUE_SOURCE="' + str(instrumented) + '"',
                   str(root / 'prospero/tests/timer_shutdown_host.cpp'), '-o', str(binary)]
        compiled = subprocess.run(command, capture_output=True, text=True, timeout=60)
        (output / f'{variant}-compile.txt').write_text(compiled.stdout + compiled.stderr)
        if compiled.returncode:
            raise RuntimeError(compiled.stderr)
        for mode in ('true', 'false'):
            for case in ('distant', 'full-ring'):
                run = subprocess.run([str(binary), mode, case], capture_output=True, text=True, timeout=12)
                expected = 1 if args.baseline_commit and case == 'distant' else 0
                capture = output / f'{variant}-{mode}-{case}.txt'
                capture.write_text(run.stdout + run.stderr + '\nEXIT_CODE=' + str(run.returncode) + '\n')
                results.append({'mode':mode,'case':case,'exit_code':run.returncode,'expected_exit_code':expected,
                    'capture':str(capture.relative_to(root)),'capture_sha256':hashlib.sha256(capture.read_bytes()).hexdigest()})
                print(run.stdout, end='', flush=True)
                if run.returncode != expected:
                    raise RuntimeError(f'{variant}/{mode}/{case}: expected {expected}, got {run.returncode}\n{run.stderr}')
                if expected and 'post_wake_wait=1 prompt=0' not in run.stdout:
                    raise RuntimeError('Baseline failure must specifically reproduce the consumed-wakeup race')
    report = {'result':'PASS','variant':variant,'baseline_commit':args.baseline_commit,
        'source_sha256':hashlib.sha256(data).hexdigest(),
        'boundary':'Complete production TimerQueue and real disruptorplus/threads, with scheduler observation gates before waits/claims, after shutdown publication and on dispatcher exit. No algorithm or wait primitive replaced; no PS5 timing claim.',
        'captures':results}
    (output / f'{variant}-results.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__':
    main()
