# simpsons - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(simpsons LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Pass-through instrumentation of the guest draw-submission functions (see
# src/draw_telemetry.cpp) is always compiled in and gated at RUNTIME by the
# `draw_telemetry` cvar, which defaults off. It used to be a build option, but
# that made a measurement session require a full rebuild of the 82k recompiled
# functions on the player's machine -- and worse, `option()` writes a CACHE
# entry, so flipping the default could not override the OFF already sitting in
# an existing build directory: the build would silently produce a
# telemetry-free binary that looked correct. With the hooks always present,
# turning the cvar off costs one relaxed atomic load and a branch per draw.
set(SIMPSONS_SOURCES
    src/main.cpp
    src/audio.cpp
    src/crt_native.cpp
    src/draw_telemetry.cpp
    src/eye_shading.cpp
    src/frame_pacing.cpp
    src/freecam.cpp
    src/game_clock.cpp
    src/ink_outlines.cpp
    src/physics_step.cpp
    src/ring_wait.cpp
    src/subtitles.cpp
    src/tick_count.cpp
)

if(WIN32)
    add_executable(simpsons WIN32 ${SIMPSONS_SOURCES})
else()
    add_executable(simpsons ${SIMPSONS_SOURCES})
endif()

rexglue_setup_target(simpsons)

