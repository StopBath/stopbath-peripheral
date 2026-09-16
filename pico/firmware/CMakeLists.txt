# The firmware build. Driven by scripts/build_firmware.sh, which pins the SDK
# and the compiler (scripts/toolchain_versions.env); invoking cmake by hand
# works too provided PICO_SDK_PATH and PICO_TOOLCHAIN_PATH are set the same way.
cmake_minimum_required(VERSION 3.13...3.27)

# The Pico 2 W. Set as a cache default so the Waveshare wiki's note about
# pico versus pico2 builds does not apply: this project has one board.
set(PICO_BOARD pico2_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(stopbath_pico C CXX ASM)

pico_sdk_init()

set(REPOSITORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
# The code both devices compile, reached by relative path and by no other
# means (peripheral spec 2.4).
set(SHARED_ROOT "${REPOSITORY_ROOT}/../shared")

# The Flipper repository's warning set, mirrored in the host Makefile. This
# project's own sources take the full set; vendored code the base set, as the
# Makefile does for the host build.
set(BASE_WARNINGS -Wall -Wextra -Werror -Wstrict-prototypes -Wredundant-decls -Wdouble-promotion -Wundef)
set(PROJECT_WARNINGS ${BASE_WARNINGS} -Wshadow -Wconversion)
set(VENDORED_WARNINGS ${BASE_WARNINGS})

# The shared modules as static libraries, compiled under this device's own
# warning sets (shared/CMakeLists.txt refuses to configure without them).
set(STOPBATH_SHARED_WARNINGS ${PROJECT_WARNINGS})
set(STOPBATH_SHARED_VENDORED_WARNINGS ${VENDORED_WARNINGS})
add_subdirectory(${SHARED_ROOT} shared)
# The images take -ffunction-sections and -fdata-sections as interface
# options of pico_standard_link (pico-sdk src/rp2_common/pico_standard_link/
# CMakeLists.txt, PICO_NO_GC_SECTIONS); the shared libraries are not SDK
# targets, so they take the same two here. Without them the linker's section
# garbage collection cannot drop the encoder's unused entry points and the
# image grows by 4.5 KB (observed 2026-09-16). Linking pico_stdlib into the
# libraries instead would compile the SDK's sources under this project's
# warning set, which is why it is not done that way.
set(SDK_SECTION_OPTIONS -ffunction-sections -fdata-sections)
target_compile_options(stopbath_shared_protocol PRIVATE ${SDK_SECTION_OPTIONS})
target_compile_options(stopbath_shared_link PRIVATE ${SDK_SECTION_OPTIONS})
target_compile_options(stopbath_shared_session PRIVATE ${SDK_SECTION_OPTIONS})
target_compile_options(stopbath_shared_qrcodegen PRIVATE ${SDK_SECTION_OPTIONS})

# Sources every image compiles: the hardware layer, the keys, the panel
# driving, the display refresher, and this device's pure logic modules.
set(SHARED_SOURCES
    DEV_Config.c
    keys.c
    panel.c
    display_refresher.c
    ${REPOSITORY_ROOT}/remote_input/remote_input_model.c
    ${REPOSITORY_ROOT}/remote_display/remote_bitmap.c
    ${REPOSITORY_ROOT}/remote_display/remote_font.c
    ${REPOSITORY_ROOT}/remote_display/remote_display_layout.c
    ${REPOSITORY_ROOT}/remote_display/remote_display_fixtures.c
    ${REPOSITORY_ROOT}/remote_display/remote_refresh_policy.c
    ${REPOSITORY_ROOT}/remote_display/remote_qr.c)

# The link: this device's side of the shared session and the two SDK facing
# files; the protocol library, the session and the transport's decision
# table come from shared/ as libraries. Only the remote links these.
set(LINK_SOURCES
    usb_link.c
    usb_descriptors.c
    ${REPOSITORY_ROOT}/session_device/remote_session_device.c)

set(VENDORED_SOURCES ${REPOSITORY_ROOT}/lib/waveshare/EPD_4in2_V2.c)

# One image per program. All bind the EPD_4in2_V2 driver, the one the
# author observed lighting the panel on 2026-09-15 (evaluation log 4.3).
function(add_firmware_image target_name program_source)
    add_executable(${target_name} ${program_source} ${SHARED_SOURCES} ${ARGN} ${VENDORED_SOURCES})
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${REPOSITORY_ROOT}/lib/waveshare)
    # The QR encoder is drawn by every image's layout; its include path comes
    # with the target.
    target_link_libraries(${target_name} PRIVATE pico_stdlib hardware_spi hardware_gpio hardware_timer stopbath_shared_qrcodegen)
    set_source_files_properties(${program_source} ${SHARED_SOURCES} ${ARGN} PROPERTIES COMPILE_OPTIONS "${PROJECT_WARNINGS}")
    set_source_files_properties(${VENDORED_SOURCES} PROPERTIES COMPILE_OPTIONS "${VENDORED_WARNINGS}")
    # No stdio of any kind: the SDK's USB stdio would translate line endings
    # on the channel the protocol uses (evaluation log 4.1), and there is
    # nothing to print.
    pico_enable_stdio_usb(${target_name} 0)
    pico_enable_stdio_uart(${target_name} 0)
    pico_add_extra_outputs(${target_name})
endfunction()

# KE1: the panel and key check. KE2: every display state and the refresh
# policy. Neither talks to anything.
add_firmware_image(stopbath_pico_first_light first_light.c)
add_firmware_image(stopbath_pico_layout_demo layout_demo.c)

# KE4: the remote itself, speaking the protocol over USB CDC.
add_firmware_image(stopbath_pico remote.c ${LINK_SOURCES})
target_link_libraries(stopbath_pico PRIVATE tinyusb_device tinyusb_board pico_unique_id stopbath_shared_protocol stopbath_shared_session stopbath_shared_link)
