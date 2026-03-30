# Library Comparison: Framework WebServer vs HttpServerAdvanced

A focused comparison between a framework-provided **WebServer** implementation and **HttpServerAdvanced**, covering memory utilization, functionality, safety, and ease of use. The contrast is useful because HttpServerAdvanced keeps the HTTP application core portable across embedded and desktop targets instead of coupling application logic to one runtime stack.

---

## Executive Summary

### Memory Findings

| Criterion | WebServer | HttpServerAdvanced |
|-----------|-----------|-------------------|
| **Memory Predictability** | ⚠️ Variable | ✅ Bounded |
| **Fragmentation Risk** | ⚠️ Moderate-High | ✅ Low |
| **Leak Risk** | Low | Very Low |
| **DoS Protection** | ❌ None | ✅ Built-in |
| **Configurability** | Limited (3 params) | Extensive (10+ params) |
| **Long-running Stability** | ⚠️ Requires monitoring | ✅ Predictable |

**Key Memory Differences:**
- WebServer has **unbounded** dynamic allocations vulnerable to memory exhaustion DoS
- HttpServerAdvanced uses **bounded buffers** and static allocations where possible
- WebServer's `readBytesWithTimeout()` uses `malloc()/realloc()` pattern causing heap fragmentation
- HttpServerAdvanced uses RAII patterns (`std::unique_ptr`, `std::vector::reserve()`) reducing leak risk

### Functionality Findings

| Category | WebServer | HttpServerAdvanced |
|----------|-----------|-------------------|
| **Setup Complexity** | ⭐⭐⭐⭐⭐ Simple | ⭐⭐⭐⭐ Moderate |
| **Type Safety** | ⭐⭐ Weak | ⭐⭐⭐⭐⭐ Strong |
| **Memory Safety** | ⭐⭐⭐ Moderate | ⭐⭐⭐⭐⭐ Excellent |
| **Input Validation** | ⭐⭐ Limited | ⭐⭐⭐⭐⭐ Comprehensive |
| **JSON Support** | ⭐⭐ Manual | ⭐⭐⭐⭐⭐ Native when optional JSON is enabled |
| **Auth Patterns** | ⭐⭐⭐⭐ Good (+ Digest) | ⭐⭐⭐⭐ Good (middleware) |
| **Routing Flexibility** | ⭐⭐⭐⭐⭐ Excellent (+ Regex) | ⭐⭐⭐⭐ Good |
| **Static Files** | ⭐⭐⭐⭐ Good | ⭐⭐⭐⭐⭐ Excellent |
| **DoS Resistance** | ⭐⭐ Poor | ⭐⭐⭐⭐⭐ Excellent |
| **Maturity** | ⭐⭐⭐⭐⭐ Stable | ⭐⭐⭐ Developing |

**Key Functionality Differences:**
- WebServer offers simpler API but lacks input size limits
- HttpServerAdvanced has optional JSON support and middleware architecture
- WebServer supports Digest Auth and Regex routing (not in HttpServerAdvanced)
- HttpServerAdvanced provides fine-grained CORS control per-route

### Recommendations

**Choose WebServer if:** Building simple prototypes, need Digest auth or regex routing, prefer imperative API, memory constraints are not critical.

**Choose HttpServerAdvanced if:** Building production applications, need robust input validation, want optional built-in JSON handling, require middleware architecture, running long-term without reboots.

---

# Part 1: Memory Utilization Analysis

## 1.1 Per-Request Fixed Allocations

### WebServer

| Component | Size (bytes) | Notes |
|-----------|--------------|-------|
| `HTTPUpload.buf[]` | 1,436 | `HTTP_UPLOAD_BUFLEN` - per-upload buffer |
| `HTTPRaw.buf[]` | 1,436 | `HTTP_RAW_BUFLEN` - raw body buffer |
| Handler linked-list node | ~40 | Per-registered route |
| `RequestArgument` struct | ~48 | Two `String` objects (key/value) |

**Key observation**: WebServer uses dynamically-allocated upload/raw buffers via `std::unique_ptr<HTTPUpload>` and `std::unique_ptr<HTTPRaw>`. These are only allocated during file upload or raw body handling.

### HttpServerAdvanced

| Component | Size (bytes) | Notes |
|-----------|--------------|-------|
| `RequestParser::buffer_[]` | 1,024 | Static array - `REQUEST_PARSER_BUFFER_LENGTH` |
| Pipeline stack buffer | 256 | `PIPELINE_STACK_BUFFER_SIZE` - read/write buffer |
| Multipart form buffer | 1,436 | `MULTIPART_FORM_DATA_BUFFER_SIZE` |
| `HttpHeaderCollection` | ~24 + N×48 | Vector of headers (N typically ≤32) |

