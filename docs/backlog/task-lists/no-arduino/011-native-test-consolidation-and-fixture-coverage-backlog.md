2026-03-21 - Copilot: created detailed Phase 9 native test consolidation and fixture coverage backlog.

# No-Arduino Phase 9 Native Test Consolidation And Fixture Coverage Backlog

## Summary

This phase grows the native validation lane from a small curated unit-test set into a stronger transport, parser, and response regression harness. The repository already has a consolidated `test/test_native/` runner and curated production-source list, but the current coverage is still limited to utilities, streams, and filesystem compatibility. The goal is to extend that lane deliberately with reusable fixtures, fake transports, and parser or pipeline coverage once the earlier seam work makes those components host-safe.

## Goal / Acceptance Criteria

- The native lane remains consolidated under one runner and one curated production-source inventory.
- Shared fixture infrastructure exists for parser and pipeline testing.
- Parser and transport-loop behavior can be validated from serialized request and response fixtures.
- Timeout and disconnect behavior can be tested deterministically once the clock seam exists.

## Tasks

### Consolidated Lane Hardening

- [ ] Audit the current `test/test_native/` layout and confirm which suites already comply with the consolidated-runner pattern.
- [ ] Ensure `test/support/include/ConsolidatedNativeSuite.h` remains the single shared hook-routing mechanism.
- [ ] Remove or prevent any suite patterns that try to reintroduce per-suite `main()` ownership.
- [ ] Keep `test/test_native/native_portable_sources.cpp` as the only curated production-source inventory file.

### Curated Production Source Expansion

- [ ] Plan the order in which new production `.cpp` files can be admitted into `native_portable_sources.cpp` as seams become host-safe.
- [ ] Add parser-related sources once the text and utility migration makes them safe.
- [ ] Add pipeline-related sources once the transport and clock seams are safe.
- [ ] Add response-related sources once the stream seam is safe.

### Shared Fixture Infrastructure

- [ ] Add or plan a `FakeClient` implementation targeting the cleaned transport interface.
- [ ] Add or plan a `FixtureLoader` capable of loading raw `.http` request and response fixtures from disk.
- [ ] Add or plan a `PipelineEventRecorder` implementing the cleaned pipeline handler interface.
- [ ] Add or plan reusable assertion helpers for byte-for-byte response comparison and ordered event comparison.
- [ ] Decide the directory layout for fixture assets under `test/support/fixtures/`.

### Parser Fixture Coverage

- [ ] Add parser-only fixture tests for simple GET requests.
- [ ] Add parser-only fixture tests for POST requests with body chunks.
- [ ] Add parser-only fixture tests for malformed request lines.
- [ ] Add parser-only fixture tests for oversized headers and oversized request-target cases.
- [ ] Add parser-only fixture tests that verify header ordering, method recognition, and completion signaling.

### Pipeline Transport-Loop Coverage

- [ ] Add fake-client tests for chunked request delivery.
- [ ] Add fake-client tests for partial response writes.
- [ ] Add fake-client tests for disconnect-before-completion behavior.
- [ ] Add fake-client tests for keep-alive handling across request lifecycle boundaries.
- [ ] Add timeout tests once the clock seam is implemented and injectable.

### Documentation

- [ ] Add or update a `test/` README describing the consolidated runner, curated production-source rule, and fixture layout.
- [ ] Document how to add a new parser or pipeline fixture case.
- [ ] Document what remains intentionally out-of-scope for the native lane until later phases complete.

## Owner

TBD

## Priority

Medium

## References

- `test/test_native/test_main.cpp`
- `test/test_native/native_portable_sources.cpp`
- `test/test_native/test_stream_available.cpp`
- `test/test_native/test_stream_utilities.cpp`
- `test/test_native/test_filesystem_compat.cpp`
- `test/test_native/test_filesystem_posix.cpp`
- `test/support/include/ConsolidatedNativeSuite.h`
- `docs/plans/no-arduino/testing-refactor-plan.md`
