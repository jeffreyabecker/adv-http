2026-03-23 - Copilot: completed Phase 2 handshake vertical slice implementation with validation, rejection mapping, dedicated upgrade handling, and seam tests.
2026-03-23 - Copilot: added decision 5 (RFC-aligned rejection codes) to locked decisions and updated rejection-behavior task.
2026-03-23 - Copilot: aligned Phase 2 backlog with accepted decisions for dedicated handshake handling and rejection-policy ownership.
2026-03-23 - Copilot: created detailed Phase 2 WebSocket handshake vertical slice backlog.

# WebSocket Phase 2 Handshake-Only Vertical Slice Backlog

## Summary

This phase proves the upgrade seam with a complete HTTP-to-WebSocket handshake flow before any full frame runtime is implemented. It should accept valid upgrade requests, reject invalid ones deterministically, generate the correct `101 Switching Protocols` response, and transfer the client into a stub upgraded session.

## Goal / Acceptance Criteria

- Valid WebSocket upgrade requests are recognized and accepted through the new internal seam using a dedicated WebSocket upgrade handler.
- Invalid upgrade requests are rejected with deterministic HTTP responses and do not partially mutate pipeline state.
- `Sec-WebSocket-Accept` generation is correct, stable, and owned by the dedicated upgrade handler.
- A successful handshake transfers control to a stub upgraded-session object that keeps the connection alive independently from the old HTTP response lifecycle.

## Locked Decisions Applied Here

- Use a dedicated WebSocket upgrade handler for validation and handshake result construction.
- Generate `Sec-WebSocket-Accept` in that handler.
- Use RFC-aligned per-failure status codes with a single shared `rejectUpgrade(UpgradeFailure)` helper that maps each failure kind to its correct HTTP status.

## Unit Test Coverage Targets

- Add native handshake tests for accepted requests with canonical header ordering and mixed-case header names through the dedicated upgrade handler path.
- Add rejection tests for wrong method, missing `Connection`, missing `Upgrade`, unsupported version, invalid key length, malformed base64 key text, and duplicate or conflicting headers.
- Verify parser-split delivery across request-line, header-name, and header-value boundaries for accepted and rejected handshake requests.
- Verify exact `101` response headers, including `Upgrade`, `Connection`, and `Sec-WebSocket-Accept` values.
- Verify that rejection paths do not instantiate the upgraded-session stub and that accepted paths do, both at the handler level and at the pipeline seam.

## Tasks

### Handshake Validation

- [x] Add the dedicated WebSocket upgrade handler and keep validation logic owned there.
- [x] Implement validator logic in that handler for method, required headers, supported version, and key presence or shape.
- [x] Reuse existing header parsing and case-insensitive lookup behavior rather than introducing a separate handshake header store.

### Handshake Response Generation

- [x] Implement `Sec-WebSocket-Accept` generation from `Sec-WebSocket-Key` in the dedicated upgrade handler.
- [x] Add the `101 Switching Protocols` response path and ensure it is clearly separated from standard HTTP response helpers.
- [x] Confirm the handshake path produces a `RequestHandlingResult` upgrade outcome rather than a normal response body stream.

### Parser And Request Integration

- [x] Review `src/pipeline/RequestParser.h` and `src/pipeline/RequestParser.cpp` for any upgrade-relevant state that must be surfaced more explicitly.
- [x] Ensure header access in `HttpRequest` is sufficient for handshake validation without introducing parser-specific leakage into the public surface.
- [x] Hand the accepted request into a stub upgraded session through the Phase 1 result-object and `IConnectionSession` seam.

### Rejection Behavior

- [x] Implement the `UpgradeFailure` enum and the shared `rejectUpgrade(UpgradeFailure)` helper in the dedicated upgrade handler; map each case to its RFC-aligned status code (405 for wrong method, 426 for missing upgrade intent, 400 for malformed headers or key).
- [x] Ensure partial validation failure does not leave the pipeline in upgraded mode.
- [x] Keep handshake errors deterministic and easy to test both directly at the handler boundary and through fake transports.

## Decision Follow-Through

- Item 4 in the pre-implementation decision backlog fixes handshake validation ownership in a dedicated upgrade handler.
- Item 5 in the pre-implementation decision backlog defines the RFC-aligned rejection matrix; this phase must implement the `UpgradeFailure` enum and `rejectUpgrade()` helper according to that policy.

## Owner

TBD

## Priority

High

## References

- `docs/plans/websocket-support-plan.md`
- `src/pipeline/RequestParser.h`
- `src/pipeline/RequestParser.cpp`
- `src/core/HttpRequest.h`
- `src/core/HttpRequest.cpp`
- `src/core/HttpHeaderCollection.h`
- `src/core/HttpHeaderCollection.cpp`
- `src/response/HttpResponse.h`
- `src/response/HttpResponse.cpp`
- `src/pipeline/HttpPipeline.h`
- `src/pipeline/HttpPipeline.cpp`
- RFC 6455 Section 4
- `docs/backlog/task-lists/web-sockets/008-websocket-pre-implementation-decisions-backlog.md`