**Key observation**: HttpServerAdvanced uses stack-allocated fixed buffers for parsing, reducing heap fragmentation.

---

## 1.2 Dynamic Allocations Per Request

### WebServer Dynamic Allocations

| Operation | Allocation Pattern | Unbounded? |
|-----------|-------------------|------------|
| `readBytesWithTimeout()` | `malloc()` + `realloc()` growth | ⚠️ **YES** - grows with body size |
| `_parseArguments()` | `new RequestArgument[_currentArgCount+1]` | ⚠️ **YES** - count depends on query string |
| `_parseForm()` | `new RequestArgument[WEBSERVER_MAX_POST_ARGS]` | Bounded to 32 |
| `collectHeaders()` | `new RequestArgument[_headerKeysCount]` | ⚠️ User-controlled |
| `String` concatenation | Multiple temporary allocations | ⚠️ **YES** |
| `FunctionRequestHandler` | `new` on route registration | Per-route |
| `StaticRequestHandler` | `new` on `serveStatic()` | Per-static-route |
| `_currentClient` | `new ClientType()` per request | Per-request |

**Critical path in `Parsing.cpp`**:
```cpp
char* readBytesWithTimeout(WiFiClient* client, size_t maxLength, ...) {
    buf = (char *) malloc(newLength + 1);        // Initial allocation
    buf = (char *) realloc(buf, dataLength + newLength + 1);  // Growth
}
```
This pattern creates unbounded heap growth for large POST bodies.

### HttpServerAdvanced Dynamic Allocations

| Operation | Allocation Pattern | Unbounded? |
|-----------|-------------------|------------|
| `HttpContext` construction | `std::unique_ptr` | Single object per request |
| `HttpHeaderCollection` | `std::vector<HttpHeader>` | Bounded by `MAX_REQUEST_HEADER_COUNT` (32) |
| `BufferingHttpHandlerBase` | `std::vector<uint8_t>` with reserve | Bounded by `MAX_BUFFERED_BODY_LENGTH` (2KB) |
| Handler factory | `std::unique_ptr<IHttpHandler>` | Single handler per request |
| Response stream | `std::unique_ptr<Stream>` | Single stream per response |
| Request text storage | Compatibility-owned text values | Bounded by defined limits |

**Bounded buffer pattern**:
```cpp
template <ssize_t MaxBuffered = MAX_BUFFERED_BODY_LENGTH>
class BufferingHttpHandlerBase {
    std::vector<uint8_t> bodyBuffer_;
    // Bounded: bodyBuffer_.reserve(std::min(announcedLength, maxBuffered));
};
```

---

## 1.3 Memory Bounds Comparison

### WebServer Limits

| Parameter | Default | Configurable? |
|-----------|---------|---------------|
| `HTTP_UPLOAD_BUFLEN` | 1,436 | Yes (`#ifndef`) |
| `HTTP_RAW_BUFLEN` | 1,436 | Yes (`#ifndef`) |
| `WEBSERVER_MAX_POST_ARGS` | 32 | Yes (`#ifndef`) |
| Body size limit | **NONE** | ❌ No |
| Header count limit | **NONE** | ❌ No |
| URI length limit | **NONE** | ❌ No |

### HttpServerAdvanced Limits

| Parameter | Default | Configurable? |
|-----------|---------|---------------|
| `MAX_REQUEST_URI_LENGTH` | 1,024 | Yes |
| `MAX_REQUEST_HEADER_COUNT` | 32 | Yes |
| `MAX_REQUEST_HEADER_NAME_LENGTH` | 64 | Yes |
| `MAX_REQUEST_HEADER_VALUE_LENGTH` | 256 | Yes |
| `MAX_BUFFERED_BODY_LENGTH` | 2,048 | Yes |
| `MAX_BUFFERED_FORM_BODY_LENGTH` | 2,048 | Yes |
| `MAX_BUFFERED_JSON_BODY_LENGTH` | 2,048 | Yes |
| `REQUEST_PARSER_BUFFER_LENGTH` | 1,024 | Yes |
| `PIPELINE_STACK_BUFFER_SIZE` | 256 | Yes |
| `MULTIPART_FORM_DATA_BUFFER_SIZE` | 1,436 | Yes |

---

## 1.4 Scenario Analysis

