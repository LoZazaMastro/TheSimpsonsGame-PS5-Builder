#!/usr/bin/env python3
"""Review the exact final linked PS5 ELF and the inputs that produced it.

Preparation inspects sources and compile commands only. Final mode requires
the frozen ELF's SHA-256 supplied after the build owner declares it final.
No command here links, packages, launches, or modifies production sources.
The final result concerns static integration, not console execution or FPS.
"""
from __future__ import annotations

import argparse
import collections
import datetime
import hashlib
import json
import mmap
from pathlib import Path
import re
import shlex
import shutil
import struct
import subprocess
import time


def digest(path: Path) -> str:
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


class Elf:
    """Small read-only ELF64-LE reader; raw tools are captured independently."""

    def __init__(self, path: Path):
        self.path = path
        self.file = path.open('rb')
        self.data = mmap.mmap(self.file.fileno(), 0, access=mmap.ACCESS_READ)
        values = struct.unpack_from('<16sHHIQQQIHHHHHH', self.data)
        if values[0][:6] != b'\x7fELF\x02\x01':
            raise ValueError(f'Not a little-endian ELF64 file: {path}')
        self.type, self.machine, self.entry = values[1], values[2], values[4]
        phoff, shoff, phsize, phnum, shsize, shnum, shstr = (
            values[5], values[6], values[9], values[10], values[11], values[12], values[13])
        self.programs = []
        for index in range(phnum):
            v = struct.unpack_from('<IIQQQQQQ', self.data, phoff + index * phsize)
            self.programs.append(dict(zip(
                ('type', 'flags', 'offset', 'vaddr', 'paddr', 'filesz', 'memsz', 'align'), v)))
        self.sections = []
        for index in range(shnum):
            v = struct.unpack_from('<IIQQQQIIQQ', self.data, shoff + index * shsize)
            self.sections.append(dict(zip(
                ('name_offset', 'type', 'flags', 'addr', 'offset', 'size',
                 'link', 'info', 'align', 'entsize'), v)))
        names = self.section_data(self.sections[shstr]) if shnum else b''
        for index, section in enumerate(self.sections):
            section.update(index=index, name=self.string(names, section['name_offset']))
        self.by_name = {section['name']: section for section in self.sections}
        self._symbols = {}

    def close(self):
        self.data.close()
        self.file.close()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()

    @staticmethod
    def string(data: bytes, offset: int) -> str:
        if offset >= len(data):
            return ''
        end = data.find(b'\0', offset)
        return data[offset:end if end >= 0 else len(data)].decode(errors='replace')

    def section_data(self, section):
        if section['type'] == 8:  # SHT_NOBITS
            return b''
        first = section['offset']
        return self.data[first:first + section['size']]

    def symbols(self, section_name='.symtab'):
        if section_name in self._symbols:
            return self._symbols[section_name]
        section = self.by_name.get(section_name)
        if not section:
            return []
        strings = self.section_data(self.sections[section['link']])
        result = []
        for offset in range(section['offset'], section['offset'] + section['size'],
                            section['entsize'] or 24):
            name, info, other, index, value, size = struct.unpack_from('<IBBHQQ', self.data, offset)
            result.append({'name': self.string(strings, name), 'bind': info >> 4,
                           'type': info & 15, 'visibility': other & 3,
                           'section_index': index, 'value': value, 'size': size})
        self._symbols[section_name] = result
        return result

    def symbol(self, name):
        found = [s for s in self.symbols() if s['name'] == name and s['section_index'] != 0]
        if len(found) != 1:
            raise ValueError(f'{self.path.name}: expected one defined {name}, got {len(found)}')
        return found[0]

    def symbol_bytes(self, symbol):
        section = self.sections[symbol['section_index']]
        first = section['offset'] + symbol['value'] - section['addr']
        return self.data[first:first + symbol['size']]

    def virtual_bytes(self, address, size):
        for section in self.sections:
            if (section['type'] != 8 and section['flags'] & 2 and
                    section['addr'] <= address and
                    address + size <= section['addr'] + section['size']):
                first = section['offset'] + address - section['addr']
                return self.data[first:first + size]
        raise ValueError(f'{self.path.name}: no file-backed section at {address:#x}')

    def dynamic(self):
        section = self.by_name.get('.dynamic')
        if not section:
            return []
        strings = self.section_data(self.sections[section['link']])
        result = []
        for offset in range(section['offset'], section['offset'] + section['size'], 16):
            tag, value = struct.unpack_from('<qQ', self.data, offset)
            result.append({'tag': tag, 'value': value,
                           'string': self.string(strings, value) if tag in (1, 14) else None})
            if tag == 0:
                break
        return result

    def relocations(self):
        result = []
        for section in self.sections:
            if section['type'] not in (4, 9):  # SHT_RELA, SHT_REL
                continue
            table = self.sections[section['link']]
            symbols = self.symbols(table['name'])
            step = section['entsize'] or (24 if section['type'] == 4 else 16)
            for pos in range(section['offset'], section['offset'] + section['size'], step):
                offset, info = struct.unpack_from('<QQ', self.data, pos)
                sym = symbols[info >> 32]
                result.append({'offset': offset, 'type': info & 0xffffffff,
                               'symbol': sym['name'], 'symbol_index': info >> 32,
                               'target_section': section['info'],
                               'relocation_section': section['name']})
        return result

    def function_references(self, symbol):
        return sorted({r['symbol'] for r in self.relocations()
                       if r['target_section'] == symbol['section_index'] and
                       symbol['value'] <= r['offset'] < symbol['value'] + symbol['size']
                       and r['symbol']})


