2026-03-23 - Copilot: completed native pipeline loop and transport semantics coverage.
2026-03-23 - Copilot: created detailed Phase 8 pipeline loop and transport semantics backlog.

# Unit Testing Phase 8 Pipeline Loop And Transport Semantics Backlog

## Summary

This phase validates the highest-value transport-adjacent behavior in the library while keeping tests fully socket-free. `HttpPipeline` already depends on abstract client and clock seams, which makes it possible to drive read and write loops through scripted fake clients instead of live networking stacks. The goal is to verify lifecycle, partial IO, keep-alive, disconnect, and timeout behavior at the pipeline boundary only.

## Goal / Acceptance Criteria

- Pipeline behavior is covered through scripted fake clients and deterministic clocks.
- Partial read, partial write, request-complete, response-complete, abort, disconnect, and timeout behavior are all testable without sockets.
- Pipeline tests assert library behavior only and do not attempt to validate OS or driver networking semantics.

## Tasks

### Fake Client Modeling

- [x] Define a scripted fake `IClient` that can model readable bytes, writable capacity, connection loss, and timeout settings.
- [x] Add recording for bytes written, stop calls, timeout mutations, and connection-state checks.
- [x] Decide whether remote or local address fields should be static fixture data or scenario-specific script values.

### Read Loop Coverage

- [x] Add tests for complete requests delivered in one read.
- [x] Add tests for requests split across multiple reads, including boundaries inside headers and body data.
- [x] Add tests for disconnects before message completion and parser error propagation.

### Write Loop Coverage

- [x] Add tests for immediate full writes, partial writes, and temporarily unavailable output windows if the abstraction exposes them.
- [x] Verify response-start and response-complete notifications to the pipeline handler.
- [x] Verify keep-alive and pipeline-finished decisions after response completion.

### Timeout And Abort Paths

- [x] Add timeout tests using `Compat::ManualClock` to cover idle read timeouts and write-side timeout behavior.
- [x] Add tests for `abort()`, `abortReadingRequest()`, and `abortWritingResponse()` state transitions.
- [x] Verify unrecoverable error and timeout state handling.

## Notes

- Native coverage now lives in `test/test_native/test_pipeline.cpp` and drives `HttpPipeline` directly with scripted fake clients, manual clocks, and a purpose-built pipeline handler rather than any socket or server loop.
- The shared fake client fixture now records timeout mutations and connection-state checks so pipeline tests can assert setup behavior as well as IO outcomes.
- Phase 8 fixed several pipeline semantics at the source: first-loop setup now runs, request/response completion is reported within the same `handleClient()` call, response-complete notifications fire when the response stream exhausts, idle read and write timeouts are checked without resetting activity on no-op loops, and timeout/error/abort states now take precedence over raw transport disconnect reporting.

## Owner

TBD

## Priority

High

## References

- `src/pipeline/HttpPipeline.h`
- `src/pipeline/HttpPipeline.cpp`
- `src/pipeline/TransportInterfaces.h`
- `src/pipeline/IPipelineHandler.h`
- `src/pipeline/PipelineError.h`
- `src/pipeline/PipelineHandleClientResult.h`
- `src/compat/Clock.h`
- `src/streams/ByteStream.h`
- `src/core/HttpTimeouts.h`
- `test/test_native/test_clock.cpp`
- `test/test_native/test_pipeline.cpp`