2026-03-24 - Copilot: created index backlog for breaking the request-owned pipeline-state implementation plan into tracked implementation phases.

# Externalize Request-Owned Pipeline State Index Backlog

## Summary

This backlog folder breaks down the implementation work from [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md) into distinct tracked phases. The goal is to move from a plan-only description to a set of implementation-sized backlog items that can be worked, committed, and validated incrementally.

The work is broader than the websocket runtime split alone. It includes:

- extracting orchestration ownership out of `HttpRequest`
- reshaping the HTTP activation object toward `HttpContext`
- introducing first-class websocket context and execution types
- preparing and then landing a shared protocol-execution seam above the current HTTP and upgraded-session seams
- simplifying `HttpPipeline` around staged execution
- final naming and compatibility cleanup

## Goal / Acceptance Criteria

- Each major implementation phase from the plan has its own backlog document under this folder.
- The folder provides one canonical index that explains ordering, dependencies, and expected deliverables.
- Each phase backlog is implementation-oriented and test-aware, not just descriptive.
- The phase set preserves the architectural constraints already documented in [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md).

## Phase Index

- [001-http-runner-ownership-under-existing-pipeline-seam-backlog.md](docs/backlog/externalize-request-owned-pipeline-state/001-http-runner-ownership-under-existing-pipeline-seam-backlog.md)
- [002-slim-httprequest-toward-httpcontext-backlog.md](docs/backlog/externalize-request-owned-pipeline-state/002-slim-httprequest-toward-httpcontext-backlog.md)
- [003-introduce-websocket-activation-snapshot-context-and-control-backlog.md](docs/backlog/externalize-request-owned-pipeline-state/003-introduce-websocket-activation-snapshot-context-and-control-backlog.md)
- [004-extract-websocket-protocol-execution-under-existing-upgrade-seam-backlog.md](docs/backlog/externalize-request-owned-pipeline-state/004-extract-websocket-protocol-execution-under-existing-upgrade-seam-backlog.md)
- [005-convert-websocket-callback-surfaces-to-context-first-backlog.md](docs/backlog/externalize-request-owned-pipeline-state/005-convert-websocket-callback-surfaces-to-context-first-backlog.md)
- [006-wire-http-to-websocket-handoff-explicitly-backlog.md](docs/backlog/externalize-request-owned-pipeline-state/006-wire-http-to-websocket-handoff-explicitly-backlog.md)
- [007-introduce-shared-protocol-execution-seam-backlog.md](docs/backlog/externalize-request-owned-pipeline-state/007-introduce-shared-protocol-execution-seam-backlog.md)
- [008-simplify-httppipeline-around-staged-protocol-execution-backlog.md](docs/backlog/externalize-request-owned-pipeline-state/008-simplify-httppipeline-around-staged-protocol-execution-backlog.md)
- [009-final-naming-and-adapter-cleanup-backlog.md](docs/backlog/externalize-request-owned-pipeline-state/009-final-naming-and-adapter-cleanup-backlog.md)

## Ordering Notes

- Phases `001` and `002` are the root architectural work because the broader `012` design starts by removing orchestration ownership from `HttpRequest`.
- Phases `003` through `006` are the websocket-side implementation slices that sit on top of that HTTP-side split.
- Phases `007` and `008` are the shared-seam and pipeline simplification work that complete the peer-context architecture.
- Phase `009` is intentionally last because naming cleanup and adapter removal should happen only after the transition is stable.

## Dependencies

- The HTTP activation constraint from [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md) remains in force throughout all phases.
- The websocket callback/send API decisions in [docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md](docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md) should be treated as binding for phases `003` through `006`.
- The websocket execution split already tracked in [docs/backlog/task-lists/web-sockets/013-implement-websocket-context-and-protocol-execution-backlog.md](docs/backlog/task-lists/web-sockets/013-implement-websocket-context-and-protocol-execution-backlog.md) overlaps heavily with phases `003` through `006`; keep the two in sync if both remain active.

## Owner

- Copilot

## Priority

- High

## References

- [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
- [docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md](docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md)
- [docs/backlog/task-lists/web-sockets/013-implement-websocket-context-and-protocol-execution-backlog.md](docs/backlog/task-lists/web-sockets/013-implement-websocket-context-and-protocol-execution-backlog.md)
