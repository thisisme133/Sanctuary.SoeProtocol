# SingleSessionPeers Sample Application
#
# This sample demonstrates client-server communication using the SOE protocol
# with a ping-pong throughput test.

cmake_minimum_required(VERSION 3.25)

# Create the executable
add_executable(single_session_peers
    main.cpp
    ping_application.cpp
    ping_application.hpp
    client_worker.cpp
    client_worker.hpp
    server_worker.cpp
    server_worker.hpp
)

# Link against the SOE protocol library
target_link_libraries(single_session_peers
    PRIVATE
        sanctuary_soe_protocol
        spdlog::spdlog
        asio
)

# Set C++23 standard
target_compile_features(single_session_peers PRIVATE cxx_std_23)

# Platform-specific threading support
if(WIN32)
    # Windows doesn't need explicit thread library linking
elseif(UNIX)
    target_link_libraries(single_session_peers PRIVATE pthread)
endif()

# Installation
install(TARGETS single_session_peers
    RUNTIME DESTINATION bin/samples
)
