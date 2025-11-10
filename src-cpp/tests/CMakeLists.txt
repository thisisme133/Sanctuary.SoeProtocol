cmake_minimum_required(VERSION 3.14)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Collect all test files
set(PACKET_TESTS
    Objects/Packets/SessionRequestTests.cpp
    Objects/Packets/SessionResponseTests.cpp
    Objects/Packets/AcknowledgeTests.cpp
    Objects/Packets/AcknowledgeAllTests.cpp
    Objects/Packets/DisconnectTests.cpp
    Objects/Packets/RemapConnectionTests.cpp
)

set(SERVICE_TESTS
    Services/Crc32Tests.cpp
    Services/Rc4CipherTests.cpp
    Services/ReliableDataChannelEndToEndTests.cpp
    Services/ReliableDataInputChannelTests.cpp
    Services/ReliableDataOutputChannelTests.cpp
)

set(UTIL_TESTS
    Util/SoePacketUtilsTests.cpp
    Util/SlidingWindowArrayTests.cpp
)

set(ALL_TESTS
    ${PACKET_TESTS}
    ${SERVICE_TESTS}
    ${UTIL_TESTS}
)

# Create test executable
add_executable(
    SoeProtocolTests
    ${ALL_TESTS}
)

# Link against GoogleTest and the main library
target_link_libraries(
    SoeProtocolTests
    GTest::gtest_main
    SanctuarySoeProtocol
)

# Add test discovery
include(GoogleTest)
gtest_discover_tests(SoeProtocolTests)

# Optional: Create individual test executables for each test suite
# This allows running specific test suites independently

# Packet Tests
add_executable(PacketTests ${PACKET_TESTS})
target_link_libraries(PacketTests GTest::gtest_main SanctuarySoeProtocol)
gtest_discover_tests(PacketTests)

# Service Tests
add_executable(ServiceTests ${SERVICE_TESTS})
target_link_libraries(ServiceTests GTest::gtest_main SanctuarySoeProtocol)
gtest_discover_tests(ServiceTests)

# Util Tests
add_executable(UtilTests ${UTIL_TESTS})
target_link_libraries(UtilTests GTest::gtest_main SanctuarySoeProtocol)
gtest_discover_tests(UtilTests)
