#!/usr/bin/env python3
"""Embed a previously built and verified PS5 title in the Windows builder.

This packaging step never includes gamedata, saves, caches or game logs.
No conversion or rewriting of the PS5 executable/metadata takes place here.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile
import zipfile


def prepare(title: Path, assets: Path, version: str) -> dict:
    title = title.resolve(strict=True)
    assets.mkdir(parents=True, exist_ok=True)
    capability = json.loads((assets / "capabilities.json").read_text(encoding="utf-8"))
    if capability["titleId"] != "PPSA74207" or capability["buildVersion"] != version:
        raise ValueError("Capability manifest does not match PPSA74207 / requested version")
    metadata = json.loads((title / "sce_sys/param.json").read_text(encoding="utf-8"))
    if metadata["titleId"] != "PPSA74207" or metadata["downloadDataSize"] != 256:
        raise ValueError("Unexpected PS5 title identity / downloadDataSize")
    if not re.fullmatch(r"[A-Z]{2}[0-9]{4}-PPSA74207_00-[A-Z0-9]{16}", metadata["contentId"]):
        raise ValueError("Unexpected PS5 content ID")
    expected_version = ".".join(["00"] + [f"{int(n):03d}" for n in version.split(".")])
    if metadata["contentVersion"] != expected_version:
        raise ValueError(f"Payload is version {metadata['contentVersion']}, expected {expected_version}")
    localized = [v["titleName"] for v in metadata["localizedParameters"].values() if isinstance(v, dict) and "titleName" in v]
    if not localized or any(name != "The Simpsons Game" for name in localized):
        raise ValueError("Unexpected title name")
    required = ["eboot.bin", "sce_module/libc.prx", "sce_sys/param.json", "sce_sys/icon0.png",
                "sce_sys/pic0.dds", "sce_sys/pic1.dds", "sce_sys/snd0.at9"]
    paths: list[Path] = [title / "eboot.bin"]
    for folder in (title / "sce_sys", title / "sce_module"):
        paths.extend(p for p in folder.rglob("*") if p.is_file())
    relative_names: set[str] = set()
    for path in paths:
        relative = path.relative_to(title).as_posix()
        if relative.casefold() in relative_names or path.is_symlink() or not path.is_file() or path.stat().st_size <= 0:
            raise ValueError(f"Invalid or duplicate payload file: {relative}")
        relative_names.add(relative.casefold())
        for ancestor in path.relative_to(title).parents:
            if (title / ancestor).is_symlink():
                raise ValueError(f"Symlink in payload path: {relative}")
    if not set(required).issubset(relative_names):
        raise ValueError("Missing required PS5 files")
    rows = []
    fd, temporary = tempfile.mkstemp(prefix="payload-", suffix=".zip.tmp", dir=assets)
    os.close(fd)
    temp_zip = Path(temporary)
    try:
        with zipfile.ZipFile(temp_zip, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as archive:
            for path in sorted(paths, key=lambda p: p.relative_to(title).as_posix()):
                relative = path.relative_to(title).as_posix()
                entry = zipfile.ZipInfo(relative, date_time=(2026, 1, 1, 0, 0, 0))
                entry.compress_type = zipfile.ZIP_DEFLATED
                entry.external_attr = (0o100755 if relative == "eboot.bin" or relative.startswith("sce_module/") else 0o100644) << 16
                digest = hashlib.sha256()
                count = 0
                with path.open("rb") as source, archive.open(entry, "w", force_zip64=True) as output:
                    while chunk := source.read(1024 * 1024):
                        output.write(chunk)
                        digest.update(chunk)
                        count += len(chunk)
                if count != path.stat().st_size:
                    raise ValueError(f"Payload source changed during packaging: {relative}")
                rows.append({"path": relative, "bytes": count, "sha256": digest.hexdigest()})
        with zipfile.ZipFile(temp_zip) as archive:
            if archive.testzip() is not None:
                raise ValueError("Generated payload ZIP failed its integrity test")
        manifest = {"schemaVersion": 1, "titleId": "PPSA74207", "gameTitle": "The Simpsons Game",
                    "buildVersion": version, "xboxTitleId": "45410809", "files": rows,
                    "emptyDirectories": ["logs", "cache", "userdata"]}
        temporary_manifest = assets / "payload.json.tmp"
        temporary_manifest.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        os.replace(temp_zip, assets / "payload.zip")
        os.replace(temporary_manifest, assets / "payload.json")
        return {"status": "PASS", "title_directory": str(title), "files": len(rows),
                "uncompressed_bytes": sum(r["bytes"] for r in rows),
                "zip_sha256": hashlib.file_digest((assets / "payload.zip").open("rb"), "sha256").hexdigest(),
                "manifest_sha256": hashlib.sha256((assets / "payload.json").read_bytes()).hexdigest(),
                "eboot_sha256": next(r["sha256"] for r in rows if r["path"] == "eboot.bin")}
    finally:
        temp_zip.unlink(missing_ok=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--title-dir", type=Path, required=True)
    parser.add_argument("--assets-dir", type=Path, default=Path(__file__).resolve().parent / "Assets")
    parser.add_argument("--version", default="3.10")
    args = parser.parse_args()
    print(json.dumps(prepare(args.title_dir, args.assets_dir, args.version), indent=2))


if __name__ == "__main__":
    main()
