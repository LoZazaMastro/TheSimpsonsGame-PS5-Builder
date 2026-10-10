#!/usr/bin/env python3
"""Run the actual FG adapter, eight stages, pacer, paths and entrypoint on a recording driver.

All production .cpp files in this scope are compiled unchanged. Only native
Vulkan/ReX objects, logging/CVar metadata and the shader construction boundary
are doubled. Shader validation has its own real SPIRV-Tools/PE report. No GPU,
PS5 runtime, proprietary DLL, output quality or frame-rate measurement runs.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--captures', type=Path, default=root/'prospero/validation/framegen-runtime-v3.10-captures')
    parser.add_argument('--report', type=Path, default=root/'prospero/validation/framegen-runtime-v3.10.json')
    args = parser.parse_args()
    sdk = root/'tools/rexglue-sdk'
    source = sdk/'src/ui/vulkan/framegen'
    captures = args.captures.resolve(); captures.mkdir(parents=True, exist_ok=True)
    fixture = root/'prospero/tests/framegen_runtime_host.cpp'
    stubs = root/'prospero/tests/framegen_host_stubs'
    stages = ['common','alpha','beta','chain','delta','gamma','generate','mipmaps']
    compiled = [sdk/'src/core/framegen_prospero.cpp', sdk/'src/ui/vulkan/vulkan_framegen_prospero.cpp',
                source/'vulkan_adapter.cpp', source/'frame_gen_pacer.cpp',
                *[source/('lsfg_'+stage+'.cpp') for stage in stages]]
    headers = [sdk/'include/rex/platform/framegen_prospero.h',
               sdk/'include/rex/platform/metrics_prospero.h',
               sdk/'include/rex/ui/vulkan/framegen_prospero.h',
               sdk/'include/rex/ui/vulkan/functions/device_1_0.inc',
               *source.glob('*.h')]
    tracked = sorted(set([Path(__file__).resolve(), fixture, *compiled, *headers,
                          *[p for p in stubs.rglob('*') if p.is_file()],
                          sdk/'thirdparty/vulkan-headers/include/vulkan/vulkan_core.h',
                          sdk/'thirdparty/fmt/include/fmt/format.h']))
    hashes = {str(p.relative_to(root)):sha(p) for p in tracked}
    runs, checks, memory = [], [], []
    status, error = 'PASS', None

    def run(name: str, command: list[str], timeout: int = 180) -> str:
        start = time.monotonic()
        env = os.environ.copy()
        sanitizer_env = {}
        if 'sanitized' in name:
            sanitizer_env = {'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1',
                             'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
            env.update(sanitizer_env)
        result = subprocess.run(command,capture_output=True,text=True,timeout=timeout,env=env,cwd=root)
        capture = captures/(name+'.txt'); capture.write_text(result.stdout+result.stderr)
        runs.append({'name':name,'command':command,'exit_code':result.returncode,
                     'elapsed_seconds':round(time.monotonic()-start,3),
                     'capture':str(capture.relative_to(root)) if capture.is_relative_to(root) else str(capture),
                     'capture_sha256':sha(capture),'sanitizer_environment':sanitizer_env})
        print(name, 'PASS' if result.returncode == 0 else 'FAIL',flush=True)
        if result.returncode:
            raise RuntimeError(name+' failed: '+result.stdout+result.stderr)
        return result.stdout

    try:
        run('compiler-version',[args.cxx,'--version'])
        include_dirs = [stubs,source,sdk/'include',sdk/'thirdparty/vulkan-headers/include',
                        sdk/'thirdparty/fmt/include']
        common = [args.cxx,'-std=c++23','-Wall','-Wextra','-Werror','-Wno-missing-field-initializers',
                  '-pthread','-DREX_PLATFORM_PROSPERO=1','-DFMT_HEADER_ONLY',
                  *['-I'+str(p) for p in include_dirs],str(fixture),*[str(p) for p in compiled]]
        for variant,flags in [('release',['-O2']),('sanitized',['-O1','-g','-fsanitize=address,undefined',
                                                           '-fno-omit-frame-pointer','-fno-sanitize-recover=all'])]:
            executable = captures/('framegen-runtime-'+variant)
            run('compile-'+variant,common+flags+['-o',str(executable)])
            output = run('runtime-'+variant,[str(executable),str(captures/('files-'+variant))])
            checks.extend(variant+': '+line for line in output.splitlines() if line.startswith('PASS '))
            for line in output.splitlines():
                if line.startswith('MEMORY '):
                    parts=line.split(); record={'build_variant':variant,'shader_boundary':parts[1]}
                    record.update({key:int(value) for key,value in (p.split('=') for p in parts[2:])})
                    memory.append(record)
        for path in tracked:
            if sha(path)!=hashes[str(path.relative_to(root))]:
                raise RuntimeError('Source changed during test; rerun after freeze: '+str(path))
    except Exception as exception:
        status,error='FAIL',str(exception)
        print(error,file=sys.stderr)
    report={'status':status,'scope':__doc__,'hardware_validated':False,'source_sha256':hashes,
            'production_cpp_compiled_unchanged':[str(p.relative_to(root)) for p in compiled],
            'runs':runs,'checks':checks,'memory_models':memory,'error':error,
            'limits':[
                'The driver records real production Vulkan commands, layouts, descriptor bindings, lifetime and failure decisions; no shader executes and no queue scans out an image.',
                'Memory requirements come from a stated synthetic driver rule: dimensions rounded to 16 pixels, then image/buffer size and alignment rounded to 16 KiB. Real RADV tiling, dedicated-allocation granularity and driver/pipeline overhead require console observation.',
                'FP16 versus FP32 in this suite selects the synthetic shader boundary; image formats are those actually requested by the full production chain. Real PE/shader selection, validation and fallback are covered separately by framegen-shaders-v3.10.json.',
                'The whole 25-module shader-construction boundary is a double; expected stage layouts are independently transcribed from the pinned Eden stages. Native Vulkan feature enabling and WSI refresh are separate target/display gates.',
                'Host path operations test open/fstat on ordinary files, zero-length files, symlinks, directories and FIFOs. PS5 syscall ABI and title permissions are not executed.',
                'ASan and UBSan instrument the fixture and every compiled production translation unit. LeakSanitizer is disabled because the host cannot enumerate /proc threads; all model GPU objects and bound allocation bytes must return to zero at every teardown.',
                'The injected C++ recording exception models a failure after command recording has begun; it is not a Vulkan device callback or validation-layer failure returned by real hardware.',
                'Planned presentation rates use synthetic native timestamps and actual-output inputs; they are not measured game FPS, latency, interpolation quality or PS5 performance.'
            ]}
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'status':status,'checks':len(checks),'runs':len(runs),'report':str(args.report)}))
    return 0 if status=='PASS' else 1


if __name__=='__main__':
    raise SystemExit(main())