### Best Case Scenario

| Metric | WebServer | HttpServerAdvanced |
|--------|-----------|-------------------|
| **Request Type** | GET with no args | GET with no args |
| **Heap Allocations** | ~3-5 | ~2-3 |
| **Peak Memory** | ~500 bytes | ~400 bytes |
| **Fragmentation Risk** | Low | Very Low |

Both libraries perform similarly for simple GET requests.

### Average Case Scenario

| Metric | WebServer | HttpServerAdvanced |
|--------|-----------|-------------------|
| **Request Type** | POST with 500B JSON | POST with 500B JSON |
| **Heap Allocations** | ~8-15 | ~4-6 |
| **Peak Memory** | ~1.5-2 KB | ~1.2 KB |
| **String Operations** | Heavy (concatenation) | Moderate (views preferred) |
| **Fragmentation Risk** | Moderate | Low |

HttpServerAdvanced uses fixed buffers and avoids realloc patterns.

### Worst Case Scenario

| Metric | WebServer | HttpServerAdvanced |
|--------|-----------|-------------------|
| **Request Type** | Large POST with file upload | Large POST with file upload |
| **Heap Allocations** | **Unbounded** | **Bounded** |
| **Peak Memory** | Up to request body size | ~3-4 KB max |
| **Fragmentation Risk** | **HIGH** | Low |
| **DoS Vulnerability** | ⚠️ **Yes** - memory exhaustion | Protected by limits |

**WebServer vulnerability**: A malicious client can send an arbitrarily large POST body, causing `readBytesWithTimeout()` to exhaust heap memory.

**HttpServerAdvanced protection**: Body handlers have explicit limits; oversized bodies are rejected or truncated.

---

## 1.5 Heap Fragmentation Analysis

### WebServer Fragmentation Patterns

1. **Realloc growth in parsing**: `readBytesWithTimeout()` starts small and grows, creating fragmentation
2. **String concatenation**: Multiple temporary String objects during header/response building
3. **Argument arrays**: Dynamically allocated based on request, then freed
4. **Handler allocation**: `new FunctionRequestHandler` per route creates stable allocations
5. **Per-request client**: `new ClientType()` allocated and freed per request

**Risk**: After many requests with varying body sizes, the heap becomes fragmented with many small free blocks.

### HttpServerAdvanced Fragmentation Patterns

1. **Static parser buffer**: `std::array` on stack/object, no heap fragmentation
2. **Pre-reserved vectors**: `bodyBuffer_.reserve()` allocates once at expected size
3. **Bounded allocations**: All dynamic allocations have maximum sizes
4. **Smart pointer cleanup**: `std::unique_ptr` ensures proper deallocation
5. **Handler factory pattern**: Single handler object per request

**Risk**: Lower fragmentation due to predictable allocation sizes and static buffers.

---

## 1.6 Memory Leak Analysis

### WebServer Potential Leaks

| Location | Risk | Description |
|----------|------|-------------|
| `authenticate()` | Low | Early return paths properly `delete[]` buffers |
| `_parseForm()` | Low | `delete[]` of `_postArgs` at end |
| Handler chain | Low | Destructor walks and deletes chain |
| Error paths | ⚠️ Moderate | Some error returns may skip cleanup |

Code review shows mostly correct cleanup, but complex error paths in `Parsing.cpp` could leak under edge conditions.

### HttpServerAdvanced Potential Leaks

| Location | Risk | Description |
|----------|------|-------------|
| Smart pointers | Very Low | RAII ensures cleanup |
| Handler factory | Very Low | `std::unique_ptr` ownership |
| Pipeline cleanup | Very Low | Destructor chain handles cleanup |

HttpServerAdvanced consistently uses RAII patterns, significantly reducing leak risk.

---

## 1.7 Memory Usage Summary

### Static/Compile-Time Memory

| Library | Code Size Impact | RAM (BSS/Data) |
|---------|-----------------|----------------|
| WebServer | ~25 KB | ~2 KB |
| HttpServerAdvanced | ~35 KB | ~3 KB |

HttpServerAdvanced has larger code size due to templates and additional features.

### Runtime Memory Per Request

| Scenario | WebServer | HttpServerAdvanced |
|----------|-----------|-------------------|
| Minimal GET | ~400 B | ~400 B |
| GET with 5 query params | ~600 B | ~500 B |
| POST 1KB body | ~1.5 KB | ~1.2 KB |
| POST 10KB body | ~12 KB | ~3 KB (truncated) |
| File upload (streaming) | ~1.5 KB | ~1.5 KB |

