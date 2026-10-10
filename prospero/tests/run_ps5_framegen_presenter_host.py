#!/usr/bin/env python3
"""Execute production presenter + real submission tracker against Vulkan fakes."""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess

root = pathlib.Path(__file__).resolve().parents[2]
sdk = root / 'tools/rexglue-sdk'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--version', default='3.10.4', help='Validation report/capture suffix')
parser.add_argument('--tracker-source', type=pathlib.Path,
                    help='Alternate tracker source, for regression negative controls')
parser.add_argument('--expect-third-paint-failure', action='store_true',
                    help='Require the pre-fix tracker to reject the third paint regression')
args = parser.parse_args()
if args.expect_third_paint_failure and not args.tracker_source:
    parser.error('an expected regression failure requires --tracker-source')
if not re.fullmatch(r'[A-Za-z0-9.-]+', args.version):
    parser.error('version must contain only letters, digits, dot or dash')
captures = root / f'prospero/validation/framegen-presenter-v{args.version}-captures'
captures.mkdir(parents=True, exist_ok=True)
source = sdk / 'src/ui/vulkan/vulkan_presenter.cpp'
header = sdk / 'include/rex/ui/vulkan/presenter.h'
base_header = sdk / 'include/rex/ui/presenter.h'
fg_header = sdk / 'include/rex/ui/vulkan/framegen_prospero.h'
tracker_header = sdk / 'include/rex/ui/vulkan/submission_tracker.h'
tracker_source = args.tracker_source or sdk / 'src/ui/vulkan/vulkan_submission_tracker.cpp'
template = root / 'prospero/tests/ps5_framegen_presenter_host.cpp.in'
cpp, hdr, base, fg, tracker_hdr, tracker_cpp = [p.read_text() for p in
    [source, header, base_header, fg_header, tracker_header, tracker_source]]

def block(text, marker):
    begin = text.index(marker)
    masked = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                    lambda match: ' ' * len(match[0]), text, flags=re.S)
    depth = 0
    for i in range(masked.index('{', begin), len(masked)):
        depth += (masked[i] == '{') - (masked[i] == '}')
        if not depth:
            end = i + 1 + (text[i+1:i+2] == ';')
            return text[begin:end]
    raise AssertionError(marker)

tokens = {
    'FRAMEGEN_STRUCTS': fg[fg.index('struct FrameGenerationSource'):fg.index('class ProsperoFrameGenerator')],
    'PAINT_CONFIG': block(base, 'class GuestOutputPaintConfig'),
    'PAINT_RESULT': block(base, 'enum class PaintResult'),
    'PAINT_TYPES': base[base.index('  struct GuestOutputProperties'):base.index('  explicit Presenter(')],
    'PAINT_BATCH': block(hdr, 'struct ProsperoPaintBatch'),
    'RECT_CONSTANTS': block(hdr, 'struct GuestOutputPaintRectangleConstants'),
    'LAYOUT_ENUM': block(hdr, 'enum GuestOutputPaintPipelineLayoutIndex'),
    'DESCRIPTOR_ENUM': block(hdr, 'enum GuestOutputDescriptorSet'),
    'WRAPPER': block(cpp, 'Presenter::PaintResult VulkanPresenter::PaintAndPresentImpl'),
    'SINGLE': block(cpp, 'Presenter::PaintResult VulkanPresenter::PaintAndPresentSingleImpl'),
    'SUBMISSION_TRACKER_CLASS': block(tracker_hdr, 'class VulkanSubmissionTracker'),
    'SUBMISSION_TRACKER_IMPLEMENTATION': tracker_cpp[
        tracker_cpp.index('VulkanSubmissionTracker::FenceAcquisition::~FenceAcquisition()'):
        tracker_cpp.rindex('}  // namespace vulkan')],
}
rendered = template.read_text()
for token, content in tokens.items(): rendered = rendered.replace('@@' + token + '@@', content)
assert '@@' not in rendered
generated = captures / 'framegen-presenter-host.cpp'
generated.write_text(rendered)
binary = captures / 'framegen-presenter-host'
command = ['g++', '-std=c++23', '-O1', '-pthread', '-I' + str(sdk / 'include'),
           '-I' + str(sdk / 'thirdparty/vulkan-headers/include'), str(generated), '-o', str(binary)]
compiled = subprocess.run(command, capture_output=True, text=True)
(captures / 'compile.txt').write_text(compiled.stdout + compiled.stderr)
assert compiled.returncode == 0, compiled.stderr
result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=40)
(captures / 'runtime.txt').write_text(result.stdout + result.stderr)
if args.expect_third_paint_failure:
    assert result.returncode != 0 and 'third_paint_result==Result::kPresented' in result.stderr, \
        'Baseline did not reproduce the specific third-paint failure:\n' + result.stdout + result.stderr
else:
    assert result.returncode == 0, result.stdout + result.stderr
tracked = [source, header, base_header, fg_header, tracker_header, tracker_source,
           template, pathlib.Path(__file__).resolve()]
status = 'EXPECTED_REGRESSION_REJECTION' if args.expect_third_paint_failure else 'PASS'
report = {'status': status, 'exit_code': result.returncode,
          'expected_third_paint_failure': args.expect_third_paint_failure,
          'scope': 'Exact production presenter wrapper and single-paint bodies with actual submission tracker class/implementation; host execution with simulated Vulkan device and frame generator',
          'hardware_validated': False, 'not_tested_here': 'Shader output quality, real GPU execution, physical display cadence',
          'checks': result.stdout.strip().splitlines(),
          'source_sha256': {str(p.relative_to(root) if p.is_relative_to(root) else p):
                            hashlib.sha256(p.read_bytes()).hexdigest() for p in tracked},
          'extracted_bodies_sha256': {key: hashlib.sha256(tokens[key].encode()).hexdigest()
              for key in ['WRAPPER', 'SINGLE', 'SUBMISSION_TRACKER_CLASS', 'SUBMISSION_TRACKER_IMPLEMENTATION']}}
report_path = root / f'prospero/validation/framegen-presenter-v{args.version}.json'
report_path.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({'status': status, 'checks': len(report['checks']), 'report': str(report_path.relative_to(root))}))