class Review:
    def __init__(self, root, report_path, final):
        self.root, self.report_path = root, report_path
        self.capture = report_path.parent / (report_path.stem + '-captures')
        self.capture.mkdir(parents=True, exist_ok=True)
        self.report = {'status': 'RUNNING' if final else 'PREPARING',
                       'scope': __doc__, 'verified_utc': datetime.datetime.now(
                           datetime.timezone.utc).isoformat(),
                       'checks': [], 'captures': [],
                       'limits': [
                           'No PS5 launch, renderer output, firmware scheduling or FPS is tested.',
                           'The final linked debug ELF is reviewed here; conversion/FSELF/staging require their separate packaging gate.',
                           'Signal checks cover registry functions and direct handler code; they do not certify native signal-frame unwinding or arbitrary indirect callbacks.',
                           'Generated guest TUs intentionally retain function symbols without new source line tables.']}

    def relative(self, path):
        return str(path.relative_to(self.root)) if path.is_relative_to(self.root) else str(path)

    def check(self, name, passed, detail=None):
        self.report['checks'].append({'name': name, 'passed': bool(passed), 'detail': detail})
        print(('PASS ' if passed else 'FAIL ') + name, flush=True)

    def run(self, name, args, cwd=None, timeout=120):
        start = time.monotonic()
        process = subprocess.run([str(a) for a in args], cwd=cwd or self.root,
                                 stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                 text=True, timeout=timeout)
        path = self.capture / (name + '.txt')
        path.write_text('COMMAND ' + shlex.join(map(str, args)) + '\n' + process.stdout +
                        '\nEXIT ' + str(process.returncode) + '\n')
        self.report['captures'].append({'path': self.relative(path), 'sha256': digest(path),
                                        'exit_code': process.returncode,
                                        'seconds': round(time.monotonic() - start, 3)})
        if process.returncode:
            raise RuntimeError(f'{name} failed; see {path}')
        return process.stdout

    def save(self, final):
        passed = all(item['passed'] for item in self.report['checks'])
        self.report['status'] = ('PASS' if final else 'PREPARED') if passed else 'FAIL'
        if 'error' in self.report:
            self.report['status'] = 'FAIL'
        self.report['runner_sha256'] = digest(Path(__file__).resolve())
        self.report_path.write_text(json.dumps(self.report, indent=2) + '\n')
        print('RESULT', self.report['status'], self.report_path, flush=True)
        return 0 if self.report['status'] in ('PASS', 'PREPARED') else 1


def inspect_commands(review, database, final):
    commands = json.loads(database.read_text())
    review.report['compile_database'] = {'path': review.relative(database),
                                          'sha256': digest(database), 'entries': len(commands)}
    shutil.copyfile(database, review.capture / 'compile_commands.json')
    guests, native = [], []
    by_source = {}
    for entry in commands:
        source = Path(entry['file']).resolve()
        args = entry.get('arguments') or shlex.split(entry['command'])
        output = Path(entry.get('output') or args[args.index('-o') + 1])
        if not output.is_absolute():
            output = Path(entry['directory']) / output
        record = {'source': review.relative(source), 'source_sha256': digest(source),
                  'output': review.relative(output), 'arguments': args}
        by_source[record['source']] = (entry, output, record)
        if re.fullmatch(r'simpsons_recomp\.\d+\.cpp', source.name):
            optimization = [a for a in args if re.fullmatch(r'-O(?:[0-3szg]|fast)', a)]
            arch = [a for a in args if a.startswith('-march=')]
            record['flags_valid'] = (
                optimization[-1:] == ['-O3'] and arch[-1:] == ['-march=x86-64-v3'] and
                '--target=x86_64-sie-ps5' in args and '-ffp-model=strict' in args and
                '-ffast-math' not in args and '-Ofast' not in args and
                '-fno-strict-aliasing' in args and '-fstrict-aliasing' not in args and
                '-femulated-tls' in args and '-fno-plt' in args)
            guests.append(record)
        elif ('/generated/' not in str(source) and '/thirdparty/' not in str(source) and
              any('/' + stem + '/' in str(source) for stem in
                  ('simpsons/src', 'tools/rexglue-sdk/src', 'prospero'))):
            debug = [a for a in args if a in ('-g0', '-gline-tables-only', '-g', '-g2', '-g3')]
            record['line_flag_valid'] = bool(debug and debug[-1] != '-g0')
            native.append(record)
    indices = sorted(int(Path(x['source']).name.split('.')[1]) for x in guests)
    review.check('90 generated guest TUs numbered 0 through 89', indices == list(range(90)))
    review.check('all guest effective optimization/architecture/FP/aliasing/TLS flags',
                 all(x['flags_valid'] for x in guests),
                 [x['source'] for x in guests if not x['flags_valid']])
    review.check('native CMake source line-table flags', all(x['line_flag_valid'] for x in native),
                 {'count': len(native), 'missing': [x['source'] for x in native if not x['line_flag_valid']]})
    emutls = [x for x in commands if x['file'].endswith('/prospero/runtime/emutls.c')]
    registry = [x for x in commands if x['file'].endswith('/core/signal_thread_prospero.cpp')]
    review.check('one explicit emutls TU and one signal registry TU', len(emutls) == len(registry) == 1)
    link_source = (review.root / 'prospero/link.sh').read_text()
    manual_c_flags = link_source[link_source.index('c_flags = '):link_source.index('support = work /')]
    review.check('manual link-support compilation requests native source lines',
                 '"-gline-tables-only"' in manual_c_flags)
    old_path = review.root / 'prospero/validation/target-runtime-v3.8.json'
    old = json.loads(old_path.read_text())
    review.report['comparison_to_old_manifest'] = {
        'manifest': review.relative(old_path), 'manifest_sha256': digest(old_path),
        'purpose': 'Compare provenance and source coverage; no old PASS result is reused.',
        'source_changes': [{'path': p, 'old_sha256': old_hash, 'current_sha256': digest(review.root / p)}
                           for p, old_hash in old['production_source_sha256'].items()
                           if (review.root / p).is_file() and digest(review.root / p) != old_hash],
        'new_native_sources': sorted(set(x['source'] for x in native) -
                                     {review.relative(Path(p)) for p in old['native_line_table_sources']})}
    if final:
        guest_imports = collections.defaultdict(list)
        native_jump_imports = collections.defaultdict(list)
        native_missing_lines = []
        empty_native = []
        for record in guests + native:
            obj = review.root / record['output']
            record['object_sha256'] = digest(obj)
            record['object_bytes'] = obj.stat().st_size
            with Elf(obj) as elf:
                if elf.machine != 62 or elf.type != 1:
                    raise RuntimeError(f'Not a target x86-64 relocatable object: {obj}')
                if record in guests:
                    for symbol in elf.symbols():
                        if symbol['section_index'] == 0 and symbol['name'] in (
                                '_setjmp', '___longjmp', '_longjmp', 'setjmp', 'longjmp'):
                            guest_imports[symbol['name']].append(record['source'])
                else:
                    for symbol in elf.symbols():
                        if symbol['section_index'] == 0 and symbol['name'] in ('setjmp', 'longjmp'):
                            native_jump_imports[symbol['name']].append(record['source'])
                    code = sum(s['size'] for s in elf.sections if s['flags'] & 4)
                    line_size = elf.by_name.get('.debug_line', {}).get('size', 0)
                    record['code_bytes'], record['line_table_bytes'] = code, line_size
                    if code and not line_size:
                        native_missing_lines.append(record['source'])
                    elif not code:
                        empty_native.append(record['source'])
        review.check('native objects with code contain source line tables', not native_missing_lines,
                     {'missing': native_missing_lines, 'no_code_objects': empty_native})
        review.check('generated guest jump references use only local SDK entry names',
                     bool(guest_imports['_setjmp']) and bool(guest_imports['___longjmp']) and
                     not any(guest_imports[n] for n in ('setjmp', 'longjmp', '_longjmp')),
                     dict(guest_imports))
        review.check('project native TUs do not acquire Sony setjmp/longjmp imports',
                     not native_jump_imports, dict(native_jump_imports))
    # Full commands are in the captured database; keep provenance rows compact.
    for record in guests + native:
        record['arguments_sha256'] = hashlib.sha256(json.dumps(record.pop('arguments')).encode()).hexdigest()
    review.report['guest_objects'] = guests
    review.report['native_objects'] = native
    return by_source


