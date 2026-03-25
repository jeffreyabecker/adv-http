2026-03-24 - Copilot: completed the first HttpPipeline simplification around staged protocol execution while preserving existing pipeline states.
2026-03-24 - Copilot: created Phase 8 backlog for simplifying HttpPipeline around staged protocol execution.

# Simplify HttpPipeline Around Staged Protocol Execution Backlog

## Summary

This phase makes `HttpPipeline` reflect the staged peer-context model directly: HTTP activation happens first, then execution continues as HTTP or transitions into a peer upgraded-protocol context through the shared execution model.

## Goal / Acceptance Criteria

- `HttpPipeline` is simplified around staged protocol execution rather than HTTP-vs-upgraded special casing.
- HTTP activation remains the first stage.
- Execution can continue as HTTP or transition cleanly to websocket through the shared seam.

## Tasks

- [x] Refactor `HttpPipeline` to operate against the staged execution model.
- [x] Preserve headers-complete activation and routing semantics.
- [x] Remove temporary duplication introduced by earlier adapters where possible.
- [x] Keep keep-alive, upgrade transition, response completion, and close behavior stable.

## References

- [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
- [src/pipeline/HttpPipeline.h](src/pipeline/HttpPipeline.h)
- [src/pipeline/HttpPipeline.cpp](src/pipeline/HttpPipeline.cpp)
