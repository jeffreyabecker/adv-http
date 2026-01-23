# HTTP Server Library Memory Comparison

**Libraries Compared:**
- **HttpServerAdvanced**: `.\libraries\HttpServerAdvanced\`
- **WebServer** (RP2040 Arduino): `.\.arduino\packages\rp2040\hardware\rp2040\5.4.4\libraries\WebServer\src`

---

## Executive Summary

| Aspect | HttpServerAdvanced | WebServer |
|--------|-------------------|-----------|
| **Architecture** | Fixed buffers + bounded heap | Dynamic allocations throughout |
| **Per-request static** | ~1.2 KB | 0 (all dynamic) |
| **Per-request dynamic** | Bounded (~13 KB max) | Unbounded |
| **Fragmentation risk** | Low | High |
| **Long-running stability** | Better | Prone to fragmentation |

---

## 1. Static vs Dynamic Allocations Per Request

### HttpServerAdvanced

| Allocation Type | Size | When Allocated | Bounded? |
|----------------|------|----------------|----------|
| `RequestParser::buffer_` | 1,024 B | Static member | ✅ Fixed |
| `http_parser` struct | ~80 B | Static member | ✅ Fixed |
| `http_parser_settings` struct | ~64 B | Static member | ✅ Fixed |
| Stack buffers (read/write) | 256 B each | Function scope | ✅ Stack |
| `MultipartFormDataHandler::buffer_` | 1,436 B | Handler instantiation | ✅ Fixed |
| URL String | ≤1,024 B | `onMessageBegin` | ✅ `MAX_REQUEST_URI_LENGTH` |
| Header Strings (×N) | ≤64+256 B each | `onHeader` callback | ✅ `MAX_REQUEST_HEADER_*` |
| Header count | ≤32 headers | Per request | ✅ `MAX_REQUEST_HEADER_COUNT` |
| Body buffer | ≤2,048 B | `BufferingHttpHandlerBase` | ✅ `MAX_BUFFERED_BODY_LENGTH` |

**Pattern**: Static buffers for parsing, bounded heap allocations for data handoff.

### WebServer

| Allocation Type | Size | When Allocated | Bounded? |
|----------------|------|----------------|----------|
| `HTTPUpload::buf` | 1,436 B | `new HTTPUpload()` | ✅ Fixed per upload |
| `HTTPRaw::buf` | 1,436 B | `new HTTPRaw()` | ✅ Fixed per raw |
| Request line parsing | Unbounded | `readStringUntil()` | ❌ **Unbounded** |
| Header parsing | Unbounded | `readStringUntil()` loop | ❌ **Unbounded** |
| `_currentArgs` array | N × (key + value) | `new RequestArgument[N]` | ⚠️ Capped at parse time |
| `_postArgs` array | 32 × (key + value) | `new RequestArgument[32]` | ✅ `WEBSERVER_MAX_POST_ARGS` |
| POST body (non-form) | `_clientContentLength` | `readBytesWithTimeout()` | ❌ **Unbounded** |
| Multipart field values | Unbounded | `readStringUntil()` + concat | ❌ **Unbounded** |
| URL decoding | 2× input size | `urlDecode()` creates copy | ❌ **Unbounded** |

**Pattern**: Almost entirely dynamic allocations with `String` and `new[]`. Many allocations are unbounded.

---

## 2. Best Case Scenarios

### HttpServerAdvanced — Minimal GET Request

```
GET /api/status HTTP/1.1
Host: 192.168.1.1
```

| Component | Memory |
|-----------|--------|
| Static pipeline overhead | ~1.2 KB |
| URL String allocation | ~20 B |
| 1 header (Host) | ~80 B |
| **Total** | **~1.3 KB** |

### WebServer — Minimal GET Request

```
GET /api/status HTTP/1.1
Host: 192.168.1.1
```

| Component | Memory |
|-----------|--------|
| Request line String | ~30 B (+ String overhead ~12 B) |
| Header parsing Strings | ~50 B (+ String overhead) |
| `_currentArgs` array | 1 × RequestArgument (~40 B) |
| **Total** | **~150 B heap** |

**Winner (best case)**: WebServer uses less memory for trivial requests because it has no static overhead.

---

## 3. Worst Case Scenarios

### HttpServerAdvanced — Maximum Request (32 headers, 2 KB body)

| Component | Memory |
|-----------|--------|
| Static pipeline overhead | ~1.2 KB |
| URL String (max) | 1,024 B |
| 32 headers × (64 + 256 + overhead) | ~11 KB |
| Body buffer (max) | 2,048 B |
| **Total** | **~14 KB (bounded)** |

### WebServer — Malicious/Large Request

| Component | Memory |
|-----------|--------|
| Request line (1 MB URL) | **1 MB+** ❌ |
| Headers (unlimited count, unlimited size) | **Unbounded** ❌ |
| POST body (`_clientContentLength` = 10 MB) | **10 MB** ❌ |
| URL decoding (doubles size) | **20 MB** ❌ |
| **Total** | **Unbounded — can OOM** |

**Winner (worst case)**: HttpServerAdvanced — memory usage is capped regardless of malicious input.

### Real-World Attack Vector: WebServer OOM

```http
POST /upload HTTP/1.1
Content-Length: 50000000
Content-Type: application/x-www-form-urlencoded

