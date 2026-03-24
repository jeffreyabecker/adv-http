2026-03-24 - Copilot: completed the first handler-result normalization slice, including IHttpHandler result expansion and green native tests.
2026-03-24 - Copilot: created backlog item for normalizing WebSocket routing and upgrade flow behind IHttpHandler.

# Normalize WebSocket Flow Behind IHttpHandler Backlog

## Summary

The current WebSocket implementation still uses a parallel route-registration and route-selection path alongside the normal HTTP handler registry. `WebSocketRoute` plus the request-factory special-case path means route matching, upgrade validation, and handler selection are not expressed through the same `IHttpHandler` abstraction as ordinary HTTP handling. This backlog item captures the work needed to collapse those flows into one handler model so WebSocket support behaves like a first-class route type instead of a side channel.

## Goal / Acceptance Criteria

- WebSocket route registration no longer depends on a separate `WebSocketRoute` side table.
- Route selection for HTTP and WebSocket requests is unified behind the normal handler/provider registry flow.
- WebSocket upgrade acceptance and deterministic rejection behavior are implemented through an `IHttpHandler`-compatible abstraction instead of a request-factory special case.
- `HttpRequestHandlerFactory` no longer contains WebSocket-specific route matching logic.
- Existing WebSocket behavior remains covered by native tests, including successful upgrade, deterministic rejection, unmatched-route fallback, and session-runtime behavior.
- The resulting design is documented clearly enough that later upgraded protocols can follow the same seam.

## Tasks

- [x] Define the normalized handler result model needed for both HTTP responses and connection upgrades.
- [x] Decide whether `IHttpHandler` should return a new handler-layer result type or the existing `RequestHandlingResult`, and document the boundary tradeoffs.
- [x] Update `IHttpHandler`, `HttpHandler`, and related handler utilities to support the chosen result model without making ordinary HTTP handlers awkward to implement.
- [ ] Change handler selection timing so header-dependent handlers can be selected through the registry without relying on the current WebSocket factory side channel.
- [ ] Introduce a WebSocket handler/provider implementation that performs upgrade-candidate checks, validates the handshake, and returns either an HTTP response or an upgraded session.
- [ ] Refactor `ProviderRegistryBuilder::websocket(...)` to register WebSocket behavior through `HandlerProviderRegistry` instead of storing `WebSocketRoute` records.
- [ ] Remove `WebSocketRoute`-driven matching and `tryCreateRequestResult(...)`-style WebSocket routing from `HttpRequestHandlerFactory` once handler-backed routing exists.
- [ ] Simplify `HttpRequest` request handling so HTTP and WebSocket routes follow the same handler execution path before the pipeline consumes the final result.
- [ ] Update request-level and pipeline-level native tests to assert the normalized handler flow rather than the current factory side-channel behavior.
- [ ] Update WebSocket design and implementation documentation to describe the normalized handler seam.

## Immediate Next Slices

- Slice 2: move handler selection to a point where header-dependent handlers, including WebSocket upgrade handlers, can be chosen through `HandlerProviderRegistry` without a separate pre-handler factory seam.
- Slice 3: replace `ProviderRegistryBuilder::websocket(...)` side-table registration with a real handler/provider registration, then remove `WebSocketRoute` and the WebSocket-specific logic from `HttpRequestHandlerFactory`.

## Owner

- Copilot

## Priority

- High

## References

- [src/handlers/IHttpHandler.h](src/handlers/IHttpHandler.h)
- [src/handlers/HttpHandler.h](src/handlers/HttpHandler.h)
- [src/routing/HandlerProviderRegistry.h](src/routing/HandlerProviderRegistry.h)
- [src/routing/ProviderRegistryBuilder.h](src/routing/ProviderRegistryBuilder.h)
- [src/core/HttpRequestHandlerFactory.h](src/core/HttpRequestHandlerFactory.h)
- [src/pipeline/RequestHandlingResult.h](src/pipeline/RequestHandlingResult.h)
- [src/websocket/WebSocketUpgradeHandler.h](src/websocket/WebSocketUpgradeHandler.h)
- [src/websocket/WebSocketRoute.h](src/websocket/WebSocketRoute.h)
- [docs/backlog/task-lists/web-sockets/005-websocket-routing-and-builder-api-backlog.md](docs/backlog/task-lists/web-sockets/005-websocket-routing-and-builder-api-backlog.md)
- [docs/backlog/task-lists/web-sockets/008-websocket-pre-implementation-decisions-backlog.md](docs/backlog/task-lists/web-sockets/008-websocket-pre-implementation-decisions-backlog.md)
