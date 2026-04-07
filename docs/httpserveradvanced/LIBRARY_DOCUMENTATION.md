# HttpServerAdvanced Library Documentation

A comprehensive, pipeline-based HTTP server library for cross-platform application code that targets both embedded and desktop environments. The HTTP core, routing, handlers, and response pipeline are designed to remain portable while transport, filesystem, clock, and framework-owned integration points stay at the platform boundary. Optional JSON support can be enabled through the library build configuration when the JSON dependency is available. HTTPS/TLS server support is not part of the maintained library surface.

Rename and extraction planning for the move to `lumalink::http` and `lumalink::platform` is tracked in `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md`. This document still describes the current `HttpServerAdvanced` surface.

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
5. [Transport Factory Migration](#transport-factory-migration)
6. [Rename Inventory](#rename-inventory)
7. [WebSocket Support Scope](#websocket-support-scope)

---

## Architecture Overview

HttpServerAdvanced uses a **pipeline-based architecture** inspired by modern web frameworks (ASP.NET, Express.js). The library separates concerns into a portable application core plus adapter-oriented platform edges:

```
┌─────────────────────────────────────────────────────────────────┐
│                         WebServer                                │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    HttpServerBase                        │    │
│  │  ┌───────────────────────────────────────────────────┐  │    │
│  │  │                  HttpPipeline                      │  │    │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌───────────┐  │  │    │
│  │  │  │RequestParser│→ │ HttpContext │→ │IHttpHandler│  │  │    │
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
- **Cross-Platform Core**: Request handling, routing, and response composition are designed to move unchanged between embedded and desktop targets
- **Adapter Boundaries**: Transport, filesystem, timing, and framework-specific integration stay behind explicit seams
- **Static Buffers**: Configurable bounded memory for embedded constraints

---

## Transport Factory Migration

Transport factories now have a compile-time primary path and a runtime compatibility path.

### Static Factory Contract

- A transport factory type should expose static `createServer(std::uint16_t)`, `createClient(std::string_view, std::uint16_t)`, and `createPeer()` methods.
- [src/httpadv/v1/transport/TransportTraits.h](src/httpadv/v1/transport/TransportTraits.h) detects that contract and exposes `IsStaticTransportFactoryV<TFactory>`.
- The same header also exposes `makeTransportFactory<TFactory>()` for legacy code that still needs a `std::unique_ptr<ITransportFactory>`.

## Rename Inventory

The concrete Phase 2 rename inventory and target include-root decisions live in `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md`.

Key outcomes locked by that note:

- Current public rename surfaces have been inventoried across package metadata, umbrella headers, namespaces, macro prefixes, and docs.
- The target HTTP include root is `lumalink/http/` with no public `v1` path segment.
- Platform-specific adapter implementations should stop leaking through the HTTP umbrella surface and move behind the separate `lumalink::platform` package boundary.


## WebSocket Support Scope

WebSocket support is implemented as an HTTP upgrade path plus a dedicated upgraded session runtime. It is intentionally not modeled as a normal `IHttpHandler` body variant.

### Registration API

- Register websocket endpoints with `ProviderRegistryBuilder::websocket(path, WebSocketCallbacks)`.
- The callback surface is `WebSocketCallbacks` with five fields:
    - `onOpen`
    - `onText`
    - `onBinary`
    - `onClose`
    - `onError`

### Upgrade Seam

- Routing first decides websocket endpoint intent by matching registered websocket paths.
- Only matched websocket routes flow into the dedicated upgrade validation path.
- The dedicated upgrade handler validates headers and generates `101 Switching Protocols` + `Sec-WebSocket-Accept`.
- Unmatched websocket upgrade candidates continue through normal HTTP handler behavior.

### Supported Frame/Session Behavior

- Client-frame parsing with masking enforcement.
- Text, binary, continuation, ping, pong, and close frame handling.
- Automatic `PONG` generation for inbound `PING`.
- Close sequencing with callback signaling.
- Synchronous direct writes with partial-write resume across loop iterations.

### Configured WebSocket Limits

All limits are compile-time constants in `src/core/Defines.h` and can be overridden via build flags:

- `WsMaxFramePayloadSize` (default `4096`)
- `WsMaxMessageSize` (default `8192`)
- `WsIdleTimeoutMs` (default `30000`)
- `WsCloseTimeoutMs` (default `2000`)

Messages exceeding `WsMaxMessageSize` are rejected with close code `1009`.

### Unsupported Features (Merge-Gate List)

The current implementation does **not** support:

- WebSocket extensions, including `permessage-deflate` and RFC 7692+ extension negotiation.
- Subprotocol negotiation (`Sec-WebSocket-Protocol` is treated as intent, but no negotiated subprotocol is selected).
- Multiplexing (exactly one WebSocket per TCP connection).
- Server-initiated `PING` frames.
- `PING`/`PONG` observation callbacks.
- Outbound sends from outside the pipeline loop.

### Native Validation Path

Use the native suite to validate websocket behavior:

```powershell
pio test -e native
```

Relevant websocket coverage currently lives in:

- `test/test_native/test_http_context.cpp`
- `test/test_native/test_pipeline.cpp`
- `test/test_native/test_websocket_frame_codec.cpp`
- `test/test_native/test_websocket_error_policy.cpp`

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
| `HttpContext` | Central request context implementing `IPipelineHandler` |
| `HttpContextPhase` | Bitmask flags for request lifecycle phases |
| `HttpTimeouts` | Timeout configuration (connect, header, body, keep-alive) |
| `HttpContentTypes` | MIME type registry with extension mapping |
| `IHttpContextHandlerFactory` | Interface for creating request handlers |
| `HttpContextHandlerFactory` | Default implementation using `HandlerProviderRegistry` |
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

#### HttpRequestContext

The abstract request surface exposed to application callbacks and middleware:

```cpp
class HttpRequestContext {
public:
    virtual std::string_view version() const = 0;
    virtual std::string_view versionView() const = 0;
    virtual const char* method() const = 0;
    virtual std::string_view methodView() const = 0;
    virtual std::string_view url() const = 0;
    virtual std::string_view urlView() const = 0;
    virtual const HttpHeaderCollection& headers() const = 0;
    virtual std::string_view remoteAddress() const = 0;
    virtual uint16_t remotePort() const = 0;
    virtual std::string_view localAddress() const = 0;
    virtual uint16_t localPort() const = 0;
    virtual std::map<std::string, std::any>& items() const = 0;
    virtual UriView& uriView() const = 0;
};
```

Use `HttpRequestContext` in consumer-facing handlers, predicates, and interceptors when you only need request metadata, URI parsing, and shared items. Construct responses directly from response types such as `StringResponse::create(...)`. `HttpRequestContext` is implemented by `HttpContext`, which remains the concrete runtime object owned by the parser and pipeline.

#### HttpContext

The concrete request context passed through the pipeline:

```cpp
class HttpContext : public HttpRequestContext, public IPipelineHandler {
    // Request properties
    std::string_view methodView() const;
    std::string_view urlView() const;
    std::string_view versionView() const;
    const HttpHeaderCollection& headers() const;
    std::string_view remoteAddress() const;
    
    // Parsed URI components
    const UriView& uriView() const;
    
    // Request items (arbitrary key-value storage)
    std::map<String, String>& items();
    
    // Lifecycle phases
    HttpContextPhase currentPhase() const;
    bool hasPhase(HttpContextPhase phase) const;
    
    // Handler invocation
    void invokeHandler(IHttpHandler& handler);
    void sendResponse(std::unique_ptr<IHttpResponse> response);
    
    // Factory method
    static PipelineHandlerPtr createPipelineHandler(HttpServerBase& server, IHttpContextHandlerFactory& factory);
};
```

#### HttpContextPhase

Bitmask flags tracking request lifecycle:

```cpp
enum class HttpContextPhase : uint16_t {
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
| `GetRequest` | User-defined | None | `(HttpRequestContext&, RouteParameters&&) → HandlerResult` or `(HttpRequestContext&) → HandlerResult` |
| `Json` | User-defined | Buffered JSON document | `(HttpRequestContext&, RouteParameters&&, JsonDocument&&) → HandlerResult` when JSON support is enabled |
| `Form` | User-defined | URL-encoded body | `(HttpRequestContext&, RouteParameters&&, QueryParameters&&) → HandlerResult` |
| `Multipart` | User-defined | multipart/form-data | `(HttpRequestContext&, RouteParameters&&, PostBodyData&&) → HandlerResult` |
| `RawBody` | User-defined | Raw bytes | `(HttpRequestContext&, RouteParameters&, RawBodyBuffer) → HandlerResult` |
| `BufferedString` | User-defined | Buffered string body | `(HttpRequestContext&, RouteParameters&&, std::string&&) → HandlerResult` |

#### IHttpHandler Interface

```cpp
class IHttpHandler {
public:
    using HandlerResult = std::unique_ptr<IHttpResponse>;
    class InvocationNext {
    public:
        HandlerResult operator()() const;
        HttpRequestContext& context() const;
    };

    using InvocationCallback = std::function<HandlerResult(HttpRequestContext&)>;
    using InterceptorCallback = std::function<HandlerResult(HttpRequestContext&, InvocationNext)>;
    using Predicate = std::function<bool(HttpRequestContext&)>;
    using Factory = std::function<std::unique_ptr<IHttpHandler>(HttpContext&)>;
    
    // Called to process request (may be called multiple times for body chunks)
    virtual HandlerResult handleStep(HttpContext& context) = 0;
    
    // Called for each body chunk received
    virtual void handleBodyChunk(HttpContext& context, const uint8_t* at, size_t length) = 0;
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
    virtual void handleBodyChunk(HttpContext& context, const uint8_t* at, size_t length) override;
    
    // Subclasses implement to process buffered body
    virtual HandlerResult handleBufferedBody(HttpContext& context) = 0;
};
```

#### Handler Type Pattern

Each handler type (GetRequest, Json, Form, etc.) follows this pattern. `Json` is only available when JSON support is enabled:

```cpp
struct Json {
    using Invocation = std::function<HandlerResult(HttpRequestContext&, RouteParameters&&, JsonDocument&&)>;
    using InvocationWithoutParams = std::function<HandlerResult(HttpRequestContext&, JsonDocument&&)>;
    
    // Restrict matcher to application/json requests
    static void restrict(HandlerMatcher& matcher);
    
    // Create handler factory
    static IHttpHandler::Factory makeFactory(Invocation callback, ExtractArgsFromRequest extractor);
    
    // Curry helpers for interceptors
    static Invocation curryInterceptor(IHttpHandler::InterceptorCallback, Invocation);
    static Invocation curryWithoutParams(InvocationWithoutParams);
    static Invocation applyFilter(IHttpHandler::InterceptorCallback, Invocation);
    
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
| `ParameterizedUri` | HandlerMatcher variant for named path-segment patterns such as `/users/:id` |
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
    HandlerMatcher(std::string_view uriPattern,
                   std::string_view allowedMethods = "",
                   const std::initializer_list<std::string_view>& allowedContentTypes = {});
    
    // Check if this matcher handles the request
    bool canHandle(HttpContext& context) const;
    
    // Extract named and glob path matches (e.g., /users/:id -> {"id": "123"})
    RouteParameters extractParameters(HttpContext& context) const;
    
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
// Matches /users/42 and captures {"id": "42"}
ParameterizedUri("/users/:id");

// Matches /users/42/devices/7 and captures {"userId": "42", "deviceId": "7"}
ParameterizedUri("/users/:userId/devices/:deviceId");
```

`RouteParameters` is a `std::map<std::string, std::string>`. Named path parameters use `:name` syntax, where the parameter name starts with `:` and continues with `[a-zA-Z0-9_.-]+`.

Glob-style URI patterns are also supported. Use `*` to match a single path segment with glob semantics such as `*.html`, and use `**` to match zero or more full path segments. Glob captures are exposed through `RouteParameters` using their ordinal position converted to a string. For example, `HandlerMatcher("**/*.html")` matches `/pages/docs/index.html` and extracts `{"0": "pages/docs", "1": "index.html"}`.

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
    HandlerBuilder& allowMethods(std::string_view methods);
    
    // Restrict content types
    HandlerBuilder& allowContentTypes(const std::initializer_list<std::string_view>& contentTypes);
};
```

#### HandlerProviderRegistry

Central handler registry:

```cpp
class HandlerProviderRegistry {
public:
    // Create handler for a request (returns nullptr if no match)
    std::unique_ptr<IHttpHandler> createContextHandler(HttpContext& context);
    
    // Set default (404) handler
    void setDefaultHandlerFactory(IHttpHandler::Factory creator);
    
    // Add handler providers
    void add(IHandlerProvider& provider, AddPosition position = AddAt::End);
    void add(IHttpHandler::Predicate predicate, IHttpHandler::Factory handler, AddPosition position = AddAt::End);
    
    // Global middleware
    void with(IHttpHandler::InterceptorCallback interceptor);     // Wraps matched and fallback handlers
    void filterRequest(IHttpHandler::Predicate predicate);        // Gates provider evaluation before fallback
    void apply(IHttpResponse::ResponseFilter filter);             // Applies to non-null responses only
};
```

`HandlerMatcher` named path parameters match one path segment at a time and are exposed by parameter name in `RouteParameters`. Glob matchers are exposed in the same map using ordinal keys such as `"0"`, `"1"`, and so on.

#### BasicAuthentication

Interceptor for HTTP Basic Auth:

```cpp
// With fixed credentials
auto auth = BasicAuth("admin", "password123", "Admin Area");

// With validator function
auto auth = BasicAuth([](std::string_view user, std::string_view pass) {
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
server.cfg().on<GetRequest>("/api/data", handler).apply(cors);
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
| `IByteSource` | Readable byte-source contract used for response bodies |
| `ChunkedHttpResponseBodyStream` | Chunked transfer encoding support |

#### IHttpResponse Interface

```cpp
class IHttpResponse {
public:
    using ResponseFilter = std::function<std::unique_ptr<IHttpResponse>(std::unique_ptr<IHttpResponse>)>;
    
    virtual HttpStatus status() const = 0;
    virtual HttpHeaderCollection& headers() = 0;
    virtual std::unique_ptr<IByteSource> getBody() = 0;
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
    // From a parsed JSON document
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const JsonDocument& doc);
    
    // With additional headers
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const JsonDocument& doc,
                                                  std::initializer_list<HttpHeader> headers);
};
```

`JsonResponse` and the `Json` handler family are excluded from JSON-disabled builds.

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

Serves static files from a platform-provided filesystem adapter, whether the content lives on embedded flash, removable storage, or a host-backed filesystem.

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
    virtual File getFile(HttpContext& context) = 0;
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
    
    File getFile(HttpContext& context) override;
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
    StaticFilesBuilder& setFileLocator(std::unique_ptr<FileLocator> locator);
    StaticFilesBuilder& onNotFound(std::string_view requestPath);
    StaticFilesBuilder& onNotFound(std::function<std::optional<std::string>(HttpRequestContext&)> resolver);
};

// Factory function for use() pattern
std::function<void(WebServerBuilder&)>& StaticFiles(FS& fs, std::function<void(StaticFilesBuilder&)> setup = nullptr);
```

**Usage:**

```cpp
auto& contentFs = getContentFileSystem();

server.use(StaticFiles(contentFs, [](StaticFilesBuilder& files) {
    files.setFilesystemContentRoot("/www")
         .setRequestPathPrefixes("/", "/api");
}));

The builder now records default-locator configuration and constructs `DefaultFileLocator` only during initialization. If `setFileLocator(...)` is used, that explicit locator is installed instead and the deferred default-locator settings are ignored.

Missing static assets default to trying `/404.html` inside the static-files provider. Use `onNotFound("/index.html")` for a different fixed fallback, or `onNotFound(resolver)` to compute a fallback per request. If the resolver returns `std::nullopt`, the static-files provider declines the request and later handlers can still run.
```

---

### 8. Streams

**Location:** `src/streams/`

Stream utilities for encoding/decoding and composition.

#### Classes

| Class | Purpose |
|-------|---------|
| `ReadStream` | Single-byte-source convenience base for decoder and encoder wrappers |
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
| `KeyValuePairView<K,V>` | Immutable key-value collection |
| `UriView` | Parsed URI with scheme, host, path, query, fragment |
| `WebUtility` | URL/HTML encoding, Base64 utilities |

#### UriView

Parsed URI components:

```cpp
class UriView {
public:
    UriView(const String& uri);
    
    std::string_view scheme() const;     // URI scheme if present, e.g. "http"
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
    static std::vector<std::pair<std::string, std::string>> ParseQueryString(std::string_view query);
    
    // URI encoding/decoding
    static std::string DecodeURIComponent(std::string_view str);
    static std::string EncodeURIComponent(std::string_view str);
    
    // HTML encoding
    static std::string HtmlEncode(std::string_view str);
    static std::string HtmlAttributeEncode(std::string_view str);
    static std::string JavaScriptStringEncode(std::string_view str, bool includeQuotes = true);
    
    // Base64
    static std::string Base64Encode(std::string_view input, bool urlCompatible = false);
    static std::string Base64DecodeToString(std::string_view input, bool urlCompatible = false);
    static std::vector<uint8_t> Base64Decode(std::string_view input, bool urlCompatible = false);
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
│           └── owns → HttpContextHandlerFactory
├── owns → WebServerConfig (facade to WebServerBuilder)
└── inherits → StandardHttpServer
                └── inherits → HttpServerBase
                                ├── owns → std::vector<HttpPipeline>
                                └── owns → HttpTimeouts

HttpPipeline
├── owns → IClient (network connection)
├── owns → RequestParser
├── refs → IPipelineHandler (HttpContext)
└── owns → Stream (response stream)

HttpContext
├── owns → HttpHeaderCollection
├── owns → UriView
├── refs → IHttpContextHandlerFactory
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
#include <lumalink/http/HttpServerAdvanced.h>
#include <WiFi.h>

using namespace lumalink::http;

FriendlyWebServer<> server(80);

void setup() {
    WiFi.begin("SSID", "password");
    while (WiFi.status() != WL_CONNECTED) delay(500);
    
    server.cfg().on<GetRequest>("/", [](HttpRequestContext& req) {
        return StringResponse::text("Hello World!");
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
    [](HttpRequestContext& req, RouteParameters&& params) {
        const std::string& userId = params.at("id");
        return StringResponse::text("User: " + userId);
    })
    .allowMethods("GET");
```

Glob captures are available through ordinal keys in the same map:

```cpp
server.cfg().on<GetRequest>(HandlerMatcher("**/*.html"),
    [](HttpRequestContext& req, RouteParameters&& params) {
        const std::string& directory = params.at("0");
        const std::string& fileName = params.at("1");
        return StringResponse::text("HTML file: " + directory + "/" + fileName);
    })
    .allowMethods("GET");
```

### JSON API

This pattern requires optional JSON support to be enabled.

```cpp
server.cfg().on<Json>("/api/data", [](HttpRequestContext& req, JsonDocument&& body) {
    const std::string name = body["name"].isNull()
        ? std::string("anonymous")
        : body["name"].template as<std::string>();

    JsonDocument response;
    response["received"] = name;
    response["contentType"] = "application/json";
    return JsonResponse::create(HttpStatus::Ok(), response);
})
    .allowMethods("POST");
```

### JSON API With Path Parameters

Use `ParameterizedUri` when the JSON body and the route path both carry request state. Named path segments are captured into `RouteParameters` by key, and glob path segments are captured using ordinal string keys.

```cpp
server.cfg().on<Json>(
    ParameterizedUri("/api/users/:userId/devices/:deviceId/commands"),
    [](HttpRequestContext& req, RouteParameters&& params, JsonDocument&& body) {
        const std::string& userId = params.at("userId");
        const std::string& deviceId = params.at("deviceId");
        const std::string action = body["action"].isNull()
            ? std::string("status")
            : body["action"].template as<std::string>();
        const int priority = body["priority"].isNull()
            ? 0
            : body["priority"].template as<int>();

        JsonDocument response;
        response["userId"] = userId;
        response["deviceId"] = deviceId;
        response["action"] = action;
        response["priority"] = priority;
        response["accepted"] = true;
        return JsonResponse::create(HttpStatus::Accepted(), response);
    })
    .allowMethods("POST");
```

For glob-aware APIs, patterns such as `HandlerMatcher("/api/content/**/*.html")` populate `params.at("0")`, `params.at("1")`, and so on in match order.

If you need explicit validation, check `params.size()` before indexing and use `Json::deserializationError(req)` to detect malformed JSON payloads.

### Form Handling

```cpp
server.cfg().on<Form>("/submit", [](HttpRequestContext& req, WebUtility::QueryParameters&& form) {
    auto name = form.get("name");
    auto email = form.get("email");
    return StringResponse::text("Received: " + name.value_or("") + ", " + email.value_or(""));
});
```

### Authentication

```cpp
auto adminAuth = BasicAuth("admin", "secret", "Admin Area");

server.cfg().on<GetRequest>("/admin", [](HttpRequestContext& req) {
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
auto& contentFs = getContentFileSystem();

server.use(StaticFiles(contentFs, [](StaticFilesBuilder& files) {
    files.setFilesystemContentRoot("/www")
         .setRequestPathPrefixes("/static", "/api");
}));
```

### Global Middleware

```cpp
// Log all requests
server.cfg().with([](HttpRequestContext& req, IHttpHandler::InvocationNext next) {
    logRequest(req.method(), req.url());
    return next();
});

// Global interceptors and response filters wrap matched handlers as well as the fallback handler.

// Filter requests
server.cfg().filterRequest([](HttpRequestContext& req) {
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

## HttpPipelineResponseSource: Incremental Response Streaming

The `HttpPipelineResponseSource` class incrementally serializes HTTP responses without building the full wire payload in memory first. Instead of exposing a legacy `Stream` tree, it builds a composed `IByteSource` and lets the pipeline drain that source into a fixed-size buffer.

### Architecture

```
HttpPipelineResponseSource : IByteSource
├── response_ : std::unique_ptr<IHttpResponse>
└── source_ : ConcatByteSource
        ├── Start line (StdStringByteSource)
        ├── Headers block (StdStringByteSource)
        ├── Header/body delimiter (SpanByteSource)
        └── Optional body source (IByteSource)
```

### Allocation Minimization Strategies

#### 1. Byte-Source Composition

```cpp
class HttpPipelineResponseSource : public IByteSource
```

The serializer exposes one readable contract to the pipeline while delegating byte sequencing to `ConcatByteSource`.

#### 2. Deferred Header Finalization

Required headers are finalized only after the body source is obtained and inspected:

```cpp
std::unique_ptr<IByteSource> bodySource = response_->getBody();
const AvailableResult bodyAvailable = bodySource ? bodySource->available() : ExhaustedResult();
EnsureRequiredHeaders(response_->headers(), knownBodySize);
```

This keeps content-length and transfer-encoding decisions close to the real body source rather than forcing a prebuilt response buffer.

#### 3. Compact Constant-String Sources

Static response components use `constexpr` character arrays and short string-backed byte sources:

```cpp
namespace ResponseStringConstants {
    constexpr const char CRLF[3] = "\r\n";
    constexpr const char START_LINE_DELIMITER[2] = " ";
    constexpr const char HEADER_DELIMITER[3] = ": ";
    constexpr const char HTTP_VERSION[10] = "HTTP/1.1 ";
}
```

#### 4. Optional Chunking Wrapper

Chunked transfer encoding is layered only when the response headers request it:

```cpp
if (response_->headers().exists(HttpHeaderNames::TransferEncoding, "chunked") && bodySource)
{
    bodySource = ChunkedHttpResponseBodyStream::create(std::move(bodySource));
}
```

### Memory Profile During Response Send

For a typical response with 5 headers and 1KB body:

| Traditional Approach | HttpPipelineResponseSource |
|---------------------|---------------------------|
| Build complete response string in RAM | Stream components on-demand |
| Allocation scales with full serialized payload | Allocation stays near the active serializer components |
| Peak memory = full response size | Peak memory = largest single component |
| All headers concatenated upfront | Each header streamed individually |

### Streaming Flow

```
read() called by HttpPipeline
    │
    ├─► HttpPipelineResponseSource::read()
    │       │
    │       └─► ConcatByteSource::read()
    │               ├─► start line source
    │               ├─► headers block source
    │               ├─► CRLF delimiter source
    │               └─► optional body source
    │
    └─► Bytes sent to client incrementally
```

### Key Benefits

1. **Constant memory overhead** regardless of response size
2. **No intermediate string concatenation** for headers
3. **Body can be arbitrarily large** (streamed from file, generated on-the-fly)
4. **Chunked transfer encoding** integrates naturally via `ChunkedHttpResponseBodyStream`
5. **Suitable for memory-constrained embedded systems** while remaining practical to validate in native desktop test runs
