#!/usr/bin/env python3
"""Exercise real PS5 display-policy code with deterministic host API doubles.

No PS5, Vulkan driver, GPU, monitor or physical refresh measurement runs here.
The WSI function bodies are extracted without edits; display.cpp is included
whole. The real SDK VideoOut declarations and real Vulkan types are retained.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def function(source: str, name: str) -> tuple[str, int]:
    pattern = re.compile(r'^(?:static\s+)?(?:bool|void|uint64_t|uint32_t)\s*\n' + re.escape(name) + r'\([^;]*?\)\s*\{', re.M)
    match = pattern.search(source)
    if match is None:
        raise RuntimeError('Function body not found: ' + name)
    opening = source.index('{', match.start())
    depth = 0
    token = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*[\s\S]*?\*/|//[^\n]*|[{}]')
    for piece in token.finditer(source, opening):
        if piece.group() == '{': depth += 1
        elif piece.group() == '}':
            depth -= 1
            if depth == 0:
                return source[match.start():piece.end()], source.count('\n', 0, match.start()) + 1
    raise RuntimeError('Unclosed function: ' + name)


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, default=root.parent / 'ps5-sdk-current')
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--captures', type=Path, default=root / 'prospero/validation/display-v3.10-captures')
    parser.add_argument('--report', type=Path, default=root / 'prospero/validation/display-v3.10.json')
    args = parser.parse_args()
    captures = args.captures.resolve(); captures.mkdir(parents=True, exist_ok=True)
    sdk = root / 'tools/rexglue-sdk'
    wsi = root / 'prospero/radv/wsi_common_videoout.c'
    display = sdk / 'src/ui/vulkan/vulkan_display_prospero.cpp'
    instance = sdk / 'src/ui/vulkan/vulkan_instance.cpp'
    abi = args.sdk.resolve() / 'target/include/ps5platform/videoout.h'
    templates = [root / 'prospero/tests/display_wsi_v310_host.c.in', root / 'prospero/tests/display_vulkan_v310_host.cpp.in']
    tracked = [Path(__file__).resolve(), wsi, wsi.with_name('wsi_common_videoout.base.c'), display, instance,
               sdk / 'include/rex/ui/vulkan/display_prospero.h', sdk / 'include/rex/ui/vulkan/functions/instance_khr_surface.inc',
               root / 'prospero/tools/build_radv_display.py', *templates]
    initial = {str(p.relative_to(root)): sha(p) for p in tracked}
    sources = {'repository': initial, 'sdk_videoout_header': {'path': str(abi), 'sha256': sha(abi)}}
    runs: list[dict] = []; checks: list[str] = []
    status, error = 'PASS', None

    def run(name: str, command: list[str], timeout: int = 45) -> str:
        env = os.environ.copy()
        sanitizer_environment = {}
        if 'sanitized' in name:
            # This host cannot enumerate /proc/<pid>/task. LeakSanitizer cannot
            # run here; retain ASan bounds/lifetime and UBSan checks explicitly.
            sanitizer_environment = {'ASAN_OPTIONS': 'detect_leaks=0', 'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}
            env.update(sanitizer_environment)
        result = subprocess.run(command, capture_output=True, text=True, timeout=timeout, env=env)
        capture = captures / (name + '.txt')
        capture.write_text(result.stdout + result.stderr)
        runs.append({'name': name, 'command': command, 'exit_code': result.returncode, 'sanitizer_environment': sanitizer_environment,
                     'capture': str(capture.relative_to(root)) if capture.is_relative_to(root) else str(capture),
                     'capture_sha256': sha(capture)})
        if result.returncode != 0:
            raise RuntimeError(name + ' failed: ' + result.stdout + result.stderr)
        return result.stdout

    try:
        source = wsi.read_text()
        names = ['videoout_output_init_once', 'videoout_title_declares_high_frame_rate',
                 'videoout_vblank_period_ns', 'videoout_open_locked', 'videoout_settle_modes',
                 'simpsons_radv_videoout_refresh_millihertz']
        extracted = []
        fragments = []
        for name in names:
            body, line = function(source, name)
            extracted.append({'name': name, 'line': line, 'sha256': hashlib.sha256(body.encode()).hexdigest()})
            fragments.append(f'#line {line} "{wsi}"\n{body}\n')
        constants = '\n'.join(re.findall(r'^#define VIDEOOUT_[^\n]*', source, re.M))
        start = source.index('struct wsi_videoout_swapchain;')
        end = source.index('static void\nvideoout_output_init_once', start)
        state = source[start:end]
        wsi_fixture = templates[0].read_text().replace('@CONSTANTS@', constants).replace('@STATE@', state).replace('@FUNCTIONS@', '\n'.join(fragments))
        (captures / 'wsi_host.c').write_text(wsi_fixture)
        (captures / 'videoout_abi.h').write_bytes(abi.read_bytes())

        stubs = captures / 'stubs'
        files = {
          'rex/ui/vulkan/instance.h': '''#pragma once
#include <vulkan/vulkan_core.h>
namespace rex::ui::vulkan {
struct FixtureFunctions {
 PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR;
 PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR;
 PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR;
 PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR;
 PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR;
 PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR;
 PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
};
struct FixtureExtensions { bool ext_KHR_display = true; };
class VulkanInstance {
public:
 FixtureFunctions fn{};
 FixtureExtensions ext{};
 VkInstance instance() const { return reinterpret_cast<VkInstance>(1); }
 const FixtureFunctions& functions() const { return fn; }
 const FixtureExtensions& extensions() const { return ext; }
};
}
''',
          'rex/cvar.h': '#pragma once\n#define REXCVAR_DEFINE_UINT32(name, value, category, description) uint32_t mock_##name = value\n#define REXCVAR_GET(name) mock_##name\n',
          'rex/logging.h': '#pragma once\n' + '\n'.join(f'#define {macro}(...) fixture_log("{macro}", __VA_ARGS__)' for macro in ['REXLOG_WARN', 'REXLOG_ERROR', 'REXLOG_INFO', 'REXGPU_ERROR', 'REXGPU_INFO']) + '\n',
          'rex/ui/surface_prospero.h': '#pragma once\nnamespace rex::ui { struct ProsperoDisplaySurface { static constexpr uint32_t kWidth = 3840, kHeight = 2160; }; }\n'
        }
        for relative, content in files.items():
            path = stubs / relative; path.parent.mkdir(parents=True, exist_ok=True); path.write_text(content)
        (captures / 'production_display.cpp').write_bytes(display.read_bytes())
        (captures / 'vulkan_host.cpp').write_bytes(templates[1].read_bytes())
        include = '-I' + str(sdk / 'thirdparty/vulkan-headers/include')
        cases = ['default-no-request', 'explicit-60', 'invalid-request', 'unsupported-zero', 'support-query-failed',
                 'configure-refused', 'measured-120', 'median-rejects-outlier', 'actually-60',
                 'first-wait-failed', 'later-wait-failed', 'zero-period', 'restore-failed', 'open-failed']
        for variant, flags in [('release', ['-O2']), ('sanitized', ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'])]:
            c_exe = captures / ('wsi-' + variant)
            cpp_exe = captures / ('vulkan-' + variant)
            run('compile-wsi-' + variant, [args.cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-pthread', *flags, include,
                                          str(captures / 'wsi_host.c'), '-o', str(c_exe)])
            run('compile-vulkan-' + variant, [args.cxx, '-std=c++23', '-Wall', '-Wextra', '-Werror', '-pthread', *flags, include,
                     '-I' + str(stubs), '-I' + str(sdk / 'include'), str(captures / 'vulkan_host.cpp'), '-Wl,--wrap=setenv', '-o', str(cpp_exe)])
            for case in cases:
                text = run('wsi-' + variant + '-' + case, [str(c_exe), case])
                checks.extend(variant + ': ' + line for line in text.splitlines() if line.startswith('PASS '))
            text = run('vulkan-' + variant, [str(cpp_exe)])
            checks.extend(variant + ': ' + line for line in text.splitlines() if line.startswith('PASS '))

        instance_source = instance.read_text()
        creation = instance_source.index('std::unique_ptr<VulkanInstance> VulkanInstance::Create')
        policy = instance_source.index('InitializeProsperoDisplayPolicy()', creation)
        allocation = instance_source.index('new VulkanInstance()', creation)
        if not creation < policy < allocation:
            raise RuntimeError('The output request is not set before Vulkan instance/driver creation')
        display_source = display.read_text()
        cvars = re.findall(r'REXCVAR_GET\((\w+)\)', display_source)
        if cvars != ['prospero_output_hz']:
            raise RuntimeError('Display policy unexpectedly reads a guest/native FPS control: ' + repr(cvars))
        checks += ['PASS static integration: output request initialized before VulkanInstance allocation',
                   'PASS static separation: production display policy reads only prospero_output_hz, never native game/menu FPS']
        if any(sha(p) != initial[str(p.relative_to(root))] for p in tracked) or sha(abi) != sources['sdk_videoout_header']['sha256']:
            raise RuntimeError('A tested source changed during this run; rerun after freeze')
    except Exception as exception:
        status, error = 'FAIL', str(exception)
        print(error, file=sys.stderr)

    report = {'status': status, 'scope': __doc__, 'hardware_validated': False,
              'source_sha256': sources, 'extracted_functions': locals().get('extracted', []),
              'runs': runs, 'checks': checks, 'error': error,
              'limits': ['VideoOut and Vulkan calls are deterministic host doubles; refresh intervals are supplied fixtures, not measurements.',
                         'The SDK declarations and Vulkan header types are used, but these tests do not validate the native library implementation or PS5 ABI execution.',
                         'LeakSanitizer is disabled because this host denies its /proc/<pid>/task enumeration. ASan address/lifetime and UBSan checks remain enabled in the sanitized variant.',
                         'Native target compilation and RADV archive byte-comparison are separate build receipts.']}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'status': status, 'checks': len(checks), 'runs': len(runs), 'report': str(args.report)}))
    return 0 if status == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
