cmake_minimum_required(VERSION 3.15)
project(extract-xiso)

string(TOUPPER __${CMAKE_SYSTEM_NAME}__ TARGET_OS)

set(SOURCE_FILES
	extract-xiso.c
)

add_executable(extract-xiso ${SOURCE_FILES})
target_compile_definitions(extract-xiso PRIVATE ${TARGET_OS})

if(WIN32)
	# Statically link the C runtime so extract-xiso.exe runs on a bare Windows
	# install. The default dynamic linkage needs ucrtbase.dll/vcruntime140.dll
	# from the Microsoft Visual C++ Redistributable, which isn't present on
	# every player's machine -- it fails to launch with "The code execution
	# cannot proceed because ucrtbase.dll was not found." Setting this property
	# (instead of passing -fms-runtime-lib=static as a flag) replaces CMake's
	# own default /MD-equivalent flags instead of stacking on top of them --
	# passing the flag directly left an earlier, CMake-injected -D_DLL in
	# place, which desynced the compiled object (dllimport-style references)
	# from the statically-linked runtime, and the link failed outright.
	set_property(TARGET extract-xiso PROPERTY
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

install(TARGETS extract-xiso RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
