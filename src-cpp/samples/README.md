# Sanctuary.SoeProtocol C++ Samples

This directory contains C++23 sample applications demonstrating how to use the Sanctuary.SoeProtocol library.

## Available Samples

### 1. SimpleServer
A basic SOE protocol server that demonstrates:
- Server setup and configuration
- Session management with encryption
- Application data packet handling
- Graceful shutdown

**Location**: `simple_server/`
**See**: [simple_server/README.md](simple_server/README.md)

### 2. SingleSessionPeers
A client-server ping-pong throughput test that demonstrates:
- Both client and server implementations
- Bidirectional communication
- Performance measurement
- Automatic session termination

**Location**: `single_session_peers/`
**See**: [single_session_peers/README.md](single_session_peers/README.md)

## Building the Samples

### Prerequisites
- C++23 compatible compiler (GCC 11+, Clang 15+, MSVC 2022+)
- CMake 3.25 or higher
- ASIO (automatically fetched if not available)
- spdlog (automatically fetched)
- ZLIB

### Build All Samples

From the repository root:

```bash
mkdir build && cd build
cmake .. -DBUILD_SAMPLES=ON
cmake --build .
```

### Build Individual Samples

```bash
cmake --build . --target simple_server
cmake --build . --target single_session_peers
```

### Run Samples

After building, executables are in:
```bash
./simple_server           # From build directory
./single_session_peers    # From build directory
```

## C++23 Features Used

These samples utilize modern C++23 features including:
- **Structured bindings**: For clean code organization
- **`std::span`**: For safe memory views
- **`std::byte`**: For type-safe byte handling
- **Concepts and constraints**: Via `[[nodiscard]]` and type safety
- **Move semantics**: For efficient resource management
- **Smart pointers**: For automatic memory management
- **Standard threading**: Using `std::thread` and `std::atomic`

## Logging

All samples use **spdlog** for logging, replacing the C# Microsoft.Extensions.Logging:
- Structured, fast logging
- Colored console output
- Configurable log levels
- Thread-safe operations

## Design Patterns

### Dependency Injection
While C++ doesn't have built-in DI like C#'s `IServiceProvider`, these samples use:
- Factory functions for object creation
- Constructor injection of dependencies
- Shared pointers for shared ownership

### Background Workers
C# `BackgroundService` is replaced with:
- Custom worker classes
- `std::thread` for background execution
- `std::atomic` for thread-safe stop flags

### Session Management
Similar to the C# implementation:
- `application_protocol_handler` interface for custom protocols
- `session_handler` for session control
- Event-driven architecture (callbacks)

## Converting from C# to C++

If you're familiar with the C# samples, here are the key mappings:

| C# | C++ |
|---|---|
| `IApplicationProtocolHandler` | `application_protocol_handler` |
| `ISessionHandler` | `session_handler` |
| `BackgroundService` | Custom worker with `std::thread` |
| `ILogger<T>` | `spdlog::logger` |
| `Span<byte>` | `std::span<const std::byte>` |
| `ReadOnlySpan<byte>` | `std::span<const std::byte>` |
| `Task` / `async` | Blocking operations or callbacks |
| `IServiceProvider` | Factory functions |

## License

These samples are part of the Sanctuary.SoeProtocol project and follow the same license as the main library.

## Original C# Samples

These samples are C++23 ports of the C# samples located at:
- `src-cs/Samples/SimpleServer/`
- `src-cs/Samples/SingleSessionPeersSample/`

The functionality remains identical to the original implementations.
