# UDP/TCP Client with Binary/Text Protocol Support

This project implements a networked client that can communicate using both UDP and TCP transport protocols, with support for both binary and text-based application protocols.

## Overview

The client supports four different protocol combinations:
- **TCP + TEXT**: ASCII-based protocol over reliable TCP transport
- **TCP + BINARY**: Binary protocol over reliable TCP transport  
- **UDP + TEXT**: ASCII-based protocol over unreliable UDP transport
- **UDP + BINARY**: Binary protocol over unreliable UDP transport

Additionally, the client supports an "ANY" protocol mode that attempts UDP first, then falls back to TCP if UDP fails.

## Usage

```bash
./client PROTOCOL://server:port/api
```

Where:
- `PROTOCOL` can be: `TCP`, `UDP`, `ANY` (case insensitive)
- `server` is the hostname or IP address
- `port` is the port number
- `api` can be: `TEXT`, `BINARY` (case insensitive)

### Examples

```bash
# TCP with text protocol
./client tcp://alice.nplab.bth.se:5000/text

# UDP with binary protocol  
./client udp://bob.nplab.bth.se:5000/binary

# Try UDP first, fallback to TCP
./client any://bob.nplab.bth.se:5000/text
```

## Protocol Details

### Text Protocol (TCP/UDP + TEXT)

- ASCII-based communication using newline delimiters
- Protocol negotiation with version information
- Arithmetic operations: `add`, `sub`, `mul`, `div`
- Format: `operation value1 value2\n`

#### TCP Text Flow:
1. Server sends supported protocols (e.g., "TEXT TCP 1.1\n")
2. Client responds with acceptance (e.g., "TEXT TCP 1.1 OK\n")
3. Server sends arithmetic assignment
4. Client calculates and sends result
5. Server responds with "OK" or "ERROR"

#### UDP Text Flow:
1. Client sends "TEXT UDP 1.1\n"
2. Server responds with arithmetic assignment
3. Client calculates and sends result
4. Server responds with "OK" or "ERROR"

### Binary Protocol (TCP/UDP + BINARY)

- Binary message structures with network byte order
- Two message types: `calcMessage` and `calcProtocol`
- Arithmetic operations encoded as integers
- Fixed-size message structures

#### Message Structures:

**calcMessage** (10 bytes):
```c
struct calcMessage {
    uint16_t type;           // Message type (22 for initial, 1/2 for responses)
    uint16_t message;        // Message code (0=initial, 1=OK, 2=NOT_OK) 
    uint16_t protocol;       // Protocol ID (17 for UDP)
    uint16_t major_version;  // Major version (1)
    uint16_t minor_version;  // Minor version (1)
};
```

**calcProtocol** (22 bytes):
```c
struct calcProtocol {
    uint16_t type;           // Message type (1)
    uint16_t major_version;  // Major version (1)
    uint16_t minor_version;  // Minor version (1) 
    uint32_t id;            // Message ID
    uint32_t arith;         // Arithmetic operation (1=add, 2=sub, 3=mul, 4=div)
    int32_t inValue1;       // First operand
    int32_t inValue2;       // Second operand
    int32_t inResult;       // Result (filled by client)
};
```

## Building

### Using Make (Linux/Mac/MinGW):
```bash
make                    # Build release version
make debug             # Build debug version with extra output
make clean             # Clean build artifacts
```

### Manual compilation:
```bash
# Compile library
gcc -c calcLib.c -o calcLib.o

# Compile main program  
g++ -std=c++11 -c clientmain.cpp -o clientmain.o

# Link (Linux/Mac)
g++ calcLib.o clientmain.o -o client

# Link (Windows)
g++ calcLib.o clientmain.o -o client -lws2_32
```

## Features

- **Cross-platform**: Supports Windows, Linux, and macOS
- **IPv4/IPv6**: Automatic address family detection
- **Timeout handling**: 2-second timeout for UDP communications
- **Error handling**: Comprehensive error reporting
- **Debug mode**: Compile with `-DDEBUG` for verbose output
- **Protocol fallback**: ANY mode tries UDP first, then TCP

## Error Handling

The client handles various error conditions:
- Network connection failures
- DNS resolution failures  
- Protocol version mismatches
- Message timeouts (UDP)
- Invalid message formats
- Server-side calculation errors

Error messages are printed to STDERR with "ERROR:" prefix.

## Testing Servers

The implementation has been designed to work with the following test servers:

- `alice.nplab.bth.se:5000` - UDP+TCP, BINARY+TEXT (32-bit, Big endian)
- `bob.nplab.bth.se:5000` - UDP+TCP, BINARY+TEXT (64-bit, Little endian)  
- `bob.nplab.bth.se:5001` - UDP, BINARY+TEXT (Bad server, loses messages)

## Implementation Notes

- Uses network byte order (big endian) for binary protocol
- Integer division truncates towards zero
- UDP messages have 2-second timeout
- Structures are packed to avoid padding issues
- Windows Winsock properly initialized and cleaned up

## Files

- `clientmain.cpp` - Main client implementation
- `calcLib.c/.h` - Arithmetic calculation library
- `protocol.h` - Binary protocol structure definitions
- `Makefile` - Build configuration
- `test_client.cpp` - Unit tests for core functionality