---

## 1.8 Response Serialization Overhead (HttpServerAdvanced)

HttpServerAdvanced uses an incremental response serializer (`HttpPipelineResponseSource`) that trades a small amount of serializer state for constant-memory response sending. The exact internal object layout changed as the library moved from legacy `Stream` composition toward `IByteSource`, but the design goal remains the same: avoid buffering the full serialized response in RAM.

### Current Serializer Shape

- `HttpPipelineResponseSource` owns the `IHttpResponse` while building a composed `IByteSource`
- start line and headers are materialized as compact string-backed byte sources
- the serializer adds a small delimiter source between headers and body
- the body stays as its original `IByteSource`, optionally wrapped by `ChunkedHttpResponseBodyStream`

### Comparison: Framework vs Direct Serialization

| Approach | Memory | CPU |
|----------|--------|-----|
| **HttpPipelineResponseSource** | Small fixed serializer state plus active body source | Extra dispatch and composition overhead, especially for header emission |
| **Direct sprintf to buffer** | Response size plus staging buffer | Lower per-byte overhead |
| **String concatenation** | 2-3x response size during copies and reallocation | Moderate overhead plus copy cost |

### Trade-off Summary

| Factor | Overhead | Benefit |
|--------|----------|---------|
| Memory | Small fixed serializer state | No large buffer needed for full response |
| CPU | Additional composition overhead for headers | Body streams with zero copy |
| Latency | Slight increase for first byte | Constant memory for any response size |
| Complexity | Byte-source composition machinery | Composable, testable stream components |

**For small-RAM embedded targets:** the serializer-state overhead is still tiny compared with buffering multi-kilobyte responses in memory.

### WebServer Response Approach (Comparison)

WebServer uses simpler direct writes:
```cpp
_currentClient.println("HTTP/1.1 200 OK");
_currentClient.printf("Content-Length: %d\r\n", contentLength);
```

- **Advantage**: Lower per-byte CPU overhead
- **Disadvantage**: Requires content to be fully available before sending headers (for Content-Length), or manual chunked encoding

---

## 1.9 Memory Recommendations

### For Memory-Constrained Deployments

If using **WebServer**:
- Define `WEBSERVER_MAX_POST_ARGS` to limit argument count
- Implement request size validation in hook
- Monitor heap fragmentation in long-running deployments
- Consider periodic reboots for stability

If using **HttpServerAdvanced**:
- Tune `MAX_BUFFERED_*` constants for your use case
- Use streaming handlers for large payloads
- Memory usage is predictable and bounded

### For DoS Resistance

**WebServer**: Add a pre-parsing hook to reject large Content-Length:
```cpp
server.addHook([](const String& method, const String& url, WiFiClient* client, ...) {
    // Manual header check needed - complex to implement
    return WebServer::CLIENT_REQUEST_CAN_CONTINUE;
});
```

**HttpServerAdvanced**: Built-in protection via defined limits.

---

# Part 2: Functionality Analysis

## 2.1 Capability Comparison

### Core HTTP Features

| Feature | WebServer | HttpServerAdvanced |
|---------|-----------|-------------------|
| HTTP/1.0 Support | ✅ | ✅ |
| HTTP/1.1 Support | ✅ | ✅ |
| GET Requests | ✅ | ✅ |
| POST Requests | ✅ | ✅ |
| PUT Requests | ✅ | ✅ |
| DELETE Requests | ✅ | ✅ |
| PATCH Requests | ✅ | ✅ |
| HEAD Requests | ✅ | ✅ |
| OPTIONS Requests | ✅ | ✅ |
| Chunked Transfer Encoding | ✅ | ✅ |
| Keep-Alive | ❌ | ❌ |
| HTTP/2 | ❌ | ❌ |

### Request Handling

| Feature | WebServer | HttpServerAdvanced |
|---------|-----------|-------------------|
| Query String Parsing | ✅ Auto-parsed | ✅ Via `UriView` |
| URL Parameters | ✅ `UriBraces` (`/user/{}`) | ✅ Pattern matching (`/user/?`) |
| Form URL-Encoded Parsing | ✅ Auto-parsed | ✅ `FormBodyHandler` |
| Multipart Form Data | ✅ Built-in | ✅ `MultipartFormDataHandler` |
| File Uploads | ✅ Callback-based | ✅ Streaming with status events |
| Raw Body Access | ✅ Callback-based | ✅ `RawBodyHandler` |
| JSON Body Parsing | ❌ Manual | ✅ `JsonBodyHandler` when optional JSON support is enabled |
| Request Header Access | ✅ By name/index | ✅ `HttpHeaderCollection` |
| Remote Address Access | ✅ `client()` | ✅ `request.remoteAddress()` |

