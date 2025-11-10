# Sanctuary.SoeProtocol C++ Samples

# Ensure ASIO is available (shared between samples)
find_package(asio QUIET)
if(NOT asio_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-28-0
    )
    FetchContent_MakeAvailable(asio)
    add_library(asio INTERFACE)
    target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
    target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
endif()

# Add sample subdirectories
add_subdirectory(simple_server)
add_subdirectory(single_session_peers)
