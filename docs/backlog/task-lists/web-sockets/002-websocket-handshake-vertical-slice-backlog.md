2026-03-23 - Copilot: created detailed Phase 2 WebSocket handshake vertical slice backlog.

# WebSocket Phase 2 Handshake-Only Vertical Slice Backlog

## Summary

This phase proves the upgrade seam with a complete HTTP-to-WebSocket handshake flow before any full frame runtime is implemented. It should accept valid upgrade requests, reject invalid ones deterministically, generate the correct `101 Switching Protocols` response, and transfer the client into a stub upgraded session.

## Goal / Acceptance Criteria

- Valid WebSocket upgrade requests are recognized and accepted through the new internal seam.
- Invalid upgrade requests are rejected with deterministic HTTP responses and do not partially mutate pipeline state.
- `Sec-WebSocket-Accept` generation is correct and stable.
- A successful handshake transfers control to a stub upgraded-session object that keeps the connection alive independently from the old HTTP response lifecycle.

## Unit Test Coverage Targets

- Add native handshake tests for accepted requests with canonical header ordering and mixed-case header names.
- Add rejection tests for wrong method, missing `Connection`, missing `Upgrade`, unsupported version, invalid key length, malformed base64 key text, and duplicate or conflicting headers.
- Verify parser-split delivery across request-line, header-name, and header-value boundaries for accepted and rejected handshake requests.
- Verify exact `101` response headers, including `Upgrade`, `Connection`, and `Sec-WebSocket-Accept` values.
- Verify that rejection paths do not instantiate the upgraded-session stub and that accepted paths do.

## Tasks

### Handshake Validation

- [ ] Add a dedicated validator for WebSocket upgrade request requirements.
- [ ] Decide whether validation lives in a WebSocket-specific upgrade handler or in a narrow routed upgrade branch and keep that placement explicit.
- [ ] Reuse existing header parsing and case-insensitive lookup behavior rather than introducing a separate handshake header store.

### Handshake Response Generation

- [ ] Implement `Sec-WebSocket-Accept` generation from `Sec-WebSocket-Key`.
- [ ] Add the `101 Switching Protocols` response path and ensure it is clearly separated from standard HTTP response helpers.
- [ ] Confirm the handshake path does not require a normal response body stream.

### Parser And Request Integration

- [ ] Review `src/pipeline/RequestParser.h` and `src/pipeline/RequestParser.cpp` for any upgrade-relevant state that must be surfaced more explicitly.
- [ ] Ensure header access in `HttpRequest` is sufficient for handshake validation without introducing parser-specific leakage into the public surface.
- [ ] Hand the accepted request into a stub upgraded session through the Phase 1 seam.

### Rejection Behavior

- [ ] Define the HTTP status codes used for malformed or unsupported upgrade attempts.
- [ ] Ensure partial validation failure does not leave the pipeline in upgraded mode.
- [ ] Keep handshake errors deterministic and easy to test from fake transports.

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