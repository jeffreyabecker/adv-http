2026-03-24 - Copilot: completed the Phase 2 HttpContext slimming pass and locked the surviving context-facing API.
2026-03-24 - Copilot: created Phase 2 backlog for slimming HttpContext toward the future HttpContext shape.

# Slim HttpContext Toward HttpContext Backlog

## Summary

Once runner ownership exists, `HttpContext` should be reduced to a context-shaped HTTP object rather than a combined context-and-orchestrator. This phase keeps the current type name during transition while intentionally moving the implementation toward the future `HttpContext` name and responsibility set.

## Goal / Acceptance Criteria

- `HttpContext` primarily owns parsed request data, local helper access, URI helpers, and request-scoped `items()`.
- Handler lifetime, result staging, and orchestration no longer live on `HttpContext`.
- The codebase is structurally ready for a later rename to `HttpContext`.

## Phase 2 Outcome

The surviving `HttpContext` surface is now intentionally context-shaped:

- request metadata: `method`, `version`, `url`, and `headers`
- endpoint data: `remoteAddress`, `remotePort`, `localAddress`, and `localPort`
- request-scoped state: `items()`
- helper access: `server()`, `uriView()`, `completedPhases()`, and `createResponse()`

Decision for this phase:

- keep `createResponse()` on `HttpContext` for now as a lightweight request-context convenience
- keep parser/runner mutation hooks private behind an internal access seam rather than exposing them as part of the context API

## Tasks

- [x] Remove orchestration-only fields from `HttpContext` once they are owned by the runner.
- [x] Keep request metadata, address data, `items()`, and URI/helper access on `HttpContext`.
- [x] Decide whether response-factory convenience remains on `HttpContext` or moves behind another helper seam.
- [x] Ensure handler code still sees stable request-context behavior during transition.
- [x] Confirm the final set of fields/methods that should survive the later `HttpContext` rename.

## References

- [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
- [src/core/HttpContext.h](src/core/HttpContext.h)
- [src/core/HttpContext.cpp](src/core/HttpContext.cpp)