def map_input_sections(path):
    result = []
    line_re = re.compile(r'^\s*([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9]+)\s+(.+):\((.+)\)\s*$')
    with path.open() as file:
        for line in file:
            match = line_re.match(line)
            if match:
                address, _, size, _, provider, section = match.groups()
                result.append({'address': int(address, 16), 'size': int(size, 16),
                               'provider': provider, 'section': section})
    return result


def inspect_final(review, args, by_source):
    elf_path = args.elf.resolve()
    starting_hash = digest(elf_path)
    review.check('ELF matches explicitly frozen SHA-256', starting_hash == args.frozen_elf_sha256,
                 {'expected': args.frozen_elf_sha256, 'actual': starting_hash})
    if starting_hash != args.frozen_elf_sha256:
        raise RuntimeError('Refusing to inspect a different or still-changing ELF')
    review.report['elf'] = {'path': review.relative(elf_path), 'bytes': elf_path.stat().st_size,
                            'sha256': starting_hash}
    # Kept at the top level for the native-conversion body-preservation gate.
    review.report['elf_sha256'] = starting_hash
    freeze_path = args.source_freeze
    freeze = json.loads(freeze_path.read_text())
    freeze_mismatches = [path for path, expected in freeze['sources'].items()
                         if not (review.root / path).is_file() or digest(review.root / path) != expected]
    review.check('all build-owner frozen production sources match their declared hashes',
                 bool(freeze['sources']) and not freeze_mismatches,
                 {'source_count': len(freeze['sources']), 'mismatches': freeze_mismatches,
                  'freeze_manifest_sha256': digest(freeze_path)})
    shutil.copyfile(freeze_path, review.capture / freeze_path.name)
    work = args.link_directory.resolve() if args.link_directory else elf_path.with_name(elf_path.name + '.link')
    manifest_path = work / 'link-manifest.json'
    manifest = json.loads(manifest_path.read_text())
    sdk = Path(manifest['sdk'])
    archive = Path(manifest['radv_archive'])
    map_path = Path(manifest['map_file'])
    response_path = Path(manifest['response_file'])
    response = shlex.split(response_path.read_text())
    review.report['link_inputs'] = {p.name: {'path': review.relative(p), 'sha256': digest(p)}
                                    for p in (manifest_path, map_path, response_path, archive,
                                              Path(manifest['builtins_archive']),
                                              Path(manifest['linker_script']),
                                              review.root / 'prospero/link.sh')}
    for path in (manifest_path, response_path, map_path, work / 'local-symbols.map'):
        shutil.copyfile(path, review.capture / path.name)
    sections = map_input_sections(map_path)
    review.check('link map input sections parsed', len(sections) > 100)
    review.check('RADV retained with whole-archive',
                 any(response[i:i + 3] == ['--whole-archive', str(archive), '--no-whole-archive']
                     for i in range(len(response) - 2)))
    wrappers = ('pthread_create', 'pthread_join', 'pthread_detach', 'pthread_exit',
                'malloc', 'calloc', 'realloc', 'free', 'posix_memalign', 'aligned_alloc',
                'memalign', 'malloc_usable_size', 'reallocf', 'reallocarray', 'getline', 'getdelim')
    review.check('required thread and heap linker wrappers retained',
                 all('--wrap=' + name in response for name in wrappers))
    review.check('linker emits explicit entry anchor without native dynamic linker',
                 '--defsym=__prospero_linked_entry=ABSOLUTE(_start)' in response and
                 '--no-dynamic-linker' in response)
    review.run('elf-headers-sections-dynamic', [args.readelf, '-h', '-l', '-S', '-d', '-W', elf_path])
    review.run('defined-and-imported-symbols', [args.nm, '--format=posix', '--print-size', elf_path])
    radv_undefined = review.run('radv-undefined-object-symbols',
                               [args.nm, '--undefined-only', '--print-file-name', '--format=posix', archive])
    radv_weak_names = {match[1] for line in radv_undefined.splitlines()
                       if (match := re.search(r'\]: (\S+) [wv] ', line))}
    expected_providers = {
        **{'__wrap_' + n: 'heap_wrap_prospero.c.o' for n in wrappers if not n.startswith('pthread_')},
        **{'__wrap_' + n: 'threads_prospero.c.o' for n in ('pthread_create', 'pthread_join', 'pthread_detach')},
        '__wrap_pthread_exit': 'link_support.o', '__wrap_abort': 'abort_prospero.cpp.o',
        'ps5_heap_owns': 'heap_prospero.c.o', 'ps5_heap_stats': 'heap_prospero.c.o',
        'mspace_malloc': 'mspace.c.o', 'mspace_free': 'mspace.c.o',
        '_setjmp': 'sdk_setjmp.o', '___longjmp': 'sdk_setjmp.o', '_longjmp': 'sdk_setjmp.o',
        '__emutls_get_address': 'emutls.c.o', 'ps5p_run_thread_destructors': 'cxa_prospero.c.o',
        'ps5___cxa_thread_atexit_impl': 'cxa_prospero.c.o',
        'prospero_fiber_switch': 'fiber_prospero.cpp.o', 'prospero_trace_event': 'trace_prospero.cpp.o',
        'prospero_metrics_start': 'metrics_prospero.cpp.o',
        'prospero_audio_report_diagnostics': 'prospero_audio_driver.cpp.o',
        'radv_GetInstanceProcAddr': 'libvulkan_radeon.ps5.a(radv_instance.c.o)',
        'vk_icdGetInstanceProcAddr': 'libvulkan_radeon.ps5.a(radv_instance.c.o)',
        **{n: 'signal_thread_prospero.cpp.o' for n in (
            'prospero_signal_bind_suspend', 'prospero_signal_unbind_suspend', 'prospero_signal_dispatch_suspend',
            'prospero_signal_seh_active', 'prospero_signal_seh_capture',
            'prospero_signal_seh_read', 'prospero_signal_seh_set_active',
            'prospero_signal_thread_clear')}}
    excluded = re.compile(r'libps5platform\.a\((?:heap|heap_wrap|mspace|threads|cxa|emutls)\.(?:c\.)?o\)')
    old_providers = sorted({s['provider'] for s in sections if s['size'] and
                            (excluded.search(s['provider']) or 'fiber_posix.cpp.o' in s['provider'])})
    review.check('no superseded heap/TLS/thread/cxa/fiber provider contributes bytes', not old_providers, old_providers)
    with Elf(elf_path) as elf:
        review.check('ELF is x86-64 PIE', elf.machine == 62 and elf.type == 3)
        review.check('no PT_INTERP', not any(p['type'] == 3 for p in elf.programs))
        review.check('load segments are in bounds, non-WX and filesz <= memsz', all(
            p['offset'] + p['filesz'] <= len(elf.data) and p['filesz'] <= p['memsz'] and
            p['flags'] & 3 != 3 for p in elf.programs if p['type'] == 1))
        dynamic = elf.dynamic()
        needed = [d['string'] for d in dynamic if d['tag'] == 1]
        allowed_libraries = {'libSceAudioOut.sprx', 'libScePad.sprx', 'libSceUserService.sprx',
                             'libSceAgc.prx', 'libSceAgcDriver.prx', 'libSceLibcInternal.sprx',
                             'libSceSystemService.sprx', 'libSceVideoOut.sprx', 'libkernel.sprx'}
        review.check('native dependencies stay within the reviewed title import set',
                     bool(needed) and set(needed) <= allowed_libraries, needed)
        relocations = elf.relocations()
        reloc_counts = dict(collections.Counter(r['type'] for r in relocations))
        plt_sections = [s['name'] for s in elf.sections if s['size'] and
                        s['name'] in ('.plt', '.plt.got', '.plt.sec', '.rela.plt', '.rel.plt')]
        plt_inputs = [s for s in sections if s['size'] and s['section'].startswith('.plt')]
        review.check('no PLT sections, merged PLT inputs, JMPREL or JUMP_SLOT relocations',
                     not plt_sections and not plt_inputs and not any(
                         d['tag'] in (2, 20, 23) and d['value'] for d in dynamic) and
                     not reloc_counts.get(7), {'sections': plt_sections, 'inputs': plt_inputs,
                                              'relocation_type_counts': reloc_counts})
        tls_relocations = [r for r in relocations if r['type'] in set(range(16, 24)) | {34, 35, 36}]
        review.check('no native TLS relocations in the emulated-TLS image', not tls_relocations,
                     {'count': len(tls_relocations)})
        imports = sorted({s['name'] for s in elf.symbols('.dynsym') if s['section_index'] == 0 and s['name']})
        export_sources = collections.defaultdict(list)
        stub_rows = []
        for path in sorted(set((sdk / 'target/lib').glob('*.so')) |
                           {Path(p) for p in manifest['native_import_stubs']}):
            with Elf(path) as stub:
                soname = next((d['string'] for d in stub.dynamic() if d['tag'] == 14), None)
                if soname not in needed:
                    continue
                symbols = {s['name'] for s in stub.symbols('.dynsym')
                           if s['section_index'] and s['name'] and s['bind'] in (1, 2)}
                for symbol in symbols:
                    export_sources[symbol].append(soname)
                stub_rows.append({'path': review.relative(path), 'soname': soname,
                                  'sha256': digest(path), 'export_count': len(symbols)})
        unknown = [name for name in imports if name not in export_sources]
        nondynamic_undefined = [s for s in elf.symbols() if s['section_index'] == 0 and
                               s['name'] and s['name'] not in imports]
        tls_hook = '_ZTHN3rex7runtime21g_access_violation_pcE'
        tls_hook_guarded = check_weak_tls_hook(review, args, by_source, elf, relocations)
        permitted_compiler_weak = {tls_hook} if tls_hook_guarded else set()
        dynamic_references = {r['symbol'] for r in relocations}
        rejected_weak = [s for s in nondynamic_undefined if
                         s['bind'] != 2 or s['value'] or s['size'] or
                         s['name'] not in radv_weak_names | permitted_compiler_weak or
                         s['name'] in dynamic_references]
        weak_capture = review.capture / 'permitted-nondynamic-weak-symbols.json'
        weak_capture.write_text(json.dumps({
            'policy': 'Only zero-valued weak references proved in the exact RADV archive, plus the individually verified guarded compiler TLS hook; no unresolved dynamic relocation is allowed.',
            'radv_archive_sha256': digest(archive),
            'radv_archive_weak_names': sorted(radv_weak_names),
            'linked_nondynamic_undefined': nondynamic_undefined,
            'rejected': rejected_weak}, indent=2) + '\n')
        review.report['captures'].append({'path': review.relative(weak_capture), 'sha256': digest(weak_capture)})
        review.check('undefined symbols are known native imports or proved zero-valued weak references',
                     not unknown and not rejected_weak,
                     {'unknown_native_imports': unknown, 'rejected_nondynamic_undefined': rejected_weak,
                      'permitted_RADV_weak_count': sum(s['name'] in radv_weak_names for s in nondynamic_undefined),
                      'permitted_compiler_weak': sorted(permitted_compiler_weak)})
        review.report['imports'] = {'needed': needed, 'symbols': {n: export_sources[n] for n in imports},
                                    'provider_stubs': stub_rows}
        providers = {}
        for name, suffix in expected_providers.items():
            symbol = elf.symbol(name)
            owner = [s for s in sections if s['size'] and s['section'].startswith(('.text', '.init', '.fini')) and
                     s['address'] <= symbol['value'] and
                     symbol['value'] + symbol['size'] <= s['address'] + s['size']]
            matches = [s for s in owner if suffix in s['provider']]
            providers[name] = {'symbol': symbol, 'expected_provider': suffix,
                               'address': hex(symbol['value']), 'size': symbol['size'],
                               'providers': sorted({s['provider'] for s in owner}),
                               'matched': len(matches) == 1 and bool(elf.sections[symbol['section_index']]['flags'] & 4)}
        review.check('all runtime symbols come from the required target providers',
                     all(p['matched'] and p['size'] > 0 and p['symbol']['type'] == 2
                         for p in providers.values()),
                     {n: p for n, p in providers.items()
                      if not p['matched'] or p['size'] == 0 or p['symbol']['type'] != 2})
        review.report['providers'] = providers
        review.check('single defined emutls entry and no imported emutls runtime',
                     sum(s['name'] == '__emutls_get_address' and s['section_index'] != 0
                         for s in elf.symbols()) == 1 and
                     not any('emutls' in name for name in imports))
        cxa_alias = elf.symbol('__cxa_thread_atexit_impl')
        review.check('local C++ thread-destructor registration alias reaches the project provider',
                     cxa_alias['value'] == elf.symbol('ps5___cxa_thread_atexit_impl')['value'] and
                     '__cxa_thread_atexit_impl' not in imports,
                     {'alias': cxa_alias, 'provider': providers['ps5___cxa_thread_atexit_impl']['address']})
        review.check('native pthread fallback imports remain separate from project wrappers',
                     all(n in imports for n in ('pthread_create', 'pthread_join', 'pthread_detach', 'pthread_exit')) and
                     not any(arg.startswith('--defsym=pthread_exit=') for arg in response))
        entry, anchor = elf.symbol('_start'), elf.symbol('__prospero_linked_entry')
        entry_bytes = elf.symbol_bytes(entry)
        immediates = [i for i in range(max(0, len(entry_bytes) - 9))
                      if 0x48 <= entry_bytes[i] <= 0x4f and 0xb8 <= entry_bytes[i + 1] <= 0xbf and
                      struct.unpack_from('<Q', entry_bytes, i + 2)[0] == elf.entry]
        review.check('ELF entry, _start and local ABS anchor agree with encoded movabs immediate',
                     entry['value'] == anchor['value'] == elf.entry and
                     anchor['section_index'] == 0xfff1 and anchor['bind'] == 0 and bool(immediates) and
                     not any(r['offset'] < entry['value'] + i + 10 and
                             r['offset'] + 8 > entry['value'] + i + 2
                             for i in immediates for r in relocations),
                     {'entry': hex(elf.entry), 'anchor': anchor, 'movabs_offsets': immediates})
        start_disassembly = review.run('entry-disassembly', [args.objdump, '-d', '--no-show-raw-insn',
                                                           '--disassemble-symbols=_start', elf_path])
        review.check('entry passes both runtime and linked anchors to trace initialization',
                     'prospero_trace_set_image_anchors' in start_disassembly and
                     any('lea' in line and '<_start>' in line for line in start_disassembly.splitlines()))
        jump_object = work / 'sdk_setjmp.o'
        sdk_jump = subprocess.check_output([args.ar, 'p', str(sdk / 'target/lib/libc.a'), '_setjmp.o'])
        review.check('selected SDK jump object matches matched libc archive bytes',
                     jump_object.read_bytes() == sdk_jump,
                     {'object_sha256': digest(jump_object),
                      'SDK_member_sha256': hashlib.sha256(sdk_jump).hexdigest()})
        with Elf(jump_object) as jump:
            body_checks = {name: elf.symbol_bytes(elf.symbol(name)) == jump.symbol_bytes(jump.symbol(name))
                           for name in ('_setjmp', '___longjmp')}
        review.check('local SDK jump instruction bodies preserved exactly by the final link',
                     all(body_checks.values()), body_checks)
        review.check('native driver setjmp/longjmp remain separate imports',
                     'setjmp' in imports and 'longjmp' in imports and
                     not any(n in imports for n in ('_setjmp', '___longjmp', '_longjmp')))
        radv_jumps = collections.defaultdict(list)
        for line in radv_undefined.splitlines():
            match = re.search(r'\[([^\]]+)\]: (setjmp|longjmp|_setjmp|___longjmp|_longjmp) U ', line)
            if match:
                radv_jumps[match[1]].append(match[2])
        radv_jumps = {member: sorted(names) for member, names in radv_jumps.items()}
        expected_radv_jumps = {'gl_spirv.c.o': ['setjmp'], 'spirv_to_nir.c.o': ['longjmp', 'setjmp']}
        review.check('Sony jump imports originate in the matched RADV compiler objects',
                     radv_jumps == expected_radv_jumps and
                     not any(arg.startswith(('--defsym=setjmp=', '--defsym=longjmp=')) for arg in response),
                     radv_jumps)
        check_object_paths(review, args, by_source, work, elf)
        source_map = native_helper_source_map(review, args, elf, work, sections, map_path)
        check_lines(review, args, elf_path, elf, source_map)
    review.check('ELF unchanged throughout this review', digest(elf_path) == starting_hash)
    dry_run = review.run('ninja-reuse', [args.ninja, '-n', 'simpsons'], cwd=args.build_directory)
    review.check('final Ninja dependency graph has no pending work', 'ninja: no work to do.' in dry_run)


