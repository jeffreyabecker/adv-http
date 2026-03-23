2026-03-23 - Copilot: updated task checkboxes to reflect completed phases 1, 2, 4, 5, 6, 7, 8; phase 3 request parser remains in progress.
2026-03-23 - Copilot: added detailed unit-testing phase backlogs 001 through 009 and aligned the index file with the renumbered phase layout.
2026-03-23 - Copilot: created the initial socket-free unit-testing backlog and mapped the first high-level test areas from the current codebase.

# Unit Testing Areas Backlog

## Summary

This backlog defines the high-level unit-testing plan for the library with a strict requirement that tests run without opening real sockets or depending on live network IO. The intent is to grow host-side coverage around the existing native lane by favoring parser fixtures, fake transports, fake clocks, in-memory byte sources, and filesystem adapters instead of board-specific runtime behavior. This file is the umbrella plan only; follow-up detailed backlogs should be split out from these areas once each slice is ready for implementation.

## Goal / Acceptance Criteria

- Core library behavior can be validated in native tests without binding, connecting, or polling real sockets.
- Request parsing, routing, handler execution, response generation, and static-file logic are testable through in-process fixtures and library-owned seams.
- Transport-sensitive behavior is covered through fake `IClient` and `IServer` implementations or direct pipeline-handler fixtures rather than concrete networking stacks.
- Timeout and partial-read or partial-write behavior are deterministic under a fake clock and scripted byte sources.
- The native test lane remains the primary validation path for these unit suites, with production sources explicitly compiled into that lane as host-safe coverage expands.

## Detailed Backlogs

- Phase 1 native harness and shared fixtures: `docs/backlog/task-lists/unit-testing/001-native-harness-and-shared-fixtures-backlog.md`
- Phase 2 core HTTP values and utility logic: `docs/backlog/task-lists/unit-testing/002-core-http-values-and-utility-logic-backlog.md`
- Phase 3 request parser and request model: `docs/backlog/task-lists/unit-testing/003-request-parser-and-request-model-backlog.md`
- Phase 4 response construction and body streaming: `docs/backlog/task-lists/unit-testing/004-response-construction-and-body-streaming-backlog.md`
- Phase 5 body handlers and request-body processing: `docs/backlog/task-lists/unit-testing/005-body-handlers-and-request-body-processing-backlog.md`
- Phase 6 routing, providers, authentication, and filters: `docs/backlog/task-lists/unit-testing/006-routing-providers-authentication-and-filters-backlog.md`
- Phase 7 static files and filesystem-backed behavior: `docs/backlog/task-lists/unit-testing/007-static-files-and-filesystem-backed-behavior-backlog.md`
- Phase 8 pipeline loop and transport semantics through fakes: `docs/backlog/task-lists/unit-testing/008-pipeline-loop-and-transport-semantics-backlog.md`
- Phase 9 validation guardrails and planning: `docs/backlog/task-lists/unit-testing/009-validation-guardrails-and-planning-backlog.md`

## Current Coverage Snapshot

- Native test infrastructure is in place under `test/test_native/` with a consolidated runner entry point.
- Shared fixtures (`ByteStreamFixtures.h`, `HttpTestFixtures.h`) cover scripted byte sources, recording channels, fake clients, fake servers, response capture, and request-factory recorders.
- Core HTTP primitive and utility tests are complete (Phase 2).
- Response construction, body streaming, and response iterator tests are complete (Phase 4).
- Body handler tests for buffered, raw, form, multipart, and JSON request bodies are complete (Phase 5).
- Routing, provider composition, authentication, and CORS tests are complete (Phase 6).
- Static file and filesystem-backed behavior tests are complete (Phase 7).
- Pipeline loop and fake-transport tests for partial IO, keep-alive, aborts, disconnects, and timeouts are complete (Phase 8).
- Production sources are compiled directly from `src/` via `platformio/native.ini` rather than an aggregation file.
- The major remaining gap is request parsing and request-lifecycle coverage (Phase 3): parser fixture suites, event recorder, error-code mapping, address propagation, item storage, and response dispatch interactions.

## High-Level Test Areas

### Phase 1: Native Harness And Shared Fixtures

- Define shared fake or scripted fixtures for byte sources, byte channels, transport clients, clocks, and request-handler factories so new suites stop rebuilding bespoke helpers.
- Standardize parser and pipeline assertions for event sequences, response capture, connection state, and error propagation.
- Keep the host-compiled production source list in sync with each newly testable module so the native lane catches compile regressions as seams expand.

### Phase 2: Core HTTP Value Types And Utility Logic

- Expand direct unit coverage for `src/core/HttpStatus.h`, `src/core/HttpMethod.h`, `src/core/HttpContentTypes.h`, `src/core/HttpHeader.h`, and `src/core/HttpHeaderCollection.*`.
- Add broader utility coverage for query parsing, URI parsing, base64, URL decoding or encoding, HTML escaping, and any remaining edge cases in `src/util/HttpUtility.*` and `src/util/UriView.*`.
- Freeze small but important invariants around standard-text storage, case-insensitive header lookup, and query access behavior.

