#!/usr/bin/env python3
"""Build the attributed local converter patch without changing Sonic's pin."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


SOURCES = ("native_app_builder.cpp", "self_container.cpp", "elf_object.cpp",
           "sce_module_writer.cpp")
HEADERS = ("self_container.hpp", "elf_object.hpp", "sce_module_writer.hpp", "hash.hpp")
FLAGS = ("-std=c++20", "-O2", "-Wall", "-Wextra")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--compiler", default="g++")
    args = parser.parse_args()
    source = args.reference.resolve() / "tooling/native"
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    patch = Path(__file__).with_name("native-pltgot.patch")
    compiler = shutil.which(args.compiler)
    if not compiler:
        parser.error(f"host C++ compiler not found: {args.compiler}")
    compiler_version = subprocess.check_output([compiler, "--version"])
    digest = hashlib.sha256()
    for path in (Path(__file__), patch, *(source / name for name in SOURCES + HEADERS)):
        digest.update(path.name.encode() + b"\0" + path.read_bytes() + b"\0")
    digest.update(compiler.encode() + b"\0" + compiler_version + repr(FLAGS).encode())
    recipe_hash = digest.hexdigest()
    manifest_path = output.with_name(output.name + ".build.json")
    try:
        previous = json.loads(manifest_path.read_text())
        if (previous["recipe_sha256"] == recipe_hash and os.access(output, os.X_OK)
                and previous["binary_sha256"] == hashlib.sha256(output.read_bytes()).hexdigest()):
            print(f"Native converter current: {recipe_hash}")
            return
    except (OSError, ValueError, KeyError):
        pass
    if not shutil.which("patch"):
        parser.error("the host 'patch' utility is required to build the local converter")
    # The checkout is only read. Keep patched source alongside the host tool
    # so its exact implementation remains available for inspection.
    copied = output.parent / "native-src"
    copied.mkdir(exist_ok=True)
    for name in SOURCES + HEADERS:
        shutil.copy2(source / name, copied / name)
    subprocess.run(["patch", "--batch", "--forward", "-p1", "-i", str(patch.resolve())],
                   cwd=copied, check=True)
    fd, temporary_name = tempfile.mkstemp(prefix=output.name + ".", dir=output.parent)
    os.close(fd)
    temporary = Path(temporary_name)
    try:
        subprocess.run([compiler, *FLAGS, *(str(copied / name) for name in SOURCES),
                        "-lz", "-o", str(temporary)], check=True)
        temporary.chmod(0o755)
        manifest = {
            "recipe_sha256": recipe_hash,
            "binary_sha256": hashlib.sha256(temporary.read_bytes()).hexdigest(),
            "patch_sha256": hashlib.sha256(patch.read_bytes()).hexdigest(),
            "reference": str(args.reference.resolve()),
            "compiler": compiler, "compiler_version": compiler_version.decode().splitlines()[0],
            "sources": list(SOURCES + HEADERS), "flags": list(FLAGS),
        }
        os.replace(temporary, output)
        manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    finally:
        temporary.unlink(missing_ok=True)
    print(f"Built patched native converter: {recipe_hash}")


if __name__ == "__main__":
    main()
