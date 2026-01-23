# HttpServerAdvanced Feature List

## Core Architecture

### Server Types
- **StandardHttpServer** — HTTP server over plain TCP/WiFi
- **SecureHttpServer** — HTTPS server with TLS (BearSSL) support
- **FriendlyWebServer** — High-level wrapper with fluent configuration API

### Memory Model
- Fixed parser buffers (no fragmentation)
- Bounded heap allocations with configurable limits
- Streaming-first body handling (default: no buffering)
- All limits tunable via preprocessor defines

---

## Request Handling

### Handler Types
| Handler | Description | Content-Type Restriction |
|---------|-------------|--------------------------|
| `GetRequest` | Simple GET/HEAD requests, no body | None |
| `Form` | URL-encoded form data (`application/x-www-form-urlencoded`) | Yes |
| `Json` | JSON body parsing with ArduinoJson | `application/json` |
| `RawBody` | Streaming raw body chunks | None |
| `Multipart` / `Upload` | Multipart form data with file uploads | `multipart/form-data` |
| `Buffered` | Buffered body as String | None |

### URL Routing
- **Exact match**: `/api/status`
- **Parameterized routes**: `/api/users/?` (wildcard captures)
- **Multiple parameters**: `/api/?/items/?`
- **Custom matchers**: Method, content-type, and URI pattern functions

### Route Registration
```cpp
server.use([](WebServerBuilder &app) {
    app.handlers()
        .on<GetRequest>("/status", handler)
        .on<Form>("/login", formHandler)
        .on<Json>("/api/data", jsonHandler)
        .on<Multipart>("/upload", uploadHandler);
});
```

---

## Response Building

### Factory Methods
- `HttpResponse::create(status, body, headers)`
- String, `const char*`, binary buffer overloads
- Streaming body via `std::unique_ptr<Stream>`

### Status Codes
- Full HTTP status code support via `HttpStatus`
- Named factory methods: `HttpStatus::Ok()`, `HttpStatus::NotFound()`, etc.

### Headers
- `HttpHeader` with 90+ standard header name constants
- `HttpHeaderCollection` for response header management
- Static factory methods: `HttpHeader::ContentType("text/html")`

---

## Middleware & Interceptors

### Request Interceptors
- Pre-handler logic (authentication, logging, validation)
- Chain via `.with(interceptor)`
- Access to next handler callback

### Response Filters
- Post-handler response modification
- Chain via `.apply(responseFilter)`

### Built-in Middleware
| Middleware | Description |
|------------|-------------|
| `BasicAuth(validator, realm)` | HTTP Basic Authentication |
| `CrossOriginRequestSharing(...)` | CORS headers (preflight, allow-origin, etc.) |

---

## Static File Serving

### StaticFilesBuilder
- Serve files from LittleFS, SPIFFS, or SD
- Configurable path prefix and content root
- Path mapping and predicate functions
- MIME type auto-detection via `HttpContentTypes`

#### Sensible Defaults
By default the static file service will serve files at request paths starting at "/" from the filesystem under "/www"
```cpp
server.use(StaticFiles(LittleFs));
```

#### configuability

These paths can be customized a such
```cpp
server.use(StaticFiles(LittleFS, [](StaticFilesBuilder &files) {
    files.setRequestPathPrefixes("/", "/api")
         .setFilesystemContentRoot("/wwwroot");
}));
```

---

## Security Features

### HTTPS/TLS
- RSA and EC certificate support
- Session caching for performance
- Configurable via `ISecureHttpServerConfig`

### Request Protection
- URI length limits
- Header count/size limits
- Body size limits
- Configurable timeouts

---

## Multipart/File Uploads

### Streaming Upload Handler
- `MultipartFormDataHandler` with chunked callbacks
- Status tracking: `FirstChunk`, `SubsequentChunk`, `FinalChunk`, `Completed`
- Access to filename, content-type, part name
- Memory-efficient (fixed buffer, not full file in memory)

```cpp
.on<Multipart>("/upload", [](HttpRequest &req, std::vector<String> &params, MultipartFormDataBuffer chunk) {
    if (chunk.status() == MultipartStatus::FirstChunk) {
        // Open file: chunk.filename()
    }
    // Write chunk.data(), chunk.size()
    if (chunk.status() == MultipartStatus::FinalChunk) {
        // Close file
    }
    return nullptr; // Continue processing
});
```

---

## Timeouts & Error Handling

### Configurable Timeouts
| Timeout | Default | Description |
|---------|---------|-------------|
| `PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS` | 5000 ms | Total request time |
| `PIPELINE_ACTIVITY_TIMEOUT` | 1000 ms | Idle between reads |
| `PIPELINE_READ_TIMEOUT` | 500 ms | Single read operation |

### Pipeline Errors
- `PipelineError` codes for timeout, parse errors, handler failures
- Handler `onError()` callback support

---

## Utilities

### String Utilities
- `StringView` — Non-owning string reference
- `MightOwnACopyStringView` — Optional ownership
- `StringUtil` — Compare, search, replace, case operations

### HTTP Utilities
- `HttpUtility::ParseQueryString()` — URL-decoded key-value parsing
- `HttpUtility::Base64Encode/Decode()` — Base64 operations
- `UriView` — URI component parsing

### Content Types
- `HttpContentTypes` — Extensible MIME type registry
- Auto-detection from file extensions

---

## Configuration Defines

All tunable via preprocessor:

```cpp
#define HTTPSERVER_ADVANCED_REQUEST_PARSER_BUFFER_LENGTH  1024
#define HTTPSERVER_ADVANCED_MAX_REQUEST_URI_LENGTH        1024
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_COUNT      32
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_NAME_LENGTH  64
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_VALUE_LENGTH 256
#define HTTPSERVER_ADVANCED_MAX_BUFFERED_BODY_LENGTH      2048
#define HTTPSERVER_ADVANCED_MULTIPART_FORM_DATABUFFER_SIZE 1436
#define HTTPSERVER_ADVANCED_PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS 5000
#define HTTPSERVER_ADVANCED_PIPELINE_ACTIVITY_TIMEOUT     1000
#define HTTPSERVER_ADVANCED_PIPELINE_READ_TIMEOUT         500
```

---

## Platform Support

- **RP2040** (Raspberry Pi Pico W)
- **ESP8266** (with appropriate defines)
- **ESP32** (with appropriate defines)
- Any Arduino-compatible board with WiFi and sufficient RAM

---

*Generated on 2026-01-22*
