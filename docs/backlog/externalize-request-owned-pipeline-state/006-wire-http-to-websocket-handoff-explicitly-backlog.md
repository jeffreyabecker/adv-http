2026-03-24 - Copilot: created Phase 6 backlog for wiring the HTTP-to-websocket handoff explicitly through websocket-owned activation state.

# Wire HTTP To WebSocket Handoff Explicitly Backlog

## Summary

This phase makes the HTTP-to-websocket transition explicit by building websocket-owned activation state at upgrade time rather than letting upgraded callbacks reach back into the old HTTP activation object.

## Goal / Acceptance Criteria

- Upgrade produces a websocket-owned activation snapshot.
- `WebSocketContext` receives stable copies or transfers of `items()`, method, version, headers, and endpoint data.
- Post-upgrade callbacks can inspect that data after the HTTP phase is no longer active.

## Tasks

- [ ] Build the activation snapshot from the current HTTP activation object.
- [ ] Transfer or copy `items()` into websocket-owned state intentionally.
- [ ] Copy request metadata and endpoint information into websocket-owned state.
- [ ] Ensure websocket code does not lazily query stale HTTP state after handoff.
- [ ] Add tests for carried-forward metadata and items after upgrade.

## References

- [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md](docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
