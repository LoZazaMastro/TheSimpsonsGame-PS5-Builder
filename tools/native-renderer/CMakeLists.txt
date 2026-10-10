# Build for the native renderer's offline tools.
#
# These link against the already-built ReXGlue runtime rather than pulling the
# whole SDK into the configure step -- they only need the shader translator,
# and rebuilding the SDK to compile a standalone tool is not worth the wait.
#
#   cmake -S tools/native-renderer -B build/native-renderer \
#         -DREXGLUE_ROOT=tools/rexglue-sdk
#   cmake --build build/native-renderer

cmake_minimum_required(VERSION 3.20)
project(native_renderer_tools CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REXGLUE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../rexglue-sdk"
    CACHE PATH "Root of the ReXGlue SDK checkout")
set(REXGLUE_LIB_DIR "${REXGLUE_ROOT}/out/linux-amd64"
    CACHE PATH "Directory holding the built ReXGlue shared libraries")

# The runtime is built with a debug postfix in the configuration we ship.
find_library(REXGLUE_RUNTIME_LIB
    NAMES rexruntimerd rexruntime
    PATHS "${REXGLUE_LIB_DIR}"
    NO_DEFAULT_PATH)
if(NOT REXGLUE_RUNTIME_LIB)
    message(FATAL_ERROR
        "ReXGlue runtime not found in ${REXGLUE_LIB_DIR}. "
        "Build the SDK first, or pass -DREXGLUE_LIB_DIR=<dir>.")
endif()

add_executable(aot_shaders aot_shaders.cpp)

target_include_directories(aot_shaders PRIVATE
    "${REXGLUE_ROOT}/include"
    "${REXGLUE_ROOT}/thirdparty/fmt/include"
    "${REXGLUE_ROOT}/thirdparty/spdlog/include"
    "${REXGLUE_ROOT}/thirdparty/simde"
    "${REXGLUE_ROOT}/thirdparty/glslang"
    "${REXGLUE_ROOT}/thirdparty/spirv-headers/include"
    "${REXGLUE_ROOT}/thirdparty/renderdoc"
    "${REXGLUE_ROOT}/thirdparty/vulkan-headers/include"
    "${REXGLUE_ROOT}/thirdparty/volk"
    "${REXGLUE_ROOT}/thirdparty"
    "${REXGLUE_ROOT}")

target_link_libraries(aot_shaders PRIVATE "${REXGLUE_RUNTIME_LIB}")
set_target_properties(aot_shaders PROPERTIES
    BUILD_RPATH "${REXGLUE_LIB_DIR}"
    INSTALL_RPATH "${REXGLUE_LIB_DIR}")
