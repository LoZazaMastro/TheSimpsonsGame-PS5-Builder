#!/usr/bin/env python3
"""Cross-link a small PS5 C++/RADV title; this does not execute it.

Requires the same PS5_PAYLOAD_SDK and SONIC_PS5_REFERENCE as the real build.
Uses the real pinned RADV archive, including weak Vulkan dispatch entries.
"""
import os
from pathlib import Path
import struct
import subprocess


def run(*command, **kwargs):
    subprocess.run(list(map(str, command)), check=True, **kwargs)


root = Path(__file__).resolve().parents[2]
sdk = Path(os.environ["PS5_PAYLOAD_SDK"]).resolve()
reference = Path(os.environ["SONIC_PS5_REFERENCE"]).resolve()
work = root / "out/prospero-link-smoke"
work.mkdir(parents=True, exist_ok=True)
source = work / "smoke.cpp"
source.write_text(r'''
#include <stdexcept>
#include <string>
#include <thread>
#include <setjmp.h>
// This link-only fixture does not include the game AudioOut backend. Its
// snapshot implementation is verified in the full title and dedicated tests.
extern "C" void prospero_audio_report_diagnostics(void) noexcept {}
extern "C" int _setjmp(jmp_buf) __attribute__((returns_twice));
extern "C" void ___longjmp(jmp_buf, int) __attribute__((noreturn));
using VulkanProc = void (*)();
extern "C" VulkanProc vk_icdGetInstanceProcAddr(void*, const char*);
int registrations = 0;
int destructors = 0;
struct ThreadValue {
    std::string value;
    ~ThreadValue() { ++destructors; }
};
thread_local ThreadValue thread_value;
int main() {
    jmp_buf jump;
    const int resumed = _setjmp(jump);
    if (resumed == 0) ___longjmp(jump, 7);
    if (resumed != 7) return 3;
    if (!vk_icdGetInstanceProcAddr(nullptr, "vkCreateInstance") ||
        !vk_icdGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties")) {
        return 2;
    }
    std::thread worker([] {
        thread_value.value = "PS5 C++ thread-local destructor canary";
        try { throw std::runtime_error(thread_value.value); }
        catch (const std::exception&) { thread_value.value += " caught"; }
    });
    worker.join();
    return registrations == 1 && destructors == 1 ? 0 : 1;
}
''')
registration = work / "registration.cpp"
registration.write_text('extern int registrations;\n'
                        'extern "C" __attribute__((constructor)) '
                        'void registration_canary() { ++registrations; }\n')
compiler = os.environ.get("PS5_CLANGXX", "clang++")
flags = ["--target=x86_64-sie-ps5", "-fPIC", "-fno-plt", "-fno-stack-protector",
         "-femulated-tls", "-fvisibility-nodllstorageclass=default", "-fexceptions",
         "-fcxx-exceptions", "-frtti", "-std=c++23", "-O1", "-isysroot", sdk,
         "-isystem", sdk / "target/include/c++/v1", "-isystem", sdk / "target/include"]
for name, path in (("smoke", source), ("registration", registration),
                   ("app_crt", root / "prospero/app_crt.cpp")):
    extra = ["-fno-builtin", "-fno-exceptions", "-fno-rtti"] if name == "app_crt" else []
    run(compiler, *flags, *extra, "-c", path, "-o", work / (name + ".o"))
# Compile the same LLVM emutls source as the real title with the actual PS5
# pthread headers. A host-built emutls.o can link yet use incompatible once and
# mutex layouts at runtime; it must never satisfy this executable's TLS calls.
emutls = work / "emutls-prospero.o"
run(os.environ.get("PS5_CLANG", "clang"), "--target=x86_64-sie-ps5",
    "-fPIC", "-fno-plt", "-fno-stack-protector", "-femulated-tls",
    "-fvisibility=hidden", "-fno-visibility-from-dllstorageclass",
    "-D__PROSPERO__=1", "-std=c11", "-O2", "-isysroot", sdk,
    "-isystem", sdk / "target/include", "-c", root / "prospero/runtime/emutls.c",
    "-o", emutls)
cxa = work / "cxa-prospero.o"
run(os.environ.get("PS5_CLANG", "clang"), "--target=x86_64-sie-ps5",
    "-fPIC", "-fno-plt", "-fno-stack-protector", "-femulated-tls",
    "-fvisibility=hidden", "-fno-visibility-from-dllstorageclass",
    "-D__PROSPERO__=1", "-std=c11", "-O2", "-isysroot", sdk,
    "-isystem", sdk / "target/include", "-c", root / "prospero/runtime/cxa_prospero.c",
    "-o", cxa)
# The current link policy wraps abort. The smoke title must include the same
# reporter and terminal wrapper as the game, rather than supplying a dummy
# symbol or silently omitting the required runtime contract.
diagnostics = []
heap = work / "heap-prospero.o"
run(os.environ.get("PS5_CLANG", "clang"), "--target=x86_64-sie-ps5",
    "-fPIC", "-fno-plt", "-fno-stack-protector", "-femulated-tls",
    "-fvisibility=hidden", "-fno-visibility-from-dllstorageclass",
    "-D__PROSPERO__=1", "-std=c11", "-O2", "-isysroot", sdk,
    "-isystem", sdk / "target/include", "-c", root / "prospero/runtime/heap_prospero.c",
    "-o", heap)
