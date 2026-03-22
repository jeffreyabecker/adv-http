# HttpServerAdvanced Library Documentation

A comprehensive, pipeline-based HTTP server library for Arduino/RP2040 (Pico W) platforms with advanced routing and handler patterns.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Functional Areas](#functional-areas)
   - [Core](#1-core)
   - [Pipeline](#2-pipeline)
   - [Handlers](#3-handlers)
   - [Routing](#4-routing)
   - [Response](#5-response)
   - [Server](#6-server)
   - [Static Files](#7-static-files)
   - [Streams](#8-streams)
   - [Utilities](#9-utilities)
3. [Class Relationships](#class-relationships)
4. [Usage Patterns](#usage-patterns)

---

## Architecture Overview

HttpServerAdvanced uses a **pipeline-based architecture** inspired by modern web frameworks (ASP.NET, Express.js). The library separates concerns into distinct layers:

```
┌─────────────────────────────────────────────────────────────────┐
│                         WebServer                                │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    HttpServerBase                        │    │
│  │  ┌───────────────────────────────────────────────────┐  │    │
│  │  │                  HttpPipeline                      │  │    │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌───────────┐  │  │    │
│  │  │  │RequestParser│→ │ HttpRequest │→ │IHttpHandler│  │  │    │
│  │  │  └─────────────┘  └─────────────┘  └───────────┘  │  │    │
│  │  │                          ↓                         │  │    │
│  │  │                   IHttpResponse                    │  │    │
│  │  └───────────────────────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
│  ┌──────────────────┐  ┌──────────────────┐                     │
│  │HandlerProviderReg│  │ProviderRegistryBld│                     │
│  └──────────────────┘  └──────────────────┘                     │
└─────────────────────────────────────────────────────────────────┘
```

**Key Design Principles:**
- **RAII/Smart Pointers**: All dynamic memory managed via `std::unique_ptr`
- **Factory Pattern**: Handlers created on-demand per request
- **Fluent Builder API**: Chainable configuration methods
- **Static Buffers**: Configurable bounded memory for embedded constraints

---

## Functional Areas

### 1. Core

**Location:** `src/core/`

The core module contains fundamental types and configuration constants.

#### Classes & Types

| Class/Type | Purpose |
|------------|---------|
| `Defines.h` | Configuration constants (buffer sizes, limits, timeouts) |
| `HttpHeader` | Name-value pair for HTTP headers |
| `HttpHeaderNames` | Static header name constants (Content-Type, Authorization, etc.) |
| `HttpHeaderCollection` | Collection of headers with find/set/exists operations |
| `HttpMethod` | HTTP method constants (GET, POST, PUT, DELETE, etc.) |
| `HttpStatus` | Status code + reason phrase with factory methods |
| `HttpRequest` | Central request context implementing `IPipelineHandler` |
| `HttpRequestPhase` | Bitmask flags for request lifecycle phases |
| `HttpTimeouts` | Timeout configuration (connect, header, body, keep-alive) |
| `HttpContentTypes` | MIME type registry with extension mapping |
| `IHttpRequestHandlerFactory` | Interface for creating request handlers |
| `HttpRequestHandlerFactory` | Default implementation using `HandlerProviderRegistry` |
| `Buffer.h` | Utility buffer templates |

#### Key Constants (Defines.h)

```cpp
// Buffer sizes
PIPELINE_STACK_BUFFER_SIZE     = 256   // Pipeline read buffer
REQUEST_PARSER_BUFFER_LENGTH   = 256   // HTTP parser buffer
MAX_REQUEST_URI_LENGTH         = 1024  // Maximum URI length
MAX_REQUEST_HEADER_COUNT       = 32    // Maximum headers per request
MAX_BUFFERED_BODY_LENGTH       = 2048  // Body buffering limit

// Concurrency
MAX_CONCURRENT_CONNECTIONS     = 4     // Simultaneous client connections

// Timeouts (milliseconds)
TIMEOUT_CLIENT_CONNECT         = 5000
TIMEOUT_CLIENT_HEADER          = 5000
TIMEOUT_CLIENT_BODY            = 10000
TIMEOUT_KEEP_ALIVE             = 15000
```

#### HttpRequest

The central context object passed through the pipeline:

```cpp
class HttpRequest : public IPipelineHandler {
    // Request properties
    const String& method() const;
    const String& uri() const;
    const String& version() const;
    const HttpHeaderCollection& headers() const;
    std::string_view remoteAddress() const;
    
    // Parsed URI components
    const UriView& uriView() const;
    
    // Request items (arbitrary key-value storage)
    std::map<String, String>& items();
    
    // Lifecycle phases
    HttpRequestPhase currentPhase() const;
    bool hasPhase(HttpRequestPhase phase) const;
    
    // Handler invocation
    void invokeHandler(IHttpHandler& handler);
    void sendResponse(std::unique_ptr<IHttpResponse> response);
    
    // Factory method
    static PipelineHandlerPtr createPipelineHandler(HttpServerBase& server, IHttpRequestHandlerFactory& factory);
};
```

#### HttpRequestPhase

Bitmask flags tracking request lifecycle:

```cpp
enum class HttpRequestPhase : uint16_t {
    None              = 0x0000,
    HeadersReceived   = 0x0001,
    BodyReceiving     = 0x0002,
    BodyReceived      = 0x0004,
    HandlerInvoked    = 0x0008,
    ResponseSending   = 0x0010,
    ResponseSent      = 0x0020,
    Completed         = 0x0040,
    Error             = 0x8000
};
```

---

### 2. Pipeline

**Location:** `src/pipeline/`

The pipeline module handles HTTP parsing and request/response flow.

#### Classes

| Class | Purpose |
|-------|---------|
| `HttpPipeline` | Orchestrates request/response lifecycle for a client connection |
| `RequestParser` | Wraps `http_parser` library for incremental HTTP parsing |
| `IPipelineHandler` | Interface for objects receiving pipeline events |
| `NetClient` | Transport seam header exposing the abstract `IClient` and `IServer` contracts |
| `PipelineError` | Error codes and messages for pipeline failures |
| `PipelineHandleClientResult` | Enum result of `handleClient()` operation |

#### HttpPipeline

Manages a single client connection through the request lifecycle:

```cpp
class HttpPipeline {
public:
    HttpPipeline(std::unique_ptr<IClient> client, PipelineHandlerPtr handler, HttpTimeouts& timeouts);
    
    // Process client data, returns when complete or need more data
    PipelineHandleClientResult handleClient();
    
    // Abort processing
    void abort();
    
    // Set custom response stream
    void setResponseStream(std::unique_ptr<Stream> stream);
    
    // Check if pipeline is complete
    bool isComplete() const;
};
```

#### RequestParser

Wraps Node.js `http_parser` for incremental HTTP parsing:

```cpp
class RequestParser {
public:
    RequestParser(IPipelineHandler& handler);
    
    // Feed data incrementally
    size_t execute(const uint8_t* data, size_t length);
    
    // Parser state
    bool isMessageComplete() const;
    bool hasError() const;
    const char* getErrorName() const;
    const char* getErrorDescription() const;
    
    // Current parsing stage
    ParserStage stage() const;
};
```

**Memory**: Uses static buffer of `REQUEST_PARSER_BUFFER_LENGTH` bytes.

#### IPipelineHandler

Interface for receiving pipeline events:

```cpp
class IPipelineHandler {
public:
    virtual void onUrl(const char* at, size_t length) = 0;
    virtual void onHeaderField(const char* at, size_t length) = 0;
    virtual void onHeaderValue(const char* at, size_t length) = 0;
    virtual void onHeadersComplete() = 0;
    virtual void onBody(const uint8_t* at, size_t length) = 0;
    virtual void onMessageComplete() = 0;
    virtual void onError(const PipelineError& error) = 0;
};
```

---

### 3. Handlers

**Location:** `src/handlers/`

Handlers process HTTP requests and generate responses.

#### Interfaces & Base Classes

| Class | Purpose |
|-------|---------|
| `IHttpHandler` | Core handler interface |
| `HttpHandler` | Simple implementation for direct callbacks |
| `IHandlerProvider` | Factory interface for creating handlers |
| `BufferingHttpHandlerBase<N>` | Template base for body-buffering handlers |

#### Handler Type Definitions

| Type | HTTP Method | Body Type | Signature |
|------|-------------|-----------|-----------|
| `GetRequest` | GET | None | `(HttpRequest&, args...) → IHttpResponse` |
| `Json` | POST/PUT | ArduinoJson | `(HttpRequest&, JsonDocument&, args...) → IHttpResponse` |
| `Form` | POST | URL-encoded | `(HttpRequest&, FormData&, args...) → IHttpResponse` |
| `Multipart` | POST | multipart/form-data | `(HttpRequest&, MultipartData&, args...) → IHttpResponse` |
| `RawBody` | POST/PUT | Raw bytes | `(HttpRequest&, Buffer&, args...) → IHttpResponse` |
| `BufferedString` | POST/PUT | String | `(HttpRequest&, String&, args...) → IHttpResponse` |

#### IHttpHandler Interface

```cpp
class IHttpHandler {
public:
    using HandlerResult = std::unique_ptr<IHttpResponse>;
    using InvocationCallback = std::function<HandlerResult(HttpRequest&)>;
    using InterceptorCallback = std::function<HandlerResult(HttpRequest&, InvocationCallback)>;
    using Predicate = std::function<bool(HttpRequest&)>;
    using Factory = std::function<std::unique_ptr<IHttpHandler>(HttpRequest&)>;
    
    // Called to process request (may be called multiple times for body chunks)
    virtual HandlerResult handleStep(HttpRequest& context) = 0;
    
    // Called for each body chunk received
    virtual void handleBodyChunk(HttpRequest& context, const uint8_t* at, size_t length) = 0;
};
```

#### BufferingHttpHandlerBase

Template base class for handlers that buffer the request body:

```cpp
template <std::size_t MaxBuffered = MAX_BUFFERED_BODY_LENGTH>
class BufferingHttpHandlerBase : public IHttpHandler {
protected:
    std::vector<uint8_t> buffer_;
    bool bufferOverflow_ = false;
    
public:
    virtual void handleBodyChunk(HttpRequest& context, const uint8_t* at, size_t length) override;
    
    // Subclasses implement to process buffered body
    virtual HandlerResult handleBufferedBody(HttpRequest& context) = 0;
};
```

#### Handler Type Pattern

Each handler type (GetRequest, Json, Form, etc.) follows this pattern:

```cpp
struct Json {
    using Invocation = std::function<HandlerResult(HttpRequest&, JsonDocument&, std::vector<String>&)>;
    using InvocationWithoutParams = std::function<HandlerResult(HttpRequest&, JsonDocument&)>;
    
    // Restrict matcher to appropriate content types/methods
    static void restrict(HandlerMatcher& matcher);
    
    // Create handler factory
    static IHttpHandler::Factory makeFactory(Invocation callback, ExtractArgsFromRequest extractor);
    
    // Curry helpers for interceptors
    static Invocation curryInterceptor(IHttpHandler::InterceptorCallback, Invocation);
    static Invocation curryWithoutParams(InvocationWithoutParams);
    
    // Response filter application
    static Invocation applyResponseFilter(IHttpResponse::ResponseFilter, Invocation);
};
```

---

### 4. Routing

**Location:** `src/routing/`

The routing module provides URL pattern matching and handler registration.

#### Classes

| Class | Purpose |
|-------|---------|
| `HandlerMatcher` | Matches requests by URI pattern, method, content type |
| `ParameterizedUri` | HandlerMatcher variant for `/path/:param` patterns |
| `HandlerBuilder<T>` | Fluent builder for configuring handlers |
| `HandlerProviderRegistry` | Central registry of handler factories |
| `ProviderRegistryBuilder` | Builder API for registering handlers |
| `BasicAuthentication.h` | Basic HTTP authentication interceptor |
| `CrossOriginRequestSharing.h` | CORS response filter |

#### HandlerMatcher

Matches incoming requests against patterns:

```cpp
class HandlerMatcher {
public:
    HandlerMatcher(const String& uriPattern, 
                   const String& allowedMethods = "",
                   const std::initializer_list<String>& allowedContentTypes = {});
    
    // Check if this matcher handles the request
    bool canHandle(HttpRequest& context) const;
    
    // Extract URI parameters (e.g., /users/:id → ["123"])
    std::vector<String> extractParameters(HttpRequest& context) const;
    
    // Setters for custom matching logic
    void setMethodChecker(MethodChecker checker);
    void setUriPatternChecker(UriPatternChecker checker);
    void setContentTypeChecker(ContentTypeChecker checker);
    void setArgsExtractor(ArgsExtractor extractor);
};
```

#### ParameterizedUri

For dynamic URL segments:

```cpp
// Matches /users/:id, /posts/:postId/comments/:commentId
ParameterizedUri("/users/:id");
```

#### HandlerBuilder

Fluent builder returned by `on()` methods:

```cpp
template <typename THandler>
class HandlerBuilder {
public:
    // Add interceptor (before/after logic)
    HandlerBuilder& with(IHttpHandler::InterceptorCallback wrapper);
    
    // Add request filter
    HandlerBuilder& filterRequest(IHttpHandler::Predicate predicate);
    
    // Add response filter
    HandlerBuilder& apply(IHttpResponse::ResponseFilter filter);
    
    // Restrict HTTP methods
    HandlerBuilder& allowMethods(const String& methods);
    
    // Restrict content types
    HandlerBuilder& allowContentTypes(const std::initializer_list<String>& contentTypes);
};
```

#### HandlerProviderRegistry

Central handler registry:

```cpp
class HandlerProviderRegistry {
public:
    // Create handler for a request (returns nullptr if no match)
    std::unique_ptr<IHttpHandler> createContextHandler(HttpRequest& context);
    
    // Set default (404) handler
    void setDefaultHandlerFactory(IHttpHandler::Factory creator);
    
    // Add handler providers
    void add(IHandlerProvider& provider, AddPosition position = AddAt::End);
    void add(IHttpHandler::Predicate predicate, IHttpHandler::Factory handler, AddPosition position = AddAt::End);
    
    // Global middleware
    void with(IHttpHandler::InterceptorCallback interceptor);     // Global interceptor
    void filterRequest(IHttpHandler::Predicate predicate);        // Global request filter
    void apply(IHttpResponse::ResponseFilter filter);             // Global response filter
};
```

#### BasicAuthentication

Interceptor for HTTP Basic Auth:

```cpp
// With fixed credentials
auto auth = BasicAuth("admin", "password123", "Admin Area");

// With validator function
auto auth = BasicAuth([](const String& user, const String& pass) {
    return user == "admin" && checkPassword(pass);
}, "Secure Zone");

// Usage
server.cfg().on<GetRequest>("/admin", handler).with(auth);
```

#### CrossOriginRequestSharing

Response filter for CORS headers:

```cpp
auto cors = CrossOriginRequestSharing(
    "*",                    // allowedOrigins
    "GET, POST, OPTIONS",   // allowedMethods  
    "Content-Type",         // allowedHeaders
    "",                     // allowedCredentials
    "",                     // exposeHeaders
    3600                    // maxAge
);

// Apply globally or per-handler
server.cfg().apply(cors);
```

---

### 5. Response

**Location:** `src/response/`

The response module provides response creation and streaming.

#### Classes

| Class | Purpose |
|-------|---------|
| `IHttpResponse` | Response interface |
| `HttpResponse` | Standard response implementation |
| `StringResponse` | Factory for text/HTML responses |
| `JsonResponse` | Factory for JSON responses |
| `FormResponse` | Factory for URL-encoded form responses |
| `HttpResponseBodyStream` | Base class for response body streams |
| `ChunkedHttpResponseBodyStream` | Chunked transfer encoding support |

#### IHttpResponse Interface

```cpp
class IHttpResponse {
public:
    using ResponseFilter = std::function<std::unique_ptr<IHttpResponse>(std::unique_ptr<IHttpResponse>)>;
    
    virtual HttpStatus status() const = 0;
    virtual HttpHeaderCollection& headers() = 0;
    virtual std::unique_ptr<HttpResponseBodyStream> getBody() = 0;
};
```

#### StringResponse Factory

```cpp
class StringResponse {
public:
    // Text response
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String& body);
    
    // With content type
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String& body, const String& contentType);
    
    // With additional headers
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String& body, 
                                                  std::initializer_list<HttpHeader> headers);
    
    // HTML shorthand
    static std::unique_ptr<IHttpResponse> html(const String& body);
    static std::unique_ptr<IHttpResponse> html(HttpStatus status, const String& body);
    
    // Plain text shorthand
    static std::unique_ptr<IHttpResponse> text(const String& body);
    static std::unique_ptr<IHttpResponse> text(HttpStatus status, const String& body);
};
```

#### JsonResponse Factory

```cpp
class JsonResponse {
public:
    // From ArduinoJson document
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const JsonDocument& doc);
    static std::unique_ptr<IHttpResponse> create(const JsonDocument& doc);  // 200 OK
    
    // With additional headers
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const JsonDocument& doc,
                                                  std::initializer_list<HttpHeader> headers);
};
```

---

### 6. Server

**Location:** `src/server/`

The server module provides the main server classes.

#### Classes

| Class | Purpose |
|-------|---------|
| `HttpServerBase` | Abstract base with connection management |
| `StandardHttpServer<T>` | Template for standard (HTTP) servers |
| `FriendlyWebServer<T>` | User-friendly wrapper with builder API |
| `WebServerBuilder` | Configuration builder |
| `WebServerConfig` | Unified configuration facade |

#### HttpServerBase

Abstract base class managing connections:

```cpp
class HttpServerBase {
public:
    virtual void begin();
    virtual void end();
    
    // Call in loop() to process clients
    void handleClient();
    
    // Timeout configuration
    HttpTimeouts& timeouts();
    void setTimeouts(const HttpTimeouts& timeouts);
    
    // Connection info
    size_t activeConnections() const;
    static constexpr size_t maxConcurrentConnections();  // MAX_CONCURRENT_CONNECTIONS
    
    virtual std::string_view localAddress() const = 0;
    virtual uint16_t localPort() const = 0;

protected:
    virtual std::unique_ptr<IClient> accept() = 0;
    
    std::vector<std::unique_ptr<HttpPipeline>> pipelines_;
    HttpTimeouts timeouts_;
};
```

#### StandardHttpServer

Template for plain HTTP servers:

```cpp
template <typename TServer = WiFiServer, uint16_t DEFAULT_PORT = 80>
class StandardHttpServer : public HttpServerBase {
public:
    StandardHttpServer(uint16_t port = DEFAULT_PORT, std::string bindAddress = std::string());
    
    void begin() override;
    void end() override;
    
    std::string_view localAddress() const override;
    uint16_t localPort() const override;
};
```

#### FriendlyWebServer (WebServer)

High-level user-friendly API:

```cpp
template <typename THttpServer = StandardHttpServer<>>
class FriendlyWebServer : public THttpServer {
public:
    // Component-based configuration
    WebServerConfig& use(std::function<void(WebServerBuilder&)> component);
    
    // Direct configuration access
    WebServerConfig& cfg();
};

// Type alias
using WebServer = FriendlyWebServer<>;
```

#### WebServerConfig

Unified configuration facade:

```cpp
class WebServerConfig {
public:
    // Handler registration (defaults to GetRequest)
    template <typename THandler = GetRequest>
    HandlerBuilder<THandler> on(const char* path, typename THandler::Invocation handler);
    
    template <typename THandler = GetRequest>
    HandlerBuilder<THandler> on(const HandlerMatcher& matcher, typename THandler::Invocation handler);
    
    // 404 handler
    void onNotFound(IHttpHandler::InvocationCallback invocation);
    
    // Global middleware
    void apply(IHttpResponse::ResponseFilter filter);
    void filterRequest(IHttpHandler::Predicate predicate);
    void with(IHttpHandler::InterceptorCallback wrapper);
    
    // Access to underlying components
    HandlerProviderRegistry& handlerProviders();
    ProviderRegistryBuilder& handlers();
    HttpContentTypes& contentTypes();
    HttpServerBase& server();
};
```

---

### 7. Static Files

**Location:** `src/staticfiles/`

Serves static files from a filesystem (LittleFS, SD, etc.).

#### Classes

| Class | Purpose |
|-------|---------|
| `FileLocator` | Abstract interface for locating files |
| `DefaultFileLocator` | Standard filesystem-based locator |
| `AggregateFileLocator` | Combines multiple locators |
| `StaticFileHandlerFactory` | Handler provider for static files |
| `StaticFilesBuilder` | Builder for static file configuration |

#### FileLocator Interface

```cpp
class FileLocator {
public:
    virtual File getFile(HttpRequest& context) = 0;
    virtual bool canHandle(const String& path) = 0;
};
```

#### DefaultFileLocator

Maps request paths to filesystem paths:

```cpp
class DefaultFileLocator : public FileLocator {
public:
    static constexpr const char* DefaultFSRoot = "/www";
    static constexpr const char* DefaultIncludePrefix = "/";
    static constexpr const char* DefaultExcludePrefix = "/api/";
    
    DefaultFileLocator(FS& fs);
    
    void setPathPredicate(RequestPathPredicate predicate);
    void setPathMapper(RequestPathMapper mapper);
    void setRequestPathPrefixes(const String& include, const String& exclude);
    void setFilesystemContentRoot(const String& root);
    
    File getFile(HttpRequest& context) override;
    bool canHandle(const String& path) override;
};
```

#### StaticFilesBuilder

Configuration builder:

```cpp
class StaticFilesBuilder {
public:
    StaticFilesBuilder& setPathPredicate(std::function<bool(const String&)> predicate);
    StaticFilesBuilder& setPathMapper(std::function<String(const String&)> mapper);
    StaticFilesBuilder& setRequestPathPrefixes(const String& prefix, const String& exclude);
    StaticFilesBuilder& setFilesystemContentRoot(const String& root);
};

// Factory function for use() pattern
std::function<void(WebServerBuilder&)>& StaticFiles(FS& fs, std::function<void(StaticFilesBuilder&)> setup = nullptr);
```

**Usage:**

```cpp
server.use(StaticFiles(LittleFS, [](StaticFilesBuilder& files) {
    files.setFilesystemContentRoot("/www")
         .setRequestPathPrefixes("/", "/api");
}));
```

---

### 8. Streams

**Location:** `src/streams/`

Stream utilities for encoding/decoding and composition.

#### Classes

| Class | Purpose |
|-------|---------|
| `ReadStream` | Abstract base for read-only streams |
| `EmptyReadStream` | Always-empty stream |
| `OctetsStream` | Stream from byte array |
| `StringStream` | Stream from Arduino String |
| `LazyStreamAdapter` | Deferred stream creation |
| `IndefiniteConcatStream` | Concatenates multiple streams |
| `Base64DecoderStream` | Decodes Base64 input |
| `Base64EncoderStream` | Encodes to Base64 output |
| `UriDecodingStream` | Decodes percent-encoding |
| `UriEncodingStream` | Encodes to percent-encoding |

#### Stream Contract

```
- available() == 0  : end of stream (no more bytes)
- available() > 0   : that many bytes are buffered and readable now  
- available() == -1 : buffer empty but more data expected (non-final)
- read() returns -1 only at end of stream
```

#### Base64 Streams

```cpp
// Decoding
auto decoder = Base64DecoderStream::create(encodedString, /* urlSafe */ false);
while (decoder.available() > 0) {
    int byte = decoder.read();
}

// Encoding
auto encoder = Base64EncoderStream::create(data, length, /* urlSafe */ false, /* emitPadding */ true);
```

#### URI Streams

```cpp
// Decode %20 → space, + → space
UriDecodingStream decoder("hello%20world");

// Encode special characters
UriEncodingStream encoder("hello world");  // → hello%20world
```

---

### 9. Utilities

**Location:** `src/util/`

Helper classes and functions.

#### Classes

| Class | Purpose |
|-------|---------|
| `StringView` | Non-owning string reference (like std::string_view) |
| `KeyValuePairView<K,V>` | Immutable key-value collection |
| `UriView` | Parsed URI with scheme, host, path, query, fragment |
| `WebUtility` | URL/HTML encoding, Base64 utilities |
| `StringUtil` | Minimal standard-text helper namespace kept for compatibility during migration |

#### StringView

Compatibility aliases for STL text views and ownership:

```cpp
using StringView = std::string_view;
using OwningStringView = std::string;
```

#### UriView

Parsed URI components:

```cpp
class UriView {
public:
    UriView(const String& uri);
    
    std::string_view scheme() const;     // "https"
    std::string_view userinfo() const;   // "user:pass"
    std::string_view host() const;       // "example.com"
    std::string_view port() const;       // "8080"
    std::string_view path() const;       // "/api/users"
    std::string_view query() const;      // "id=123&name=test"
    std::string_view fragment() const;   // "section1"
    
    // Parsed query parameters
    const KeyValuePairView<std::string, std::string>& queryView() const;
};
```

#### WebUtility

Encoding/decoding utilities:

```cpp
class WebUtility {
public:
    // Query string parsing
    static std::vector<std::pair<String, String>> ParseQueryString(const String& query);
    
    // URI encoding/decoding
    static String DecodeURIComponent(const String& str);
    static String EncodeURIComponent(const String& str);
    
    // HTML encoding
    static String HtmlEncode(const String& str);
    static String HtmlAttributeEncode(const String& str);
    static String JavaScriptStringEncode(const String& str, bool includeQuotes = true);
    
    // Base64
    static String Base64Encode(const String& input, bool urlCompatible = false);
    static String Base64DecodeToString(const String& input, bool urlCompatible = false);
    static std::vector<uint8_t> Base64Decode(const String& input, bool urlCompatible = false);
};
```

---

## Class Relationships

### Inheritance Hierarchy

```
IHttpHandler
├── HttpHandler (simple callback wrapper)
└── BufferingHttpHandlerBase<N>
    ├── JsonBodyHandler
    ├── FormBodyHandler
    ├── MultipartFormDataHandler
    ├── RawBodyHandler
    └── BufferedStringBodyHandler

IHttpResponse
└── HttpResponse

IHandlerProvider
├── HandlerProviderRegistry (internal provider)
└── StaticFileHandlerFactory

HttpServerBase
└── StandardHttpServer<TServer, PORT>

FriendlyWebServer<THttpServer>
└── (wraps any StandardHttpServer)

FileLocator
├── DefaultFileLocator
└── AggregateFileLocator

Stream
└── ReadStream
    ├── EmptyReadStream
    ├── OctetsStream
    │   └── StringStream
    ├── LazyStreamAdapter
    ├── IndefiniteConcatStream<It>
    ├── Base64DecoderStream
    ├── Base64EncoderStream
    ├── UriDecodingStream
    └── UriEncodingStream
```

### Composition Relationships

```
FriendlyWebServer
├── owns → WebServerBuilder
│           ├── owns → HandlerProviderRegistry
│           ├── owns → ProviderRegistryBuilder
│           ├── owns → HttpContentTypes
│           └── owns → HttpRequestHandlerFactory
├── owns → WebServerConfig (facade to WebServerBuilder)
└── inherits → StandardHttpServer
                └── inherits → HttpServerBase
                                ├── owns → std::vector<HttpPipeline>
                                └── owns → HttpTimeouts

HttpPipeline
├── owns → IClient (network connection)
├── owns → RequestParser
├── refs → IPipelineHandler (HttpRequest)
└── owns → Stream (response stream)

HttpRequest
├── owns → HttpHeaderCollection
├── owns → UriView
├── refs → IHttpRequestHandlerFactory
└── owns → std::unique_ptr<IHttpHandler>

HandlerBuilder<THandler>
├── refs → addHandler callback
├── owns → HandlerMatcher
├── owns → Invocation callback
└── owns → ResponseFilter

StaticFilesBuilder
├── owns → StaticFileHandlerFactory
└── owns → DefaultFileLocator
```

---

## Usage Patterns

### Basic Server Setup

```cpp
#include <HttpServerAdvanced.h>
#include <WiFi.h>

using namespace HttpServerAdvanced;

FriendlyWebServer<> server(80);

void setup() {
    WiFi.begin("SSID", "password");
    while (WiFi.status() != WL_CONNECTED) delay(500);
    
    server.cfg().on<GetRequest>("/", [](HttpRequest& req) {
        return StringResponse::text("Hello World!");
    });
    
    server.cfg().on<GetRequest>("/json", [](HttpRequest& req) {
        JsonDocument doc;
        doc["message"] = "Hello";
        doc["time"] = millis();
        return JsonResponse::create(doc);
    });
    
    server.begin();
}

void loop() {
    server.handleClient();
}
```

### Route Parameters

```cpp
server.cfg().on<GetRequest>(ParameterizedUri("/users/:id"), 
    [](HttpRequest& req, std::vector<String>& args) {
        String userId = args[0];
        return StringResponse::text("User: " + userId);
    });
```

### JSON API

```cpp
server.cfg().on<Json>("/api/data", [](HttpRequest& req, JsonDocument& body, std::vector<String>& args) {
    String name = body["name"] | "anonymous";
    
    JsonDocument response;
    response["received"] = name;
    response["timestamp"] = millis();
    return JsonResponse::create(response);
});
```

### Form Handling

```cpp
server.cfg().on<Form>("/submit", [](HttpRequest& req, FormData& form, std::vector<String>& args) {
    auto name = form.get("name");
    auto email = form.get("email");
    return StringResponse::text("Received: " + name.value_or("") + ", " + email.value_or(""));
});
```

### Authentication

```cpp
auto adminAuth = BasicAuth("admin", "secret", "Admin Area");

server.cfg().on<GetRequest>("/admin", [](HttpRequest& req) {
    return StringResponse::html("<h1>Admin Panel</h1>");
}).with(adminAuth);
```

### CORS Support

```cpp
auto cors = CrossOriginRequestSharing("*", "GET, POST, OPTIONS");

// Apply globally
server.cfg().apply(cors);

// Or per-route
server.cfg().on<GetRequest>("/api/data", handler).apply(cors);
```

### Static Files

```cpp
#include <LittleFS.h>

server.use(StaticFiles(LittleFS, [](StaticFilesBuilder& files) {
    files.setFilesystemContentRoot("/www")
         .setRequestPathPrefixes("/static", "/api");
}));
```

### Global Middleware

```cpp
// Log all requests
server.cfg().with([](HttpRequest& req, IHttpHandler::InvocationCallback next) {
    Serial.printf("[%s] %s\n", req.method().c_str(), req.uri().c_str());
    return next(req);
});

// Filter requests
server.cfg().filterRequest([](HttpRequest& req) {
    return req.remoteAddress() != "192.168.1.100";  // Block IP
});

// Modify all responses
server.cfg().apply([](std::unique_ptr<IHttpResponse> resp) {
    resp->headers().set("X-Powered-By", "HttpServerAdvanced");
    return resp;
});
```

---

## Memory Considerations

| Component | Memory Strategy |
|-----------|-----------------|
| RequestParser | Static buffer (`REQUEST_PARSER_BUFFER_LENGTH` = 256 bytes) |
| HttpPipeline | Stack buffer (`PIPELINE_STACK_BUFFER_SIZE` = 256 bytes) |
| Body Buffering | Configurable via `BufferingHttpHandlerBase<N>` template |
| Handlers | Created per-request, destroyed after response |
| Connections | Limited to `MAX_CONCURRENT_CONNECTIONS` (4) |
| Headers | Limited to `MAX_REQUEST_HEADER_COUNT` (32) |
| URI | Limited to `MAX_REQUEST_URI_LENGTH` (1024) |

All limits are configurable in `core/Defines.h`.

---

## HttpPipelineResponseStream: Zero-Copy Response Streaming

The `HttpPipelineResponseStream` class demonstrates a sophisticated approach to minimizing memory allocations during HTTP response transmission. Rather than building the complete response in memory before sending, it streams each component lazily.

### Architecture

```
HttpPipelineResponseStream : ConcatStream<5>
├── [0] Start Line Stream (IndefiniteConcatStream<HttpHeadersStartLineIterator>)
│       └── "HTTP/1.1 " + status_code + " " + reason + "\r\n"
├── [1] CRLF Stream (OctetsStream)
├── [2] Headers Stream (IndefiniteConcatStream<HttpHeaderCollectionStreamIterator>)
│       └── For each header: name + ": " + value + "\r\n"
├── [3] CRLF Stream (OctetsStream)
└── [4] Body Stream (HttpResponseBodyStream)
```

### Allocation Minimization Strategies

#### 1. Fixed-Size Stream Concatenation

```cpp
class HttpPipelineResponseStream : public ConcatStream<5>
```

Uses a compile-time fixed array of 5 streams (`std::array<std::unique_ptr<Stream>, 5>`) instead of a dynamic `std::vector`. This eliminates heap allocation for the stream container itself.

#### 2. Lazy Iterator-Based Stream Generation

The iterator classes (`HttpHeadersStartLineIterator`, `HttpHeaderCollectionStreamIterator`) don't pre-allocate all content. They generate streams on-demand as bytes are read:

```cpp
// FixedStreamIterable<N> - knows size at compile time
class HttpHeadersStartLineIterator : public FixedStreamIterable<HttpHeadersStartLineIterator, 5>
{
protected:
    value_type getAt(size_t index) const override {
        switch (index) {
            case 0: return std::make_unique<OctetsStream>(ResponseStringConstants::HTTP_VERSION);
            case 1: return std::make_unique<OctetsStream>(String(static_cast<uint16_t>(status_)));
            // ... streams created only when iterator advances to them
        }
    }
};
```

#### 3. Zero-Copy Constant Strings

Static response components use `constexpr` character arrays:

```cpp
namespace ResponseStringConstants {
    constexpr const char CRLF[3] = "\r\n";
    constexpr const char START_LINE_DELIMITER[2] = " ";
    constexpr const char HEADER_DELIMITER[3] = ": ";
    constexpr const char HTTP_VERSION[10] = "HTTP/1.1 ";
}
```

`OctetsStream` holds only a pointer and length—no string copying:

```cpp
class OctetsStream : public ReadStream {
    const uint8_t* data_;  // Points directly to source data
    size_t length_;
    size_t position_ = 0;
    bool ownsData_ = false;  // False for string literals
};
```

#### 4. IndefiniteConcatStream Iterator Pattern

`IndefiniteConcatStream<ForwardIt>` consumes streams one at a time:

```cpp
template <typename ForwardIt, typename Sentinel = ForwardIt>
class IndefiniteConcatStream : public ReadStream {
    ForwardIt current_;
    Sentinel end_;
    // Only one sub-stream active at a time
    // Previous streams deallocated as iterator advances
};
```

**Memory lifecycle:**
1. Iterator starts at first element
2. First stream created and read until exhausted
3. First stream deallocated, iterator advances
4. Next stream created (previous memory now available)
5. Repeat until all elements consumed

#### 5. Move Semantics for Body Stream

The response body is moved, never copied:

```cpp
std::unique_ptr<Stream> getBodyStream() {
    return std::move(bodyStream_);  // Ownership transfer, no copy
}
```

#### 6. Deferred Header Finalization

Required headers (Content-Length, Date) are added only after the body stream is obtained:

```cpp
std::unique_ptr<Stream> getHeadersStream() {
    bodyStream_ = response_->getBody();
    EnsureRequiredHeaders(response_->headers(), bodyStream_->available());
    // Headers stream created after body size is known
}
```

### Memory Profile During Response Send

For a typical response with 5 headers and 1KB body:

| Traditional Approach | HttpPipelineResponseStream |
|---------------------|---------------------------|
| Build complete response string in RAM | Stream components on-demand |
| ~1.5KB+ allocation for response buffer | ~50 bytes for active stream + iterators |
| Peak memory = full response size | Peak memory = largest single component |
| All headers concatenated upfront | Each header streamed individually |

### Streaming Flow

```
read() called by HttpPipeline
    │
    ├─► ConcatStream<5>::read()
    │       │
    │       ├─► streams_[0]->read() (start line)
    │       │       │
    │       │       └─► IndefiniteConcatStream iterates through:
    │       │               "HTTP/1.1 " → "200" → " " → "OK" → "\r\n"
    │       │               (each OctetsStream created/destroyed in sequence)
    │       │
    │       ├─► streams_[1]->read() (CRLF)
    │       │
    │       ├─► streams_[2]->read() (headers)
    │       │       │
    │       │       └─► For each header in collection:
    │       │               name → ": " → value → "\r\n"
    │       │
    │       ├─► streams_[3]->read() (CRLF)
    │       │
    │       └─► streams_[4]->read() (body)
    │
    └─► Bytes sent to client incrementally
```

### Key Benefits

1. **Constant memory overhead** regardless of response size
2. **No intermediate string concatenation** for headers
3. **Body can be arbitrarily large** (streamed from file, generated on-the-fly)
4. **Chunked transfer encoding** integrates naturally via `ChunkedHttpResponseBodyStream`
5. **Suitable for memory-constrained embedded systems** like RP2040 (264KB RAM)
