#!/usr/bin/env bash
# Native PS5 link adapter for CMake's OBJECTS / LINK_LIBRARIES / -o TARGET.
# The RADV link policy derives from Sonic-Reference/tools/radv-link.sh:
# Copyright (C) 2026 Mihawk-99, SPDX-License-Identifier: GPL-3.0-or-later.
# This adaptation uses the current SDK's C++ destructor registry and does not
# generate Sonic's older, conflicting ps5-emutls-cxa implementation.
set -euo pipefail

exec python3 - "$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)" "$@" <<'PY'
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import struct
import subprocess
import sys


def fail(message):
    print(f"PS5 link: {message}", file=sys.stderr)
    raise SystemExit(2)


def required_directory(name):
    value = os.environ.get(name)
    if not value:
        fail(f"set {name}; run prospero/bootstrap.sh before building")
    directory = Path(value).resolve()
    if not directory.is_dir():
        fail(f"{name} is not a directory: {directory}")
    return directory


def require_file(path):
    if not path.is_file():
        fail(f"missing {path}")
    return path


def run(command):
    if os.environ.get("PROSPERO_LINK_VERBOSE") == "1":
        print(shlex.join(map(str, command)), flush=True)
    result = subprocess.run(list(map(str, command)))
    if result.returncode:
        raise SystemExit(result.returncode)


def expand_response_files(arguments, ancestors=()):
    # Parse response files as arguments, never as executable shell text. This
    # also finds rexruntime when Ninja places libraries inside its response.
    expanded = []
    for argument in arguments:
        if argument.startswith("@"):
            path = Path(argument[1:]).resolve()
            if path in ancestors or len(ancestors) >= 16:
                fail(f"recursive linker response file: {path}")
            require_file(path)
            expanded.extend(expand_response_files(shlex.split(path.read_text()),
                                                   (*ancestors, path)))
        else:
            expanded.append(argument)
    return expanded


def llvm_response_arg(argument):
    # LLVM's response-file tokenizer accepts double quotes and escaped slashes.
    return '"' + str(argument).replace('\\', '\\\\').replace('"', '\\"') + '"'


script_directory = Path(sys.argv[1])
sdk = required_directory("PS5_PAYLOAD_SDK")
reference = required_directory("SONIC_PS5_REFERENCE")
archive = Path(os.environ.get(
    "RADV_ARCHIVE", Path(os.environ.get("PROSPERO_DEPS", script_directory.parent / ".deps/prospero")) /
    "radv-display/libvulkan_radeon.ps5.a"
)).resolve()
require_file(archive)
display_receipt_path = require_file(archive.with_suffix(archive.suffix + ".build.json"))
display_receipt = json.loads(display_receipt_path.read_text())
with archive.open("rb") as stream:
    display_archive_sha = hashlib.file_digest(stream, "sha256").hexdigest()
display_source_sha = hashlib.sha256((script_directory / "radv/wsi_common_videoout.c").read_bytes()).hexdigest()
display_recipe_sha = hashlib.sha256((script_directory / "tools/build_radv_display.py").read_bytes()).hexdigest()
if (display_receipt.get("status") != "PASS" or
        display_receipt.get("archive_sha256") != display_archive_sha or
        display_receipt.get("inputs", {}).get("wsi_source") != display_source_sha or
        display_receipt.get("inputs", {}).get("recipe") != display_recipe_sha or
        display_receipt.get("sdk_revision") != (sdk / ".ps5-sdk-revision").read_text().strip() or
        not display_receipt.get("all_other_members_byte_identical")):
    fail("PS5 display archive or source changed; rebuild it with prospero/tools/build_radv_display.py")
linker = require_file(sdk / "bin/prospero-lld")
platform = require_file(sdk / "target/lib/libps5platform.a")
unwind_script = require_file(reference / "tooling/psbc/ps5-pie-unwind.ld")
require_file(reference / "tooling/native/ps5-pie.ld")
compiler_name = os.environ.get("PS5_CLANG", "clang")
compiler = shutil.which(compiler_name)
if not compiler:
    fail(f"compiler not found: {compiler_name}")
resource = subprocess.check_output([compiler, "--print-resource-dir"], text=True).strip()
builtins_candidates = [
    Path(resource) / "lib/linux/libclang_rt.builtins-x86_64.a",
    Path(resource) / "lib/x86_64-unknown-linux-gnu/libclang_rt.builtins.a",
    reference / "vendor/ps5/sdk/libclang_rt.builtins-x86_64.a",
]
builtins = next((path for path in builtins_candidates if path.is_file()), None)
if builtins is None:
    fail("Clang's x86-64 builtins archive is missing; a host libgcc is not a PS5 substitute")