def check_weak_tls_hook(review, args, by_source, linked, linked_relocations):
    """Admit the one compiler weak hook only after checking its actual guards.

    Clang emits an optional dynamic-initializer call at an extern thread_local
    use. The defining TU supplies a zero-initialized uintptr_t, so no dynamic
    initializer exists. Each actual call must therefore remain guarded by a
    comparison of its zero, non-relocated GOT slot, with a branch over the call.
    This is deliberately specific to this object, symbol and instruction form.
    """
    hook = '_ZTHN3rex7runtime21g_access_violation_pcE'
    source = 'tools/rexglue-sdk/src/system/xmemory.cpp'
    definition = review.root / 'tools/rexglue-sdk/src/system/mmio_handler.cpp'
    caller = '_ZN3rex6memory6Memory23AccessViolationCallbackENSt3__111unique_lockINS2_15recursive_mutexEEEPvb'
    plain_zero = bool(re.search(r'thread_local\s+uintptr_t\s+g_access_violation_pc\s*=\s*0\s*;',
                               definition.read_text()))
    object_path = by_source[source][1]
    rows = []
    with Elf(object_path) as obj:
        obj_symbol = obj.symbol(caller)
        refs = [r for r in obj.relocations() if r['symbol'] == hook]
        comparisons = [r for r in refs if r['type'] == 9]  # R_X86_64_GOTPCREL
        calls = [r for r in refs if r['type'] == 4]  # R_X86_64_PLT32 (object only)
        obj_body = obj.symbol_bytes(obj_symbol)
        final_symbol = linked.symbol(caller)
        final_body = linked.symbol_bytes(final_symbol)
        shape_valid = bool(comparisons) and len(refs) == len(comparisons) + len(calls) and \
            len(comparisons) == len(calls) and all(
                r['target_section'] == obj_symbol['section_index'] and
                obj_symbol['value'] <= r['offset'] < obj_symbol['value'] + obj_symbol['size']
                for r in refs)
        for comparison in comparisons:
            offset = comparison['offset'] - obj_symbol['value']
            paired = any(call['offset'] == comparison['offset'] + 8 for call in calls)
            # cmpq $0, disp32(%rip); je +5; call rel32.
            obj_shape = (obj_body[offset - 3:offset] == b'\x48\x83\x3d' and
                         obj_body[offset + 4:offset + 8] == b'\x00\x74\x05\xe8')
            final_shape = (final_body[offset - 3:offset] == b'\x48\x83\x3d' and
                           final_body[offset + 4:offset + 8] == b'\x00\x74\x05\xe8')
            valid, got_address, call_target = False, None, None
            if paired and obj_shape and final_shape:
                got_address = final_symbol['value'] + offset + 5 + struct.unpack_from('<i', final_body, offset)[0]
                call_target = final_symbol['value'] + offset + 12 + struct.unpack_from('<i', final_body, offset + 8)[0]
                valid = (linked.virtual_bytes(got_address, 8) == bytes(8) and call_target == 0 and
                         not any(got_address <= r['offset'] < got_address + 8 for r in linked_relocations))
            rows.append({'comparison_offset': offset, 'paired_call': paired,
                         'object_guard': obj_shape, 'linked_guard': final_shape,
                         'zero_GOT_slot': hex(got_address) if got_address is not None else None,
                         'call_target': call_target, 'verified': valid})
    review.run('guarded-compiler-TLS-hook-object', [args.objdump, '-dr', '--no-show-raw-insn',
                                                 '--disassemble-symbols=' + caller, object_path])
    review.run('guarded-compiler-TLS-hook-linked', [args.objdump, '-d', '--no-show-raw-insn',
                                                 '--disassemble-symbols=' + caller, args.elf])
    passed = plain_zero and shape_valid and all(row['verified'] for row in rows)
    review.check('optional compiler TLS initializer hook is zero and every call is guarded', passed,
                 {'symbol': hook, 'source': source, 'constant_initialized_definition': plain_zero,
                  'reference_shape': shape_valid, 'guards': rows})
    return passed