<50 MB of data>
```

WebServer's `readBytesWithTimeout()` will attempt to allocate a 50 MB buffer:
```cpp
// Parsing.cpp line 47-68
char* plainBuf = readBytesWithTimeout(client, _clientContentLength, plainLength, HTTP_MAX_POST_WAIT);
```

This causes immediate heap exhaustion and crash on RP2040 (262 KB RAM) or ESP8266 (80 KB heap).

**HttpServerAdvanced** rejects this at the handler level:
- `BufferingHttpHandlerBase` clamps to `MAX_BUFFERED_BODY_LENGTH` (2 KB default)
- `RawBodyHandler` streams chunks without buffering entire body

---

## 4. Unbounded Allocations Analysis

### WebServer — Unbounded Allocations

| Location | Code | Risk |
|----------|------|------|
| **Request line parsing** | `client->readStringUntil('\r')` | No URL length limit |
| **Header value parsing** | `headerValue = req.substring(...)` | No header length limit |
| **POST body buffering** | `readBytesWithTimeout(client, _clientContentLength, ...)` | Allocates full Content-Length |
| **Form field values** | `argValue += line` in loop | Concatenation without limit |
| **URL decoding** | `decoded += decodedChar` char-by-char | Creates 2× size copy |
| **Multipart non-file fields** | `readStringUntil()` in loop | No field size limit |

**Critical**: 6 distinct code paths that can exhaust memory with crafted input.

### HttpServerAdvanced — All Allocations Bounded

| Allocation | Limit | Enforcement |
|------------|-------|-------------|
| URL | `MAX_REQUEST_URI_LENGTH` (1,024 B) | `appendToBuffer()` returns false |
| Header name | `MAX_REQUEST_HEADER_NAME_LENGTH` (64 B) | `appendToBuffer()` returns false |
| Header value | `MAX_REQUEST_HEADER_VALUE_LENGTH` (256 B) | `appendToBuffer()` returns false |
| Header count | `MAX_REQUEST_HEADER_COUNT` (32) | Parser rejects excess |
| Body buffer | `MAX_BUFFERED_BODY_LENGTH` (2,048 B) | Clamps `Content-Length` |
| Multipart buffer | `MULTIPART_FORM_DATA_BUFFER_SIZE` (1,436 B) | Fixed buffer, streams overflow |

**All limits configurable** via `#define HTTPSERVER_ADVANCED_*` before include.

---

## 5. Memory Fragmentation Analysis

### WebServer — High Fragmentation Risk

**Fragmentation sources:**

1. **`String` concatenation patterns**
   ```cpp
   // Parsing.cpp urlDecode()
   decoded += decodedChar;  // Reallocates on each character!
   ```
   This creates O(n²) allocations for URL decoding.

