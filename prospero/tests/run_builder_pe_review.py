#!/usr/bin/env python3
"""Read the real Windows EXE's bundle, imports and embedded PS5 resources.

No Windows/app code is executed. Bundle v6 layout is specified by Microsoft's
Microsoft.NET.HostModel/Bundle/{Manifest,FileEntry,FileType}.cs. Marker bytes are
the fixed .NET bundle marker also used by ILSpy's SingleFileBundle reader.
"""
from __future__ import annotations
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

SIGNATURE = bytes.fromhex('8b1202b96a612038727b930214d7a03213f5b9e6efae3318ee3b2dce24b36aae')
ROOT = Path(__file__).resolve().parents[2]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def pe_imports(data):
    assert data[:2] == b'MZ'
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    assert data[pe:pe + 4] == b'PE\0\0'
    machine, count = struct.unpack_from('<HH', data, pe + 4)
    optional_size = struct.unpack_from('<H', data, pe + 20)[0]
    optional = pe + 24
    magic = struct.unpack_from('<H', data, optional)[0]
    assert (machine, magic) in ((0x8664, 0x20b), (0x14c, 0x10b))
    sections = []
    for i in range(count):
        virtual_size, rva, size, raw = struct.unpack_from('<IIII', data, optional + optional_size + 40 * i + 8)
        sections.append((rva, size, raw))
    def offset(rva, size=1):
        for begin, stored, raw in sections:
            if begin <= rva and rva + size <= begin + stored:
                result = raw + rva - begin
                assert 0 <= result <= len(data) - size
                return result
        raise ValueError(f'Unmapped PE RVA {rva:x}')
    def string_at(rva):
        off = offset(rva)
        end = data.index(b'\0', off, min(off + 4096, len(data)))
        return data[off:end].decode('ascii')
    directory = optional + (112 if magic == 0x20b else 96)
    imports = []
    import_rva, import_size = struct.unpack_from('<II', data, directory + 8)
    if import_rva:
        begin = offset(import_rva, import_size)
        for off in range(begin, begin + import_size, 20):
            row = struct.unpack_from('<IIIII', data, off)
            if not any(row):
                break
            imports.append(string_at(row[3]).lower())
    delayed = []
    delay_rva, delay_size = struct.unpack_from('<II', data, directory + 13 * 8)
    if delay_rva:
        begin = offset(delay_rva, delay_size)
        for off in range(begin, begin + delay_size, 32):
            row = struct.unpack_from('<8I', data, off)
            if not any(row):
                break
            assert row[0] & 1, 'Expected RVA-based delay descriptor'
            delayed.append(string_at(row[1]).lower())
    return {'machine': hex(machine), 'optional_magic': hex(magic),
            'subsystem': struct.unpack_from('<H', data, optional + 68)[0],
            'imports': sorted(set(imports)), 'delay_imports': sorted(set(delayed))}