def check_object_paths(review, args, by_source, work, linked):
    tls_path = by_source['prospero/runtime/emutls.c'][1]
    with Elf(tls_path) as tls:
        sizes = {name: tls.symbol(name)['size'] for name in
                 ('emutls_init_once.once', 'emutls_mutex', 'emutls_pthread_key')}
        review.check('emutls object has PS5 once/mutex/key storage sizes',
                     sizes == {'emutls_init_once.once': 16, 'emutls_mutex': 8, 'emutls_pthread_key': 4}, sizes)
    critical = {
        'tools/rexglue-sdk/src/core/signal_thread_prospero.cpp':
            ('prospero_signal_dispatch_suspend', 'prospero_signal_seh_active',
             'prospero_signal_seh_capture', 'prospero_signal_seh_read', 'prospero_signal_thread_clear'),
        'tools/rexglue-sdk/src/core/threading_posix.cpp':
            ('_ZN3rex6threadL14signal_handlerEiP9__siginfoPv',),
        'tools/rexglue-sdk/src/core/seh_posix.cpp':
            ('_ZN3rex8platformL14signal_handlerEiP9__siginfoPv',)}
    rows = []
    for source, names in critical.items():
        obj_path = by_source[source][1]
        with Elf(obj_path) as obj:
            if source.endswith('/signal_thread_prospero.cpp'):
                imports = [s['name'] for s in obj.symbols() if not s['section_index'] and s['name']]
                rows.append({'source': source, 'object_sha256': digest(obj_path),
                             'whole_registry_has_no_emutls': not any('emutls' in s['name'] for s in obj.symbols())})
                review.check('signal registry object contains no emutls import', rows[-1]['whole_registry_has_no_emutls'])
                slots = [s for s in obj.symbols() if s['name'].endswith('5slotsE')]
                review.check('signal registry has bounded static 1024-slot storage',
                             len(slots) == 1 and slots[0]['size'] == 1024 * 48,
                             [{'name': s['name'], 'size': s['size']} for s in slots])
            for name in names:
                refs = obj.function_references(obj.symbol(name))
                code = review.run('signal-body-' + name, [args.objdump, '-d', '--no-show-raw-insn',
                                                        '--disassemble-symbols=' + name, args.elf])
                no_tls = not any('emutls' in ref for ref in refs) and 'emutls' not in code
                review.check('signal path has no direct emutls access: ' + name, no_tls, refs)
                rows.append({'source': source, 'symbol': name, 'object_references': refs,
                             'direct_emutls_absent': no_tls})
    review.report['signal_code'] = rows
    with Elf(work / 'link_support.o') as support:
        review.check('manually compiled link_support also includes line tables',
                     support.by_name.get('.debug_line', {}).get('size', 0) > 0)
    trace_path = by_source['tools/rexglue-sdk/src/core/trace_prospero.cpp'][1]
    with Elf(trace_path) as trace:
        imports = [s['name'] for s in trace.symbols() if not s['section_index'] and s['name']]
        bad = [n for n in imports if any(t in n for t in
               ('emutls', 'malloc', 'calloc', 'realloc', '_Znwm', '_Znam'))]
        initialized = [s['name'] for s in trace.sections if s['size'] and s['name'].startswith('.init_array')]
        review.check('trace recorder has no allocating/TLS imports or dynamic initializers', not bad and not initialized,
                     {'forbidden_imports': bad, 'initializers': initialized})
    fiber = by_source['tools/rexglue-sdk/src/core/fiber_prospero.cpp'][1]
    with Elf(fiber) as obj:
        native_context = [s['name'] for s in obj.symbols() if not s['section_index'] and
                          s['name'] in ('getcontext', 'setcontext', 'swapcontext', 'makecontext')]
        review.check('PS5 fiber object does not import native ucontext ABI', not native_context, native_context)