arguments = expand_response_files(sys.argv[2:])
normalized = []
output = None
runtime_seen = False
index = 0
while index < len(arguments):
    argument = arguments[index]
    index += 1
    if argument == "-o":
        if index == len(arguments) or output is not None:
            fail("expected exactly one '-o TARGET'")
        output = Path(arguments[index]).resolve()
        index += 1
    elif argument.startswith("-Wl,"):
        normalized.extend(argument[4:].split(","))
    elif argument == "-Xlinker":
        if index == len(arguments):
            fail("-Xlinker requires an argument")
        normalized.append(arguments[index])
        index += 1
    elif argument in ("-pthread", "-lpthread", "-lm", "-ldl", "-lc", "-lc++", "-lc++abi",
                       "-lunwind"):
        # These desktop driver flags are supplied below by the matched SDK's
        # C++ archives and native system import libraries, not host libraries.
        continue
    elif argument == "-lstdc++":
        fail("host libstdc++ cannot be mixed with the PS5 SDK's libc++ ABI")
    elif Path(argument).name == "librexruntime.a" or argument == "-lrexruntime":
        # HLE export and CVar registration use static initializers that may
        # have no ordinary symbol reference. Keep every ReXGlue object.
        normalized.extend(("--whole-archive", argument, "--no-whole-archive"))
        runtime_seen = True
    else:
        normalized.append(argument)
if output is None:
    fail("missing '-o TARGET'")
output.parent.mkdir(parents=True, exist_ok=True)
work = output.parent / (output.name + ".link")
stubs = work / "stubs"
stubs.mkdir(parents=True, exist_ok=True)

# Recompiled guest setjmp buffers use the matched SDK's known 72-byte
# register layout. Select that exact assembly member, without importing
# Sony's separate setjmp ABI or pulling unrelated libc replacement objects.
sdk_libc = require_file(sdk / "target/lib/libc.a")
archiver = shutil.which(os.environ.get("PS5_AR", "llvm-ar"))
if not archiver:
    fail("llvm-ar is required to select the matched SDK's _setjmp.o")
sdk_jump = work / "sdk_setjmp.o"
jump_bytes = subprocess.check_output([archiver, "p", str(sdk_libc), "_setjmp.o"])
if (len(jump_bytes) < 64 or jump_bytes[:6] != b"\x7fELF\x02\x01" or
        struct.unpack_from("<H", jump_bytes, 18)[0] != 62):
    fail("the matched SDK libc.a does not provide a valid x86-64 _setjmp.o")
sdk_jump.write_bytes(jump_bytes)

c_flags = [compiler, "--target=x86_64-sie-ps5", "-fPIC", "-fno-plt",
           "-fno-stack-protector", "-femulated-tls", "-fvisibility-nodllstorageclass=default",
           "-D__PROSPERO__=1",
           "-isysroot", str(sdk), "-isystem", str(sdk / "target/include"), "-std=c11", "-O2",
           "-gline-tables-only"]
support = work / "link_support.o"
run([*c_flags, "-c", script_directory / "link_support.c", "-o", support])

# These tiny shared objects are import descriptions for the native converter.
# Their placeholder function bodies are never embedded or packaged as code.
stub_sources = {
    "libSceAgc": "agc_canary_link_stub.c",
    "libSceAgcDriver": "agc_driver_canary_link_stub.c",
}
for library, source in stub_sources.items():
    path = require_file(reference / "vendor/ps5/sdk/stubs" / source)
    obj = work / (library + ".o")
    run([*c_flags, "-c", path, "-o", obj])
    run([linker, "--shared", "-soname", library + ".prx", "-o", stubs / (library + ".so"), obj])

wraps = ["pthread_create", "pthread_join", "pthread_detach", "pthread_exit", "fclose", "fflush", "abort",
         "malloc", "calloc", "realloc", "free", "posix_memalign", "aligned_alloc", "memalign",
         "malloc_usable_size", "reallocf", "reallocarray", "getline", "getdelim"]
# Bind only helpers that do not call the same public symbol themselves.
# In particular, ps5_sysconf and ps5_pthread_exit must not be global aliases:
# their native fallback would recurse. Explicit pthread_exit is wrapped above.
aliases = """
qsort_r mkstemps openlog popen pclose open_memstream __xuname __assert
__memset_chk regcomp regexec regfree regerror localtime_r newlocale freelocale
strtod_l strtof_l dladdr utimensat localeconv localeconv_l strtoll_l strtoull_l strtold_l
snprintf_l sscanf_l asprintf_l strcoll_l strxfrm_l strftime_l wcscoll_l wcsxfrm_l
btowc_l wctob_l iswctype_l mbrlen_l mbrtowc_l mbsrtowcs_l mbsnrtowcs_l wcrtomb_l
wcsnrtombs_l mbtowc_l ___mb_cur_max_l ___runetype_l ___tolower_l ___toupper_l
__runes_for_locale catopen catgets catclose backtrace backtrace_symbols_fd
__cxa_thread_atexit_impl ffs timegm
arc4random arc4random_buf arc4random_uniform gmtime_r statvfs fstatvfs
futimens clock_nanosleep getaddrinfo freeaddrinfo if_nameindex if_freenameindex
opendir fdopendir readdir rewinddir dirfd closedir nl_langinfo nl_langinfo_l getpwuid_r
posix_fallocate access openat unlinkat fchmodat fstatat mkdirat renameat memfd_create
isatty mkstemp fork readlink symlink link pathconf
""".split()
definitions = {name: "ps5_" + name for name in aliases}
definitions["___mb_cur_max"] = "ps5____mb_cur_max_l"
visibility = work / "local-symbols.map"
visibility.write_text("{\n  local:\n    _Zn*;\n    _Zd*;\n" +
                      "    _setjmp;\n    ___longjmp;\n    _longjmp;\n" +
                      "    __prospero_linked_entry;\n" +
                      "".join(f"    {name};\n" for name in sorted(definitions)) + "};\n")