def read_bundle(data):
    marker = data.find(SIGNATURE)
    assert marker >= 8 and data.count(SIGNATURE) == 1
    header = struct.unpack_from('<q', data, marker - 8)[0]
    assert 0 < header <= len(data) - 12
    major, minor, count = struct.unpack_from('<IIi', data, header)
    assert major == 6 and minor == 0 and 0 < count < 10000
    cursor = header + 12
    def text():
        nonlocal cursor
        length = 0
        for shift in range(0, 35, 7):
            value = data[cursor]
            cursor += 1
            length |= (value & 127) << shift
            if not value & 128:
                break
        else:
            raise ValueError('Invalid bundle string length')
        assert 0 <= length <= len(data) - cursor
        result = data[cursor:cursor + length].decode('utf-8')
        cursor += length
        return result
    bundle_id = text()
    deps_off, deps_size, config_off, config_size, flags = struct.unpack_from('<qqqqQ', data, cursor)
    cursor += 40
    files = {}
    for _ in range(count):
        offset, size, compressed, kind = struct.unpack_from('<qqqB', data, cursor)
        cursor += 25
        name = text()
        assert name not in files and not name.startswith('/') and '..' not in Path(name).parts
        stored = compressed or size
        assert 0 <= offset <= len(data) - stored and size >= 0 and compressed >= 0
        payload = data[offset:offset + stored]
        if compressed:
            payload = zlib.decompress(payload, -15)
        assert len(payload) == size
        files[name] = {'offset': offset, 'bytes': size, 'compressed_bytes': compressed,
                       'kind': kind, 'sha256': sha(payload), 'data': payload}
    config = json.loads(data[config_off:config_off + config_size])
    deps = json.loads(data[deps_off:deps_off + deps_size])
    return {'version': [major, minor], 'header_offset': header, 'bundle_id': bundle_id,
            'flags': flags, 'configuration': config, 'deps': deps, 'files': files}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('--dotnet', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--assets', type=Path, help='Require exact final embedded resources from this directory')
    parser.add_argument('--ui-framework', choices=['winforms', 'wpf'], default='winforms',
                        help='Require references and UI resources for the actual application framework')
    args = parser.parse_args()
    data = args.exe.read_bytes()
    bundle = read_bundle(data)
    app = pe_imports(data)
    assert app['machine'] == '0x8664' and app['optional_magic'] == '0x20b'
    assert app['subsystem'] == 2, 'Expected Windows GUI executable'
    runtime = bundle['configuration']['runtimeOptions']
    assert 'framework' not in runtime and 'frameworks' not in runtime
    frameworks = {row['name']: row['version'] for row in runtime['includedFrameworks']}
    assert frameworks == {'Microsoft.NETCore.App': '10.0.12', 'Microsoft.WindowsDesktop.App': '10.0.12'}
    required_assemblies = ['TheSimpsonsGame-PS5-Builder.dll', 'Builder.Core.dll', 'System.Private.CoreLib.dll']
    required_assemblies += (['PresentationFramework.dll', 'PresentationCore.dll', 'WindowsBase.dll']
                           if args.ui_framework == 'wpf' else ['System.Windows.Forms.dll'])
    for name in required_assemblies:
        assert name in bundle['files'], name
    # Every PE within the bundle is inspected, including native runtime images
    # that .NET may label as an Assembly rather than NativeBinary.
    pe_files = {'<apphost>': app}
    for name, entry in bundle['files'].items():
        if entry['data'][:2] == b'MZ':
            pe_files[name] = pe_imports(entry['data'])
    with tempfile.TemporaryDirectory(prefix='builder-pe-review-') as temporary:
        work = Path(temporary)
        assembly = work / 'builder.dll'
        assembly.write_bytes(bundle['files']['TheSimpsonsGame-PS5-Builder.dll']['data'])
        command = [str(args.dotnet), 'run', '--project',
                   str(ROOT / 'prospero/tests/builder_pe/ResourceReader.csproj'),
                   '-c', 'Release', '--disable-build-servers',
                   '-p:UseSharedCompilation=false', '-p:BuildInParallel=false',
                   '--', str(assembly), str(work / 'resources')]
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        assert result.returncode == 0, result.stdout
        start = result.stdout.index('{')
        resources = json.loads(result.stdout[start:])
        assert resources['status'] == 'PASS' and not resources['assemblyExecuted']
        references = set(resources['assemblyReferences'])
        if args.ui_framework == 'wpf':
            assert {'PresentationFramework', 'PresentationCore', 'WindowsBase'} <= references
            assert 'System.Windows.Forms' not in references, 'WPF application still references WinForms'
            compiled = {row['name'].lower() for row in resources['compiledMarkup']}
            assert {'app.baml', 'mainwindow.baml', 'aboutwindow.baml', 'colourwindow.baml',
                    'assets/builder.ico'} <= compiled, compiled
        else:
            assert 'System.Windows.Forms' in references
        by_name = {row['name']: row for row in resources['resources']}
        # Check the bytes extracted from the delivered EXE against an independent
        # game identity. Comparing an embedded manifest only to its source asset
        # allowed both copies of the old, incorrect title ID to pass together.
        embedded_manifest = json.loads((work / 'resources/Builder.payload.json').read_bytes())
        assert embedded_manifest.get('xboxTitleId') == '45410809', (
            'The embedded payload manifest must identify The Simpsons Game as Xbox 360 title '
            f"45410809; found {embedded_manifest.get('xboxTitleId')!r}")
        embedded_game_identity = {
            'resource': 'Builder.payload.json',
            'resource_sha256': by_name['Builder.payload.json']['sha256'],
            'xboxTitleId': embedded_manifest['xboxTitleId'],
            'expected_xboxTitleId': '45410809',
            'titleId': embedded_manifest.get('titleId'),
            'gameTitle': embedded_manifest.get('gameTitle'),
            'buildVersion': embedded_manifest.get('buildVersion'),
        }
        matches = {}
        if args.assets:
            for resource, filename in [('Builder.payload.zip', 'payload.zip'),
                                       ('Builder.payload.json', 'payload.json'),
                                       ('Builder.capabilities.json', 'capabilities.json'),
                                       ('Builder.ps5-notices.txt', 'PS5-THIRD-PARTY-NOTICES.txt')]:
                expected = sha((args.assets / filename).read_bytes())
                assert by_name[resource]['sha256'] == expected, resource
                matches[resource] = expected
            assert embedded_manifest['buildVersion'] == '3.10'
    report = {
        'status': 'PASS', 'verified_utc': datetime.now(timezone.utc).isoformat(),
        'scope': 'Read actual PE/bundle/runtime configuration and embedded resources without executing Windows or application code.',
        'exe': {'path': str(args.exe.resolve()), 'bytes': len(data), 'sha256': sha(data)},
        'bundle_version': bundle['version'], 'bundle_header_offset': bundle['header_offset'],
        'bundle_file_count': len(bundle['files']), 'bundle_flags': bundle['flags'],
        'included_frameworks': frameworks, 'external_framework_reference': False,
        'runtime_configuration': bundle['configuration'], 'apphost': app,
        'bundle_files': [{k: v for k, v in entry.items() if k != 'data'} | {'name': name}
                         for name, entry in bundle['files'].items()],
        'pe_imports': pe_files, 'application_resources': resources['resources'],
        'ui_framework': args.ui_framework, 'application_assembly_references': resources['assemblyReferences'],
        'compiled_markup': resources['compiledMarkup'],
        'embedded_game_identity': embedded_game_identity,
        'final_resource_matches': matches, 'windows_gui_execution': 'NOT_RUN',
        'source_sha256': {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in
                          [Path(__file__).resolve(), ROOT / 'prospero/tests/builder_pe/Program.cs',
                           ROOT / 'prospero/tests/builder_pe/ResourceReader.csproj']},
        'format_references': [
            'https://github.com/dotnet/runtime/blob/main/src/installer/managed/Microsoft.NET.HostModel/Bundle/Manifest.cs',
            'https://github.com/dotnet/runtime/blob/main/src/installer/managed/Microsoft.NET.HostModel/Bundle/FileEntry.cs',
            'https://github.com/icsharpcode/ILSpy/blob/master/ICSharpCode.Decompiler/SingleFileBundle.cs'],
        'limits': ['PE imports are static evidence; runtime P/Invoke and Windows behavior are not executed here.',
                   'QA invocation without --assets does not establish final PS5 payload identity.']}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'status': 'PASS', 'exe': report['exe'], 'files': report['bundle_file_count'],
                      'frameworks': frameworks, 'resources': resources['resources'],
                      'embedded_game_identity': embedded_game_identity,
                      'final_resources_checked': len(matches), 'apphost': app}, indent=2))


if __name__ == '__main__':
    main()
