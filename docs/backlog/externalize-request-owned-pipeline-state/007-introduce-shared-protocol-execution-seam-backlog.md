2026-03-24 - Copilot: created Phase 7 backlog for introducing the shared protocol-execution seam above the current HTTP and upgraded-session seams.

# Introduce Shared Protocol Execution Seam Backlog

## Summary

This phase begins landing the higher-level execution abstraction recommended in backlog `012`, so HTTP and websocket can eventually become peer protocol executions instead of living behind unrelated lower-level seams.

## Goal / Acceptance Criteria

- A concrete shared execution seam exists above `IPipelineHandler` and `IConnectionSession`.
- HTTP and websocket can both be mapped to that seam with transitional adapters where needed.
- The shared seam models lifecycle progression rather than HTTP parser shape or websocket session-loop shape.

## Tasks

- [ ] Confirm the final `IProtocolExecution` shape.
- [ ] Map the HTTP runner to the shared seam.
- [ ] Map websocket execution to the shared seam.
- [ ] Keep transitional adapters explicit and minimal.
- [ ] Add tests covering coexistence of the old seams with the new seam during transition.

## References

- [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
