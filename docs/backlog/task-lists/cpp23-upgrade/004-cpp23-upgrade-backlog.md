# C++23 Upgrade Backlog

_Changelog: 2026-04-10 — initial draft (GitHub Copilot)_

Purpose: define and sequence the work needed to raise the C++ language standard from C++17 to C++23, replace the platform-provided `lumalink::span` wrapper with `std::span`, modernize handler and stream interfaces that still use raw pointer+length pairs, and apply targeted C++23 improvements (std::format, std::flat_map, std::expected, std::byteswap, std::views::enumerate) where they directly improve code clarity or eliminate manual bookkeeping.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Implementation Status

Current status: not started.

The project currently targets C++17 via `set(CMAKE_CXX_STANDARD 17)` in `CMakeLists.txt`. The codebase uses `lumalink::span` (provided by `LumaLinkPlatform.h`) aliased into each sub-namespace with `using lumalink::span;`. Handler and stream interfaces still carry raw `(const uint8_t *at, std::size_t length)` pointer+size pairs at their C++ boundaries. Several sites build strings with manual concatenation or write into raw character buffers. The native test suite running under CTest provides the safety net for this upgrade.

## Design Intent

- C++23 is the new language-standard baseline for this library; C++17 is no longer the floor.
- `std::span` (from `<span>`) replaces `lumalink::span` everywhere. The platform wrapper alias is no longer needed in this repo.
- `handleBodyChunk` and all other handler/stream C++ interfaces that accept `(const uint8_t *at, std::size_t length)` pairs are updated to `std::span<const uint8_t>`. The C-callback boundary to `llhttp` is unchanged.
- `std::format` replaces manual string concatenation and raw character-buffer writes at identified sites.
- `std::flat_map` replaces `std::map` for the static MIME-type lookup table in `HttpContentTypes`.
- `std::byteswap` replaces the manual shift-and-OR big-endian reader in `WebSocketFrameCodec`.
- `std::expected<T, E>` replaces enum-plus-output-param return patterns where a success value and an error type are naturally paired.
- `std::views::enumerate` and `std::ranges` algorithms replace manual index-plus-range loops where the ranged form improves clarity without obscuring intent.

## Scope

- `CMakeLists.txt` — standard bump to 23
- All files including or aliasing `lumalink::span` — span replacement
- Handler interfaces and call sites: `IHttpHandler`, `RawBodyHandler`, `BufferedStringBodyHandler`, `MultipartFormDataHandler`, `RequestParser` forwarding layer
- Stream constructors: `UriDecodingStream`, `UriEncodingStream`, `Base64DecoderStream`
- `HttpUtility` decoder overloads
- String-assembly sites in `HttpPipelineResponseSource`, `ChunkedHttpResponseBodyStream`, `BasicAuthentication`, `UriStream`
- `HttpContentTypes` static map
- `WebSocketFrameCodec` big-endian reader
- `WebSocketFrameCodec` / pipeline fallible return types
- Parsing loops in `HttpUtility`, `UriStream`, `UriView`

## Non-Goals

- Do not alter the `llhttp` C callback signatures (`const char *at, size_t length`) — only the C++ forwarding layer is updated
- Do not apply `std::views::enumerate` where the loop is already clear; clarity is the test, not novelty
- Do not add `std::expected<void, E>` where an existing bool or void return is unambiguous
- Do not upgrade third-party vendored sources (`llhttp`, ArduinoJson, Unity)
- Do not add new features or behavioural changes under the cover of this upgrade

## Architectural Rules

- After Task C23-01, all new code in this repo must compile under C++23; no C++17-only constructs may be introduced.
- Replace `lumalink::span` holistically — do not leave partial aliasing in place after Task C23-02.
- Handler interface changes (Task C23-03) must be one atomic commit so the virtual dispatch chain is never transiently inconsistent.
- All tasks must leave the CTest suite green before merge.

## Work Phases

## Phase 1 — Language Standard Baseline

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| C23-01 | todo | Change `CMAKE_CXX_STANDARD` from `17` to `23` in `CMakeLists.txt`; confirm `CMAKE_CXX_STANDARD_REQUIRED ON` remains set; fix any compile errors surfaced by the stricter standard | none | `cmake -S . -B build` and `cmake --build build` succeed; `ctest --test-dir build` exits zero with no failing tests |

