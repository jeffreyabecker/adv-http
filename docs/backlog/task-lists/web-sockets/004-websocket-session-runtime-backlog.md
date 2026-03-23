2026-03-23 - Copilot: created detailed Phase 4 WebSocket session runtime backlog.

# WebSocket Phase 4 Session Runtime And Outbound Send Model Backlog

## Summary

This phase turns the handshake and codec pieces into a long-lived upgraded connection runtime. The session layer should own inbound frame dispatch, outbound send policy, close sequencing, ping or pong behavior, and clean return into pipeline cleanup once the session is done.

## Goal / Acceptance Criteria

- A server-owned WebSocket session runtime can stay alive after handshake and process multiple inbound and outbound frames.
- Session lifetime, close behavior, and transport cleanup are explicit and deterministic.
- Outbound send behavior is bounded and does not let one upgraded connection monopolize the main loop.
- The runtime is compatible with the eventual public callback model without forcing the public API before the internal mechanics are stable.

## Unit Test Coverage Targets

- Add fake-transport runtime tests covering multiple inbound frames on one upgraded connection.
- Verify close-handshake behavior for server-initiated close, client-initiated close, protocol-error close, and disconnect during close.
- Verify ping or pong handling, including automatic pong generation if the first release keeps ping exposure internal.
- Verify partial reads, partial writes, temporarily unavailable write windows, and idle timeout behavior under a manual clock.
- Verify that outbound queue or synchronous-write policy obeys configured bounds and session completion semantics.

## Tasks

### Session Lifecycle

- [ ] Define the minimal session interface used by the pipeline after handshake.
- [ ] Implement open, read-loop, dispatch, close, and terminal cleanup behavior.
- [ ] Ensure session completion is observable to the pipeline without ambiguous mixed state.

### Outbound Send Policy

- [ ] Decide whether writes are synchronous in the pipeline loop or buffered in a bounded session-owned queue.
- [ ] Document the tradeoff that drove the first implementation choice.
- [ ] Keep the policy narrow enough that later public send APIs cannot violate backpressure assumptions.

### Control Frames And Shutdown

- [ ] Add automatic control-frame handling needed for the first release.
- [ ] Implement close sequencing and terminal state rules.
- [ ] Ensure remote disconnect and transport failure collapse into deterministic session cleanup.

### Pipeline Integration

- [ ] Re-enter the pipeline loop with upgraded-session work after handshake success.
- [ ] Ensure session teardown returns client ownership and completion state cleanly to pipeline cleanup.
- [ ] Keep timeout and activity accounting pipeline-owned rather than reimplemented per session.

## Owner

TBD

## Priority

High

## References

- `docs/plans/websocket-support-plan.md`
- `src/pipeline/HttpPipeline.h`
- `src/pipeline/HttpPipeline.cpp`
- `src/pipeline/TransportInterfaces.h`
- `src/compat/Clock.h`
- `src/core/HttpTimeouts.h`
- `test/test_native/`
- RFC 6455 Sections 5 and 7