diagnostics.append(heap)
for name in ("fault_prospero", "abort_prospero", "trace_prospero", "metrics_prospero",
             "signal_thread_prospero"):
    obj = work / (name + ".o")
    run(compiler, *flags, "-D__PROSPERO__=1", "-I", root / "tools/rexglue-sdk/include",
        "-c", root / "tools/rexglue-sdk/src/core" / (name + ".cpp"), "-o", obj)
    diagnostics.append(obj)
runtime = work / "librexruntime.a"
radv = Path(os.environ.get("RADV_ARCHIVE", reference /
                          ".deps/native/radv-release/lib/libvulkan_radeon.ps5.a")).resolve()
if not radv.is_file():
    raise FileNotFoundError(f"build the pinned RADV archive before this check: {radv}")
run("llvm-ar", "rcs", runtime, work / "registration.o")
# Put the otherwise unreferenced registration library into a response file
# to verify both response parsing and whole-archive behavior.
objects = work / "objects.rsp"
objects.write_text("\n".join('"' + str(p) + '"' for p in
                              (work / "smoke.o", work / "app_crt.o", emutls, cxa, *diagnostics, runtime)) + "\n")
target = work / "smoke.elf"
environment = dict(os.environ, RADV_ARCHIVE=str(radv))
run("bash", root / "prospero/link.sh", "@" + str(objects), "-o", target, env=environment)
symbols = subprocess.check_output(["llvm-nm", "-S", str(target)], text=True)
for name in ("registration_canary", "__emutls_get_address", "ps5p_run_thread_destructors",
             "ps5___cxa_thread_atexit_impl", "__wrap_pthread_exit", "__eh_frame_start",
             "vk_icdGetInstanceProcAddr", "radv_CreateInstance", "prospero_startup_log",
             "ps5_pathconf", "__wrap_abort", "prospero_heap_failure", "_setjmp", "___longjmp"):
    matches = [line for line in symbols.splitlines() if line.split()[-1:] == [name]]
    if len(matches) != 1 or matches[0].split()[-2].upper() == "U":
        raise RuntimeError(f"expected one defined native symbol: {name}: {matches}")
for name in ("_setjmp", "___longjmp"):
    if any(line.split()[-2:] == ["U", name] for line in symbols.splitlines()):
        raise RuntimeError(f"guest context jump still imports a native symbol: {name}")
# The matched Sonic RADV still uses its own native setjmp/longjmp imports
# in gl_spirv.c.o / spirv_to_nir.c.o. They are outside the guest replacement.
# These named objects expose the exact ABI error that a host emutls archive
# silently introduced. Check the final linked image, not only the source's
# static assertions, so accidental extraction of that archive member fails.
for name, expected_size in (("emutls_init_once.once", 16), ("emutls_mutex", 8),
                            ("emutls_pthread_key", 4)):
    matches = [line.split() for line in symbols.splitlines()
               if line.split()[-1:] == [name]]
    if len(matches) != 1 or len(matches[0]) != 4 or int(matches[0][1], 16) != expected_size:
        raise RuntimeError(f"wrong PS5 emutls ABI for {name}: {matches}")
object_symbols = subprocess.check_output(["readelf", "-sW", str(emutls)], text=True)
entries = [line.split() for line in object_symbols.splitlines()
           if line.split()[-1:] == ["__emutls_get_address"]]
if len(entries) != 1 or entries[0][3:6] != ["FUNC", "GLOBAL", "HIDDEN"]:
    raise RuntimeError(f"expected a strong hidden explicit emutls definition: {entries}")
elf = target.read_bytes()
if elf[:6] != b"\x7fELF\x02\x01" or struct.unpack_from("<H", elf, 16)[0] != 3:
    raise RuntimeError("expected a little-endian 64-bit PIE")
program_offset = struct.unpack_from("<Q", elf, 32)[0]
program_size, program_count = struct.unpack_from("<HH", elf, 54)
program_types = [struct.unpack_from("<I", elf, program_offset + index * program_size)[0]
                 for index in range(program_count)]
if 3 in program_types:
    raise RuntimeError("expected a PIE with no desktop program interpreter")
for index, kind in enumerate(program_types):
    if kind != 2:  # PT_DYNAMIC
        continue
    header = program_offset + index * program_size
    offset, _, _, size = struct.unpack_from("<4Q", elf, header + 8)
    for at in range(offset, offset + size, 16):
        tag, value = struct.unpack_from("<qQ", elf, at)
        if tag == 0:
            break
        if tag == 2 and value:  # DT_PLTRELSZ
            raise RuntimeError("PS5 smoke link unexpectedly extracted PLT-based runtime calls")
print(f"PASS: PS5 C++/TLS/unwind/RADV link and retained archive constructor: {target}")
print("One explicit target TLS runtime, PS5 pthread object sizes, and no PLT calls verified.")
print("The real RADV archive was linked. The executable was not run; no hardware claim is made.")