## Phase 2 — std::span Migration

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| C23-02 | todo | Remove all `using lumalink::span;` shims and replace every `lumalink::span<T>` and `lumalink::span<T>(ptr, count)` usage with `std::span<T>`; add `#include <span>` where needed | C23-01 | No reference to `lumalink::span` or `using lumalink::span` remains in `src/`; all affected files include `<span>`; CTest suite passes |
| C23-03 | todo | Convert all `(const uint8_t *at, std::size_t length)` and `(const char *at, std::size_t length)` C++ interface parameters to `std::span<const uint8_t>` across `IHttpHandler`, `RawBodyHandler`, `BufferedStringBodyHandler`, `MultipartFormDataHandler`, `RequestParser` forwarding layer, `HttpUtility::DecodeURIComponent`, `UriDecodingStream`/`UriEncodingStream` constructors, and `Base64DecoderStream` constructor; update all call sites; leave `llhttp` C callbacks unchanged | C23-02 | No `(const uint8_t *at, std::size_t length)` or `(const char *at, std::size_t length)` C++ parameter pairs remain in handler or stream interfaces; CTest suite passes |

## Phase 3 — std::format Adoption

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| C23-04 | todo | Replace response-line string concatenation in `HttpPipelineResponseSource` with `std::format("{} {} {}\r\n", version, code, description)` | C23-01 | String assembly uses `std::format`; no `+` concatenation of response-line parts remains |
| C23-05 | todo | Replace manual hex-write into `char headerBuf_[12]` in `ChunkedHttpResponseBodyStream` with `std::format("{:x}\r\n", chunkSize)` | C23-01 | Raw character-buffer write is removed; CTest suite passes |
| C23-06 | todo | Replace `std::string("Basic realm=\"") + realm + "\""` in `BasicAuthentication` with `std::format("Basic realm=\"{}\"", realm)` | C23-01 | String concatenation replaced; CTest suite passes |
| C23-07 | todo | Replace manual `%XX` nibble-write in `UriStream` percent-encoding with `std::format("%{:02X}", ch)` | C23-01 | Lookup-table nibble extraction removed; CTest suite passes |

## Phase 4 — Container And Utility Improvements

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| C23-08 | todo | Change `std::map<std::string, const char *, std::less<>>` in `HttpContentTypes` to `std::flat_map<std::string, const char *, std::less<>>`; add `#include <flat_map>` | C23-01 | `HttpContentTypes` uses `std::flat_map`; all content-type lookup tests pass |
| C23-09 | todo | Replace the manual shift-and-OR big-endian reader (`readBigEndian` or equivalent) in `WebSocketFrameCodec` with a `std::byteswap`-based implementation; add `#include <bit>` | C23-01 | Manual multi-byte shift logic is removed; WebSocket codec tests pass |

## Phase 5 — std::expected For Fallible Returns

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| C23-10 | todo | Identify all functions that return a `WebSocketCodecStatus` or `WebSocketCodecError` enum alongside a success value (via out-param or union member); convert them to `std::expected<T, WebSocketCodecError>`; update all call sites to use `.value()` / `.error()` / boolean test | C23-01 | No out-param pattern remains for WebSocket codec results; call sites use `std::expected` idioms; WebSocket tests pass |
| C23-11 | todo | Identify pipeline functions that pair a `PipelineErrorCode` return with a separately-produced value; convert to `std::expected<Result, PipelineErrorCode>`; update call sites | C23-01 | Identified pipeline functions use `std::expected`; do not convert void-returning error-only paths; pipeline and integration tests pass |

## Phase 6 — Range And View Clarity Improvements

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| C23-12 | todo | Evaluate and selectively apply `std::views::enumerate` or `std::ranges::find`/`find_if` to the query-string parser in `HttpUtility.cpp`, the percent-encoding state machine in `UriStream.cpp`, and the component cursor in `UriView.cpp`; only convert where the ranged form is genuinely clearer | C23-01 | Converted loops use range algorithms or `std::views::enumerate`; manual index variables are eliminated at converted sites; all utility and URI tests pass |

## References

- `CMakeLists.txt` — `CMAKE_CXX_STANDARD 17` (current)
- `src/lumalink/http/handlers/IHttpHandler.h` — body-chunk interface
- `src/lumalink/http/websocket/WebSocketFrameCodec.h` — big-endian reader, codec status types
- `src/lumalink/http/core/HttpContentTypes.h` — flat_map candidate
- `src/lumalink/http/response/ChunkedHttpResponseBodyStream.h` — chunk-header buffer
- `src/lumalink/http/response/HttpPipelineResponseSource.*` — response-line assembly
- `src/lumalink/http/streams/UriStream.*` — percent-encoding
- cppreference: [std::span](https://en.cppreference.com/w/cpp/container/span), [std::expected](https://en.cppreference.com/w/cpp/utility/expected), [std::flat_map](https://en.cppreference.com/w/cpp/container/flat_map), [std::byteswap](https://en.cppreference.com/w/cpp/numeric/byteswap), [std::views::enumerate](https://en.cppreference.com/w/cpp/ranges/enumerate_view)