if(REXGLUE_PLATFORM_PROSPERO)
    # One capability contract feeds both the Windows builder and the console
    # menu. Generate immutable UI metadata at configure time, never at runtime.
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    set(PROSPERO_SETTINGS_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/../prospero/builder/Assets/capabilities.json")
    set(PROSPERO_SETTINGS_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/../prospero/tools/embed_settings_capabilities.py")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${PROSPERO_SETTINGS_MANIFEST}" "${PROSPERO_SETTINGS_GENERATOR}")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${PROSPERO_SETTINGS_GENERATOR}"
        "${PROSPERO_SETTINGS_MANIFEST}" "${CMAKE_CURRENT_BINARY_DIR}/prospero_settings_embedded.h"
        COMMAND_ERROR_IS_FATAL ANY)
    target_include_directories(rexui PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    # SDK directory-wide options do not propagate to this parent target.
    # Recompiled PPC code changes the floating-point environment at runtime
    # and views guest memory through multiple types. Match Sonic and the
    # runtime's compiler contract on the game itself, not only SDK objects.
    target_compile_options(simpsons PRIVATE -fno-strict-aliasing -ffp-model=strict)
    enable_language(C)
    set(SONIC_PS5_REFERENCE "$ENV{SONIC_PS5_REFERENCE}" CACHE PATH "Pinned Sonic PS5 reference tools")
    if(NOT EXISTS "${SONIC_PS5_REFERENCE}/tooling/native/app_crt.cpp")
        message(FATAL_ERROR "SONIC_PS5_REFERENCE must point to the pinned reference checkout (run prospero/bootstrap.sh)")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/../prospero/profile.toml")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../prospero/profile.toml" PROSPERO_PROFILE_CONTENT)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../prospero/profile_embedded.h.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/prospero_profile_embedded.h" @ONLY)
    target_sources(simpsons PRIVATE src/prospero_profile.cpp src/prospero_guest_support.cpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../prospero/app_crt.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../prospero/runtime/emutls.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/../prospero/runtime/cxa_prospero.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/../prospero/runtime/threads_prospero.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/../prospero/runtime/heap_prospero.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/../prospero/runtime/heap_wrap_prospero.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/../prospero/runtime/dlmalloc/mspace.c")
    # The host LLVM builtins archive contains a Linux pthread ABI emutls.o.
    # Link a target-built definition explicitly so that member is not pulled
    # from the archive. The matching SDK destructor registry is supplied below.
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../prospero/runtime/emutls.c"
        PROPERTIES COMPILE_OPTIONS "-O2;-fno-plt;-femulated-tls;-fvisibility=hidden;-fno-visibility-from-dllstorageclass;-fno-stack-protector")
    # Publish the matched SDK registry's ready flag with release/acquire so a
    # thread that exits before registering any TLS destructor can inspect it
    # safely. This explicit provider replaces only libps5platform.a(cxa.o).
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../prospero/runtime/cxa_prospero.c"
        PROPERTIES COMPILE_OPTIONS "-O2;-fno-plt;-femulated-tls;-fvisibility=hidden;-fno-visibility-from-dllstorageclass;-fno-stack-protector")
    # Keep the SDK's direct-stack policy for library workers, and extend it
    # to ReX's explicit 4/16 MiB requests. This explicit object replaces only
    # the pinned SDK thread-wrapper archive member, not the SDK as a whole.
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../prospero/runtime/threads_prospero.c"
        PROPERTIES COMPILE_OPTIONS "-O2;-fno-plt;-femulated-tls;-fvisibility=hidden;-fno-visibility-from-dllstorageclass;-fno-stack-protector")
    # Replace the complete mspace implementation, whose private layout must
    # be consistent across all its functions. This object supplies the
    # title's atomic lock and corruption data.
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../prospero/runtime/dlmalloc/mspace.c"
        PROPERTIES COMPILE_OPTIONS "-O2;-fno-plt;-femulated-tls;-fvisibility=hidden;-fno-visibility-from-dllstorageclass;-fno-stack-protector")
    # This complete explicit provider keeps the matched SDK ownership and
    # fallback behavior, with bounded diagnostic events around every API.
    # None of libps5platform.a(heap_wrap.o)'s symbols remain unresolved.
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../prospero/runtime/heap_wrap_prospero.c"
        PROPERTIES COMPILE_OPTIONS "-O2;-fno-plt;-femulated-tls;-fvisibility=hidden;-fno-visibility-from-dllstorageclass;-fno-stack-protector")
    # The matched SDK heap implementation with three diagnostic counters
    # made atomic. Its allocation, ownership and segment-lock policy stays
    # intact; the metrics worker reads these counters without taking locks.
    # This explicit complete provider excludes libps5platform.a(heap.o).
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../prospero/runtime/heap_prospero.c"
        PROPERTIES COMPILE_OPTIONS "-O2;-fno-plt;-femulated-tls;-fvisibility=hidden;-fno-visibility-from-dllstorageclass;-fno-stack-protector")
    set_property(TARGET simpsons APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/../prospero/link.sh")
    # Pre-libc diagnostics must not have their byte loops rewritten as calls
    # to strlen/memcpy, nor depend on C++ runtime initialization.
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/../prospero/app_crt.cpp"
        PROPERTIES COMPILE_OPTIONS "-fno-builtin;-fno-exceptions;-fno-rtti")
    target_include_directories(simpsons PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    set_target_properties(simpsons PROPERTIES SUFFIX ".elf")

    # Keep source locations for native crash diagnostics in the linked debug
    # ELF. These are source-specific metadata flags: retain the existing
    # optimization/FP/aliasing options, and leave generated guest translation
    # units and dependency builds unchanged. TARGET_DIRECTORY selects the
    # actual defining directory for each ReX object/library target.
    function(prospero_append_native_line_tables trace_target)
        get_target_property(trace_sources "${trace_target}" SOURCES)
        get_target_property(trace_directory "${trace_target}" SOURCE_DIR)
        foreach(trace_source IN LISTS trace_sources)
            if(trace_source MATCHES "^\\$<" OR NOT trace_source MATCHES "\\.(c|cc|cpp|cxx)$")
                continue()
            endif()
            get_filename_component(trace_absolute "${trace_source}" ABSOLUTE
                BASE_DIR "${trace_directory}")
            if(trace_absolute MATCHES "/generated/" OR trace_absolute MATCHES "/thirdparty/")
                continue()
            endif()
            set_property(SOURCE "${trace_absolute}" TARGET_DIRECTORY "${trace_target}"
                APPEND PROPERTY COMPILE_OPTIONS "-gline-tables-only")
        endforeach()
    endfunction()
    foreach(trace_target rexcore rexfilesystem rexui rexinput rexaudio rexgraphics rexruntime simpsons)
        if(TARGET "${trace_target}")
            get_target_property(trace_imported "${trace_target}" IMPORTED)
            if(NOT trace_imported)
                prospero_append_native_line_tables("${trace_target}")
            endif()
        endif()
    endforeach()
endif()

# draw_telemetry.cpp calls fmt directly; the prebuilt SDK exposes fmt headers
# transitively, but REXSDK_DIR (add_subdirectory) builds -- what CI uses --
# need the explicit link for the target to see fmt's usage requirements.
if(TARGET fmt::fmt)
    target_link_libraries(simpsons PRIVATE fmt::fmt)
endif()

# The generated translation units are where nearly all guest CPU time goes,
# but CMake's stock RelWithDebInfo only compiles them at -O2, and local builds
# never got the -march=x86-64-v3 that CI already ships in every release
# (build.yml/release.yml pass it on the command line). Apply both directly on
# the target so every build gets them: target_compile_options land after the
# per-config flags on the compile line, so -O3 wins over the default -O2.
# x86-64-v3 (AVX2/BMI2/FMA, Haswell/Zen1+) matches what releases already
# require, so this adds no new hardware exposure. Releases also ship an
# x86-64-v2 build for older CPUs (release.yml sets SIMPSONS_X86_MARCH).
set(SIMPSONS_X86_MARCH "x86-64-v3" CACHE STRING
    "x86-64 microarchitecture level to compile the game for (x86-64-v3 or x86-64-v2)")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU"
   AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    target_compile_options(simpsons PRIVATE
        $<$<CONFIG:RelWithDebInfo,Release>:-O3>
        -march=${SIMPSONS_X86_MARCH}
    )
endif()

# Windows POSIX shims for the few generated TUs that reference <sys/mman.h>
# (mprotect, neutered/no-op) and <unistd.h> (usleep). Scoped to this target.
if(WIN32)
    target_include_directories(simpsons PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/compat_win)
endif()
