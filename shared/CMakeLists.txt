# The CMake way into shared/: one static library target per consumable
# module, so a device takes the protocol library alone if that is all it
# needs (peripheral spec Part 3). Consumed by add_subdirectory from a device
# build that has already set up its toolchain; never configured on its own.
#
# The warning flags are the consumer's, because the rule is that shared code
# compiles warning clean under every device's flag set (peripheral spec
# 0.16) and only the device knows its set. The consumer sets both variables
# before add_subdirectory; there is no default, so a device that forgets
# fails to configure rather than compiling the shared code under a weaker
# set than its own.
cmake_minimum_required(VERSION 3.13...3.27)

foreach(required_variable STOPBATH_SHARED_WARNINGS STOPBATH_SHARED_VENDORED_WARNINGS)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} must be set by the consuming device build before add_subdirectory(shared)")
    endif()
endforeach()

# The protocol library: parser, encoder and the tables generated from
# protocol.json. Untrusted bytes reach this first (peripheral spec Part 5).
add_library(stopbath_shared_protocol STATIC
    protocol/remote_protocol.c
    protocol/remote_protocol_tables.c)
target_include_directories(stopbath_shared_protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(stopbath_shared_protocol PRIVATE ${STOPBATH_SHARED_WARNINGS})

# The link edge decision table: the pure mapping from the two transport
# facts to session events, with no I/O, which both transports must agree on.
add_library(stopbath_shared_link STATIC link/remote_link_edge.c)
target_include_directories(stopbath_shared_link PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(stopbath_shared_link PRIVATE ${STOPBATH_SHARED_WARNINGS})

# The client session, display agnostic: the device supplies a struct of
# function pointers at initialisation (peripheral spec SD4) and reads the
# held record and the link state back.
add_library(stopbath_shared_session STATIC session/remote_session.c)
target_include_directories(stopbath_shared_session PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(stopbath_shared_session PRIVATE ${STOPBATH_SHARED_WARNINGS})
target_link_libraries(stopbath_shared_session PUBLIC stopbath_shared_protocol)

# The development peer is a host tool and has no target here: nothing under
# peer/ is ever part of a device image (peripheral spec 2.4).

# The vendored QR encoder, held to the vendored warning set as both devices
# already do (lib/qrcodegen/ is upstream's, unmodified; PROVENANCE.md at the
# root). Consumers include <qrcodegen.h>.
add_library(stopbath_shared_qrcodegen STATIC lib/qrcodegen/qrcodegen.c)
target_include_directories(stopbath_shared_qrcodegen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/qrcodegen)
target_compile_options(stopbath_shared_qrcodegen PRIVATE ${STOPBATH_SHARED_VENDORED_WARNINGS})
