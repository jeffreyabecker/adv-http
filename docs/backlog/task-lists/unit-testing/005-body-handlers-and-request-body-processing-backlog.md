2026-03-23 - Copilot: completed JSON-enabled native coverage and documented current malformed-JSON behavior.
2026-03-23 - Copilot: completed multipart and extractor coverage and left JSON-enabled native coverage explicitly open.
2026-03-23 - Copilot: completed text raw form handler coverage and documented current raw-body passthrough behavior.
2026-03-23 - Copilot: completed buffering-base native coverage and documented current replay-on-completion behavior.
2026-03-23 - Copilot: created detailed Phase 5 body handlers and request-body processing backlog.

# Unit Testing Phase 5 Body Handlers And Request-Body Processing Backlog

## Summary

This phase focuses on the request-body handling stack that sits between parsed HTTP messages and user handler invocation. The core buffering path is already isolated behind `BufferingHttpHandlerBase`, and the higher-level body handlers build on that mechanism for text, form, multipart, raw, and optionally JSON request bodies. The goal is to test buffering limits, parsing behavior, and invocation timing directly against fake request objects rather than through network-style integration.

## Goal / Acceptance Criteria

- Body buffering and parse behavior are covered for text, raw, form, multipart, and JSON-enabled configurations.
- Content-length handling, max-buffer enforcement, and late `handleStep()` invocation semantics are explicit.
- Handler tests remain in-process and do not require the transport or pipeline loop.

## Tasks

### Buffering Base Behavior

- [x] Add direct tests for `tryParseHeaderLength(...)` and content-length parsing edge cases.
- [x] Verify behavior when buffering is disabled, unlimited, exactly full, and over capacity.
- [x] Verify when `handleBody(...)` is invoked from `handleBodyChunk(...)` versus from `handleStep(...)`.
- [x] Verify empty-body and missing-content-length semantics.

## Notes

- Native coverage now freezes the current `BufferingHttpHandlerBase` replay behavior: when buffering fills early from `Content-Length` or configured max, `handleBody(...)` runs once from `handleBodyChunk(...)` and then again from `handleStep()` after `CompletedReadingMessage`.
- Native coverage also freezes the current `RawBodyHandler` passthrough behavior: large body chunks are delivered intact, followed by a zero-length completion callback carrying final `receivedLength()` and parsed `contentLength()` metadata.
- Native coverage now freezes several multipart quirks as currently implemented: metadata views are only reliable during the handler callback, empty named parts still emit a handler event before `Completed`, and small truncated payload tails can be dropped entirely before the final completion event.
- The native PlatformIO lane now includes `ArduinoJson`, and JSON body plus JSON response tests execute in the default native suite. Current JSON behavior is now frozen as follows: malformed JSON still invokes the handler with an empty `JsonDocument`, and empty payloads remain unhandled because the buffering base suppresses zero-length body callbacks.

### Text, Raw, And Form Bodies

- [x] Add tests for `BufferedStringBodyHandler` covering encoding-neutral byte capture and empty payload handling.
- [x] Add tests for `RawBodyHandler` covering exact byte preservation and large-body truncation at configured limits.
- [x] Add tests for `FormBodyHandler` covering key-value parsing, repeated fields, empty values, and invalid form payload tolerance.

### Multipart And JSON Paths

- [x] Add multipart handler tests for boundary recognition, missing boundary headers, empty parts, and truncated payloads.
- [x] Add JSON handler tests for valid JSON, malformed JSON, empty payloads, and JSON-disabled build expectations.
- [x] Decide which JSON error behaviors are currently intended and which should remain open for correction before lock-in.

### Extraction And Route Parameter Interactions

- [x] Verify that handler parameter extractors are called at the intended phase and receive stable request data.
- [x] Verify that route parameters and parsed bodies can coexist without clobbering request items or headers.

## Owner

TBD

## Priority

High

## References

- `src/handlers/BufferingHttpHandlerBase.h`
- `src/handlers/BufferedStringBodyHandler.h`
- `src/handlers/BufferedStringBodyHandler.cpp`
- `src/handlers/RawBodyHandler.h`
- `src/handlers/RawBodyHandler.cpp`
- `src/handlers/FormBodyHandler.h`
- `src/handlers/FormBodyHandler.cpp`
- `src/handlers/MultipartFormDataHandler.h`
- `src/handlers/MultipartFormDataHandler.cpp`
- `src/handlers/JsonBodyHandler.h`
- `src/handlers/JsonBodyHandler.cpp`
- `src/handlers/HandlerRestrictions.h`