### Response Features

| Feature | WebServer | HttpServerAdvanced |
|---------|-----------|-------------------|
| Plain Text Response | ✅ `send()` | ✅ `StringResponse` |
| JSON Response | ✅ Manual | ✅ `JsonResponse` when JSON support is enabled |
| Binary Response | ✅ `send()` with length | ✅ Stream-based |
| File Streaming | ✅ `streamFile()` | ✅ `StaticFileHandler` |
| Chunked Response | ✅ `chunkedResponseModeStart()` | ✅ `ChunkedHttpResponseBodyStream` |
| Custom Headers | ✅ `sendHeader()` | ✅ `HttpHeaderCollection` |
| Status Codes | ✅ Numeric | ✅ `HttpStatus` enum |

### Routing

| Feature | WebServer | HttpServerAdvanced |
|---------|-----------|-------------------|
| Exact Path Matching | ✅ | ✅ |
| Wildcard Patterns | ✅ `UriGlob` | ✅ `REQUEST_MATCHER_PATH_WILDCARD_CHAR` (`?` by default) |
| Parameterized Routes | ✅ `UriBraces` | ✅ `ParameterizedUri` |
| Regex Routes | ✅ `UriRegex` | ❌ Not built-in |
| Method Filtering | ✅ Per-handler | ✅ Per-handler + content-type |
| Content-Type Filtering | ❌ | ✅ Built-in |
| Route Removal | ✅ `removeRoute()` | ❌ Not implemented |
| Filter Functions | ✅ `setFilter()` | ✅ Predicates |

### Static File Serving

| Feature | WebServer | HttpServerAdvanced |
|---------|-----------|-------------------|
| Filesystem Integration | ✅ `serveStatic()` | ✅ `StaticFileHandler` |
| MIME Type Detection | ✅ Built-in table | ✅ `HttpContentTypes` |
| Cache Headers | ✅ Optional | ✅ ETag support |
| GZip Pre-compression | ✅ Auto-detect `.gz` | ✅ Configurable |
| Directory Index | ✅ `index.htm` | ✅ Configurable |
| File Locator Abstraction | ❌ | ✅ `FileLocator` interface |
| Aggregate File Sources | ❌ | ✅ `AggregateFileLocator` |

### Security Features

| Feature | WebServer | HttpServerAdvanced |
|---------|-----------|-------------------|
| Basic Authentication | ✅ `authenticate()` | ✅ `BasicAuth()` middleware |
| Digest Authentication | ✅ Built-in | ❌ Not implemented |
| CORS Support | ✅ `enableCORS()` | ✅ `CrossOriginRequestSharing()` |
| HTTPS/TLS | ✅ `WebServerSecure` | ❌ Removed from library scope |
| Request Size Limits | ❌ | ✅ Built-in |
| Header Size Limits | ❌ | ✅ Built-in |
| Timeout Protection | ✅ Fixed values | ✅ Configurable |

### Architecture

| Feature | WebServer | HttpServerAdvanced |
|---------|-----------|-------------------|
| Handler Pattern | Callback functions | Handler objects with lifecycle |
| Middleware/Interceptors | ✅ Hooks | ✅ `InterceptorCallback` |
| Request Context | Global server state | Dedicated `HttpContext` object |
| Response Building | Procedural `send()` calls | Response object pattern |
| Body Streaming | Limited | Full streaming support |
| Type Safety | Moderate | Strong (templates) |

---

## 2.2 Safety Comparison

### Input Validation

| Concern | WebServer | HttpServerAdvanced |
|---------|-----------|-------------------|
| URI Length Validation | ❌ Unbounded | ✅ `MAX_REQUEST_URI_LENGTH` |
| Header Count Limit | ❌ Unbounded | ✅ `MAX_REQUEST_HEADER_COUNT` |
| Header Size Limit | ❌ Unbounded | ✅ `MAX_REQUEST_HEADER_*_LENGTH` |
| Body Size Limit | ❌ Unbounded | ✅ `MAX_BUFFERED_BODY_LENGTH` |
| Query Param Count | Bounded (32) | ✅ Bounded by handler |
| Multipart Boundary | ✅ Validated | ✅ Validated |

### Resource Protection

