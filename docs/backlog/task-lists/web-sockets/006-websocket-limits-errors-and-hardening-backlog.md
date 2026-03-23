2026-03-23 - Copilot: created detailed Phase 6 WebSocket limits, errors, and hardening backlog.

# WebSocket Phase 6 Limits, Errors, And Hardening Backlog

## Summary

This phase tightens resource and failure behavior so the initial WebSocket implementation is safe on embedded targets and predictable under malformed or abusive clients. The work should turn implicit assumptions into explicit constants, deterministic close behavior, and bounded memory usage.

## Goal / Acceptance Criteria

- Frame, message, queue, and idle-lifetime limits are explicit and configurable through the project’s constant pattern.
- Protocol violations and transport failures produce deterministic internal errors and close behavior.
- Memory and buffer ownership remain bounded and reviewable on RP2040 and ESP-class targets.
- Stuck or malformed clients cannot leave the pipeline in a half-live state indefinitely.

## Unit Test Coverage Targets

- Add native tests for frame-size, message-size, queue-depth, and idle-lifetime limit enforcement.
- Verify protocol-error closes for masking violations, invalid control frames, oversized payloads, and illegal continuation sequences.
- Verify cleanup after failed writes, repeated temporary backpressure, timeouts, and abrupt disconnects.
- Verify that configured constants alter runtime behavior deterministically where practical in host-side tests.
- Add regression coverage for any previously discovered non-terminal stuck-state bugs.

## Tasks

### Limits And Constants

- [ ] Add overrideable compile-time constants for payload, message, queue, and idle-lifetime limits using the project macro-plus-`static constexpr` pattern.
- [ ] Ensure production code reads the `static constexpr` symbols rather than raw macros except in preprocessor branches.
- [ ] Review whether limits belong globally, per session, or in a future server configuration object.

### Error Mapping And Close Policy

- [ ] Map codec and transport failures into deterministic close status or forced disconnect behavior.
- [ ] Distinguish clean remote close, protocol error, internal error, timeout, and transport failure in code paths where the distinction matters.
- [ ] Keep error handling centralized enough that later features do not fork close behavior inconsistently.

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