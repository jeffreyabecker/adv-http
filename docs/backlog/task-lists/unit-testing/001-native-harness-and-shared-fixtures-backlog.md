2026-03-23 - Copilot: completed the remaining Phase 1 fixture work with response-capture helpers, a fake server, and native test conventions documented under `test/`.
2026-03-23 - Copilot: added shared request-factory, pipeline-recorder, and fake-client fixtures plus native coverage for those support types.
2026-03-23 - Copilot: added shared byte-stream test fixtures and migrated the response-stream native suite onto the new helpers.
2026-03-23 - Copilot: created detailed Phase 1 native harness and shared fixtures backlog.

# Unit Testing Phase 1 Native Harness And Shared Fixtures Backlog

## Summary

This phase establishes the shared host-side testing infrastructure that the later unit suites depend on. The repository already has a consolidated native lane, a curated portable-source list, and a few local fake helpers inside individual tests, but those helpers are still fragmented and not sufficient for parser, request, or pipeline-heavy coverage. The goal is to define common fixtures once so later phases can focus on behavior instead of rebuilding scaffolding.

## Goal / Acceptance Criteria

- Shared fixture types exist for scripted byte sources, fake byte channels, fake clients, fake handler factories, and captured response sinks.
- Parser and pipeline tests can record ordered events and compare them against expected sequences without ad hoc per-suite helpers.
- Timeout-sensitive tests can use a deterministic fake clock rather than wall-clock sleeps.
- New native suites follow the consolidated runner pattern and do not duplicate test harness ownership.

## Tasks

### Shared Fixture Inventory

- [x] Audit the current helper types already embedded in `test/test_native/test_response_streams.cpp`, `test/test_native/test_stream_utilities.cpp`, `test/test_native/test_filesystem_compat.cpp`, and `test/test_native/test_utilities.cpp`.
- [x] Classify which helpers should move into shared support and which should remain suite-local.
- [x] Define a minimal naming and directory convention under `test/support/` for reusable unit-test fixtures.

Outcome: the shared-versus-local fixture split and naming conventions are documented in `test/README.md` and `test/support/README.md`. Byte-stream, HTTP, transport, and recorder helpers now live under `test/support/include/`, while narrow semantic probes and filesystem-only fakes remain suite-local.

### Byte-Source And Channel Fixtures

- [x] Add a scripted readable byte source that can model available bytes, temporary unavailability, exhaustion, and read chunking.
- [x] Add a capture sink or fake byte channel for collecting bytes written by response and pipeline code.
- [x] Add reusable assertion helpers for reading an `IByteSource` into a native string or byte vector without rewriting the same loops per suite.

### Fake Request And Response Helpers

- [x] Add a reusable fake `IHttpRequestHandlerFactory` that records handler creation and created responses.
- [x] Add helpers to capture `IHttpResponse` status, headers, and serialized body content for assertion-heavy suites.
- [x] Decide whether request-driving helpers should wrap `HttpRequest` directly or expose lower-level pipeline callbacks.

Decision: default to lower-level pipeline callbacks plus shared recorders for parser and transport tests. Use `HttpRequest` directly only when request lifecycle or response-dispatch behavior is the actual unit under test.

### Fake Transport And Timing Helpers

- [x] Add a scripted fake `IClient` for partial read, partial write, disconnect, and timeout scenarios.
- [x] Add a minimal fake or null `IServer` only if a suite needs server-side acceptance semantics.
- [x] Standardize `Compat::ManualClock` usage in native tests and document how timeout-oriented suites should advance time.

### Runner And Source Inventory Guardrails

- [x] Keep `test/support/include/ConsolidatedNativeSuite.h` as the single common suite-entry wrapper.
- [x] Document when a newly testable production source must be added to `test/test_native/native_portable_sources.cpp`.
- [x] Prevent new suites from adding duplicate `main()` entrypoints or bypassing the shared runner.

Outcome: the repository currently has a single native `main()` in `test/test_native/test_main.cpp`, and `test/README.md` documents the consolidated-runner rule, source-admission rule, and no-direct-`.cpp`-include policy for native suites.

## Owner

TBD

## Priority

High

## References

- `test/test_native/test_main.cpp`
- `test/support/include/ConsolidatedNativeSuite.h`
- `test/test_native/native_portable_sources.cpp`
- `test/test_native/test_response_streams.cpp`
- `test/test_native/test_stream_utilities.cpp`
- `test/test_native/test_filesystem_compat.cpp`
- `test/test_native/test_clock.cpp`
- `src/streams/ByteStream.h`
- `src/pipeline/TransportInterfaces.h`
- `src/compat/Clock.h`