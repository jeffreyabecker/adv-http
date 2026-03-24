2026-03-23 - Copilot: added authoritative websocket limit constants in Defines, introduced centralized WsErrorCategory/policyFor mapping, integrated runtime policy usage, and added exhaustive native policy-table tests.
2026-03-23 - Copilot: added locked decisions for limits naming (12) and centralized error-mapping policy (13); updated tasks accordingly.
2026-03-23 - Copilot: created detailed Phase 6 WebSocket limits, errors, and hardening backlog.

# WebSocket Phase 6 Limits, Errors, And Hardening Backlog

## Summary

This phase tightens resource and failure behavior so the initial WebSocket implementation is safe on embedded targets and predictable under malformed or abusive clients. The work should turn implicit assumptions into explicit constants, deterministic close behavior, and bounded memory usage.

## Goal / Acceptance Criteria

- Frame, message, queue, and idle-lifetime limits are explicit and configurable through the project’s constant pattern.
- Protocol violations and transport failures produce deterministic internal errors and close behavior.
- Memory and buffer ownership remain bounded and reviewable on RP2040 and ESP-class targets.
- Stuck or malformed clients cannot leave the pipeline in a half-live state indefinitely.

## Locked Decisions Applied Here

- All WebSocket limits are compile-time `HTTPSERVER_ADVANCED_WEBSOCKET_*` overrideable constants defined in `src/core/Defines.h`, following the existing `#ifndef / constexpr / #else / constexpr / #endif` pattern.
- All codec and transport failures map through a centralized `WsErrorCategory` enum and a single `policyFor(WsErrorCategory)` mapping table.

## Unit Test Coverage Targets

- Add native tests for frame-size, message-size, queue-depth, and idle-lifetime limit enforcement.
- Verify protocol-error closes for masking violations, invalid control frames, oversized payloads, and illegal continuation sequences.
- Verify cleanup after failed writes, repeated temporary backpressure, timeouts, and abrupt disconnects.
- Verify that configured constants alter runtime behavior deterministically where practical in host-side tests.
- Add regression coverage for any previously discovered non-terminal stuck-state bugs.

## Tasks

### Limits And Constants

- [x] Add the four authoritative WebSocket limit constants to `src/core/Defines.h`: `WsMaxFramePayloadSize` (default 4096), `WsMaxMessageSize` (default 8192), `WsIdleTimeoutMs` (default 30000), `WsCloseTimeoutMs` (default 2000).
- [x] Follow the `#ifndef HTTPSERVER_ADVANCED_WEBSOCKET_* / static constexpr / #else / static constexpr / #endif` pattern for each constant.
- [x] Ensure all production code reads the `constexpr` variable names, never the raw `HTTPSERVER_ADVANCED_WEBSOCKET_*` macros except in `#if`/`#ifndef` preprocessor branches.

### Error Mapping And Close Policy

- [x] Define the `WsErrorCategory` enum with all seven categories: `FrameParseError`, `ProtocolViolation`, `MessageTooLarge`, `WriteFailure`, `IdleTimeout`, `CloseHandshakeTimeout`, `RemoteDisconnect`.
- [x] Implement the centralized `policyFor(WsErrorCategory)` mapping function using the authoritative table from the pre-implementation decision backlog (item 13).
- [x] Add a safe default policy (no close handshake, code 1006) for any unmapped category to prevent silent undefined behavior.
- [x] Cover the full policy table exhaustively in the native test suite.

### Embedded Resource Review

- [ ] Review buffer allocation, reuse, and ownership for bounded behavior on constrained targets.
- [ ] Ensure incremental parse and send behavior remains preferred over whole-message accumulation by default.
- [ ] Document any remaining resource tradeoffs that must remain implicit until a later configuration phase.

## Owner

TBD

## Priority

High

## References

- `docs/plans/websocket-support-plan.md`
- `src/core/Defines.h`
- `src/pipeline/HttpPipeline.h`
- `src/pipeline/HttpPipeline.cpp`
- `src/core/HttpTimeouts.h`
- `src/pipeline/TransportInterfaces.h`
- `test/test_native/`
- RFC 6455 Sections 5, 7, and 10