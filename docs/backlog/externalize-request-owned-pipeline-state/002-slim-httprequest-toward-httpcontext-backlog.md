2026-03-24 - Copilot: completed the Phase 2 HttpRequest slimming pass and locked the surviving context-facing API.
2026-03-24 - Copilot: created Phase 2 backlog for slimming HttpRequest toward the future HttpContext shape.

# Slim HttpRequest Toward HttpContext Backlog

## Summary

Once runner ownership exists, `HttpRequest` should be reduced to a context-shaped HTTP object rather than a combined context-and-orchestrator. This phase keeps the current type name during transition while intentionally moving the implementation toward the future `HttpContext` name and responsibility set.

## Goal / Acceptance Criteria

- `HttpRequest` primarily owns parsed request data, local helper access, URI helpers, and request-scoped `items()`.
- Handler lifetime, result staging, and orchestration no longer live on `HttpRequest`.
- The codebase is structurally ready for a later rename to `HttpContext`.

## Phase 2 Outcome

The surviving `HttpRequest` surface is now intentionally context-shaped:

- request metadata: `method`, `version`, `url`, and `headers`
- endpoint data: `remoteAddress`, `remotePort`, `localAddress`, and `localPort`
- request-scoped state: `items()`
- helper access: `server()`, `uriView()`, `completedPhases()`, and `createResponse()`

Decision for this phase:

- keep `createResponse()` on `HttpRequest` for now as a lightweight request-context convenience
- keep parser/runner mutation hooks private behind an internal access seam rather than exposing them as part of the context API

## Tasks

- [x] Remove orchestration-only fields from `HttpRequest` once they are owned by the runner.
- [x] Keep request metadata, address data, `items()`, and URI/helper access on `HttpRequest`.
- [x] Decide whether response-factory convenience remains on `HttpRequest` or moves behind another helper seam.
- [x] Ensure handler code still sees stable request-context behavior during transition.
- [x] Confirm the final set of fields/methods that should survive the later `HttpContext` rename.

## References

- [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
- [src/core/HttpRequest.h](src/core/HttpRequest.h)
- [src/core/HttpRequest.cpp](src/core/HttpRequest.cpp)