def native_helper_source_map(review, args, elf, work, sections, link_map):
    """Resolve the low-address CU exactly, avoiding discarded COMDAT ranges.

    The stock address lookup can choose another CU whose discarded functions
    have zero-based DWARF ranges. The correct line program is still present.
    Select it by the actual link-map contribution, then prove source checksum,
    function ownership, exclusive bounds and complete non-overlapping coverage.
    """
    output_path = review.capture / 'native-helper-lines.json'
    # A failed rerun must not leave an earlier valid sidecar available.
    output_path.unlink(missing_ok=True)
    source = review.root / 'prospero/link_support.c'
    provider = str(work / 'link_support.o')
    contribution = [s for s in sections if s['provider'] == provider and s['section'] == '.debug_line']
    if len(contribution) != 1:
        raise RuntimeError('Cannot identify one exact link-support DWARF line contribution')
    offset = contribution[0]['address']
    output = review.run('native-helper-exact-CU-lines',
                        [args.dwarfdump, '--debug-line=' + hex(offset), args.elf])
    review.run('native-helper-generic-lookup-ambiguity',
               [args.dwarfdump, '--lookup=' + hex(elf.symbol('__wrap_pthread_exit')['value']), args.elf])
    if not re.search(r'debug_line\[0x0*' + format(offset, 'x') + r'\]', output):
        raise RuntimeError('DWARF tool did not return the selected link-map line contribution')
    directories, files, entries = {}, {}, []
    current_file = None
    for line in output.splitlines():
        if match := re.match(r'include_directories\[\s*(\d+)\]\s*=\s*"(.*)"', line):
            directories[int(match[1])] = Path(match[2])
        elif match := re.match(r'file_names\[\s*(\d+)\]:', line):
            current_file = int(match[1])
            files[current_file] = {}
        elif current_file is not None and (match := re.match(r'\s+name:\s*"(.*)"', line)):
            files[current_file]['name'] = match[1]
        elif current_file is not None and (match := re.match(r'\s+dir_index:\s*(\d+)', line)):
            files[current_file]['directory_index'] = int(match[1])
        elif current_file is not None and (match := re.match(r'\s+md5_checksum:\s*([0-9a-f]+)', line)):
            files[current_file]['md5'] = match[1]
        elif match := re.match(r'^0x([0-9a-f]+)\s+(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+\d+\s+\d+\s*(.*)$', line):
            current_file = None
            entries.append({'address': int(match[1], 16), 'line': int(match[2]),
                            'column': int(match[3]), 'file_index': int(match[4]),
                            'end_sequence': 'end_sequence' in match[5]})
    source_md5 = hashlib.md5(source.read_bytes()).hexdigest()
    for record in files.values():
        path = Path(record['name'])
        if not path.is_absolute():
            path = directories[record['directory_index']] / path
        record['resolved_file'] = path.resolve()
    if not entries or any(files[e['file_index']]['resolved_file'] != source or
                          files[e['file_index']].get('md5') != source_md5 for e in entries):
        raise RuntimeError('Selected DWARF helper CU has missing or mismatching source checksum/file metadata')
    functions = []
    for name in ('__wrap_pthread_exit', 'ps5_ffs', 'ps5_pathconf', 'ps5_timegm'):
        symbol = elf.symbol(name)
        owners = [s for s in sections if s['provider'] == provider and s['size'] and
                  s['section'].startswith('.text') and
                  s['address'] <= symbol['value'] and
                  symbol['value'] + symbol['size'] <= s['address'] + s['size']]
        if symbol['type'] != 2 or not symbol['size'] or len(owners) != 1:
            raise RuntimeError('Cannot prove helper code ownership: ' + name)
        functions.append({'start': hex(symbol['value']), 'end': hex(symbol['value'] + symbol['size']),
                          'symbol': name})
    mappings = []
    for index, entry in enumerate(entries[:-1]):
        if entry['end_sequence']:
            continue
        begin, end = entry['address'], entries[index + 1]['address']
        if end == begin:
            continue  # The last row at one address controls the next interval.
        owners = [f for f in functions if int(f['start'], 16) <= begin < end <= int(f['end'], 16)]
        if end < begin or len(owners) != 1:
            raise RuntimeError('DWARF helper line interval is outside a proved function')
        mappings.append({'start': hex(begin), 'end': hex(end), 'symbol': owners[0]['symbol'],
                         'file': 'prospero/link_support.c', 'line': entry['line'], 'column': entry['column']})
    mappings.sort(key=lambda entry: int(entry['start'], 16))
    if any(int(before['end'], 16) > int(after['start'], 16)
           for before, after in zip(mappings, mappings[1:])):
        raise RuntimeError('Overlapping helper source line intervals')
    for function in functions:
        current = int(function['start'], 16)
        for mapping in (m for m in mappings if m['symbol'] == function['symbol']):
            if int(mapping['start'], 16) != current:
                raise RuntimeError('Incomplete helper line coverage: ' + function['symbol'])
            current = int(mapping['end'], 16)
        if current != int(function['end'], 16):
            raise RuntimeError('Incomplete helper line coverage at end: ' + function['symbol'])
    result = {'schema_version': 1, 'status': 'PASS', 'elf_sha256': review.report['elf_sha256'],
              'scope': 'Actual linked helper CU; precise exclusive intervals. line=0 is intentionally unassigned by the compiler and must not be replaced with an invented line.',
              'source_provenance': {'file': 'prospero/link_support.c', 'sha256': digest(source), 'md5': source_md5},
              'cu': {'provider': provider, 'debug_line_offset': hex(offset),
                     'debug_line_bytes': contribution[0]['size'],
                     'link_map_sha256': digest(link_map),
                     'line_capture_sha256': digest(review.capture / 'native-helper-exact-CU-lines.txt')},
              'functions': functions, 'mappings': mappings}
    output_path.write_text(json.dumps(result, indent=2) + '\n')
    review.report['captures'].append({'path': review.relative(output_path), 'sha256': digest(output_path)})
    review.report['native_helper_line_map'] = {'path': review.relative(output_path),
                                               'sha256': digest(output_path), 'functions': functions,
                                               'mapping_count': len(mappings),
                                               'compiler_unassigned_line_intervals': sum(m['line'] == 0 for m in mappings)}
    review.check('low-address helper sidecar covers exact proved CU and every function byte', True,
                 review.report['native_helper_line_map'])
    return result


