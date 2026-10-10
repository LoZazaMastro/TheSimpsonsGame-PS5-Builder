#!/usr/bin/env python3
"""Host tests of the production PE parser, native SPIR-V adopter and validator.

Synthetic, redistributable PE/SPIR-V fixtures only. Actual SPIRV-Tools assembly
and Vulkan validation run. Device properties and module creation are doubles;
no proprietary Lossless.dll, GPU, PS5 ABI or visual output is exercised.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def namespace(path: Path) -> tuple[str, int]:
    text = path.read_text()
    offset = text.index('namespace rex::ui::vulkan::framegen {')
    return text[offset:], text.count('\n', 0, offset) + 1


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host-tools', type=Path, default=root.parent / 'host-tools')
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--captures', type=Path, default=root / 'prospero/validation/framegen-shaders-v3.10-captures')
    parser.add_argument('--report', type=Path, default=root / 'prospero/validation/framegen-shaders-v3.10.json')
    args = parser.parse_args()
    sdk = root / 'tools/rexglue-sdk'
    source = sdk / 'src/ui/vulkan/framegen'
    template = root / 'prospero/tests/framegen_shader_host.cpp.in'
    captures = args.captures.resolve(); captures.mkdir(parents=True, exist_ok=True)
    library = args.host_tools.resolve() / 'usr/lib/x86_64-linux-gnu/libSPIRV-Tools.a'
    spv_header = args.host_tools.resolve() / 'usr/include/spirv-tools/libspirv.h'
    tracked = [Path(__file__).resolve(), template, *[source / name for name in [
        'lossless_dll.cpp', 'lossless_dll.h', 'lsfg_translate.cpp', 'lsfg_translate.h',
        'lsfg_shaders.cpp', 'lsfg_shaders.h', 'types.h']],
        sdk / 'thirdparty/spirv-headers/include/spirv/unified1/spirv.h',
        sdk / 'thirdparty/vulkan-headers/include/vulkan/vulkan_core.h']
    initial = {str(path.relative_to(root)): sha(path) for path in tracked}
    externals = {str(path): sha(path) for path in [library, spv_header]}
    runs, checks, extracted = [], [], []
    status, error = 'PASS', None

    def run(name: str, command: list[str], timeout: int = 55) -> str:
        environment = os.environ.copy()
        sanitizer_environment = {}
        if 'sanitized' in name:
            sanitizer_environment = {'ASAN_OPTIONS': 'detect_leaks=0', 'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
            environment.update(sanitizer_environment)
        result = subprocess.run(command, text=True, capture_output=True, timeout=timeout, env=environment)
        capture = captures / (name + '.txt'); capture.write_text(result.stdout + result.stderr)
        runs.append({'name': name, 'command': command, 'exit_code': result.returncode,
                     'capture': str(capture.relative_to(root)) if capture.is_relative_to(root) else str(capture),
                     'capture_sha256': sha(capture), 'sanitizer_environment': sanitizer_environment})
        if result.returncode != 0:
            raise RuntimeError(name + ' failed: ' + result.stdout + result.stderr)
        return result.stdout

    try:
        rendered = template.read_text()
        for token, path in [('SHADER_HEADER_NAMESPACE', source / 'lsfg_shaders.h'),
                            ('SHADER_CPP_NAMESPACE', source / 'lsfg_shaders.cpp')]:
            body, line = namespace(path)
            rendered = rendered.replace('@' + token + '@', f'#line {line} "{path}"\n' + body + '\n#line 1 "fixture"\n')
            extracted.append({'source': str(path.relative_to(root)), 'start_line': line,
                              'range': 'entire namespace through end of file, byte-identical',
                              'sha256': hashlib.sha256(body.encode()).hexdigest()})
        generated = captures / 'framegen-shaders-host.cpp'; generated.write_text(rendered)
        includes = ['-I' + str(source), '-I' + str(args.host_tools.resolve() / 'usr/include'),
                    '-I' + str(sdk / 'thirdparty/spirv-headers/include'),
                    '-I' + str(sdk / 'thirdparty/vulkan-headers/include')]
        wraps = ['-Wl,--wrap=' + name for name in ['open', 'fstat', 'read', 'close']]
        for variant, flags in [('release', ['-O2']), ('sanitized', ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'])]:
            executable = captures / ('framegen-shaders-' + variant)
            run('compile-' + variant, [args.cxx, '-std=c++23', '-Wall', '-Wextra', '-Werror', '-Wno-missing-field-initializers',
                '-pthread', *flags, *includes, str(generated), str(source / 'lossless_dll.cpp'), str(source / 'lsfg_translate.cpp'),
                str(library), *wraps, '-o', str(executable)])
            output = run('runtime-' + variant, [str(executable), str(captures / ('fixtures-' + variant))])
            checks.extend(variant + ': ' + line for line in output.splitlines() if line.startswith('PASS '))
        if any(sha(path) != initial[str(path.relative_to(root))] for path in tracked):
            raise RuntimeError('A tested source changed during the run; rerun after freeze')
        if any(sha(Path(path)) != digest for path, digest in externals.items()):
            raise RuntimeError('SPIRV-Tools changed during the run')
    except Exception as exception:
        status, error = 'FAIL', str(exception)
        print(error, file=sys.stderr)
    report = {'status': status, 'scope': __doc__, 'hardware_validated': False,
              'source_sha256': initial, 'external_sha256': externals, 'extracted_namespaces': extracted,
              'runs': runs, 'checks': checks, 'error': error,
              'limits': ['The actual lossless_dll.cpp and lsfg_translate.cpp are compiled unchanged. All validator helpers, ValidateShaderModule and the complete LsfgShaders methods are extracted unchanged from the production namespace.',
                         'Vulkan Device property access and CreateWrappedShaderModule are host doubles. The real SPIRV-Tools library assembles and validates every shader fixture.',
                         'POSIX I/O errors and short reads are injected through link wrappers around the real parser; normal filesystem, directory, symlink and FIFO cases use host syscalls. Native PS5 syscall ABI is not exercised.',
                         'LeakSanitizer is disabled because this host denies /proc thread enumeration; ASan address/lifetime and UBSan remain active for fixture and production sources. The prebuilt SPIRV-Tools static library is not instrumented.',
                         'No actual proprietary DLL, generated frame image, GPU dispatch, real shader compatibility or PS5 performance claim follows from these synthetic fixtures.']}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'status': status, 'checks': len(checks), 'runs': len(runs), 'report': str(args.report)}))
    return 0 if status == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