libraries = [require_file(sdk / "target/lib" / name)
             for name in ("libc++.a", "libc++abi.a", "libunwind.a")]
# Project policy: use the matched SDK's replacements for these POSIX gaps
# instead of adding optional module dependencies. This is not a universal
# title-loader prohibition: Sonic's published 1.0.3-PS5 binary also imports
# libkernel_sys and libScePosixForWebKit.
private_stubs = {"libkernel_sys.so", "libkernel_web.so", "libScePosixForWebKit.so"}
system_stubs = sorted(path for path in (sdk / "target/lib").glob("*.so")
                      if path.name not in private_stubs)
if not system_stubs:
    fail(f"native system import libraries are missing in {sdk / 'target/lib'}")
link_arguments = [
    "-L", str(reference / "tooling/native"), "-L", str(sdk / "target/lib"),
    "--eh-frame-hdr", "--no-dynamic-linker", "--exclude-libs=ALL",
    "--version-script", str(visibility), "-e", "_start", "-o", str(output),
    "--defsym=__prospero_linked_entry=ABSOLUTE(_start)",
    "-Map=" + str(work / "link-map.txt"),
    *["--wrap=" + name for name in wraps],
    *["--defsym=" + name + "=" + target for name, target in definitions.items()],
    str(support), str(sdk_jump),
    "--start-group", *normalized,
    # RADV dispatch entry points are weakly referenced; archive extraction
    # otherwise leaves apparently available Vulkan functions null at runtime.
    "--whole-archive", str(archive), "--no-whole-archive",
    *map(str, libraries), str(builtins), str(platform), "--end-group",
    "--as-needed", str(stubs / "libSceAgc.so"), str(stubs / "libSceAgcDriver.so"),
    *map(str, system_stubs),
]
response = work / "link.rsp"
response.write_text("\n".join(map(llvm_response_arg, link_arguments)) + "\n")
# -T must be outside the response file: the SDK wrapper scans its own argv to
# decide whether to disable its default payload linker script.
print(f"PS5 link: {output.name} (native C++ runtime, RADV, ReXGlue)", flush=True)
run([linker, "-T", unwind_script, "@" + str(response)])
revision_file = sdk / ".ps5-sdk-revision"
manifest = {
    "output": str(output), "sdk": str(sdk),
    "sdk_revision": revision_file.read_text().strip() if revision_file.is_file() else None,
    "sonic_reference": str(reference), "radv_archive": str(archive),
    "radv_display_receipt": str(display_receipt_path),
    "radv_display_receipt_sha256": hashlib.sha256(display_receipt_path.read_bytes()).hexdigest(),
    "radv_unchanged_gpu_members": display_receipt["unchanged_member_count"],
    "builtins_archive": str(builtins), "runtime_whole_archive": runtime_seen,
    "cxx_tls_destructors": "matched SDK registry with atomic ready publication; explicit cxa_prospero.c; pthread_exit wrapper",
    "thread_stack_policy": "explicit target object; requested usable size backed by direct memory",
    "heap_lock_policy": "explicit mspace object; 32-bit atomic lock with bounded spinning and timed backoff; matched heap algorithm retained locally with atomic diagnostic counters; matched ownership wrappers instrumented locally",
    "image_anchor_policy": "runtime &_start paired with linker ABSOLUTE(_start) immediate; verified in both linked ELF and converted image",
    "abort_diagnostics": "linked-code abort caller captured separately from native signal RIP",
    "guest_jump_policy": "explicit matched SDK libc.a(_setjmp.o); direct returns_twice _setjmp / noreturn ___longjmp; no native setjmp ABI",
    "guest_jump_object": str(sdk_jump),
    "guest_jump_object_sha256": hashlib.sha256(jump_bytes).hexdigest(),
    "map_file": str(work / "link-map.txt"),
    "linker_script": str(unwind_script), "response_file": str(response),
    "native_import_stubs": [str(stubs / (name + ".so")) for name in stub_sources],
    "excluded_private_system_stubs": sorted(private_stubs),
}
(work / "link-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
PY