| Concern | WebServer | HttpServerAdvanced |
|---------|-----------|-------------------|
| Memory Exhaustion DoS | ⚠️ **Vulnerable** | ✅ Protected |
| Slow Loris Protection | ⚠️ Limited | ✅ Activity timeouts |
| Connection Timeout | ✅ Fixed timeouts | ✅ Configurable |
| Request Duration Limit | ⚠️ Per-phase only | ✅ Total request limit |
| Malformed Request Handling | ⚠️ Basic | ✅ Parser error codes |

### Error Handling

| Concern | WebServer | HttpServerAdvanced |
|---------|-----------|-------------------|
| Parse Errors | ✅ `CLIENT_MUST_STOP` | ✅ `PipelineErrorCode` enum |
| Handler Exceptions | ❌ Undefined behavior | ✅ Contained in handler |
| Response After Error | ✅ 404 fallback | ✅ Error response chain |
| Client Disconnect | ✅ Detected | ✅ `onClientDisconnected()` |
| Timeout Notification | ❌ Silent | ✅ `onError(Timeout)` |

### Memory Safety

| Concern | WebServer | HttpServerAdvanced |
|---------|-----------|-------------------|
| Buffer Overflows | ⚠️ Possible in parsing | ✅ Bounded buffers |
| Use After Free | ⚠️ Manual delete | ✅ RAII/smart pointers |
| Double Free | ⚠️ Manual management | ✅ Smart pointers |
| Null Pointer Access | ⚠️ Some unchecked | ✅ Null checks throughout |
| String Growth Risk | ⚠️ Framework-owned dynamic strings | ✅ Explicit size checks |

---

## 2.3 Ease of Use

### Getting Started

Network bootstrap is intentionally omitted from both snippets below because connection setup is platform-owned. The comparison focuses on handler registration and request processing shape.

**WebServer - Hello World:**
```cpp
#include <WebServer.h>

WebServer server(80);

void handleRoot() {
    server.send(200, "text/plain", "Hello World!");
}

void setup() {
    server.on("/", handleRoot);
    server.begin();
}

void loop() {
    server.handleClient();
}
```

**HttpServerAdvanced - Hello World:**
```cpp
#include <HttpServerAdvanced.h>

WebServer server;

Response helloHandler(HttpContext &request) {
    return StringResponse::create(HttpStatus::Ok(), "text/plain", "Hello World!");
}

void setup() {
    server.cfg().on<GetRequest>("/", helloHandler);
    server.begin();
}

void loop() {
    server.handleClient();
}
```

**Verdict**: Both are similarly simple for basic use cases. HttpServerAdvanced requires slightly more typing but provides type safety.

### Request Data Access

**WebServer - Accessing Parameters:**
```cpp
void handleUser() {
    String id = server.pathArg(0);      // URL parameter
    String name = server.arg("name");   // Query/form parameter
    String auth = server.header("Authorization");
    
    server.send(200, "text/plain", "User: " + id);
}
```

**HttpServerAdvanced - Accessing Parameters:**
```cpp
Response handleUser(HttpContext &request, std::vector<String> &&params) {
    String id = params[0];              // URL parameter
    String name = request.uriView().query("name");
    auto auth = request.headers().find("Authorization");
    
    return StringResponse::create(HttpStatus::Ok(), "text/plain", "User: " + id);
}
```

**Verdict**: WebServer has a simpler global-access model; HttpServerAdvanced uses explicit parameter passing which is safer but more verbose.

### JSON Handling

**WebServer - JSON Response:**
```cpp
void handleJson() {
    // Manual JSON construction
    String json = "{\"status\":\"ok\",\"value\":" + String(42) + "}";
    server.send(200, "application/json", json);
}

void handleJsonPost() {
    // Manual JSON parsing
    String body = server.arg("plain");
    // Parse JSON with the application's chosen JSON library...
}
```

**HttpServerAdvanced - JSON Response:**
```cpp
Response handleJson(HttpContext &request) {
    JsonDocument doc;
    doc["status"] = "ok";
    doc["value"] = 42;
    return JsonResponse::create(HttpStatus::Ok(), doc);
}

Response handleJsonPost(HttpContext &request, JsonDocument &&body) {
    // Body is already parsed!
    String status = body["status"];
    return JsonResponse::create(HttpStatus::Ok(), body);
}
```

**Verdict**: HttpServerAdvanced has significant advantage when its optional JSON support is enabled.

### Authentication

