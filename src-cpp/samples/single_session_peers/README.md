# SingleSessionPeers Sample

## Overview

This sample demonstrates client-server communication using the SOE protocol with a ping-pong throughput test. The application:
- Starts both a server and client in the same process
- Establishes a connection between them
- Exchanges "Ping!" and "Pong!" messages continuously
- Measures throughput (messages per second)
- Automatically terminates after the test duration (10 seconds)

## Features

- **Application Protocol**: `Ping_1`
- **Default Port**: 12345
- **Test Duration**: 10 seconds
- **Encryption**: Disabled (for simplicity)
- **Compression**: Disabled (for simplicity)
- **Logging**: Debug-level logging with spdlog

## Components

### ping_application
Implements the application protocol handler for both client and server. It:
- Sends the initial "Ping!" message when the client session opens
- Responds to each received message with the opposite message ("Ping!" ↔ "Pong!")
- Tracks the number of messages received
- Terminates the client session after the test duration
- Calculates and reports throughput statistics

### client_worker
Background worker for the client session manager. It:
- Waits 500ms for the server to start
- Binds to any available local port
- Connects to the server
- Runs the client session until termination

### server_worker
Background worker for the server session manager. It:
- Binds to the specified port
- Listens for incoming connections
- Runs the server session until termination

### main.cpp
Entry point that:
- Configures logging with spdlog
- Parses command-line arguments
- Starts both server and client workers
- Waits for test completion

## Building

From the build directory:

```bash
cmake --build . --target single_session_peers
```

## Running

Run with default port (12345):
```bash
./single_session_peers
```

Run with custom port:
```bash
./single_session_peers 15000
```

## Output

The application will log:
- Server and client startup
- Session open events for both client and server
- Final throughput statistics when sessions close
- Test completion

Example output:
```
[2025-11-10 12:34:56.789] [info] SingleSessionPeers starting...
[2025-11-10 12:34:56.790] [info] Port: 12345
[2025-11-10 12:34:56.791] [info] Server started on port 12345
[2025-11-10 12:34:56.792] [info] Client started, connecting to server on port 12345
[2025-11-10 12:34:56.793] [info] Ping-pong test running...
[2025-11-10 12:34:57.294] [info] <Server> Session opened. Running ping throughput test for 10s...
[2025-11-10 12:34:57.295] [info] <Client> Session opened. Running ping throughput test for 10s...
[2025-11-10 12:35:07.298] [info] <Client> Session closed. Throughput: 12543/s
[2025-11-10 12:35:07.299] [info] <Server> Session closed. Throughput: 12543/s
[2025-11-10 12:35:07.300] [info] Ping-pong test completed
```

## How It Works

1. **Server starts**: Binds to the specified port and waits for connections
2. **Client starts**: Waits 500ms, then connects to the server
3. **Session established**: Both client and server create a session
4. **Client initiates**: Sends the first "Ping!" message
5. **Ping-pong loop**: Each side responds to messages with the opposite message
6. **Duration check**: Client monitors elapsed time
7. **Termination**: After 10 seconds, client terminates the session
8. **Statistics**: Both sides report their message throughput
9. **Shutdown**: Both workers detect session termination and stop

## C# Equivalent

This sample is a C++23 port of the C# SingleSessionPeersSample located at:
`src-cs/Samples/SingleSessionPeersSample/`

The functionality remains identical to the original C# implementation.
