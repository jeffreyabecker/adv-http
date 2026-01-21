# HttpServerAdvanced Memory Analysis

## Static (Compile-Time) Memory

| Component | Size | Notes |
|-----------|------|-------|
| `RequestParser::buffer_` | **1,024 B** | `std::array<char, REQUEST_PARSER_BUFFER_LENGTH>` — fixed buffer for URI/headers during parsing |
| `ChunkedHttpResponseBodyStream` | **~20 B** | State machine with 12-byte header buffer (no large chunk buffer) |
| `http_parser` struct | ~80 B | Part of `RequestParser` |
| `http_parser_settings` struct | ~64 B | Part of `RequestParser` |
| **Total static per-pipeline** | **~1.2 KB** | |

## Stack Memory (During Request Processing)

| Component | Size | Notes |
|-----------|------|-------|
| `readFromClientIntoParser()` buffer | **256 B** | `PIPELINE_STACK_BUFFER_SIZE` — stack-allocated during read loop |
| `writeResponseToClientFromStream()` buffer | **256 B** | Stack-allocated during write loop |

## Dynamic (Heap) Allocations Per-Request

| Allocation | When | Max Size | Bounded? |
|------------|------|----------|----------|
| **URL String** | `onMessageBegin` | `MAX_REQUEST_URI_LENGTH` (1,024 B) | ✅ Yes |
| **Header name Strings** (×N) | `onHeader` callback | `MAX_REQUEST_HEADER_NAME_LENGTH` (64 B) each | ✅ Yes |
| **Header value Strings** (×N) | `onHeader` callback | `MAX_REQUEST_HEADER_VALUE_LENGTH` (256 B) each | ✅ Yes |
| **Header count** | `onHeader` | `MAX_REQUEST_HEADER_COUNT` (32) | ✅ Yes |
| `HttpHeadersCollection` (vector) | `HttpRequest` construction | 32 × (64 + 256 + ~16 overhead) ≈ **11 KB worst case** | ✅ Bounded |
| `BufferingHttpHandlerBase::bodyBuffer_` | `handleBodyChunk` | `MAX_BUFFERED_BODY_LENGTH` (2,048 B or uncapped if negative) | ⚠️ Configurable |
| Response body stream | Handler returns response | **Unbounded** (user-supplied) | ❌ Handler's responsibility |

## Potential Unbounded Allocations

1. **Response body** — The handler can create an `HttpResponse` with an arbitrarily large `String` or `Stream`. This is outside the library's control.

2. **`BufferingHttpHandlerBase` body buffer** — Now bounded by `MAX_BUFFERED_BODY_LENGTH` (defaults to 2 KB). If set negative, it's uncapped.

3. **Streaming request body** — Body chunks are passed directly to the handler via `onBody`; the library itself does **not** buffer them (unless `BufferingHttpHandlerBase` is used).

## Memory Profile Summary

| Scenario | Heap Usage (Parsing Phase) |
|----------|---------------------------|
| **Minimal request** (GET, no body, ~5 headers) | ~1.5 KB |
| **Average request** (GET/POST, ~15 headers, small body buffered) | ~5–8 KB |
| **Maximum request** (32 headers at max lengths, 2 KB body buffered) | ~13 KB |

## Key Observations

1. **Parsing is bounded** — The `RequestParser` uses a single fixed buffer (`REQUEST_PARSER_BUFFER_LENGTH` = 1,024 B) and enforces length limits on URI, header names, and header values. Oversized data returns an error.

2. **Header storage is the main heap cost** — `HttpHeadersCollection` is a `std::vector<HttpHeader>` where each `HttpHeader` holds two `String` objects. With 100 headers at max lengths, this can reach ~60 KB.

3. **Body buffering is now bounded** — `BufferingHttpHandlerBase` respects `MAX_BUFFERED_BODY_LENGTH` (2 KB default). Setting it to 0 disables buffering entirely; negative means uncapped.

4. **No allocations during chunk parsing** — The parser accumulates into its fixed buffer and only allocates `String` objects at handoff points (URL complete, header complete).

5. **Response memory is handler-controlled** — The library streams responses efficiently but cannot prevent handlers from allocating large response bodies.

6. **Chunked encoding is buffer-free** — `ChunkedHttpResponseBodyStream` uses a state machine to emit chunk headers/trailers on-the-fly, streaming body bytes directly from the inner stream without buffering entire chunks. This saves ~1.4 KB per response compared to the previous buffered implementation.

## Configurable Limits (Defines.h)