def check_lines(review, args, elf_path, elf, source_map):
    exact = {'_start': 'app_crt.cpp', '__wrap_pthread_exit': 'link_support.c',
             '__emutls_get_address': 'emutls.c', 'prospero_trace_event': 'trace_prospero.cpp',
             'ps5_heap_stats': 'heap_prospero.c',
             'prospero_signal_dispatch_suspend': 'signal_thread_prospero.cpp',
             'prospero_signal_seh_capture': 'signal_thread_prospero.cpp',
             'prospero_signal_thread_clear': 'signal_thread_prospero.cpp',
             '_ZN3rex6threadL14signal_handlerEiP9__siginfoPv': 'threading_posix.cpp',
             '_ZN3rex8platformL14signal_handlerEiP9__siginfoPv': 'seh_posix.cpp',
             '_ZN3rex6system7XThread4ExitEi': 'xthread.cpp',
             '_ZN3rex6system7XThread9TerminateEi': 'xthread.cpp',
             '_ZN3rex6system11KernelState14TerminateTitleEv': 'kernel_state.cpp',
             'sub_82718710': 'frame_pacing.cpp'}
    for fragment, source in (('ApplyPipelineCreation', 'pipeline_cache.cpp'),
                             ('ApplyCompletedPipelineCreations', 'pipeline_cache.cpp'),
                             ('PosixConditionINS0_6ThreadEE14post_execution', 'threading_posix.cpp')):
        candidates = [s for s in elf.symbols() if fragment in s['name'] and
                      s['type'] == 2 and s['section_index'] and s['size']]
        if not candidates:
            raise RuntimeError('Missing new runtime method: ' + fragment)
        exact[max(candidates, key=lambda s: s['size'])['name']] = source
    queries, rows = [], []
    for name, expected in exact.items():
        symbol = elf.symbol(name)
        offsets = sorted({min(delta, symbol['size'] - 1) for delta in
                          (0, 1, 8, 16, 32, 64, 128, symbol['size'] // 2,
                           symbol['size'] * 3 // 4, symbol['size'] - 1) if symbol['size']})
        addresses = [symbol['value'] + offset for offset in offsets]
        queries.extend(hex(address) for address in addresses)
        rows.append({'symbol': name, 'expected_source': expected, 'address': hex(symbol['value']),
                     'sample_offsets': offsets, 'addresses': addresses})
    output = review.run('new-runtime-source-lines', [args.addr2line, '-a', '-f', '-C', '-i',
                                                   '-e', elf_path, *queries])
    address_lines = collections.defaultdict(list)
    current = None
    for line in output.splitlines():
        if re.fullmatch(r'0x[0-9a-fA-F]+', line):
            current = int(line, 16)
        elif current is not None:
            address_lines[current].append(line)
    for row in rows:
        addresses = row.pop('addresses')
        resolved = '\n'.join('\n'.join(address_lines[a]) for a in addresses)
        row['generic_addr2line_resolved'] = bool(re.search(re.escape(row['expected_source']) + r':[1-9][0-9]*', resolved))
        row['resolved'] = row['generic_addr2line_resolved']
        row['resolution_method'] = 'generic_addr2line'
        row['output'] = resolved
        if not row['resolved']:
            overrides = []
            for address in addresses:
                mappings = [m for m in source_map['mappings'] if
                            int(m['start'], 16) <= address < int(m['end'], 16) and
                            m['symbol'] == row['symbol'] and
                            Path(m['file']).name == row['expected_source'] and m['line'] > 0]
                if len(mappings) == 1:
                    overrides.append({'address': hex(address), **mappings[0]})
            if len(overrides) == len(addresses):
                row['resolved'] = True
                row['resolution_method'] = 'exact_link_map_CU_with_source_checksum_and_matching_ELF_sidecar'
                row['generic_lookup_ambiguity'] = 'A discarded COMDAT zero-based range from another CU overlaps this real low virtual address; generic LLVM address lookup selects the unrelated CU.'
                row['proved_source_mappings'] = overrides
    review.check('new native runtime points resolve to actual source lines',
                 all(row['resolved'] for row in rows),
                 [row['symbol'] for row in rows if not row['resolved']])
    review.report['source_locations'] = rows


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare', action='store_true')
    parser.add_argument('--elf', type=Path)
    parser.add_argument('--frozen-elf-sha256')
    parser.add_argument('--link-directory', type=Path)
    parser.add_argument('--build-directory', type=Path, default=root / 'out/prospero-current')
    parser.add_argument('--version', default='v3.10', help='Version used for new report paths; historical reports are preserved')
    parser.add_argument('--source-freeze', type=Path)
    parser.add_argument('--report', type=Path)
    for name, default in (('readelf', 'llvm-readelf'), ('nm', 'llvm-nm'), ('objdump', 'llvm-objdump'),
                          ('addr2line', 'llvm-addr2line'), ('ar', 'llvm-ar'), ('ninja', 'ninja'),
                          ('dwarfdump', 'llvm-dwarfdump')):
        parser.add_argument('--' + name, default=default)
    args = parser.parse_args()
    if not re.fullmatch(r'v[0-9]+\.[0-9]+', args.version):
        parser.error('--version must have the form v3.10')
    args.source_freeze = (args.source_freeze or
                         root / f'prospero/validation/source-freeze-{args.version}.json').resolve()
    args.report = args.report or root / f'prospero/validation/target-runtime-{args.version}.json'
    if not args.prepare and (not args.elf or not args.frozen_elf_sha256):
        parser.error('Final review requires --elf and --frozen-elf-sha256 after the build owner freezes it')
    if args.prepare and (args.elf or args.frozen_elf_sha256):
        parser.error('--prepare never inspects an ELF; omit final-image arguments')
    args.build_directory = args.build_directory.resolve()
    args.report = args.report.resolve()
    review = Review(root, args.report, not args.prepare)
    try:
        for name in ('readelf', 'nm', 'objdump', 'addr2line', 'ar', 'ninja', 'dwarfdump'):
            tool = shutil.which(getattr(args, name))
            if not tool:
                raise RuntimeError('Missing required tool: ' + getattr(args, name))
            setattr(args, name, tool)
        by_source = inspect_commands(review, args.build_directory / 'compile_commands.json', not args.prepare)
        critical = {
            'prospero/link.sh', 'prospero/link_support.c', 'prospero/pins.sh',
            'simpsons/src/guest_jump.h',
            'tools/rexglue-sdk/include/rex/platform/signal_thread_prospero.h',
            'tools/rexglue-sdk/include/rex/platform/seh.h',
            'tools/rexglue-sdk/include/rex/platform/exceptions.h',
            'tools/rexglue-sdk/include/rex/graphics/vulkan/pipeline_cache.h',
            'tools/rexglue-sdk/include/rex/graphics/vulkan/pipeline_completion_queue.h',
            'tools/rexglue-sdk/include/rex/system/xthread.h'}
        critical.update(row['source'] for row in review.report['guest_objects'] + review.report['native_objects'])
        review.report['production_source_sha256'] = {
            path: digest(root / path) for path in sorted(critical)}
        if not args.prepare:
            inspect_final(review, args, by_source)
            changed = [path for path, expected in review.report['production_source_sha256'].items()
                       if digest(root / path) != expected]
            review.check('reviewed production sources stayed frozen during binary inspection', not changed, changed)
        else:
            review.report['pending'] = 'Final ELF explicitly not inspected; wait for build-owner freeze and SHA-256.'
    except Exception as error:
        review.report['error'] = str(error)
        print('ERROR', error, flush=True)
    return review.save(not args.prepare)


if __name__ == '__main__':
    raise SystemExit(main())
