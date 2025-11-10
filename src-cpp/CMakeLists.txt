cmake_minimum_required(VERSION 3.25)
project(SanctuarySoeProtocol
    VERSION 6.1.1
    DESCRIPTION "C++23 implementation of the SOE Protocol"
    LANGUAGES CXX)

# C++23 standard required
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_SAMPLES "Build sample applications" ON)
option(ENABLE_IPO "Enable interprocedural optimization" ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Find required packages
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

# FetchContent for dependencies
include(FetchContent)

# Google Test for unit tests
if(BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

# spdlog for logging (replaces Microsoft.Extensions.Logging)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Main library
add_library(sanctuary_soe_protocol)

# Library sources
target_sources(sanctuary_soe_protocol
    PRIVATE
        # Core
        src/endpoint.cpp
        src/soe_protocol_handler.cpp
        src/soe_socket_handler.cpp

        # Objects
        src/objects/application_parameters.cpp
        src/objects/session_parameters.cpp
        src/objects/rc4_key_state.cpp
        src/objects/native_span.cpp

        # Packets
        src/objects/packets/session_request.cpp
        src/objects/packets/session_response.cpp
        src/objects/packets/disconnect.cpp
        src/objects/packets/acknowledge.cpp
        src/objects/packets/acknowledge_all.cpp
        src/objects/packets/remap_connection.cpp
        src/objects/packets/unknown_sender.cpp

        # Services
        src/services/crc32.cpp
        src/services/rc4_cipher.cpp
        src/services/reliable_data_input_channel.cpp
        src/services/reliable_data_output_channel.cpp
        src/services/reliable_data_output_channel2.cpp
        src/services/udp_socket_network_interface.cpp
        src/services/socket_network_writer.cpp

        # Utilities
        src/util/data_utils.cpp
        src/util/soe_packet_utils.cpp
        src/util/multi_packet_utils.cpp
        src/util/native_span_pool.cpp

        # Zlib
        src/util/zlib/zlib_exception.cpp
        src/util/zlib/zlib_deflator.cpp
        src/util/zlib/zlib_inflater.cpp

    PUBLIC
        # Headers
        include/sanctuary/soe_protocol/endpoint.hpp
        include/sanctuary/soe_protocol/soe_protocol_handler.hpp
        include/sanctuary/soe_protocol/soe_socket_handler.hpp
        include/sanctuary/soe_protocol/soe_op_code.hpp
        include/sanctuary/soe_protocol/soe_constants.hpp

        # Abstractions
        include/sanctuary/soe_protocol/abstractions/application_protocol_handler.hpp
        include/sanctuary/soe_protocol/abstractions/session_handler.hpp
        include/sanctuary/soe_protocol/abstractions/soe_session.hpp
        include/sanctuary/soe_protocol/abstractions/services/network_interface.hpp
        include/sanctuary/soe_protocol/abstractions/services/network_reader.hpp
        include/sanctuary/soe_protocol/abstractions/services/network_writer.hpp

        # Objects
        include/sanctuary/soe_protocol/objects/application_parameters.hpp
        include/sanctuary/soe_protocol/objects/session_parameters.hpp
        include/sanctuary/soe_protocol/objects/session_state.hpp
        include/sanctuary/soe_protocol/objects/session_mode.hpp
        include/sanctuary/soe_protocol/objects/rc4_key_state.hpp
        include/sanctuary/soe_protocol/objects/native_span.hpp
        include/sanctuary/soe_protocol/objects/socket_handler_params.hpp
        include/sanctuary/soe_protocol/objects/data_input_stats.hpp
        include/sanctuary/soe_protocol/objects/data_output_stats.hpp
        include/sanctuary/soe_protocol/objects/soe_packet_validation_result.hpp

        # Packets
        include/sanctuary/soe_protocol/objects/packets/session_request.hpp
        include/sanctuary/soe_protocol/objects/packets/session_response.hpp
        include/sanctuary/soe_protocol/objects/packets/disconnect.hpp
        include/sanctuary/soe_protocol/objects/packets/acknowledge.hpp
        include/sanctuary/soe_protocol/objects/packets/acknowledge_all.hpp
        include/sanctuary/soe_protocol/objects/packets/remap_connection.hpp
        include/sanctuary/soe_protocol/objects/packets/unknown_sender.hpp

        # Services
        include/sanctuary/soe_protocol/services/crc32.hpp
        include/sanctuary/soe_protocol/services/rc4_cipher.hpp
        include/sanctuary/soe_protocol/services/reliable_data_input_channel.hpp
        include/sanctuary/soe_protocol/services/reliable_data_output_channel.hpp
        include/sanctuary/soe_protocol/services/reliable_data_output_channel2.hpp
        include/sanctuary/soe_protocol/services/udp_socket_network_interface.hpp
        include/sanctuary/soe_protocol/services/socket_network_writer.hpp

        # Utilities
        include/sanctuary/soe_protocol/util/data_utils.hpp
        include/sanctuary/soe_protocol/util/soe_packet_utils.hpp
        include/sanctuary/soe_protocol/util/multi_packet_utils.hpp
        include/sanctuary/soe_protocol/util/sliding_window_array.hpp
        include/sanctuary/soe_protocol/util/native_span_pool.hpp

        # Zlib
        include/sanctuary/soe_protocol/util/zlib/zlib.hpp
        include/sanctuary/soe_protocol/util/zlib/zlib_deflator.hpp
        include/sanctuary/soe_protocol/util/zlib/zlib_inflater.hpp
        include/sanctuary/soe_protocol/util/zlib/zlib_exception.hpp
        include/sanctuary/soe_protocol/util/zlib/zlib_types.hpp

        # Extensions
        include/sanctuary/soe_protocol/extensions/extensions.hpp
        include/sanctuary/soe_protocol/extensions/span_extensions.hpp
        include/sanctuary/soe_protocol/extensions/binary_primitives_extensions.hpp
        include/sanctuary/soe_protocol/extensions/binary_primitive_helper_extensions.hpp
)

# Include directories
target_include_directories(sanctuary_soe_protocol
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link libraries
target_link_libraries(sanctuary_soe_protocol
    PUBLIC
        ZLIB::ZLIB
    PRIVATE
        spdlog::spdlog
        Threads::Threads
)

# Interprocedural optimization
if(ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set_property(TARGET sanctuary_soe_protocol PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

# Tests
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Samples
if(BUILD_SAMPLES)
    add_subdirectory(samples)
endif()

# Installation (commented out for now - needs fixing for spdlog dependency)
# install(TARGETS sanctuary_soe_protocol
#     EXPORT SanctuarySoeProtocolTargets
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib
#     RUNTIME DESTINATION bin
#     INCLUDES DESTINATION include
# )
#
# install(DIRECTORY include/
#     DESTINATION include
# )
#
# install(EXPORT SanctuarySoeProtocolTargets
#     FILE SanctuarySoeProtocolTargets.cmake
#     NAMESPACE Sanctuary::
#     DESTINATION lib/cmake/SanctuarySoeProtocol
# )
#
# # Package configuration
# include(CMakePackageConfigHelpers)
# write_basic_package_version_file(
#     "${CMAKE_CURRENT_BINARY_DIR}/SanctuarySoeProtocolConfigVersion.cmake"
#     VERSION ${PROJECT_VERSION}
#     COMPATIBILITY AnyNewerVersion
# )
#
# configure_package_config_file(
#     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SanctuarySoeProtocolConfig.cmake.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/SanctuarySoeProtocolConfig.cmake"
#     INSTALL_DESTINATION lib/cmake/SanctuarySoeProtocol
# )
#
# install(FILES
#     "${CMAKE_CURRENT_BINARY_DIR}/SanctuarySoeProtocolConfig.cmake"
#     "${CMAKE_CURRENT_BINARY_DIR}/SanctuarySoeProtocolConfigVersion.cmake"
#     DESTINATION lib/cmake/SanctuarySoeProtocol
# )
