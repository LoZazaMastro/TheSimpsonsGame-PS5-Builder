#!/usr/bin/env python3
"""Exercise the v3.10 upstream merge at explicit host boundaries.

The real layout classifier, GLSL generators/system constants, SharedMemory and
Vulkan mirror allocation/use functions are compiled. Controlled device calls
record lifecycle/barrier decisions; they do not execute a GPU or PS5 firmware.
All reports/captures use new v3.10 paths. Historical reports are not modified.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import time

from run_shared_memory_host import STUBS
from run_shared_memory_barriers_host import FIXTURE as BARRIER_FIXTURE


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def method(source, signature):
    start = source.index(signature)
    # Used only for C++ methods with no raw shader source literals.
    return source[start:source.index('\n}\n', start) + 3]


def between(source, start, end):
    first = source.index(start)
    return source[first:source.index(end, first)]


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='clang++')
    parser.add_argument('--glslang', default='glslangValidator')
    parser.add_argument('--spirv-val', default='spirv-val')
    parser.add_argument('--report', type=Path,
                        default=root / 'prospero/validation/upstream-runtime-v3.10.json')
    args = parser.parse_args()
    args.report = args.report.resolve()
    capture = args.report.with_suffix('')
    capture.mkdir(parents=True, exist_ok=True)
    sdk = root / 'tools/rexglue-sdk'
    source_paths = [
        'src/graphics/vulkan/shared_memory.cpp',
        'src/graphics/vulkan/texture_cache.cpp',
        'include/rex/graphics/vulkan/texture_cache.h',
        'src/graphics/shared_memory.cpp', 'include/rex/graphics/shared_memory.h',
        'src/graphics/vulkan/pipeline_cache.cpp',
        'src/graphics/vulkan/command_processor.cpp',
        'src/graphics/pipeline/shader/smooth_tessellation.cpp',
        'include/rex/graphics/pipeline/shader/smooth_tessellation.h',
        'include/rex/graphics/pipeline/shader/spirv_translator.h',
    ]
    test_paths = [Path(__file__).resolve(),
                  root / 'prospero/tests/upstream_mirror_v310_host.cpp.in',
                  root / 'prospero/tests/upstream_shader_v310_host.cpp.in',
                  root / 'prospero/tests/shared_memory_host.cpp',
                  root / 'prospero/tests/run_shared_memory_host.py',
                  root / 'prospero/tests/run_shared_memory_barriers_host.py',
                  root / 'prospero/tests/run_pipeline_cache_host.py',
                  root / 'tools/bench/smooth_tessellation_layout_check.cpp',
                  root / 'tools/bench/copy_free_resolve_check.py',
                  root / 'tools/bench/native_gpu_check.py',
                  root / 'tools/native-renderer/check_translated_set.py',
                  root / 'tools/native-renderer/translated_shaders.tar.xz']
    paths = [sdk / path for path in source_paths] + test_paths
    hashes = {str(path.relative_to(root)): digest(path) for path in paths}
    report = {'result': 'FAIL', 'scope': __doc__, 'source_and_test_sha256': hashes,
              'sanitizers': {'address': True, 'undefined': True, 'leak': False,
                             'leak_reason': 'LSan cannot enumerate /proc/<pid>/task in this host environment; the failed probe is retained separately.'},
              'executions': [], 'generated': [], 'limits': [
                  'Controlled Vulkan calls model success/failure and resource ownership; no GPU commands run.',
                  'SharedMemory uses the real class, bitmap/watch/streaming implementation and compiler atomics, with a 2 MiB guest-memory fixture at 4/16 KiB host pages.',
                  'Shader modules compile with real glslang and validate with SPIRV-Tools; no image-quality or frame-rate claim is made.',
                  'The pipeline tessellation guard is extracted from production; the v3.9 publication regression is a separate report.',
                  'GPU trace A/B replay and actual PS5 gameplay are NOT_RUN for this revision here.',
              ]}

    def run(label, command, *, env=None, timeout=60):
        started = time.monotonic()
        result = subprocess.run(command, cwd=root, env=env, capture_output=True,
                                text=True, timeout=timeout)
        output = capture / (label + '.txt')
        output.write_text('COMMAND ' + shlex.join(map(str, command)) + '\n' +
                          result.stdout + result.stderr + '\nEXIT ' + str(result.returncode) + '\n')
        report['executions'].append({'label': label, 'exit_code': result.returncode,
                                     'elapsed_seconds': round(time.monotonic() - started, 3),
                                     'capture': str(output.relative_to(root)),
                                     'capture_sha256': digest(output)})
        print(label, 'PASS' if result.returncode == 0 else 'FAIL', flush=True)
        if result.returncode:
            raise RuntimeError(f'{label} failed: {output}\n{result.stdout}{result.stderr}')
        return result.stdout

    def generated(name, data):
        path = capture / name
        path.write_text(data)
        report['generated'].append({'path': str(path.relative_to(root)), 'sha256': digest(path)})
        return path

    includes = []
    for directory in ('include', 'thirdparty/fmt/include', 'thirdparty/vulkan-headers/include',
                      'thirdparty/glslang', 'thirdparty/spirv-headers/include',
                      'thirdparty/xxHash', 'thirdparty/simde', 'thirdparty/volk',
                      'thirdparty/renderdoc'):
        includes += ['-I', str(sdk / directory)]
    flags = [args.cxx, '-std=c++23', '-O1', '-g', '-DNDEBUG', '-DFMT_HEADER_ONLY',
             '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-pthread']
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    try:
        run('compiler-version', [args.cxx, '--version'])
        with tempfile.TemporaryDirectory(prefix='simpsons-upstream-v310-') as directory:
            temporary = Path(directory)
            layout = temporary / 'layout'
            run('layout-compile', flags + includes + [
                str(root / 'tools/bench/smooth_tessellation_layout_check.cpp'),
                str(sdk / 'src/graphics/pipeline/shader/smooth_tessellation.cpp'), '-o', str(layout)])
            report['layout_case_output'] = run('layout-execute', [str(layout)], env=env)

            shared = (sdk / 'src/graphics/vulkan/shared_memory.cpp').read_text()
            texture = (sdk / 'src/graphics/vulkan/texture_cache.cpp').read_text()
            texture_header = (sdk / 'include/rex/graphics/vulkan/texture_cache.h').read_text()
            selected = '\n'.join(method(shared, signature) for signature in (
                'bool VulkanSharedMemory::Initialize(', 'bool VulkanSharedMemory::EnsureHostBuffer(',
                'bool VulkanSharedMemory::CreateBuffer(', 'void VulkanSharedMemory::DestroyBuffer(',
                'void VulkanSharedMemory::Shutdown(')) + '\n' + '\n'.join(
                    method(texture, signature) for signature in (
                        'bool VulkanTextureCache::InitializeScaledResolveBuffer(',
                        'void VulkanTextureCache::ShutdownScaledResolveBuffer('))
            declaration = re.search(r'  bool InitializeScaledResolveBuffer\([^;]+;',
                                    texture_header).group(0).strip()
            mirror_fixture = test_paths[1].read_text().replace('@METHODS@', selected).replace(
                '@SCALED_DECLARATION@', declaration)
            mirror_source = generated('mirror-generated.cpp', mirror_fixture)
            for platform in (0, 1):
                binary = temporary / f'mirror-{platform}'
                run(f'mirror-platform{platform}-compile', flags + includes + [
                    '-DREX_PLATFORM_PROSPERO=' + str(platform), str(mirror_source), '-o', str(binary)])
                run(f'mirror-platform{platform}-execute', [str(binary)], env=env)

            # Keep the existing RAW/WAW assertions, compiling today's real Use
            # and GetUsageMasks, with explicit new diagnostic/allocation sinks.
            barrier = BARRIER_FIXTURE
            diagnostics = '''
bool flag_shared_memory_census = false;
unsigned address_lookups = 0, census_lookups = 0;
#define REXCVAR_GET(name) flag_##name
struct NativeGpuDiagnostics {
  enum class Dependency { kMemoryMirror };
  bool active = false;
  bool enabled() const { return active; }
  void RecordDependency(Dependency, uint64_t, const std::string&) {}
};
'''
            barrier = barrier.replace('#include <vulkan/vulkan.h>',
                                      '#include <vulkan/vulkan.h>\n#include <string>\n#include <fmt/format.h>\n' + diagnostics)
            barrier = barrier.replace('struct Commands {', '''struct Commands {
  NativeGpuDiagnostics diagnostics;
  NativeGpuDiagnostics& native_gpu_diagnostics() { return diagnostics; }
''')
            barrier = barrier.replace('  bool FlushGpuWrittenRange', '''
  bool host_available = true;
  bool EnsureHostBuffer() { return host_available; }
  std::string DescribeCodeAddress(const void*) { ++address_lookups; return "fixture"; }
  void CensusRecord(const void*, uint32_t, uint64_t) {}
  const void*& CensusTrigger() { ++census_lookups; static const void* value = nullptr; return value; }
  bool FlushGpuWrittenRange''')
            barrier_methods = '\n'.join(method(shared, signature) for signature in (
                'void VulkanSharedMemory::Use(', 'void VulkanSharedMemory::GetUsageMasks('))
            barrier = barrier.replace('#include "production.inc"', barrier_methods)
            barrier = barrier.replace('  return failures ? 1 : 0;', '''
  Check(address_lookups == 0 && census_lookups == 0,
        "disabled diagnostics skip formatting/address lookup and census TLS");
  n = memory.command_processor_.barriers.size();
  memory.host_available = false;
  memory.Use(Usage::kRead);
  Check(memory.command_processor_.barriers.size() == n, "no buffer barrier after failed allocation");
  memory.host_available = true;
  memory.command_processor_.diagnostics.active = true;
  flag_shared_memory_census = true;
  memory.Use(Usage::kRead);
  Check(address_lookups == 1 && census_lookups != 0, "enabled diagnostics retain observation path");
  return failures ? 1 : 0;''')
            barrier_source = generated('barriers-generated.cpp', barrier)
            binary = temporary / 'barriers'
            run('barriers-compile', flags + includes + [str(barrier_source), '-o', str(binary)])
            run('barriers-execute', [str(binary)], env=env)

            # The full shared-memory fixture compiles the real implementation,
            # also proving its new EnsureHostBuffer failure reaches the caller.
            stubs = temporary / 'stubs'
            for name, data in STUBS.items():
                if name == 'rex/logging.h':
                    data += '\n#define REXGPU_INFO(...) ((void)0)\n'
                target = stubs / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(data)
            shared_fixture = test_paths[3].read_text().replace(
                '  std::vector<uint8_t> mirror;', '''
  bool host_available = true;
  bool EnsureHostBuffer() override { return host_available; }
  std::vector<uint8_t> mirror;''').replace(
                '  std::fill(memory.bytes.begin()', '''
  shared.host_available = false;
  Require(!shared.RequestRange(128, 64) && shared.upload_calls == 0,
          "unavailable host buffer rejects upload before reading guest data");
  shared.host_available = true;
  std::fill(memory.bytes.begin()''').replace('int main(int argc, char** argv)',
                                          'int original_main(int argc, char** argv)')
            shared_fixture += '''
extern bool& FLAGS_shared_memory_census_storage_();
int main(int argc, char** argv) {
  if (argc != 4) return 2;
  FLAGS_shared_memory_census_storage_() = std::strcmp(argv[3], "on") == 0;
  return original_main(argc - 1, argv);
}
'''
            shared_source = generated('shared-memory-generated.cpp', shared_fixture)
            binary = temporary / 'shared-memory'
            run('shared-memory-compile', flags + ['-D__PROSPERO__=1', '-I', str(stubs)] +
                includes + [str(shared_source), str(sdk / 'src/graphics/shared_memory.cpp'),
                            str(sdk / 'src/core/math_gcc.cpp'), '-o', str(binary)])
            for page in (4096, 16384):
                for census in ('off', 'on'):
                    run(f'shared-memory-{page}-{census}', [str(binary), str(page), 'functional', census], env=env)
            # Atomics are retained from v3.9. This repeats the concurrent workload
            # under ASan/UBSan only and does not claim to replace the TSAN report.
            run('shared-memory-concurrent-16384', [str(binary), '16384', 'concurrent', 'off'], env=env)

            cache = (sdk / 'src/graphics/vulkan/pipeline_cache.cpp').read_text()
            cp = (sdk / 'src/graphics/vulkan/command_processor.cpp').read_text()
            helpers = between(cache, '// Smooth tessellation:', '\n}  // namespace\n\nVulkanPipelineCache::')
            colors = between(cp, 'bool IsSwapColorGradeEnabled()',
                             'const char* GetSwapApplyGammaTablePixelRbSwapSource()')
            shader_source = generated('shader-generated.cpp', test_paths[2].read_text().replace(
                '@TESS_HELPERS@', helpers).replace('@COLOR_HELPERS@', colors))
            binary = temporary / 'shader-generator'
            run('shader-generator-compile', flags + ['-D__PROSPERO__=1'] + includes +
                [str(shader_source), '-o', str(binary)])
            shaders = capture / 'shaders'
            run('shader-generator-execute', [str(binary), str(shaders)], env=env)
            shader_paths = sorted(path for path in shaders.iterdir() if path.suffix in ('.comp', '.tesc', '.tese'))
            if len(shader_paths) != 14:
                raise RuntimeError('expected exactly 14 generated shader variants')
            for path in shader_paths:
                spv = path.with_suffix(path.suffix + '.spv')
                run('shader-compile-' + path.name,
                    [args.glslang, '-V', '--target-env', 'vulkan1.1', str(path), '-o', str(spv)])
                run('shader-validate-' + path.name, [args.spirv_val, '--target-env', 'vulkan1.1', str(spv)])
                report['generated'].append({'path': str(path.relative_to(root)), 'sha256': digest(path),
                                            'spirv_path': str(spv.relative_to(root)), 'spirv_sha256': digest(spv)})

            ensure = method(cache, 'bool VulkanPipelineCache::EnsurePipelineCreated(')
            start = ensure.index('    if (creation_arguments.tessellation_control_shader == VK_NULL_HANDLE',
                                 ensure.index('  if (smooth_tessellation)'))
            guard = ensure[start:ensure.index('\n    const VkShaderStageFlagBits stages[]', start)]
            guard_source = generated('tessellation-guard-generated.cpp', '''
#include <cstdint>
#include <cstdio>
#include <cstdlib>
constexpr uintptr_t VK_NULL_HANDLE = 0;
struct Arguments { uintptr_t tessellation_control_shader, tessellation_evaluation_shader;
                   uint32_t tessellation_patch_control_points; };
unsigned terminal_failures = 0;
bool Validate(const Arguments& creation_arguments) {
  auto fail = [] { ++terminal_failures; return false; };
''' + guard + '''
  return true;
}
int main() {
  const Arguments invalid[] = {{0,1,3},{1,0,3},{1,1,0},{1,1,2},{1,1,4}};
  for (const auto& args : invalid) if (Validate(args)) return 1;
  if (terminal_failures != 5 || !Validate({1,1,3})) return 2;
  std::puts("PASS tessellation: five invalid inputs use terminal completion, valid input proceeds");
  return 0;
}
''')
            binary = temporary / 'tess-guard'
            run('tessellation-guard-compile', flags + [str(guard_source), '-o', str(binary)])
            run('tessellation-guard-execute', [str(binary)], env=env)

        run('translated-set-hash', ['python3', str(root / 'tools/native-renderer/check_translated_set.py')])
        for path in (root / 'tools/bench/copy_free_resolve_check.py', root / 'tools/bench/native_gpu_check.py'):
            compile(path.read_text(), str(path), 'exec')
        report['bench_python_syntax'] = 'PASS; trace replay not run'
        report['sources_unchanged_during_test'] = all(digest(root / path) == value for path, value in hashes.items())
        if not report['sources_unchanged_during_test']:
            raise RuntimeError('a reviewed input changed during the run; rerun after edits finish')
        report['result'] = 'PASS'
    finally:
        args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