### Phase 3: Request Parser And Request Model

- Add fixture-driven tests for `src/pipeline/RequestParser.h` covering valid requests, malformed requests, oversized URLs, oversized headers, repeated callbacks, body chunk delivery, EOF completion, and parser error mapping.
- Test `src/core/HttpRequest.*` as a request-lifecycle object by driving its pipeline callbacks with fake handlers and verifying phase transitions, header accumulation, address propagation, item storage, and response dispatch behavior.
- Treat parser and request tests as fully in-process: feed raw bytes directly and capture events without any socket implementation.

### Phase 4: Response Construction And Body Streaming

- Extend coverage for `src/response/HttpResponse.*`, `src/response/StringResponse.*`, `src/response/FormResponse.*`, `src/response/JsonResponse.*`, `src/response/ChunkedHttpResponseBodyStream.*`, and `src/response/HttpResponseIterators.*`.
- Validate header emission, body ownership, iterator ordering, chunk framing, empty-body behavior, and temporarily unavailable source behavior.
- Keep these tests transport-free by consuming the produced byte sources directly.

### Phase 5: Body Handlers And Request-Body Processing

- Add direct tests for `src/handlers/BufferingHttpHandlerBase.h`, `src/handlers/BufferedStringBodyHandler.*`, `src/handlers/FormBodyHandler.*`, `src/handlers/JsonBodyHandler.*`, `src/handlers/MultipartFormDataHandler.*`, and `src/handlers/RawBodyHandler.*`.
- Cover content-length parsing, maximum-buffer enforcement, chunk aggregation, empty-body semantics, and final handler invocation timing.
- Prefer fake `HttpRequest` setup and direct `handleStep` or `handleBodyChunk` driving instead of end-to-end networking.

### Phase 6: Routing, Providers, Authentication, And Filters

- Expand tests for `src/routing/HandlerMatcher.*`, `src/routing/HandlerBuilder.*`, `src/routing/HandlerProviderRegistry.*`, `src/routing/BasicAuthentication.h`, and `src/routing/CrossOriginRequestSharing.h`.
- Verify URI pattern matching, method and content-type filtering, provider ordering, default-handler behavior, interceptor chaining, response-filter application, auth header parsing, and CORS header composition.
- Keep these suites in-process by constructing requests and handlers directly.

### Phase 7: Static Files And Filesystem-Backed Behavior

- Grow host-side coverage for `src/staticfiles/` using the portable filesystem seam and POSIX-backed or fake filesystem implementations instead of board-specific FS objects.
- Validate file lookup order, default-document fallback, metadata-driven headers, content-type selection, missing-file behavior, and directory handling.
- Reuse the existing filesystem compatibility work rather than introducing any runtime socket or device dependency.

### Phase 8: Pipeline Loop And Transport Semantics Through Fakes

- Add socket-free behavioral tests for `src/pipeline/HttpPipeline.*`, `src/pipeline/PipelineError.*`, and related transport-facing control flow using scripted fake `IClient` objects.
- Cover partial reads, partial writes, request completion, response completion, abort paths, disconnects, keep-alive decisions, and timeout handling under a fake clock.
- Keep these tests at the library boundary only: the goal is pipeline correctness, not OS networking validation.

### Phase 9: Validation Guardrails And Detailed Planning Split

- Split each phase above into its own detailed backlog file once implementation begins so acceptance criteria, fixtures, and exact test cases stay reviewable.
- Document the expected native commands and scope for unit validation, including when a new production source must be added to `test/test_native/native_portable_sources.cpp`.
- Keep the umbrella backlog updated as detailed plans land so coverage growth remains visible at a glance.

## Tasks

- [x] Create a shared native-fixture baseline for scripted byte sources, fake clients, fake clocks, and handler or response capture.
- [x] Expand core HTTP primitive and utility tests until the remaining edge cases are explicitly frozen.
- [ ] Add request parser fixtures and request-lifecycle tests that run entirely from in-memory byte input.
- [x] Extend response and stream serialization coverage for direct, chunked, and temporarily unavailable body sources.
- [x] Add dedicated handler-body parsing tests for buffered, raw, form, multipart, and JSON request bodies.
- [x] Add routing and provider-composition tests covering matcher rules, interceptors, default handlers, auth, and CORS.
- [x] Expand static-file and filesystem tests using portable adapters or host-backed implementations only.
- [x] Add fake-transport pipeline tests for partial IO, keep-alive, aborts, disconnects, and timeout behavior.
- [x] Split detailed backlog files out of each major phase so implementation can proceed phase-by-phase.

## Owner

TBD

## Priority

High

## References

- `test/test_native/`
- `test/test_native/native_portable_sources.cpp`
- `src/pipeline/RequestParser.h`
- `src/pipeline/HttpPipeline.h`
- `src/pipeline/TransportInterfaces.h`
- `src/core/HttpRequest.h`
- `src/handlers/`
- `src/routing/`
- `src/response/`
- `src/staticfiles/`
