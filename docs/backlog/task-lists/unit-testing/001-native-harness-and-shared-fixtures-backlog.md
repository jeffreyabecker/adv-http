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

- [ ] Audit the current helper types already embedded in `test/test_native/test_response_streams.cpp`, `test/test_native/test_stream_utilities.cpp`, `test/test_native/test_filesystem_compat.cpp`, and `test/test_native/test_utilities.cpp`.
- [ ] Classify which helpers should move into shared support and which should remain suite-local.
- [ ] Define a minimal naming and directory convention under `test/support/` for reusable unit-test fixtures.

### Byte-Source And Channel Fixtures

- [ ] Add a scripted readable byte source that can model available bytes, temporary unavailability, exhaustion, and read chunking.
- [ ] Add a capture sink or fake byte channel for collecting bytes written by response and pipeline code.
- [ ] Add reusable assertion helpers for reading an `IByteSource` into a native string or byte vector without rewriting the same loops per suite.

### Fake Request And Response Helpers

- [ ] Add a reusable fake `IHttpRequestHandlerFactory` that records handler creation and created responses.
- [ ] Add helpers to capture `IHttpResponse` status, headers, and serialized body content for assertion-heavy suites.
- [ ] Decide whether request-driving helpers should wrap `HttpRequest` directly or expose lower-level pipeline callbacks.

### Fake Transport And Timing Helpers

- [ ] Add a scripted fake `IClient` for partial read, partial write, disconnect, and timeout scenarios.
- [ ] Add a minimal fake or null `IServer` only if a suite needs server-side acceptance semantics.
- [ ] Standardize `Compat::ManualClock` usage in native tests and document how timeout-oriented suites should advance time.

### Runner And Source Inventory Guardrails

- [ ] Keep `test/support/include/ConsolidatedNativeSuite.h` as the single common suite-entry wrapper.
- [ ] Document when a newly testable production source must be added to `test/test_native/native_portable_sources.cpp`.
- [ ] Prevent new suites from adding duplicate `main()` entrypoints or bypassing the shared runner.

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