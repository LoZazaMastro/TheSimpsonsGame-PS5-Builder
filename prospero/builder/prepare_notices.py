#!/usr/bin/env python3
"""Retain selected existing PS5 license texts and their exact provenance.

This is a collection of the requested project/ReX/runtime/RADV/Eden notices,
not a claim to be an exhaustive dependency or legal compatibility inventory.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path

MESA = '0b2d6d1a61d9bbf89cf8beb88a696144f67c61f8'
EDEN = '5f142c7926d0c7fcbbd0ce30794d72f638a43b2a'
EDEN_ARCHIVE_SHA256 = '35ec0bb96b197181e66c0aeb8f44f0ff05334be832c631c50a4f1e271277285f'
PROSPERO_EDEN = '8ed1b84ab56e435792d6ad9469b44f3d025e77ba'
SDK = 'b5efad528e0ac1b8289f39d72a0e890ff68ff0a6'


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    builder = Path(__file__).resolve().parent
    root = builder.parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mesa-source', type=Path, required=True)
    parser.add_argument('--eden-source', type=Path, required=True)
    parser.add_argument('--eden-archive', type=Path)
    args = parser.parse_args()
    if args.eden_archive and digest(args.eden_archive.read_bytes()) != EDEN_ARCHIVE_SHA256:
        raise RuntimeError('The Eden archive differs from the recorded source pin')
    assets = builder / 'Assets'
    output = assets / 'licenses'
    output.mkdir(parents=True, exist_ok=True)
    entries = [
        ('The Simpsons Game project', root / 'LICENSE', 'PROJECT-GPL-3.0.txt',
         'TheSimpsonsGameRecomp source repository, LICENSE; this PS5 delivery uses the project license.'),
        ('ReX and its Xenia attribution', root / 'tools/rexglue-sdk/LICENSE', 'REX-BSD-3-CLAUSE.txt',
         'Vendored tools/rexglue-sdk/LICENSE; the original Tom Clay and Xenia copyright notices are retained.'),
        ('LLVM runtime license and exceptions', root / 'prospero/runtime/LICENSE.TXT', 'LLVM-APACHE-2.0-EXCEPTIONS.txt',
         'The license text already supplied with prospero/runtime; LLVM Apache 2.0 with LLVM Exceptions.'),
        ('PS5 SDK license', root / 'prospero/runtime/LICENSE-SDK-GPL-3.0.txt', 'PS5-SDK-GPL-3.0.txt',
         'The SDK license text already retained in prospero/runtime; pinned SDK revision ' + SDK + '.'),
        ('Mesa/RADV license information', args.mesa_source / 'docs/license.rst', 'RADV-MESA-LICENSE-INFORMATION.txt',
         'PS5_Mesa revision ' + MESA + ', docs/license.rst; source-file SPDX notices also apply.'),
        ('Mesa MIT license text', args.mesa_source / 'licenses/MIT', 'RADV-MESA-MIT.txt',
         'PS5_Mesa revision ' + MESA + ', licenses/MIT; retained verbatim from the source used for the VideoOut build.'),
        ('Eden frame-generation source license', args.eden_source / 'LICENSE.txt', 'EDEN-GPL-3.0.txt',
         'Eden revision ' + EDEN + ', LICENSE.txt. Frame-generation source files retain GPL-3.0-or-later SPDX notices, Copyright 2026 Eden Emulator Project and, where present, Copyright 2025 lsfg-vk.'),
    ]
    parts = [
        'THE SIMPSONS GAME PS5 TITLE — SELECTED LICENSE TEXTS AND PROVENANCE\n\n',
        'The Windows builder embeds a compiled PS5 title. This file retains the existing license texts for the project, ReX, LLVM runtime, PS5 SDK, Mesa/RADV and the imported Eden frame-generation sources. Source-file notices are preserved in the corresponding source distribution. This collection is not an exhaustive dependency inventory.\n\n',
        'FRAME-GENERATION SOURCE PROVENANCE\n',
        f'Eden core revision: {EDEN}\n',
        f'Eden source archive SHA256: {EDEN_ARCHIVE_SHA256}\n',
        f'Eden source URL: https://github.com/eden-emulator/mirror/tree/{EDEN}\n',
        f'ProsperoEden port reference revision: {PROSPERO_EDEN}\n',
        'Imported headers retain their original SPDX copyright and GPL-3.0-or-later notices. ReX/PS5 adaptations are identified separately in the source. No proprietary Lossless.dll is included in the builder or title payload; a player may supply a local file.\n\n',
    ]
    recorded = []
    for title, source, name, provenance in entries:
        data = source.read_bytes()
        (output / name).write_bytes(data)
        text = data.decode('utf-8')
        parts.append('=' * 72 + '\n' + title + '\n' + provenance + '\n' + '=' * 72 + '\n\n')
        parts.append(text + ('' if text.endswith('\n') else '\n') + '\n')
        recorded.append({'component': title, 'file': 'licenses/' + name,
                         'source': str(source.relative_to(root)) if source.is_relative_to(root) else str(source),
                         'provenance': provenance, 'bytes': len(data), 'sha256': digest(data)})
    aggregate = ''.join(parts).encode('utf-8')
    (assets / 'PS5-THIRD-PARTY-NOTICES.txt').write_bytes(aggregate)
    manifest = {'schemaVersion': 1, 'scope': __doc__, 'files': recorded,
                'aggregate': {'file': 'PS5-THIRD-PARTY-NOTICES.txt', 'bytes': len(aggregate), 'sha256': digest(aggregate)},
                'pins': {'mesa': MESA, 'sdk': SDK, 'eden': EDEN, 'prosperoEdenReference': PROSPERO_EDEN,
                         'edenArchiveSha256': EDEN_ARCHIVE_SHA256},
                'edenArchiveVerified': args.eden_archive is not None}
    (assets / 'licenses.provenance.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({'status': 'PASS', 'files': len(recorded), 'aggregateSha256': digest(aggregate)}))


if __name__ == '__main__':
    main()
