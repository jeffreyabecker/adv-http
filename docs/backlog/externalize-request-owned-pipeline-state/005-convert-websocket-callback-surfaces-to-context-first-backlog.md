2026-03-24 - Copilot: completed the websocket callback-surface conversion to `WebSocketContext &` signatures.
2026-03-24 - Copilot: created Phase 5 backlog for converting websocket callback surfaces to context-first signatures.

# Convert WebSocket Callback Surfaces To Context First Backlog

## Summary

This phase aligns websocket callback registration, upgrade plumbing, and tests with the `WebSocketContext &` callback model already defined in the design backlogs.

## Goal / Acceptance Criteria

- All runtime websocket callbacks receive `WebSocketContext &`.
- Builder and upgrade paths can register the new callback shape cleanly.
- Tests and fixtures no longer assume receive-only callback signatures.

## Tasks

- [x] Update `WebSocketCallbacks` to accept `WebSocketContext &`.
- [x] Update websocket upgrade plumbing for context-first callbacks.
- [x] Update websocket builder/routing registration code as needed.
- [x] Update affected test fixtures and native tests.
- [x] Verify callback ownership stays directly on `WebSocketContext`.

## References

- [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md](docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md)
- [src/websocket/WebSocketCallbacks.h](src/websocket/WebSocketCallbacks.h)
