# NetInterface Library

A general-purpose network interface abstraction library for Arduino-compatible boards, providing platform-independent networking code that works across ESP8266, ESP32, and Raspberry Pi Pico.

## Features

- **Platform Independent**: Works across ESP8266, ESP32, and Raspberry Pi Pico
- **Type-Safe Interfaces**: C++ templates with SFINAE for compile-time safety
- **Modern C++**: Uses C++17 features including smart pointers and proper RAII
- **Comprehensive Documentation**: Doxygen-style comments throughout
- **PlatformIO Ready**: Full PlatformIO support with testing framework

## Architecture

This library is a platform independent wrapper around the (annoyingly) subtly different TCP Server, TCP Client, and UDP Peer interfaces exposed by various arduino boards like the esp8266, esp32, raspberry pi pico (variants).

**Why interfaces (pure abstract base classes):**
Interfaces allow us to provide a clean break from the underlying implementation type. For example: a web server using templates for the server and client types.

**Why smart pointers (vs objects that overload casting to bool):**
Testable objects with clear ownership semantics.

The library provides three main interface abstractions:

- **`IClient`**: Abstract interface for TCP client connections
- **`IServer`**: Abstract interface for TCP server implementations  
- **`IPeer`**: Abstract interface for UDP peer-to-peer communications

Each interface has corresponding template implementations:

- **`ClientImpl<T>`**: Template wrapper for any client type `T`
- **`ServerImpl<T>`**: Template wrapper for any server type `T`
- **`PeerImpl<T>`**: Template wrapper for any UDP peer type `T`

## Connection Status

The library includes a comprehensive `ConnectionStatus` enum based on the TCP state machine:

```cpp
enum class ConnectionStatus {
    CLOSED,      // No connection
    LISTEN,      // Server waiting for connections
    SYN_SENT,    // Client waiting for SYN+ACK
    SYN_RCVD,    // Server received SYN, sent SYN+ACK
    ESTABLISHED, // Connection ready for data transfer
    FIN_WAIT_1,  // Active close initiated
    FIN_WAIT_2,  // Waiting for remote FIN
    CLOSE_WAIT,  // Remote initiated close
    CLOSING,     // Simultaneous close
    LAST_ACK,    // Waiting for final ACK
    TIME_WAIT    // Waiting for old packets to expire
};
```

## Quick Start

### Basic Client Example

```cpp
#include <NetClient.h>
#include <WiFi.h>
#include <WiFiClient.h>

using namespace NetInterface;

void setup() {
    // Connect to WiFi first
    WiFi.begin("your-ssid", "your-password");
    
    // Create client wrapper
    WiFiClient wifiClient;
    auto client = std::make_unique<ClientImpl<WiFiClient>>(wifiClient);
    
    // Use the client
    Serial.println(client->connected());
}
```

### Basic Server Example

```cpp
#include <NetClient.h>
#include <WiFi.h>
#include <WiFiServer.h>

using namespace NetInterface;

void setup() {
    // Connect to WiFi
    WiFi.begin("your-ssid", "your-password");
    
    // Create server wrapper
    auto server = std::make_unique<ServerImpl<WiFiServer>>(80);
    server->begin();
}

void loop() {
    auto client = server->accept();
    if (client) {
        // Handle client connection
        const char* response = "HTTP/1.1 200 OK\\r\\n\\r\\nHello World!";
        client->write((const uint8_t*)response, strlen(response));
        client->stop();
    }
}
```

## PlatformIO Usage

### Installation

Add to your `platformio.ini`:

```ini
[env:your_board]
platform = espressif32  ; or espressif8266, raspberrypi
board = esp32dev
framework = arduino
lib_deps = 
    NetInterface
```

### Building and Testing

```bash
# Build for all platforms
pio run

# Build for specific platform
pio run -e esp32dev

# Run tests
pio test -e esp32dev_test

# Upload example
pio run -e esp32dev -t upload
```

## Supported Platforms

- **ESP32**: All ESP32 variants
- **ESP8266**: NodeMCU, Wemos D1, etc.
- **Raspberry Pi Pico**: With Arduino-Pico core

## Examples

The library includes several examples:

- **BasicClient**: Simple client connection example
- **BasicServer**: HTTP server example

## Testing

The library includes comprehensive unit tests using the Unity framework:

```bash
# Run all tests
pio test

# Run tests for specific platform
pio test -e esp32dev_test
```

## API Reference

### IClient Interface

```cpp
class IClient {
public:
    virtual size_t write(const uint8_t *buf, size_t size) = 0;
    virtual int available() = 0;
    virtual int read(uint8_t *rbuf, size_t size) = 0;
    virtual void flush() = 0;
    virtual void stop() = 0;
    virtual ConnectionStatus status() = 0;
    virtual uint8_t connected() = 0;
    virtual IPAddress remoteIP() = 0;
    virtual uint16_t remotePort() = 0;
    virtual IPAddress localIP() = 0;
    virtual uint16_t localPort() = 0;
    virtual void setTimeout(uint32_t timeoutMs) = 0;
    virtual uint32_t getTimeout() const = 0;
};
```

## Contributing

1. Fork the repository
2. Create your feature branch
3. Add tests for new functionality
4. Ensure all tests pass on all platforms
5. Submit a pull request

## License

MIT License - see LICENSE file for details.

## Author

Jeffrey Becker (jeffrey.a.becker@gmail.com) 