#!/usr/bin/env python3
"""Host tests of production PS5 settings mailboxes and atomic persistence."""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import tomllib

root = pathlib.Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report', type=pathlib.Path,
                    help='JSON report path; captures use an adjacent <report-stem>-captures directory.')
args = parser.parse_args()
report_path = (root / args.report).resolve() if args.report else root / 'prospero/validation/settings-v3.10.json'
captures = (report_path.with_name(report_path.stem + '-captures') if args.report
            else root / 'prospero/validation/settings-v3.10-captures')
captures.mkdir(parents=True, exist_ok=True)
sdk = root / 'tools/rexglue-sdk'
sources = [root / 'prospero/tests/ps5_settings_host.cpp', sdk / 'src/core/settings_store_prospero.cpp']
exe = captures / 'settings-host'
command = ['g++', '-std=c++23', '-O2', '-pthread', '-Wall', '-Wextra', '-Werror',
           '-I' + str(sdk / 'include'), '-I' + str(sdk / 'thirdparty/tomlplusplus/include'),
           *map(str, sources), '-Wl,--wrap=fsync', '-o', str(exe)]
compiled = subprocess.run(command, capture_output=True, text=True)
(captures / 'host-compile.txt').write_text(compiled.stdout + compiled.stderr)
assert compiled.returncode == 0, compiled.stderr
result = subprocess.run([str(exe), str(captures / 'filesystem')], capture_output=True, text=True, timeout=40)
(captures / 'host-runtime.txt').write_text(result.stdout + result.stderr)
assert result.returncode == 0, result.stdout + result.stderr

manifest_path = root / 'prospero/builder/Assets/capabilities.json'
manifest = json.loads(manifest_path.read_bytes())
defaults = {s['key']: s['default'] for s in manifest['settings']} | manifest['lockedSettings']
baseline = tomllib.loads((root / 'prospero/profile.toml').read_text())
# The shared startup preset deliberately differs from the legacy migration
# fallback. Check each authorized difference and preserve all other comparisons.
preset_changes = {'present_effect': ('bilinear', 'fsr'),
                  'swap_post_effect': ('none', 'fxaa'),
                  'eye_shading': ('original', 'clean'),
                  'menu_frame_rate': (60, 30),
                  'vsync': (True, False)}
assert set(preset_changes) <= set(baseline)
for key, value in baseline.items():
    if key in preset_changes:
        previous, current = preset_changes[key]
        assert value == previous, ('legacy fallback changed', key, value, previous)
        assert defaults[key] == current, ('startup preset changed', key, defaults.get(key), current)
    else:
        assert defaults[key] == value, (key, defaults.get(key), value)
assert len(manifest['settings']) == 28
live = {s['key'] for s in manifest['settings'] if s['applyMode'] == 'live'}
assert live == {'frame_rate', 'menu_frame_rate', 'anisotropic_override', 'swap_post_effect',
                'present_effect', 'present_fsr_sharpness_reduction', 'present_letterbox',
                'audio_mute', 'subtitles', 'frame_gen', 'vsync', 'color_vibrance',
                'color_saturation', 'color_contrast', 'color_brightness'}
assert manifest['lockedSettings']['vulkan_device'] == -1
assert defaults['audio_maxqframes'] == 8
assert defaults['prospero_output_hz'] == 120
assert defaults['present_fsr_sharpness_reduction'] == 0.0
framegen = next(g for g in manifest['groups'] if g['id'] == 'framegen')
assert framegen['requiresFile'] == 'lossless.dll'
assert defaults['frame_gen'] is True and defaults['frame_gen_target_rate'] == 120 and defaults['frame_gen_multiplier'] == 2
for setting in manifest['settings']:
    if setting['group'] == 'framegen':
        expected_apply = 'live' if setting['key'] == 'frame_gen' else 'restart'
        assert setting['applyMode'] == expected_apply and setting['status'] == 'experimental'
for name in ['frame_rate', 'menu_frame_rate']:
    setting = next(s for s in manifest['settings'] if s['key'] == name)
    assert [c['value'] for c in setting['choices']] == [30, 60]
assert 'color_preset' not in defaults and 'ink_color_preset' not in defaults
for preset in manifest['presets']:
    assert set(preset['values']) <= set(defaults)
for name in ['gpu_memory_mirror_on_demand', 'native_resolve_surface_aliases',
             'native_resolve_fresh_initial', 'native_resolve_composed_reads']:
    assert manifest['lockedSettings'][name] is False

# Confirm the manifest's persisted value types against actual CVar definitions,
# including values converted by the PC launcher (e.g. vulkan_device auto -> -1).
definitions = {}
for base in [root / 'simpsons/src', sdk / 'src']:
    for path in base.rglob('*.cpp'):
        for kind, key in re.findall(r'REXCVAR_DEFINE_(BOOL|INT32|UINT32|INT64|UINT64|DOUBLE|STRING)\s*\(\s*(\w+)', path.read_text(errors='replace')):
            definitions.setdefault(key, kind)
for key, value in defaults.items():
    kind = definitions[key]
    expected = str if kind == 'STRING' else bool if kind == 'BOOL' else float if kind == 'DOUBLE' else int
    assert isinstance(value, expected) and not (expected is int and isinstance(value, bool)), (key, kind, value)
ui = (sdk / 'src/ui/overlay/settings_prospero.cpp').read_text()
assert 'SetFlagByName' not in ui and 'REXCVAR_SET' not in ui

tracked = [pathlib.Path(__file__).resolve(), manifest_path, root / 'prospero/profile.toml',
           root / 'prospero/tools/embed_settings_capabilities.py', *sources,
           sdk / 'include/rex/platform/settings_prospero.h', sdk / 'src/core/settings_prospero.cpp',
           sdk / 'include/rex/platform/settings_store_prospero.h',
           sdk / 'src/ui/overlay/settings_prospero.cpp', sdk / 'src/ui/overlay/settings_overlay.cpp',
           sdk / 'src/ui/rex_app.cpp', sdk / 'src/ui/presenter.cpp',
           sdk / 'src/ui/vulkan/vulkan_presenter.cpp', sdk / 'src/graphics/vulkan/command_processor.cpp',
           sdk / 'src/graphics/vulkan/texture_cache.cpp', sdk / 'src/audio/prospero/prospero_audio_driver.cpp',
           root / 'simpsons/src/frame_pacing.cpp', root / 'simpsons/src/subtitles.cpp']
report = {'status': 'PASS', 'scope': 'Host production mailbox and persistence tests, manifest/default/type audit',
          'hardware_validated': False, 'ui_target_compilation': 'recorded by root final build',
          'checks': result.stdout.strip().splitlines() + [
              'PASS 28-setting manifest matches the requested startup preset; legacy fallback differences are explicit and all actual CVar types are preserved',
              'PASS fifteen reviewed live controls; derived presets do not introduce fake TOML keys',
              'PASS four new native fast paths fixed off; generic raw-CVar editing unreachable on PS5'],
          'source_sha256': {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest() for p in tracked}}
report_path.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({'status': report['status'], 'checks': len(report['checks']), 'report': str(report_path)}))
