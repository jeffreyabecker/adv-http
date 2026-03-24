2026-03-24 - Copilot: created Phase 9 backlog for final naming cleanup and removal of transitional adapters.

# Final Naming And Adapter Cleanup Backlog

## Summary

This final phase removes transitional naming and compatibility scaffolding once the peer-context architecture is stable. The key public naming goal is to rename `HttpRequest` to `HttpContext` so the final model reads as peer contexts rather than a request object plus a websocket context.

## Goal / Acceptance Criteria

- `HttpRequest` is renamed to `HttpContext` once the implementation split is stable.
- Transitional adapters and compatibility wrappers that no longer carry their weight are removed.
- Final docs and tests describe the stable context/execution architecture directly.

## Tasks

- [ ] Rename `HttpRequest` to `HttpContext`.
- [ ] Remove obsolete adapters and compatibility wrappers.
- [ ] Delete `WebSocketSessionRuntime` if it still exists.
- [ ] Update docs, examples, and tests to final naming.
- [ ] Run the full native test lane and any relevant compile validation after cleanup.

## References

- [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
