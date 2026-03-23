2026-03-23 - Copilot: added locked decisions for message delivery (7), outbound send model (8), and control-frame exposure (9); updated tasks accordingly.
2026-03-23 - Copilot: aligned Phase 4 backlog with the accepted single-step session boundary.
2026-03-23 - Copilot: created detailed Phase 4 WebSocket session runtime backlog.

# WebSocket Phase 4 Session Runtime And Outbound Send Model Backlog

## Summary

This phase turns the handshake and codec pieces into a long-lived upgraded connection runtime. The session layer should own inbound frame dispatch, outbound send policy, close sequencing, ping or pong behavior, and clean return into pipeline cleanup once the session is done.

## Goal / Acceptance Criteria

- A server-owned WebSocket session runtime can stay alive after handshake and process multiple inbound and outbound frames behind a single-step `IConnectionSession` interface.
- Session lifetime, close behavior, and transport cleanup are explicit and deterministic.
- Outbound send behavior is bounded and does not let one upgraded connection monopolize the main loop.
- The runtime is compatible with the eventual public callback model without forcing the public API before the internal mechanics are stable.

## Locked Decisions Applied Here

- Use a single-step `IConnectionSession` interface driven once per pipeline loop iteration.
- Keep timeout tracking, disconnect detection, and final cleanup pipeline-owned.
- Assemble continuation frames into a complete message in the session runtime using a bounded working buffer; fire callbacks only when the final fragment (FIN) arrives.
- Use synchronous direct write in the pipeline loop with partial-write resume state tracked across loop iterations; no per-session outbound queue.
- Keep PING and PONG handling fully internal; expose only `onClose(code, reason)` for connection close events.

## Unit Test Coverage Targets

- Add fake-transport runtime tests covering multiple inbound frames on one upgraded connection driven through `IConnectionSession::handle(...)`.
- Verify close-handshake behavior for server-initiated close, client-initiated close, protocol-error close, and disconnect during close.
- Verify ping or pong handling, including automatic pong generation if the first release keeps ping exposure internal.
- Verify partial reads, partial writes, temporarily unavailable write windows, and idle timeout behavior under a manual clock.
- Verify that outbound queue or synchronous-write policy obeys configured bounds and session completion semantics without widening the session boundary.

## Tasks

### Session Lifecycle

- [ ] Define the single-step `IConnectionSession` interface used by the pipeline after handshake.
- [ ] Implement open, read-loop, dispatch, close, and terminal cleanup behavior.
- [ ] Ensure session completion is observable through the session step result without ambiguous mixed state.

### Outbound Send Policy

- [ ] Implement synchronous direct write in the pipeline loop with no per-session outbound queue.
- [ ] Track partial-write resume state across loop iterations so interrupted writes drain the remaining serialized bytes on the next `handle()` call without re-entering user callbacks.
- [ ] Keep the policy narrow enough that later public send APIs cannot violate backpressure assumptions.

### Control Frames And Shutdown

- [ ] Implement automatic PONG generation for any incoming PING frame during a session step; no PING or PONG observation callback is exposed in the first release.
- [ ] Implement close sequencing: remote-initiated CLOSE completes the handshake and fires `onClose(code, reason)`; abnormal disconnect or timeout fires `onError` then `onClose(1006, "")`.
- [ ] Ensure remote disconnect and transport failure collapse into deterministic session cleanup.

### Pipeline Integration

- [ ] Re-enter the pipeline loop with upgraded-session work after handshake success through one session-step call per iteration.
- [ ] Ensure session teardown returns client ownership and completion state cleanly to pipeline cleanup.
- [ ] Keep timeout and activity accounting pipeline-owned rather than reimplemented per session.

## Decision Follow-Through

- Item 2 in the pre-implementation decision backlog fixes this phase to a single-step session boundary; if later fairness problems emerge, solve them inside the session runtime first before widening this interface.
- Item 7 in the pre-implementation decision backlog fixes this phase to complete-message assembly using a bounded working buffer; enforce `WsMaxMessageSize` as a hard limit with an immediate 1009 close on excess.
- Item 8 in the pre-implementation decision backlog fixes this phase to synchronous direct write with partial-write resume state across loop iterations.
- Item 9 in the pre-implementation decision backlog fixes the control-frame exposure: PING/PONG handled internally; `onClose` fires with the raw close code and reason for graceful and abnormal close.

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
- `docs/backlog/task-lists/web-sockets/008-websocket-pre-implementation-decisions-backlog.md`