2. **Repeated `new[]` / `delete[]` cycles**
   ```cpp
   if (_currentArgs) delete[] _currentArgs;
   _currentArgs = new RequestArgument[_currentArgCount + 1];
   ```
   Different-sized arrays each request fragment the heap.

3. **`readStringUntil()` internal reallocations**
   Arduino's `String::concat()` uses a doubling strategy that can temporarily allocate 3× final size.

4. **Variable-sized `RequestArgument` arrays**
   - First request: 5 args → 5-element array
   - Second request: 20 args → 20-element array (previous slot unusable)
   - Third request: 3 args → 3-element array (leaves 20-element hole)

**Fragmentation pattern over 100 requests:**
```
[used][hole][used][hole][hole][used][hole]...
```

Eventually, a 1 KB allocation may fail even with 10 KB free (but fragmented).

### HttpServerAdvanced — Low Fragmentation Risk

**Anti-fragmentation design:**

1. **Static buffers for parsing** — `RequestParser::buffer_` never allocates/frees
2. **Fixed-size containers** — Header storage uses `std::vector<HttpHeader>` with predictable growth
3. **Bounded String sizes** — All strings have known max sizes, reducing variance
4. **Handler reuse** — Same handler types allocated each request (same size allocations reuse same slots)
5. **Stack buffers for I/O** — 256 B read/write buffers live on stack, not heap

**Fragmentation pattern over 100 requests:**
```
[static buffer][predictable heap allocations of similar sizes]
```

The heap sees consistent allocation sizes, allowing efficient slot reuse.

---

## 6. Memory Profile Comparison

| Scenario | HttpServerAdvanced | WebServer |
|----------|-------------------|-----------|
| **Idle (no active request)** | ~1.2 KB static | 0 B |
| **Simple GET** | ~1.5 KB | ~200 B |
| **GET with 10 headers** | ~4 KB | ~800 B |
| **POST with 2 KB body** | ~5 KB | ~3 KB |
| **POST with 100 KB body** | ~3 KB (streaming) | **100 KB+ (buffers all)** ❌ |
| **Multipart 10 MB upload** | ~3 KB (streaming) | ~3 KB (streaming) |
| **After 1000 requests** | Stable | **Fragmented, may fail** ⚠️ |
| **Malicious 50 MB POST** | Rejected (2 KB cap) | **OOM crash** ❌ |

---

## 7. Configurable Limits

### HttpServerAdvanced

All limits can be overridden before including the library:

```cpp
#define HTTPSERVER_ADVANCED_MAX_REQUEST_URI_LENGTH 512
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_COUNT 16
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_VALUE_LENGTH 128
#define HTTPSERVER_ADVANCED_MAX_BUFFERED_BODY_LENGTH 1024
#define HTTPSERVER_ADVANCED_MULTIPART_FORM_DATABUFFER_SIZE 512
#include <HttpServerAdvanced.h>
```

### WebServer

Limited configurability:

```cpp
#define HTTP_UPLOAD_BUFLEN 1436     // Upload chunk size
#define HTTP_RAW_BUFLEN 1436        // Raw body chunk size  
#define WEBSERVER_MAX_POST_ARGS 32  // Max form fields
```

**No limits for**: URL length, header sizes, header count, POST body size.

---

## 8. Recommendations

### Use HttpServerAdvanced When:
- ✅ Running on memory-constrained devices (RP2040, ESP8266, ESP32)
- ✅ Server must run indefinitely without reboot
- ✅ Accepting requests from untrusted clients (DoS protection)
- ✅ Handling large file uploads (streaming multipart)
- ✅ Predictable memory footprint required

### Use WebServer When:
- ✅ Prototyping / quick development
- ✅ Trusted client environment (e.g., local-only access)
- ✅ Simple applications with small payloads
- ✅ Minimal static memory overhead needed
- ✅ Compatibility with existing ESP8266WebServer code

---
## HelloWebServer sample (what the examples demonstrate)

