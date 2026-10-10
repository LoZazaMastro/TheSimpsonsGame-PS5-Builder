#!/usr/bin/env python3
"""Exercise the actual device-feature selection blocks with Vulkan header types.

This runs host C++ policy fixtures, not vkCreateDevice, a GPU, or the PS5 ABI.
The production macros and feature blocks are copied without modifications.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def extract(source: str) -> tuple[str, str, str]:
    names = ['XE_UI_VULKAN_FEATURE', 'XE_UI_VULKAN_FEATURE_2', 'XE_UI_VULKAN_FEATURE_IMPLIED']
    macros = []
    for name in names:
        match = re.search(r'^#define ' + name + r'\(', source, re.M)
        if match is None:
            raise ValueError('Missing production macro: ' + name)
        lines = source[match.start():].splitlines(keepends=True)
        count = 1
        while lines[count - 1].rstrip('\r\n').endswith('\\'):
            count += 1
        macros.append(''.join(lines[:count]))
    declaration = re.search(r'  const bool frame_gen_(?:requested|device_features) =', source)
    if declaration is None:
        raise ValueError('Missing production framegen feature gate')
    start = source.rfind('#if REX_PLATFORM_PROSPERO', 0, declaration.start())
    end = source.index('  if (properties.apiVersion >= VK_MAKE_API_VERSION(0, 1, 3, 0))', start)
    robustness = source.index('  if (device->extensions_.ext_EXT_robustness2) {', end)
    robustness_end = source.index('\n\n', robustness)
    create = source.index('ifn.vkCreateDevice(', robustness_end)
    if not 0 <= start < end < robustness < robustness_end < create:
        raise ValueError('Feature selection must precede vkCreateDevice')
    return '\n'.join(macros), source[start:end], source[robustness:robustness_end]


FIXTURE = r'''
#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#define REXLOG_INFO(...) ((void)0)
namespace rex::platform::prospero {
bool dll_present = false;
bool startup_enabled = false;
unsigned dll_queries = 0;
struct Options { bool enabled; };
Options ReadFrameGenerationOptions() { return {startup_enabled}; }
bool IsFrameGenerationDllPresent() { ++dll_queries; return dll_present; }
}
struct StoredProperties : VkPhysicalDeviceFeatures, VkPhysicalDeviceVulkan12Features {
  VkBool32 nullDescriptor = VK_FALSE;
};
struct Device {
  StoredProperties properties_{};
  struct { bool ext_EXT_robustness2 = false; } extensions_;
};
@MACROS@
static void Check(bool condition, const char* message) {
  if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::exit(1); }
}
static unsigned Run(bool with_swapchain, bool with_gpu_emulation, bool dll, bool startup,
                    uint32_t api, unsigned support, bool null_extension, bool null_support) {
  namespace fp = rex::platform::prospero;
  fp::dll_present = dll; fp::startup_enabled = startup; fp::dll_queries = 0;
  Device storage{}; Device* device = &storage;
  storage.extensions_.ext_EXT_robustness2 = null_extension;
  [[maybe_unused]] VkPhysicalDeviceFeatures supported_features{}, enabled_features{};
  supported_features.shaderStorageImageExtendedFormats = bool(support & 1);
  supported_features.shaderStorageImageReadWithoutFormat = bool(support & 2);
  supported_features.shaderStorageImageWriteWithoutFormat = bool(support & 4);
  struct { VkPhysicalDeviceVulkan12Features supported{}, enabled{}; } features_1_2;
  features_1_2.supported.vulkanMemoryModel = bool(support & 8);
  features_1_2.supported.vulkanMemoryModelDeviceScope = bool(support & 16);
  features_1_2.supported.vulkanMemoryModelAvailabilityVisibilityChains = bool(support & 32);
  features_1_2.supported.shaderFloat16 = bool(support & 64);
  struct { VkPhysicalDeviceRobustness2FeaturesEXT supported{}, enabled{}; } features_EXT_robustness2;
  features_EXT_robustness2.supported.nullDescriptor = null_support;
  struct { uint32_t apiVersion; } properties{api};
  const bool ext_1_2_KHR_sampler_mirror_clamp_to_edge = false;
@POLICY@
@ROBUSTNESS@
  // Compare enabled values intended for vkCreateDevice with a requirements
  // matrix. Unsupported features may never become enabled; scope/chains
  // additionally depend on VulkanMemoryModel.
  const bool available = bool(REX_PLATFORM_PROSPERO) && with_swapchain && dll;
  const bool extended = available && api >= VK_API_VERSION_1_2;
  const unsigned expected = (available ? support & 7 : 0) |
      (extended ? support & (8 | 64) : 0) |
      (extended && (support & 8) ? support & (16 | 32) : 0);
  const unsigned actual =
      (enabled_features.shaderStorageImageExtendedFormats ? 1 : 0) |
      (enabled_features.shaderStorageImageReadWithoutFormat ? 2 : 0) |
      (enabled_features.shaderStorageImageWriteWithoutFormat ? 4 : 0) |
      (features_1_2.enabled.vulkanMemoryModel ? 8 : 0) |
      (features_1_2.enabled.vulkanMemoryModelDeviceScope ? 16 : 0) |
      (features_1_2.enabled.vulkanMemoryModelAvailabilityVisibilityChains ? 32 : 0) |
      (features_1_2.enabled.shaderFloat16 ? 64 : 0);
  Check(actual == expected, "enabled feature set differs from DLL/support/API requirements");
  const unsigned stored =
      (storage.properties_.shaderStorageImageExtendedFormats ? 1 : 0) |
      (storage.properties_.shaderStorageImageReadWithoutFormat ? 2 : 0) |
      (storage.properties_.shaderStorageImageWriteWithoutFormat ? 4 : 0) |
      (storage.properties_.vulkanMemoryModel ? 8 : 0) |
      (storage.properties_.vulkanMemoryModelDeviceScope ? 16 : 0) |
      (storage.properties_.vulkanMemoryModelAvailabilityVisibilityChains ? 32 : 0) |
      (storage.properties_.shaderFloat16 ? 64 : 0);
  Check(stored == actual, "shader capability reader differs from device enabled bits");
  Check(bool(features_EXT_robustness2.enabled.nullDescriptor) ==
        (null_extension && null_support && with_gpu_emulation), "nullDescriptor policy changed");
  Check(storage.properties_.nullDescriptor == features_EXT_robustness2.enabled.nullDescriptor,
        "nullDescriptor reader differs from device enabled bit");
  Check(fp::dll_queries == unsigned(bool(REX_PLATFORM_PROSPERO) && with_swapchain),
        "DLL query unexpectedly depends on startup on/off state");
  return actual;
}
int main() {
  unsigned cases = 0;
  for (const uint32_t api : {VK_API_VERSION_1_1, VK_API_VERSION_1_2, VK_API_VERSION_1_3})
    for (unsigned flags = 0; flags < 32; ++flags)
      for (unsigned support = 0; support < 128; ++support) {
        const bool swapchain = flags & 1, gpu = flags & 2, dll = flags & 4;
        const bool ext = flags & 8, null_supported = flags & 16;
        const unsigned off = Run(swapchain, gpu, dll, false, api, support, ext, null_supported);
        const unsigned on = Run(swapchain, gpu, dll, true, api, support, ext, null_supported);
        Check(off == on, "startup off cannot prepare the same supported features as startup on");
        cases += 2;
      }
  std::printf("PASS platform=%d cases=%u startup-off/on feature parity; supported-only; "
              "API gates; memory-model dependency; shader capability parity; nullDescriptor retained\n",
              REX_PLATFORM_PROSPERO, cases);
}
'''


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--captures', type=Path, default=root / 'prospero/validation/framegen-device-features-v3.10.4-framegen-toggle-captures')
    parser.add_argument('--report', type=Path, default=root / 'prospero/validation/framegen-device-features-v3.10.4-framegen-toggle.json')
    args = parser.parse_args()
    source_path = root / 'tools/rexglue-sdk/src/ui/vulkan/vulkan_device.cpp'
    tracked = [Path(__file__).resolve(), source_path]
    hashes = {str(p.relative_to(root)): digest(p) for p in tracked}
    args.captures.mkdir(parents=True, exist_ok=True)
    runs, error = [], None
    try:
        macros, policy, robustness = extract(source_path.read_text())
        generated = args.captures / 'device-features-host.cpp'
        generated.write_text(FIXTURE.replace('@MACROS@', macros).replace('@POLICY@', policy).replace('@ROBUSTNESS@', robustness))
        for platform in [1, 0]:
            binary = args.captures / f'device-features-platform-{platform}'
            commands = [
                ('compile', [args.cxx, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-O2',
                             f'-DREX_PLATFORM_PROSPERO={platform}', '-include', 'initializer_list',
                             '-I' + str(root / 'tools/rexglue-sdk/thirdparty/vulkan-headers/include'),
                             str(generated), '-o', str(binary)]),
                ('run', [str(binary)]),
            ]
            for kind, command in commands:
                result = subprocess.run(command, capture_output=True, text=True, timeout=45)
                capture = args.captures / f'{kind}-platform-{platform}.txt'
                capture.write_text(result.stdout + result.stderr)
                runs.append({'command': command, 'exit_code': result.returncode,
                             'capture': str(capture.relative_to(root)) if capture.is_relative_to(root) else str(capture),
                             'capture_sha256': digest(capture)})
                if result.returncode:
                    raise RuntimeError(capture.read_text())
        if any(digest(p) != hashes[str(p.relative_to(root))] for p in tracked):
            raise RuntimeError('Production source or fixture changed during verification')
    except Exception as exception:
        error = str(exception)
    report = {'status': 'FAIL' if error else 'PASS', 'scope': __doc__, 'source_sha256': hashes,
              'cases_per_platform': 24576, 'platforms': [1, 0], 'runs': runs, 'error': error,
              'limits': ['Only feature-selection C++ blocks and macros run with host state doubles.',
                         'No vkCreateDevice, GPU, driver, PS5 ABI or actual generator initialization is executed.',
                         'Full PS5 target compilation and live-toggle rendering tests are separate.']}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'status': report['status'], 'report': str(args.report), 'error': error}))
    return 1 if error else 0


if __name__ == '__main__':
    raise SystemExit(main())
