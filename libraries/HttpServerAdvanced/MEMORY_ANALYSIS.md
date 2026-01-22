# Memory Analysis: HttpServerAdvanced

## Fixed Per-Request Buffer Allocations (Static)

| Component | Size | Location |
|-----------|------|----------|
| `RequestParser::buffer_` | **1024 B** | `std::array<char, REQUEST_PARSER_BUFFER_LENGTH>` |
| `http_parser` struct | ~144 B | Part of `RequestParser` |
| `http_parser_settings` | ~80 B | Part of `RequestParser` |
| Pipeline control state | ~64 B | Booleans, timestamps, pointers |
| **Total fixed per-pipeline** | **~1.3 KB** | |

These are allocated once per active connection and reused for the lifetime of that connection.

---

## Dynamic Allocations During Parsing

### Bounded Allocations (Safe)

| Field | Max Size | Bounds Check |
|-------|----------|--------------|
| `url_` (String) | 1024 B | `MAX_REQUEST_URI_LENGTH` enforced in parser |
| Header name | 64 B | `MAX_REQUEST_HEADER_NAME_LENGTH` |
| Header value | 256 B | `MAX_REQUEST_HEADER_VALUE_LENGTH` |
| Header count | 32 headers | `MAX_REQUEST_HEADER_COUNT` |

**Maximum headers allocation**: 32 × (64 + 256 + ~48 String overhead) ≈ **11.8 KB**

### ⚠️ Unbounded Allocations

| Component | Risk | Mitigation |
|-----------|------|------------|
| `HttpHeadersCollection` | `std::vector` grows with headers | Bounded to 32 headers, safe |
| `BufferingHttpHandlerBase::bodyBuffer_` | `std::vector<uint8_t>` | Bounded by `MaxBuffered` template param (default 2 KB) |
| `std::map<String, std::any> items_` | User-extensible | **Application-dependent** - user must manage |

**There are no truly unbounded allocations during HTTP parsing itself.** All parser-driven allocations have configurable limits.

---

## Per-Request Memory Summary

### Minimum Request (GET, no body, 3 headers)
| Component | Bytes |
|-----------|-------|
| Fixed parser buffer | 1,024 |
| Parser structs | 224 |
| URL String (~50 chars) | ~70 |
| 3 headers | ~300 |
| **Total** | **~1.6 KB** |

### Maximum Request (POST, full body, 32 headers)
| Component | Bytes |
|-----------|-------|
| Fixed parser buffer | 1,024 |
| Parser structs | 224 |
| URL String (1024 chars) | ~1,050 |
| 32 headers (max sizes) | ~11,800 |
| Body buffer (default max) | 2,048 |
| **Total** | **~16 KB** |

### Average Request (typical browser GET)
Browsers send ~8-12 headers. Typical URL is 100-200 chars.
| Component | Bytes |
|-----------|-------|
| Fixed parser buffer | 1,024 |
| Parser structs | 224 |
| URL String (~150 chars) | ~175 |
| 10 headers (avg 30+80) | ~1,300 |
| **Total** | **~2.7 KB** |

---

## Multi-Connection Scaling

With `MAX_CONCURRENT_CONNECTIONS = N`:
- **Fixed memory**: N × 1.3 KB = **1.3×N KB**
- **Peak memory**: N × 16 KB = **16×N KB**
- **Typical memory**: N × 2.7 KB ≈ **2.7×N KB**

| Connections | Fixed | Typical | Peak |
|-------------|-------|---------|------|
| 1 (default) | 1.3 KB | 2.7 KB | 16 KB |
| 2 | 2.6 KB | 5.4 KB | 32 KB |
| 4 | 5.2 KB | 10.8 KB | 64 KB |

---

## Key Configurable Limits

All can be overridden via preprocessor defines:

```cpp
#define HTTPSERVER_ADVANCED_REQUEST_PARSER_BUFFER_LENGTH  1024  // Fixed parse buffer
#define HTTPSERVER_ADVANCED_MAX_REQUEST_URI_LENGTH        1024  // URL cap
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_COUNT      32    // Header count
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_NAME_LENGTH  64  // Header name
#define HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_VALUE_LENGTH 256 // Header value
#define HTTPSERVER_ADVANCED_MAX_BUFFERED_BODY_LENGTH      2048  // Body buffer
#define HTTPSERVER_ADVANCED_MAX_CONCURRENT_CONNECTIONS    1     // Pipelines
```

---

## Comparison: Dynamic vs Static Strategy

The library uses a **hybrid approach**:

1. **Static**: `RequestParser::buffer_` is a fixed `std::array` - no fragmentation risk during parsing
2. **Dynamic but bounded**: Headers and URL use `String` but are size-checked before allocation
3. **Streaming-first**: Body data is streamed to handlers by default, not buffered

This design prevents heap fragmentation on long-running embedded systems while allowing reasonable flexibility for varying request sizes.
