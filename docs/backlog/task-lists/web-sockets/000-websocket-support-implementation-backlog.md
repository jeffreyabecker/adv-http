2026-03-23 - Copilot: split the WebSocket implementation backlog into phase-specific backlog files with unit test coverage targets.
2026-03-23 - Copilot: created detailed WebSocket support implementation backlog from the architecture plan.
2026-03-23 - Copilot: added a pre-implementation decision punch list for design work that does not require code changes.

# WebSocket Support Implementation Backlog

## Summary

This file is the umbrella backlog for WebSocket support. The detailed work is now split into phase-specific backlog files so architecture changes, handshake behavior, codec work, session runtime, public API changes, hardening, and documentation can each carry their own acceptance criteria and native test targets. The initial version stays narrow: HTTP/1.1 upgrade only, no extensions, no compression, and no subprotocol negotiation unless a concrete requirement appears.

## Goal / Acceptance Criteria

- The server pipeline can represent an HTTP request outcome as either a normal HTTP response or a successful protocol upgrade into a long-lived connection session.
- WebSocket handshake validation and `101 Switching Protocols` generation are implemented without overloading the existing response-only handler model.
- A dedicated WebSocket session layer owns framed bidirectional IO, close handling, ping or pong handling, and bounded connection lifetime behavior after upgrade.
- The public builder API can register WebSocket routes independently from normal HTTP handlers and construct session instances through routing.
- Native tests cover each implementation phase through fake transports, in-memory frame fixtures, and deterministic clocks.
- Documentation explicitly states the first shipped scope and deferred features.

## Detailed Backlogs

- Phase 1 upgrade seam and connection lifecycle refactor: `docs/backlog/task-lists/web-sockets/001-upgrade-seam-and-connection-lifecycle-backlog.md`
- Phase 2 handshake-only vertical slice: `docs/backlog/task-lists/web-sockets/002-websocket-handshake-vertical-slice-backlog.md`
- Phase 3 core protocol types and frame codec: `docs/backlog/task-lists/web-sockets/003-websocket-frame-codec-backlog.md`
- Phase 4 session runtime and outbound send model: `docs/backlog/task-lists/web-sockets/004-websocket-session-runtime-backlog.md`
- Phase 5 public routing and builder API: `docs/backlog/task-lists/web-sockets/005-websocket-routing-and-builder-api-backlog.md`
- Phase 6 limits, errors, and hardening: `docs/backlog/task-lists/web-sockets/006-websocket-limits-errors-and-hardening-backlog.md`
- Phase 7 documentation and examples: `docs/backlog/task-lists/web-sockets/007-websocket-documentation-and-examples-backlog.md`

## Supporting Planning

- Pre-implementation design decision punch list: `docs/backlog/task-lists/web-sockets/008-websocket-pre-implementation-decisions-backlog.md`

## Scope Guardrails

- [ ] Keep first implementation limited to HTTP/1.1 upgrade requests.
- [ ] Do not add WebSocket extensions or per-message compression in this phase.
- [ ] Do not add subprotocol negotiation unless a concrete use case is approved.
- [ ] Prefer incremental parsing and bounded buffers over whole-message accumulation by default.
- [ ] Keep transport supervision, timeouts, and disconnect cleanup centralized in the pipeline rather than in per-route code.

## Unit Test Strategy

- Keep WebSocket coverage in the native lane only until the protocol seams stabilize.
- Prefer dedicated WebSocket test files under `test/test_native/` rather than folding protocol coverage into existing HTTP-focused suites.
- Reuse fake clients, byte channels, and deterministic clocks from the native harness where possible; add WebSocket-specific fixtures only when the existing seams are insufficient.
- Expand `test/test_native/native_portable_sources.cpp` as new production sources become host-safe and directly testable.

## Tasks

- [x] Split the umbrella WebSocket plan into phase-specific backlog files.
- [ ] Work through the pre-implementation decision punch list before Phase 1 code changes begin.
- [ ] Implement Phase 1 using the dedicated upgrade-seam backlog before touching public APIs.
- [ ] Keep unit test additions aligned with each implementation phase rather than deferring coverage until the end.
- [ ] Revisit this umbrella backlog when new phases need to be inserted or renumbered.

## Suggested Implementation Order

- [ ] Land the pipeline upgrade seam and stub upgraded-session path before adding any public WebSocket API.
- [ ] Merge handshake validation and `101` response generation behind internal seams before implementing the full frame codec.
- [ ] Finish codec tests before exposing message callbacks on the public builder API.
- [ ] Add session timeout, close, and backpressure handling before documenting the feature as complete.
- [ ] Keep documentation and examples until after core behavior and limits are frozen.

## Owner

TBD

## Priority

High

## References

- `docs/plans/websocket-support-plan.md`
- `docs/backlog/task-lists/web-sockets/001-upgrade-seam-and-connection-lifecycle-backlog.md`
- `docs/backlog/task-lists/web-sockets/002-websocket-handshake-vertical-slice-backlog.md`
- `docs/backlog/task-lists/web-sockets/003-websocket-frame-codec-backlog.md`
- `docs/backlog/task-lists/web-sockets/004-websocket-session-runtime-backlog.md`
- `docs/backlog/task-lists/web-sockets/005-websocket-routing-and-builder-api-backlog.md`
- `docs/backlog/task-lists/web-sockets/006-websocket-limits-errors-and-hardening-backlog.md`
- `docs/backlog/task-lists/web-sockets/007-websocket-documentation-and-examples-backlog.md`
- `docs/backlog/task-lists/web-sockets/008-websocket-pre-implementation-decisions-backlog.md`
- `test/test_native/`
- RFC 6455