The `examples/HelloWebServer.ino` sketch in `HttpServerAdvanced` demonstrates several architectural and API patterns that shape the library's capabilities and how it's used in production sketches:

- **Service registration and composition** — Uses `server.use(WebServerBuilder(...))` and a builder pattern (via `WebServerBuilder` / `WebServerBuilder`) to register modular components before `server.begin()`.
- **Pluggable components** — Shows `builder.use(StaticFiles(...))` to attach static file serving backed by a filesystem (`LittleFS` via `FSConfig.h`), demonstrating dependency injection for services and content-type registration.
- **Factory-based handlers** — Uses `Form::makeFactory` and other `*::makeFactory` helpers to create handlers that integrate with the server's service registry (no global state required).
- **Control flow** — Uses `server.handleClient()` in `loop()` for polling (rather than blocking), exposing explicit lifecycle control to the sketch.

Example pattern from the sketch:

```cpp
server.use(WebServerBuilder([](WebServerBuilder &builder) {
  builder.use(StaticFiles(LittleFS, [](StaticFilesBuilder &b) {
    b.setFilesystemContentRoot("/wwwroot");
  }));
}));

server.on(HttpMethod::POST, "/api/postForm", [](HttpRequest& request, Form::PostBodyData data){
  String response = "Received form data:\n";
  for (const auto &pair : bodyData.pairs())
  {
    response += pair.first + ": " + pair.second + "\n";
  }
  return HttpResponse::create(HttpStatus::Ok(), response, {{"Content-Type", "text/plain"}});
});

void loop() { server.handleClient(); }
```

**Impact on comparison**

- The sample highlights the library's **modularity**: services and features (static files, content types, handler registries) are registered declaratively and composed using builders — a level of structure not present in the simpler `WebServer` examples.
- Because components are pluggable services, the server can be configured to include only the functionality a sketch needs (reducing footprint and enabling cleaner separation of concerns).
- The example reinforces that `HttpServerAdvanced` is intended for **production-like usage** with clear initialization, service injection, and lifecycle control.

---
## 9. Code Examples: Memory-Safe Patterns

### HttpServerAdvanced — Large File Upload (Streaming)

```cpp
server.on(HttpMethod::POST, "/upload",[](HttpRequest& request, MultipartFormDataBuffer buffer){
     static File uploadFile;
            
            if (buffer.status() == MultipartStatus::FirstChunk) {
                uploadFile = LittleFS.open("/upload.bin", "w");
            }
            
            uploadFile.write(buffer.data(), buffer.size());  // ~1.4 KB chunks
            
            if (buffer.status() == MultipartStatus::FinalChunk) {
                uploadFile.close();
                return HttpResponse::create(HttpStatus::OK(), "Upload complete");
            }
            return nullptr;  // Continue streaming
});
```

**Memory usage**: ~3 KB constant regardless of file size.

### WebServer — Large File Upload (Streaming)

```cpp
server.on("/upload", HTTP_POST, 
    []() { server.send(200, "text/plain", "Upload complete"); },
    []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            uploadFile = LittleFS.open("/upload.bin", "w");
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            uploadFile.write(upload.buf, upload.currentSize);  // ~1.4 KB chunks
        } else if (upload.status == UPLOAD_FILE_END) {
            uploadFile.close();
        }
    });
```

**Memory usage**: ~3 KB constant (similar streaming pattern).

**Note**: WebServer's file upload streaming works well. The memory issues are primarily with non-file POST bodies and header parsing.

---

## 10. Conclusion

| Criteria | Winner |
|----------|--------|
| Memory predictability | **HttpServerAdvanced** |
| Worst-case protection | **HttpServerAdvanced** |
| Best-case efficiency | WebServer (marginally) |
| Long-running stability | **HttpServerAdvanced** |
| Fragmentation resistance | **HttpServerAdvanced** |
| Development simplicity | WebServer |
| Configuration flexibility | **HttpServerAdvanced** |

**For production embedded HTTP servers handling untrusted input, HttpServerAdvanced provides significantly better memory safety guarantees.**
