#!/usr/bin/env python3
"""Exercise the actual thread-safe toggle and explicit session controller without a GPU."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
sdk = root/'tools/rexglue-sdk'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report', type=Path, default=root/'prospero/validation/framegen-session-v3.10.4.json')
args = parser.parse_args()
path = args.report.resolve()
captures = path.with_name(path.stem + '-captures')
captures.mkdir(parents=True, exist_ok=True)
source = sdk/'src/core/framegen_prospero.cpp'
header = sdk/'include/rex/platform/framegen_prospero.h'
fixture = root/'prospero/tests/framegen_session_host.cpp'
stubs = root/'prospero/tests/framegen_host_stubs'
binary = captures/'framegen-session-host'
command = ['g++','-std=c++23','-O2','-Wall','-Wextra','-Werror','-pthread','-DFMT_HEADER_ONLY',
           '-I'+str(stubs),'-I'+str(sdk/'include'),'-I'+str(sdk/'thirdparty/fmt/include'),
           str(fixture),str(source),'-o',str(binary)]
compiled = subprocess.run(command,capture_output=True,text=True,timeout=60)
(captures/'compile.txt').write_text(compiled.stdout+compiled.stderr)
assert compiled.returncode == 0, compiled.stderr
result = subprocess.run([str(binary)],capture_output=True,text=True,timeout=30)
(captures/'runtime.txt').write_text(result.stdout+result.stderr)
assert result.returncode == 0, result.stdout+result.stderr
inputs = [Path(__file__).resolve(),source,header,fixture,stubs/'rex/cvar.h',stubs/'rex/logging.h']
report = {'status':'PASS','scope':__doc__,'hardware_validated':False,
          'limits':['CVar registry and logging are simulated. The production atomic controller is compiled unchanged.',
                    'No PS5 controller, rendering, GPU scheduling or image output runs in this test.'],
          'checks':result.stdout.splitlines(),
          'source_sha256':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}}
path.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'status':'PASS','checks':len(report['checks']),'report':str(path.relative_to(root))}))
