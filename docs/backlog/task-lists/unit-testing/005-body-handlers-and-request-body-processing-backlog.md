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

- [ ] Add direct tests for `tryParseHeaderLength(...)` and content-length parsing edge cases.
- [ ] Verify behavior when buffering is disabled, unlimited, exactly full, and over capacity.
- [ ] Verify when `handleBody(...)` is invoked from `handleBodyChunk(...)` versus from `handleStep(...)`.
- [ ] Verify empty-body and missing-content-length semantics.

### Text, Raw, And Form Bodies

- [ ] Add tests for `BufferedStringBodyHandler` covering encoding-neutral byte capture and empty payload handling.
- [ ] Add tests for `RawBodyHandler` covering exact byte preservation and large-body truncation at configured limits.
- [ ] Add tests for `FormBodyHandler` covering key-value parsing, repeated fields, empty values, and invalid form payload tolerance.

### Multipart And JSON Paths

- [ ] Add multipart handler tests for boundary recognition, missing boundary headers, empty parts, and truncated payloads.
- [ ] Add JSON handler tests for valid JSON, malformed JSON, empty payloads, and JSON-disabled build expectations.
- [ ] Decide which JSON error behaviors are currently intended and which should remain open for correction before lock-in.

### Extraction And Route Parameter Interactions

- [ ] Verify that handler parameter extractors are called at the intended phase and receive stable request data.
- [ ] Verify that route parameters and parsed bodies can coexist without clobbering request items or headers.

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