# HttpServerAdvanced Memory Analysis

## Static (Compile-Time) Memory

| Component | Size | Notes |
|-----------|------|-------|
| `RequestParser::buffer_` | **1,024 B** | `std::array<char, REQUEST_PARSER_BUFFER_LENGTH>` — fixed buffer for URI/headers during parsing |
| `HttpPipeline::_requestBuffer` | **1,436 B** | `uint8_t[REQUEST_BUFFER_SIZE]` — one ethernet frame for incoming data |
| `ChunkedHttpResponseBodyStream` | **~20 B** | State machine with 12-byte header buffer (no large chunk buffer) |
| `http_parser` struct | ~80 B | Part of `RequestParser` |
| `http_parser_settings` struct | ~64 B | Part of `RequestParser` |
| **Total static per-pipeline** | **~2.6 KB** | |

## Dynamic (Heap) Allocations Per-Request

| Allocation | When | Max Size | Bounded? |
|------------|------|----------|----------|
| **URL String** | `onMessageBegin` | `MAX_REQUEST_URI_LENGTH` (1,024 B) | ✅ Yes |
| **Header name Strings** (×N) | `onHeader` callback | `MAX_REQUEST_HEADER_NAME_LENGTH` (64 B) each | ✅ Yes |
| **Header value Strings** (×N) | `onHeader` callback | `MAX_REQUEST_HEADER_VALUE_LENGTH` (512 B) each | ✅ Yes |
| **Header count** | `onHeader` | `MAX_REQUEST_HEADER_COUNT` (100) | ✅ Yes |
| `HttpHeadersCollection` (vector) | `HttpRequest` construction | 100 × (64 + 512 + ~16 overhead) ≈ **60 KB worst case** | ✅ Bounded |
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
| **Maximum request** (100 headers at max lengths, 2 KB body buffered) | ~62 KB |

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
| `HTTPSERVER_ADVANCED_REQUEST_BUFFER_SIZE` | 1,436 B | Incoming data buffer size |
| `HTTPSERVER_ADVANCED_REQUEST_BODY_BUFFER_SIZE` | 1,436 B | Body buffer size |
| `HTTPSERVER_ADVANCED_CHUNKED_RESPONSE_BUFFER_SIZE` | 1,436 B | Max chunk data size (no static buffer; streams directly) |
| `HTTPSERVER_ADVANCED_MAX_REQUEST_URI_LENGTH` | 1,024 B | Maximum URL length |
| `HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_COUNT` | 100 | Maximum number of headers |
| `HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_NAME_LENGTH` | 64 B | Maximum header name length |
| `HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_VALUE_LENGTH` | 512 B | Maximum header value length |
| `HTTPSERVER_ADVANCED_REQUEST_PARSER_BUFFER_LENGTH` | max(URI, name+value) | Parser scratch buffer |
| `HTTPSERVER_ADVANCED_MAX_BUFFERED_BODY_LENGTH` | 2,048 B | Body buffering limit (0=disabled, <0=uncapped) |

## Recommendations for Memory-Constrained Devices

For devices with limited RAM (e.g., RP2040 with 264 KB, ESP8266 with 80 KB):

1. **Reduce header count**: Set `HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_COUNT` to 20–30
2. **Reduce header value length**: Set `HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_VALUE_LENGTH` to 256
3. **Disable body buffering**: Set `HTTPSERVER_ADVANCED_MAX_BUFFERED_BODY_LENGTH` to 0 and handle body chunks directly
4. **Reduce URI length**: If your endpoints have short paths, reduce `HTTPSERVER_ADVANCED_MAX_REQUEST_URI_LENGTH` to 256–512

Example minimal configuration (add before including library headers):

```cpp
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_COUNT 25
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_NAME_LENGTH 48
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_VALUE_LENGTH 256
#define HTTPSERVER_ADVANCED_MAX_REQUEST_URI_LENGTH 512
#define HTTPSERVER_ADVANCED_MAX_BUFFERED_BODY_LENGTH 0
```

This reduces worst-case heap usage from ~62 KB to ~10 KB per request.
