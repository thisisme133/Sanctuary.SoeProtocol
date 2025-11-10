# SimpleServer Sample

## Overview

This sample demonstrates how to create a simple SOE protocol server that:
- Listens for incoming connections on a specified port
- Handles login sessions with RC4 encryption enabled
- Processes application data packets
- Manages session lifecycle (open, data handling, close)

## Features

- **Encryption**: RC4 encryption enabled by default
- **Compression**: Data compression enabled
- **Application Protocol**: `LoginUdp_18`
- **Default Port**: 20042
- **Logging**: Uses spdlog for structured logging

## Components

### login_application
Implements the application protocol handler for login sessions. It:
- Sets up RC4 encryption with a key
- Handles incoming application data packets
- Manages session open/close events
- Logs all operations with appropriate detail

### soe_session_worker
Background worker that manages the SOE socket handler. It:
- Configures and binds the socket to the specified port
- Runs the main server loop in a background thread
- Handles graceful shutdown

### main.cpp
Entry point that:
- Configures logging with spdlog
- Parses command-line arguments
- Sets up signal handlers for graceful shutdown
- Coordinates worker lifecycle

## Building

From the build directory:

```bash
cmake --build . --target simple_server
```

## Running

Run with default port (20042):
```bash
./simple_server
```

Run with custom port:
```bash
./simple_server 30000
```

## Output

The server will log:
- Server startup and configuration
- Session open events with session IDs
- Application packets received with their OP codes
- Session close events with disconnect reasons
- Shutdown events

## Stopping

Press `Ctrl+C` to gracefully shutdown the server.

## C# Equivalent

This sample is a C++23 port of the C# SimpleServer sample located at:
`src-cs/Samples/SimpleServer/`

The functionality remains identical to the original C# implementation.
