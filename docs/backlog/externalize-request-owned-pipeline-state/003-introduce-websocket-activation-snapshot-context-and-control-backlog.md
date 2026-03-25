2026-03-24 - Copilot: completed the websocket activation snapshot, context, and control abstractions with native coverage.
2026-03-24 - Copilot: created Phase 3 backlog for introducing websocket activation snapshot, context, and control abstractions.

# Introduce WebSocket Activation Snapshot Context And Control Backlog

## Summary

This phase introduces the first websocket-side implementation types without extracting the runtime yet. It establishes websocket-owned upgrade state, the callback-facing `WebSocketContext`, and the narrow session-control seam used by public send/close APIs.

## Goal / Acceptance Criteria

- `WebSocketActivationSnapshot` exists and can carry HTTP activation data into websocket-owned state.
- `WebSocketContext` exists with direct callback ownership and context-owned metadata/state.
- `IWebSocketSessionControl` exists so send/close APIs do not own raw transport buffers.

## Tasks

- [x] Add `WebSocketActivationSnapshot`.
- [x] Add `WebSocketContext` with direct callback ownership.
- [x] Add lifecycle and close-state accessors on `WebSocketContext`.
- [x] Add carried-forward HTTP metadata and `items()` accessors on `WebSocketContext`.
- [x] Add `IWebSocketSessionControl` for send/close delegation.
- [x] Add unit tests for context-owned state and fake-control send/close mapping.

## References

- [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md](docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md)
- [docs/backlog/task-lists/web-sockets/013-implement-websocket-context-and-protocol-execution-backlog.md](docs/backlog/task-lists/web-sockets/013-implement-websocket-context-and-protocol-execution-backlog.md)
