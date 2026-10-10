#!/usr/bin/env python3
"""Exercise production Vulkan publication/worker/drain/retirement methods on host.

The actual queue header, Pipeline types, owner publication, handle lookup,
worker scheduler, EndSubmission and shutdown join/drain statements are compiled.
Only the shader/Vulkan compile body, Vulkan objects/fences and rex::thread native
boundary are fixtures. This is not a GPU or PS5 execution test.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import tempfile
import time


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def between(text, start, end):
    first = text.index(start)
    return text[first:text.index(end, first)]


def method(text, signature):
    first = text.index(signature)
    return text[first:text.index('\n}\n', first) + 3]


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='clang++')
    parser.add_argument('--ubsan', action='store_true')
    parser.add_argument('--report', type=Path,
                        default=root / 'prospero/validation/pipeline-publication-v3.9.json')
    args = parser.parse_args()
    args.report = args.report.resolve()
    capture = args.report.with_suffix('')
    capture.mkdir(parents=True, exist_ok=True)
    paths = {
        'cache_cpp': root / 'tools/rexglue-sdk/src/graphics/vulkan/pipeline_cache.cpp',
        'cache_h': root / 'tools/rexglue-sdk/include/rex/graphics/vulkan/pipeline_cache.h',
        'cp_cpp': root / 'tools/rexglue-sdk/src/graphics/vulkan/command_processor.cpp',
        'queue': root / 'tools/rexglue-sdk/include/rex/graphics/vulkan/pipeline_completion_queue.h',
        'fixture': root / 'prospero/tests/vulkan_pipeline_publication_host.cpp.in',
        'runner': Path(__file__).resolve(),
    }
    cache = paths['cache_cpp'].read_text()
    header = paths['cache_h'].read_text()
    cp = paths['cp_cpp'].read_text()
    methods = [method(cache, 'void VulkanPipelineCache::' + name + '(') for name in (
        'ApplyPipelineCreation', 'ApplyCompletedPipelineCreations',
        'GetPipelineAndLayoutByHandle', 'ProcessDeferredPipelineDestructions',
        'CreationThread', 'CreateQueuedPipelinesOnProcessorThread', 'EndSubmission')]
    # A test-only barrier parks the real consumer between its handle and layout
    # reads. The producer queues a real result in this window; publication must
    # remain on the owner, so the reader's old pending snapshot stays coherent.
    methods[2] = methods[2].replace(
        '  pipeline_layout_out = pipeline->pipeline_layout.load',
        '  BetweenHandleAndLayout();\n  pipeline_layout_out = pipeline->pipeline_layout.load')
    ensure = between(cache, 'bool VulkanPipelineCache::EnsurePipelineCreated(',
                     '\nvoid VulkanPipelineCache::ApplyPipelineCreation(')
    prelude_start = ensure.index('  const bool creating_placeholder')
    prelude = ensure[prelude_start:ensure.index('  // This function preferably', prelude_start)]
    stop = between(cache, '  // Shut down creation threads before destroying any pipelines',
                   '  // Pipeline workers may still be inside vkCreateGraphicsPipelines')
    substitutions = {
        '@PIPELINE_TYPES@': between(header, '  struct Pipeline {',
                                   '  // Description that can be passed'),
        '@METHODS@': '\n\n'.join(methods),
        '@ENSURE_HANDOFF@': prelude,
        '@STOP_WORKERS@': stop,
    }
    fixture = paths['fixture'].read_text()
    for key, value in substitutions.items():
        if fixture.count(key) != 1:
            raise RuntimeError('missing or duplicate fixture insertion ' + key)
        fixture = fixture.replace(key, value)

    routing = {
        'worker_queues_complete_results': 'EnsurePipelineCreated(creation_arguments, VK_NULL_HANDLE, true);' in methods[4],
        'owner_helper_publishes_directly': 'EnsurePipelineCreated(creation_arguments);' in methods[5],
        'preload_queues_and_drains_after_join': cache.index('ApplyCompletedPipelineCreations();', cache.index('auto creation_worker')) > cache.index('rex::thread::Wait(creation_thread.get(), false);', cache.index('auto creation_worker')),
        'configure_drains_before_selecting_pipeline': cache.index('ApplyCompletedPipelineCreations();', cache.index('bool VulkanPipelineCache::ConfigurePipeline(')) < cache.index('if (last_pipeline_', cache.index('bool VulkanPipelineCache::ConfigurePipeline(')),
        'ensure_never_reads_submission_or_stores_publication': 'GetCurrentSubmission' not in ensure and '.pipeline.store(' not in ensure and '.pipeline.exchange(' not in ensure and '.pipeline_layout.store(' not in ensure and '.is_placeholder.store(' not in ensure,
        'shutdown_drains_after_join_before_destruction': stop.index('ApplyCompletedPipelineCreations();') > stop.index('rex::thread::Wait(creation_threads_[i].get(), false);'),
        'draw_never_binds_unavailable_after_async_toggle': 'if (pipeline_is_unavailable) {\n    frame_used_async_placeholder_pipeline_ = true;\n    return true;\n  }' in cp,
        'incomplete_present_filter_independent_of_async_toggle': 'bool skip_present_due_async_placeholder = REXCVAR_GET(vulkan_async_skip_incomplete_frames) &&' in cp,
        'pipeline_failures_visible_in_telemetry': 'failed total | edram_transfers_skipped=' in cp and 'pipelines_failed()' in cp,
    }
    if not all(routing.values()):
        raise RuntimeError('integration routing failed: ' + repr(routing))
    report = {'result': 'FAIL', 'scope': __doc__, 'compiler': args.cxx,
              'production_and_test_sha256': {str(p.relative_to(root)): digest(p) for p in paths.values()},
              'integration_routing': routing, 'executions': [], 'limits': [
                  'Fake pipeline compilation deliberately returns controlled handles or failures; real shader translator/driver behavior is not simulated.',
                  'Host event/thread fixtures bound waits and join actual host threads; PS5 scheduling latency, VideoOut, RADV and firmware are not executed.',
                  'A test-only callback inserted between the real lookup method handle/layout reads makes completion timing deterministic.',
                  'The full game translation units and final target ELF require a separate native build gate.']}
    try:
        generated = capture / 'integration_generated.cpp'
        generated.write_text(fixture)
        with tempfile.TemporaryDirectory(prefix='simpsons-vulkan-publication-') as temporary:
            binary = Path(temporary) / 'pipeline_publication'
            cmd = [args.cxx, '-std=c++23', '-O2' if args.ubsan else '-O3', '-pthread',
                   '-Wall', '-Wextra', '-Werror', '-I', str(root / 'tools/rexglue-sdk/include'),
                   str(generated), '-o', str(binary)]
            if args.ubsan:
                cmd += ['-fsanitize=undefined', '-fno-sanitize-recover=all']
            for label, command in (('compile', cmd), ('execute', [str(binary)])):
                started = time.monotonic()
                result = subprocess.run(command, cwd=root, capture_output=True, text=True, timeout=60)
                output = capture / (label + '.txt')
                output.write_text('COMMAND ' + shlex.join(command) + '\n' + result.stdout + result.stderr + '\nEXIT ' + str(result.returncode) + '\n')
                report['executions'].append({'phase': label, 'exit_code': result.returncode,
                                             'elapsed_seconds': round(time.monotonic() - started, 3),
                                             'capture': str(output.relative_to(root)), 'capture_sha256': digest(output)})
                print(label, 'PASS' if result.returncode == 0 else 'FAIL', flush=True)
                if result.returncode:
                    print(result.stdout + result.stderr, flush=True)
                    raise RuntimeError(label + ' failed')
                if label == 'execute':
                    print(result.stdout, end='', flush=True)
                    report['case_lines'] = result.stdout.splitlines()
        report['result'] = 'PASS'
    finally:
        args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
