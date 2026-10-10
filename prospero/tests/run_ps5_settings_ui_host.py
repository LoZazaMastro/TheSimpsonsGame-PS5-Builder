#!/usr/bin/env python3
"""Run the actual menu with real ImGui and the shipped font, without PS5 GPU."""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess

root = pathlib.Path(__file__).resolve().parents[2]
sdk = root / 'tools/rexglue-sdk'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report', type=pathlib.Path,
                    help='JSON report path; captures use an adjacent <report-stem>-captures directory.')
args = parser.parse_args()
report_path = (root / args.report).resolve() if args.report else root / 'prospero/validation/settings-ui-v3.10.json'
capture = (report_path.with_name(report_path.stem + '-captures') if args.report
           else root / 'prospero/validation/settings-v3.10-captures')
capture.mkdir(parents=True, exist_ok=True)
subprocess.run(['python3', str(root / 'prospero/tools/embed_settings_capabilities.py'),
               str(root / 'prospero/builder/Assets/capabilities.json'),
               str(capture / 'prospero_settings_embedded.h')], check=True)
drawer = (sdk / 'src/ui/imgui_drawer.cpp').read_text()
font = re.search(r'const char kProggyTinyCompressedDataBase85\[.*?\] =\s*R"\(.*?\)";', drawer, re.S).group()
(capture / 'test_settings_font.h').write_text('namespace rex::ui {\n' + font + '\n}\n')
includes = [sdk / 'include', capture, sdk / 'thirdparty/imgui', sdk / 'thirdparty/fmt/include',
            sdk / 'thirdparty/spdlog/include', sdk / 'thirdparty/tomlplusplus/include', sdk / 'thirdparty/simde',
            sdk / 'thirdparty/vulkan-headers/include', sdk / 'thirdparty/renderdoc']
sources = [root / 'prospero/tests/ps5_settings_ui_host.cpp', sdk / 'src/core/settings_store_prospero.cpp',
           *[sdk / 'thirdparty/imgui' / name for name in ['imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp']]]
binary = capture / 'settings-ui-host'
command = ['g++', '-std=c++23', '-O1', '-pthread', '-DFMT_HEADER_ONLY', '-D__PROSPERO__',
           *['-I' + str(p) for p in includes], *map(str, sources), '-o', str(binary)]
compiled = subprocess.run(command, capture_output=True, text=True)
(capture / 'ui-host-compile.txt').write_text(compiled.stdout + compiled.stderr)
assert compiled.returncode == 0, compiled.stderr
result = subprocess.run([str(binary), str(capture / 'ui-settings.toml')], capture_output=True, text=True, timeout=45)
(capture / 'ui-host-runtime.txt').write_text(result.stdout + result.stderr)
assert result.returncode == 0, result.stdout + result.stderr
report = {'status': 'PASS', 'scope': 'Actual PS5 ImGui menu, real gamepad navigation and shipped font, host execution',
          'native_presenter': 'stubbed; no PS5 GPU execution is claimed', 'checks': result.stdout.splitlines(),
          'framegen_session_api': 'stubbed at the UI boundary; the real atomic setter/shortcut implementation is tested by framegen-session-v3.10.5-ui-menu.json',
          'source_sha256': {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [
              pathlib.Path(__file__).resolve(), root / 'prospero/tools/embed_settings_capabilities.py',
              root / 'prospero/tests/ps5_settings_ui_host.cpp', sdk / 'src/ui/overlay/settings_prospero.cpp',
              sdk / 'include/rex/platform/settings_prospero.h', sdk / 'include/rex/platform/framegen_prospero.h',
              sdk / 'src/ui/imgui_drawer.cpp', root / 'prospero/builder/Assets/capabilities.json']}}
report_path.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({'status': 'PASS', 'checks': len(report['checks']), 'report': str(report_path)}))
