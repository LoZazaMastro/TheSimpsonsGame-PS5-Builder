#!/usr/bin/env python3
"""Host checks of live colour/VSync ownership and actual swap GLSL interfaces.

The production mailbox, guest VSync policy and shader-generation bodies run.
GLSL is compiled and checked by real glslang/SPIRV-Tools; there is no GPU dispatch,
PS5 scanout, console performance measurement or Windows GUI execution here.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, default=root / 'prospero/validation/live-render-settings-v3.10.5-ui-menu.json')
    parser.add_argument('--host-tools', type=Path, default=root.parent / 'host-tools/usr/bin')
    args = parser.parse_args()
    args.report = args.report.resolve()
    args.host_tools = args.host_tools.resolve()
    captures = args.report.with_suffix('').with_name(args.report.stem + '-captures')
    captures.mkdir(parents=True, exist_ok=True)
    sdk = root / 'tools/rexglue-sdk'
    source = sdk / 'src/graphics/vulkan/command_processor.cpp'
    header = sdk / 'include/rex/graphics/vulkan/command_processor.h'
    mailbox = sdk / 'include/rex/platform/settings_prospero.h'
    core = sdk / 'src/core/settings_prospero.cpp'
    worker = sdk / 'src/graphics/graphics_system.cpp'
    processor = sdk / 'src/graphics/command_processor.cpp'
    template = root / 'prospero/tests/ps5_live_graphics_host.cpp.in'
    tracked = [Path(__file__), source, header, mailbox, core, worker, processor, template]
    hashes = {str(p.relative_to(root)): sha(p) for p in tracked}
    runs, checks = [], []

    def run(name, command):
        env = os.environ.copy()
        env.update(ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        result = subprocess.run([str(part) for part in command], text=True, capture_output=True, env=env, timeout=55)
        capture = captures / (name + '.txt')
        capture.write_text(result.stdout + result.stderr)
        runs.append({'name': name, 'command': list(map(str, command)), 'exit_code': result.returncode,
                     'capture': str(capture.relative_to(root)), 'sha256': sha(capture)})
        if result.returncode:
            raise RuntimeError(name + ': ' + result.stdout + result.stderr)
        return result.stdout

    src, hdr, worker_text, processor_text = [p.read_text() for p in (source, header, worker, processor)]
    start = worker_text.index('while (vsync_worker_running_) {') + len('while (vsync_worker_running_) {')
    end = worker_text.index('DeliverDueVblanks();', start) + len('DeliverDueVblanks();')
    vblank = worker_text[start:end]
    start = processor_text.index('#if REX_PLATFORM_PROSPERO', processor_text.index('        PrepareForWait();'))
    end = processor_text.index('        if (!vsync_enabled)', start)
    wait = processor_text[start:end]
    assert 'Acknowledge' not in wait and 'REXCVAR_GET(vsync)' in wait
    assert vblank.index('g_vblank_interval_ticks.store') < vblank.index('Acknowledge') < vblank.index('DeliverDueVblanks')
    generated = captures / 'live-graphics-host.cpp'
    generated.write_text(template.read_text().replace('@@VBLANK_POLICY@@', vblank).replace('@@WAIT_POLICY@@', wait))
    for variant, flags in [('release', ['-O2']), ('sanitized', ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'])]:
        binary = captures / ('live-graphics-' + variant)
        run('compile-' + variant, ['g++', '-std=c++23', '-Wall', '-Wextra', '-Werror', '-DREX_PLATFORM_PROSPERO=1',
                                  '-I' + str(sdk / 'include'), *flags, generated, '-o', binary])
        checks += [variant + ': ' + line for line in run('run-' + variant, [binary]).splitlines() if line.startswith('PASS ')]

    # Execute the exact production GLSL source builders, including both PS5
    # push constants and the unchanged non-PS5 static-colour branch.
    bodies = src[src.index('bool IsSwapColorGradeEnabled()'):src.index('const char* GetSwapApplyGammaTablePixelRbSwapSource()')]
    original = subprocess.check_output(['git', 'show', 'HEAD:tools/rexglue-sdk/src/graphics/vulkan/command_processor.cpp'], cwd=root, text=True)
    old_bodies = original[original.index('bool IsSwapColorGradeEnabled()'):original.index('const char* GetSwapApplyGammaTablePixelRbSwapSource()')]
    constants = hdr[hdr.index('  struct SwapFxaaConstants {'):hdr.index('  // Framebuffer for the current presenter', hdr.index('  struct SwapFxaaConstants {'))]
    prelude = '''#include <cassert>\n#include <cstddef>\n#include <cstdint>\n#include <string>\n#include <string_view>\n#include <fstream>\n#define FMT_HEADER_ONLY\n#include <fmt/format.h>\n#define assert_true(x) assert(x)\n#define REXCVAR_GET(x) x\ndouble color_saturation=1.0, color_vibrance=0.0, color_contrast=1.0, color_brightness=0.0;\n'''
    main_body = '''\nint main(int argc,char** argv) { assert(argc==2); for(int preset=0;preset<2;++preset) { if(preset) { color_saturation=1.15; color_vibrance=.5; color_contrast=1.1; color_brightness=.02; } for(int kind=0;kind<3;++kind) { std::string text=WithSwapColorGrade(kind==0?GetSwapColorGradeComputeSource():GetSwapFxaaComputeSource(kind==2)); std::ofstream out(std::string(argv[1])+"/shader-"+std::to_string(preset)+"-"+std::to_string(kind)+".comp"); out<<text; assert(out.good()); } } }\n'''
    for name, platform, body in [('ps5', 1, bodies), ('pc', 0, bodies), ('pc-before', 0, old_bodies)]:
        emitter = captures / ('emit-' + name + '.cpp')
        emitter.write_text(prelude + constants + body + main_body)
        binary = captures / ('emit-' + name)
        directory = captures / name
        directory.mkdir(exist_ok=True)
        run('compile-emitter-' + name, ['g++', '-std=c++23', '-O2', '-DREX_PLATFORM_PROSPERO=' + str(platform),
                                      '-I' + str(sdk / 'thirdparty/fmt/include'), emitter, '-o', binary])
        run('emit-' + name, [binary, directory])
    for preset in range(2):
        for kind in range(3):
            name = f'shader-{preset}-{kind}.comp'
            assert (captures / 'pc' / name).read_bytes() == (captures / 'pc-before' / name).read_bytes()
            assert (captures / 'ps5' / name).read_bytes() == (captures / 'ps5' / f'shader-0-{kind}.comp').read_bytes()
    checks.append('PASS six non-PS5 generated shaders are byte-identical to HEAD; PS5 shader code is independent of startup colour values')
    modules = []
    for kind in range(3):
        path = captures / 'ps5' / f'shader-0-{kind}.comp'
        module = path.with_suffix('.spv')
        run('glslang-' + str(kind), [args.host_tools / 'glslang', '-V', '--target-env', 'vulkan1.0', '-S', 'comp', path, '-o', module])
        run('spirv-val-' + str(kind), [args.host_tools / 'spirv-val', '--target-env', 'vulkan1.0', module])
        disassembly = run('spirv-dis-' + str(kind), [args.host_tools / 'spirv-dis', module])
        assert 'OpMemberDecorate %XeApplyGammaRampConstants 2 Offset 16' in disassembly
        assert 'OpTypePointer PushConstant %v4float' in disassembly
        assert 'OpVariable %_ptr_PushConstant_XeApplyGammaRampConstants PushConstant' in disassembly
        modules.append({'path': str(module.relative_to(root)), 'sha256': sha(module), 'push_constant_bytes': 32})
    checks.append('PASS grading-only, FXAA and extreme FXAA compile and validate for Vulkan 1.0 with vec4 push constants at offset 16 in the actual 32-byte C++ layout')
    assert 'color_request.WriteConstants(swap_fxaa_constants.color_grade)' in src
    assert 'color_request.Complete(rex::platform::prospero::GetLiveSettings(),\n                               use_compute_gamma && use_fxaa, submitted)' in src
    assert 'color_request.NeedsPass() && swap_color_grade_pipeline_ != VK_NULL_HANDLE' in src
    assert 'const bool create_color_grade_pipeline = true;' in src
    assert all(sha(root / name) == value for name, value in hashes.items())
    report = {'status': 'PASS', 'scope': __doc__, 'hardware_validated': False,
              'checks': checks, 'source_sha256': hashes, 'runs': runs, 'shader_modules': modules,
              'limits': ['No GPU image dispatch or PS5 performance measurement; shaders are compiled and validated offline.',
                         'VSync is the existing guest vblank/wait policy, not output refresh or swapchain presentation mode.',
                         'Host fixture executes the real mailbox and extracted worker/read policy; kernel scheduling and full command-processor integration are not emulated here.']}
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'status': 'PASS', 'checks': len(checks), 'report': str(args.report)}, indent=2))


if __name__ == '__main__':
    main()
