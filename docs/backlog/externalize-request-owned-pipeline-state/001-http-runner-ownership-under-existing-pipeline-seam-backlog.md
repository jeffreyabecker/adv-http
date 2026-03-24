2026-03-24 - Copilot: created Phase 1 backlog for extracting HTTP runner ownership under the existing pipeline seam.

# HTTP Runner Ownership Under Existing Pipeline Seam Backlog

## Summary

This phase removes orchestration ownership from `HttpRequest` without changing `HttpPipeline` to a new seam yet. The key move is to introduce an HTTP runner that owns handler lifetime, result staging, phase progression, and error-to-response mapping while the existing pipeline still talks to an `IPipelineHandler`-shaped surface.

## Goal / Acceptance Criteria

- `HttpRequest` no longer directly owns cached handler lifetime, pending result staging, parser-driven orchestration, or response-start/complete phase progression.
- An HTTP runner abstraction owns that orchestration under the current `IPipelineHandler` seam.
- Existing HTTP and websocket behavior stays green while ownership moves.

## Tasks

- [ ] Define the concrete `HttpRequestRunner`-style interface and its ownership responsibilities.
- [ ] Introduce an `HttpRequestPipelineAdapter`-style adapter if needed to preserve the current `IPipelineHandler` seam.
- [ ] Move handler lifetime ownership out of `HttpRequest`.
- [ ] Move pending result staging out of `HttpRequest`.
- [ ] Move parser-phase and response-phase progression bookkeeping out of `HttpRequest`.
- [ ] Move parser/pipeline error-to-response mapping out of `HttpRequest`.
- [ ] Keep existing request/pipeline/native tests green throughout the extraction.

## References

- [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
- [src/core/HttpRequest.h](src/core/HttpRequest.h)
- [src/core/HttpRequest.cpp](src/core/HttpRequest.cpp)
- [src/pipeline/IPipelineHandler.h](src/pipeline/IPipelineHandler.h)
