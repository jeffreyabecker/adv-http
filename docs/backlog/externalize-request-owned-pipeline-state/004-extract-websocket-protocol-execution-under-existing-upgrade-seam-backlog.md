2026-03-24 - Copilot: created Phase 4 backlog for extracting WebSocketProtocolExecution under the existing upgrade seam.

# Extract WebSocket Protocol Execution Under Existing Upgrade Seam Backlog

## Summary

This phase splits the fused `WebSocketSessionRuntime` into context-owned and execution-owned responsibilities while preserving the existing upgrade seam. The new execution object remains compatible with the current upgraded-session path during transition.

## Goal / Acceptance Criteria

- `WebSocketProtocolExecution` owns parser state, write-progress state, handshake progression, and close-handshake mechanics.
- `WebSocketContext` owns callback-visible state and direct callback storage.
- Existing websocket pipeline behavior remains stable while the runtime split lands.

## Tasks

- [ ] Add `WebSocketProtocolExecution`.
- [ ] Move parser and fragmented-message state out of `WebSocketSessionRuntime`.
- [ ] Move pending write buffer and flush offset into execution.
- [ ] Move handshake progression and close-handshake sequencing into execution.
- [ ] Decide whether `WebSocketSessionRuntime` becomes a compatibility wrapper or is removed immediately.
- [ ] Keep automatic pong, close handling, and protocol-error behavior stable.

## References

- [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
- [src/websocket/WebSocketSessionRuntime.h](src/websocket/WebSocketSessionRuntime.h)
- [src/websocket/WebSocketSessionRuntime.cpp](src/websocket/WebSocketSessionRuntime.cpp)