**WebServer - Basic Auth:**
```cpp
void handlePrivate() {
    if (!server.authenticate("user", "pass")) {
        server.requestAuthentication(BASIC_AUTH, "Realm");
        return;
    }
    server.send(200, "text/plain", "Authenticated!");
}
```

**HttpServerAdvanced - Basic Auth:**
```cpp
// In setup:
handlers.on<GetRequest>("/private", privateHandler)
        .with(BasicAuth("user", "pass"));

// Handler doesn't need auth logic
Response privateHandler(HttpContext &request) {
    return StringResponse::create(HttpStatus::Ok(), "text/plain", "Authenticated!");
}
```

**Verdict**: HttpServerAdvanced provides cleaner separation of concerns with middleware pattern.

### CORS Configuration

**WebServer - CORS:**
```cpp
server.enableCORS(true);  // Global, all origins, limited options
```

**HttpServerAdvanced - CORS:**
```cpp
handlers.on<GetRequest>("/api/data", dataHandler)
        .withResponseFilter(CrossOriginRequestSharing(
            "https://example.com",  // Specific origin
            "GET,POST",             // Allowed methods
            "Content-Type",         // Allowed headers
            "true",                 // Credentials
            "X-Custom",             // Exposed headers
            3600                    // Max age
        ));
```

**Verdict**: HttpServerAdvanced provides fine-grained CORS control per-route.

---

## 2.4 API Design Patterns

### Handler Signatures

| Pattern | WebServer | HttpServerAdvanced |
|---------|-----------|-------------------|
| No parameters | `void handler()` | `Response handler(HttpContext&)` |
| With params | N/A | `Response handler(HttpContext&, vector<String>&&)` |
| With form data | `void handler()` + `server.arg()` | `Response handler(HttpContext&, FormData&&)` |
| With JSON | Manual | `Response handler(HttpContext&, JsonDocument&&)` |
| With raw body | Callback-based | `Response handler(HttpContext&, RawBody&&)` |

### Type Safety

**WebServer**: Uses string-based content types and numeric status codes.
```cpp
server.send(200, "application/json", json);  // Typo-prone
```

**HttpServerAdvanced**: Uses enums and strong types.
```cpp
return JsonResponse::create(HttpStatus::Ok(), doc);  // Type-checked
```

### Response Building

**WebServer**: Procedural, mutable server state.
```cpp
server.setContentLength(100);
server.sendHeader("X-Custom", "value");
server.send(200, "text/plain", "...");
```

**HttpServerAdvanced**: Immutable response objects.
```cpp
return StringResponse::create(
    HttpStatus::Ok(), 
    "text/plain", 
    "...",
    {HttpHeader("X-Custom", "value")}
);
```

---

## 2.5 Example Coverage

### WebServer Examples

| Example | Description |
|---------|-------------|
| HelloServer | Basic GET endpoint |
| AdvancedWebServer | Multiple routes, sensor data |
| PostServer | POST handling |
| HttpBasicAuth | Basic authentication |
| HttpAdvancedAuth | Digest authentication |
| FSBrowser | File system browser with upload |
| WebUpdate | OTA firmware update |
| HelloServerBearSSL | HTTPS server |
| Filters | Route filtering |
| UploadHugeFile | Large file upload handling |
| SimpleAuthentication | Auth patterns |
| Note | HTTPS examples remain in WebServer scope; this repository does not ship a corresponding HTTPS example |

**Total: 11 examples**

### HttpServerAdvanced Examples

| Category | Example | Description |
|----------|---------|-------------|
| Getting Started | HelloWorld | Basic GET endpoints |
| Getting Started | MultipleRoutes | Multiple handlers |
| Getting Started | StaticFiles | File serving |
| Request Data | FormParsing | Form handling |
| Request Data | JsonApi | JSON request/response |
| Request Data | QueryStrings | Query parameter access |
| Request Data | UrlParameters | Path parameters |
| Security | BasicAuthentication | Auth middleware |
| Security | CorsSupport | CORS configuration |

**Total: 9 examples (organized in categories)**

### Example Quality Comparison

| Criterion | WebServer | HttpServerAdvanced |
|-----------|-----------|-------------------|
| Beginner Friendly | ✅ Very | ✅ Good |
| Real-world Patterns | ⚠️ Limited | ✅ More comprehensive |
| Error Handling | ⚠️ Often omitted | ✅ Included |
| Comments | ⚠️ Sparse | ✅ Documented |
| Platform Bootstrap | Inline | Adapter-owned |
| Progressive Complexity | ⚠️ Flat | ✅ Categorized |