| Macro Override | Default | Description |
|----------------|---------|-------------|
| `HTTPSERVER_ADVANCED_PIPELINE_STACK_BUFFER_SIZE` | 256 B | Stack buffer for read/write loops |
| `HTTPSERVER_ADVANCED_MAX_REQUEST_URI_LENGTH` | 1,024 B | Maximum URL length |
| `HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_COUNT` | 32 | Maximum number of headers |
| `HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_NAME_LENGTH` | 64 B | Maximum header name length |
| `HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_VALUE_LENGTH` | 256 B | Maximum header value length |
| `HTTPSERVER_ADVANCED_REQUEST_PARSER_BUFFER_LENGTH` | max(URI, name+value) | Parser scratch buffer (1,024 B default) |
| `HTTPSERVER_ADVANCED_MAX_BUFFERED_BODY_LENGTH` | 2,048 B | Body buffering limit (0=disabled, <0=uncapped) |
| `ETHERNET_FRAME_BUFFER_SIZE` | 1,436 B | Max chunk data size for chunked encoding |

## Recommendations for Memory-Constrained Devices

The current defaults are already tuned for memory-constrained devices:
- 32 headers max (reduced from 100)
- 256 B header values (reduced from 512)
- 256 B stack buffers (reduced from 1,436)
- Worst-case ~13 KB (reduced from ~62 KB)

For **extreme memory constraints** (e.g., ESP8266 with 80 KB):

1. **Reduce header count further**: Set `HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_COUNT` to 16–20
2. **Disable body buffering**: Set `HTTPSERVER_ADVANCED_MAX_BUFFERED_BODY_LENGTH` to 0 and handle body chunks directly
3. **Reduce URI length**: If your endpoints have short paths, reduce `HTTPSERVER_ADVANCED_MAX_REQUEST_URI_LENGTH` to 256–512

Example ultra-minimal configuration:

```cpp
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_COUNT 16
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_NAME_LENGTH 48
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_VALUE_LENGTH 128
#define HTTPSERVER_ADVANCED_MAX_REQUEST_URI_LENGTH 256
#define HTTPSERVER_ADVANCED_MAX_BUFFERED_BODY_LENGTH 0
```

This reduces worst-case heap usage to ~4 KB per request.

---

## Design Analysis: Static vs Dynamic Buffer for URL/Header Parsing

### Current Approach: Fixed Static Buffer (1,024 B)

```cpp
RequestParser::buffer_ = std::array<char, REQUEST_PARSER_BUFFER_LENGTH>
```

**Pros:**
- Zero heap allocations during parsing phase
- No fragmentation risk
- Predictable memory footprint
- Single memcpy per field vs. multiple String reallocations
- Works well with streaming parsers (data arrives in chunks)

**Cons:**
- Always consumes 1,024 B even for tiny requests
- Wasted for typical requests (GET with ~200-byte URL, small headers)

### Alternative: Dynamic Allocation (e.g., `String` directly)

```cpp
String url_;  // grows as chunks arrive
String headerName_, headerValue_;
```

**Pros:**
- Only allocates what's needed
- A 50-byte URL uses ~50 bytes, not 1,024
- Average request memory could drop by 500–800 B

**Cons:**
- **Fragmentation risk**: Multiple `String::concat()` calls can fragment the heap, especially on embedded systems without a compacting allocator
- **Reallocation overhead**: Each append may trigger `realloc()`, copying data
- **Unpredictable latency**: Memory allocation during parsing can stall
- **Arduino String pitfalls**: On ESP8266/ESP32, `String` uses a doubling strategy that can temporarily use 3× the final size

### Quantitative Comparison

| Scenario | Static Buffer | Dynamic (String) | Notes |
|----------|--------------|------------------|-------|
| Minimal GET (50-byte URL, 5 small headers) | 1,024 B static | ~300 B heap | Dynamic wins |
| Average request (200-byte URL, 15 headers) | 1,024 B static | ~800 B heap + fragmentation | Roughly equal |
| Max request (1 KB URL, 100 headers) | 1,024 B static | 1 KB heap (URL alone) | Static wins (no fragmentation) |

### Hybrid Approach (Potential Optimization)

A **small inline buffer with overflow to heap** could be considered:

```cpp
// Small buffer for typical cases, heap fallback for large
static constexpr size_t INLINE_SIZE = 256;
char inlineBuffer_[INLINE_SIZE];
String overflow_;  // only allocated if needed
```

This would:
- Handle 90%+ of URLs without heap allocation
- Only touch the heap for unusually long URLs/headers
- Reduce static footprint from 1,024 B to 256 B

### Verdict

| Factor | Winner | Why |
|--------|--------|-----|
| Worst-case predictability | Static | No fragmentation, bounded |
| Average-case efficiency | Dynamic | Typical requests use less |
| Real-time/latency | Static | No malloc during parse |
| Long-running stability | Static | Fragmentation accumulates over time |

**Conclusion**: The current static buffer is the **safer choice** for an HTTP server that must run indefinitely without reboot. The ~800 B average savings from dynamic allocation isn't worth the fragmentation risk on embedded systems without virtual memory.

The hybrid approach with a 256-byte inline buffer could be considered if profiling shows memory pressure in real workloads, but adds implementation complexity.
