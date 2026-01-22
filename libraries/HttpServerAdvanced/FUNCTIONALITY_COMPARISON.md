# Functionality Comparison: HttpServerAdvanced vs WebServer

**Paths compared:**
- **HttpServerAdvanced**: `./libraries/HttpServerAdvanced/`
- **WebServer**: `.arduino/packages/rp2040/hardware/rp2040/5.4.4/libraries/WebServer/src`

---

## Executive Summary

| Area | HttpServerAdvanced | WebServer |
|---|---:|---|
| Purpose / Target | Production-grade embedded HTTP server — **bounded**, configurable, streaming-first | Lightweight server for sketches — simple but **unbounded** by default |
| Concurrency | Pipelined / multi-connection capable via `HttpPipeline` | Single simultaneous client (documented) |
| Parsing strategy | `http_parser` streaming parser with fixed buffer and strict limits | `readStringUntil()` / `String`-heavy parsing — simpler but unbounded |
| Memory model | **Bounded & predictable** (static parser buffer, stack I/O buffers, configurable limits) | **Dynamic & unbounded** (many `String` allocations, risk of fragmentation/OOM) |
| Request handling model | Factories + handler classes (Request, Form, RawBody, Multipart) | `RequestHandler` callbacks with legacy-style upload/raw handlers |
| Multipart uploads | Streaming `MultipartFormDataHandler` — chunked callbacks, low memory | `HTTPUpload` with chunk buffer and callbacks — works, but parsing uses strings |
| Raw body | `RawBodyHandler` streaming + buffered options | `HTTPRaw` with buffer and `raw()` callbacks |
| Response streaming | Stream-based `IHttpResponse`, chunked iterator, no big buffers | `streamFile()` and chunk methods; simpler API for sketches |
| Safety & DoS resistance | High — enforces URI/header/body limits, timeouts, explicit `PipelineError` | Low — lacks limits, large Content-Length can OOM |
| Extensibility | High: modular handler headers, `NetClient` abstraction | Moderate: easier to hack for sketches, less modular internals |
| Timeouts & lifecycle | Fine-grained: `HttpTimeouts` (total/activity/read) and abort logic | Macros and client timeouts; less centralized |
| API ergonomics | More boilerplate but flexible and safe | Minimal and rapid for prototyping |

---

## Key Tradeoffs

- Use **HttpServerAdvanced** when stability, predictability, and safety for untrusted input are required.
- Use **WebServer** for quick prototyping and simple single-client sketches where memory constraints and untrusted input are not a concern.

---

## Detailed Notes

### Memory & Parsing
- **HttpServerAdvanced** enforces strict bounds: `REQUEST_PARSER_BUFFER_LENGTH`, `MAX_REQUEST_HEADER_*`, `MAX_BUFFERED_BODY_LENGTH`, `MULTIPART_FORM_DATA_BUFFER_SIZE`.
- **WebServer** uses `String` and dynamic `new[]` allocations (`readStringUntil`, `readBytesWithTimeout`), which can cause fragmentation and OOM with large or malicious input.

### Multipart & Uploads
- Both support streaming uploads. `HttpServerAdvanced` emphasizes chunked delivery to handlers with `MultipartStatus` (First/Next/Final) and small internal buffers. `WebServer` implements `HTTPUpload` with a pre-sized buffer and `upload()` callbacks.

### Timeouts & Error Handling
- **HttpServerAdvanced** offers explicit `HttpTimeouts` and `PipelineError` codes, not exposing `http_errno*` to public API. Timeouts trigger handler `onError()` and pipeline abort where needed.
- **WebServer** relies on macros and lower-level client timeouts; error reporting is less centralized.

### API Patterns
- **HttpServerAdvanced**: explicit handler factories, interceptor/filter utilities, streaming response composition.
- **WebServer**: concise `on(uri, handler)` pattern with optional upload callback; very familiar to Arduino community.

---

## Practical Recommendations

- For embedded production: prefer **HttpServerAdvanced** and tune `Defines.h` to your board (RP2040, ESP8266, ESP32).
- For sketches and prototypes: prefer **WebServer** for minimal boilerplate.
- When migrating: convert `WebServer` upload handlers to `Multipart::makeFactory` or `RawBody` factories in `HttpServerAdvanced` for streaming and bounded memory.

---

## Next Steps (optional)
- Provide a migration guide from `WebServer` to `HttpServerAdvanced`. ✅
- Provide a short checklist of `Defines.h` settings optimized for low-memory devices (ESP8266, RP2040). ✅

---

*Generated on 2026-01-21.*