---

## 2.6 Advanced Features Comparison

### Middleware/Pipeline

**WebServer Hooks:**
```cpp
server.addHook([](const String& method, const String& url, 
                  WiFiClient* client, ContentTypeFunction) {
    if (url.startsWith("/admin")) {
        // Check auth...
    }
    return WebServer::CLIENT_REQUEST_CAN_CONTINUE;
});
```
- Limited to URL/method inspection
- No access to parsed body
- No chaining syntax

**HttpServerAdvanced Interceptors:**
```cpp
handlers.on<GetRequest>("/admin", adminHandler)
        .with(BasicAuth("admin", "pass"))
        .with(LoggingMiddleware())
        .withResponseFilter(CrossOriginRequestSharing());
```
- Full request access
- Chainable
- Separate request/response filters

### Streaming Large Responses

**WebServer:**
```cpp
server.setContentLength(CONTENT_LENGTH_UNKNOWN);
server.send(200, "text/plain", "");
while (hasData) {
    server.sendContent(chunk);
}
server.sendContent("");  // End chunked
```

**HttpServerAdvanced:**
```cpp
// Return a stream-based response
auto stream = std::make_unique<MyDataStream>();
return StreamResponse::create(HttpStatus::Ok(), std::move(stream));
```

### File Uploads with Progress

**WebServer:**
```cpp
server.on("/upload", HTTP_POST, 
    []() { server.send(200); },
    []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) { /* init */ }
        else if (upload.status == UPLOAD_FILE_WRITE) { /* write chunk */ }
        else if (upload.status == UPLOAD_FILE_END) { /* finalize */ }
    });
```

**HttpServerAdvanced:**
```cpp
handlers.on<Multipart>("/upload", 
    [](HttpContext&, vector<String>&, MultipartFormDataBuffer buffer) {
        if (buffer.status() == MultipartStatus::FirstChunk) { /* init */ }
        else if (buffer.status() == MultipartStatus::SubsequentChunk) { /* write */ }
        else if (buffer.status() == MultipartStatus::FinalChunk) { /* finalize */ }
        return nullptr;  // Continue processing
    });
```

---

# Part 3: Migration Guide

## 3.1 From WebServer to HttpServerAdvanced

| WebServer Pattern | HttpServerAdvanced Equivalent |
|-------------------|------------------------------|
| `server.on("/path", handler)` | `handlers.on<GetRequest>("/path", handler)` |
| `server.send(200, type, body)` | `return StringResponse::create(...)` |
| `server.arg("name")` | `request.uriView().query("name")` |
| `server.header("X")` | `request.headers().find("X")` |
| `server.authenticate()` | `.with(BasicAuth(...))` |
| `server.serveStatic()` | `.useStaticFiles(...)` |
| `server.addHook(...)` | `.with(interceptor)` |

## 3.2 Key Differences to Note

1. **Handlers return response objects** instead of calling `send()`
2. **Parameters are passed to handlers**, not accessed globally
3. **Authentication is middleware**, not in-handler
4. **JSON parsing is automatic** with `Json` handler type when optional JSON support is enabled
5. **Memory limits are enforced** by default

---

# Appendix: Quick Reference

## WebServer Configuration Defines

```cpp
#define HTTP_UPLOAD_BUFLEN 1436
#define HTTP_RAW_BUFLEN 1436
#define WEBSERVER_MAX_POST_ARGS 32
```

## HttpServerAdvanced Configuration Defines

```cpp
#define HTTPSERVER_ADVANCED_MAX_REQUEST_URI_LENGTH 1024
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_COUNT 32
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_NAME_LENGTH 64
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_VALUE_LENGTH 256
#define HTTPSERVER_ADVANCED_MAX_BUFFERED_BODY_LENGTH 2048
#define HTTPSERVER_ADVANCED_MAX_BUFFERED_FORM_BODY_LENGTH 2048
#define HTTPSERVER_ADVANCED_MAX_BUFFERED_JSON_BODY_LENGTH 2048
#define HTTPSERVER_ADVANCED_PIPELINE_STACK_BUFFER_SIZE 256
#define HTTPSERVER_ADVANCED_MULTIPART_FORM_DATABUFFER_SIZE 1436
#define HTTPSERVER_ADVANCED_PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS 5000
#define HTTPSERVER_ADVANCED_PIPELINE_ACTIVITY_TIMEOUT 1000
#define HTTPSERVER_ADVANCED_PIPELINE_READ_TIMEOUT 500
```
