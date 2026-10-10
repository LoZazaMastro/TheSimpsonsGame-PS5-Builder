# rexinput - Input system library
# Input system and drivers for Xbox 360 controller emulation

add_library(rexinput OBJECT
    input_system.cpp
    nop/nop_input_driver.cpp
)
add_library(rex::input ALIAS rexinput)

target_include_directories(rexinput PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
if(REXGLUE_PLATFORM_PROSPERO)
    target_sources(rexinput PRIVATE prospero/prospero_input_driver.cpp)
    target_link_libraries(rexinput PRIVATE ScePad SceUserService)
else()
    target_sources(rexinput PRIVATE mnk/mnk_input_driver.cpp sdl/sdl_input_driver.cpp)
    if(WIN32)
        target_sources(rexinput PRIVATE xinput/xinput_input_driver.cpp)
    endif()
    target_link_libraries(rexinput PUBLIC SDL3::SDL3)
endif()

target_link_libraries(rexinput PUBLIC rexcore rexui)
