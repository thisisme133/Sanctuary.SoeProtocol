# SimpleServer Sample Application
#
# This sample demonstrates how to create a basic SOE protocol server
# that handles login sessions with encryption and compression enabled.

cmake_minimum_required(VERSION 3.25)

# Create the executable
add_executable(simple_server
    main.cpp
    login_application.cpp
    login_application.hpp
    soe_session_worker.cpp
    soe_session_worker.hpp
)

# Link against the SOE protocol library
target_link_libraries(simple_server
    PRIVATE
        sanctuary_soe_protocol
        spdlog::spdlog
        asio
)

# Set C++23 standard
target_compile_features(simple_server PRIVATE cxx_std_23)

# Platform-specific threading support
if(WIN32)
    # Windows doesn't need explicit thread library linking
elseif(UNIX)
    target_link_libraries(simple_server PRIVATE pthread)
endif()

# Installation
install(TARGETS simple_server
    RUNTIME DESTINATION bin/samples
)
