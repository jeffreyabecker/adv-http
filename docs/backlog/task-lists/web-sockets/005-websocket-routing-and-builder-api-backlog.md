2026-03-23 - Copilot: added locked decisions for route registration shape (10) and public callback surface (11); updated tasks accordingly.
2026-03-23 - Copilot: aligned Phase 5 backlog with the accepted dedicated upgrade-handler ownership split.
2026-03-23 - Copilot: created detailed Phase 5 WebSocket routing and builder API backlog.

# WebSocket Phase 5 Public Routing And Builder API Backlog

## Summary

This phase exposes WebSocket support through the server builder and route-selection layers without collapsing WebSocket behavior into the existing response-centric HTTP handler model. The API should stay intentionally small at first and create sessions only after the routed endpoint has approved the handshake path.

## Goal / Acceptance Criteria

- The builder can register WebSocket routes independently from existing HTTP routes.
- Route matching can select a WebSocket endpoint and hand off into the dedicated upgrade handler, with session creation occurring only for accepted upgrade requests.
- The first public callback surface is small, explicit, and suitable for embedded use.
- Existing non-WebSocket HTTP routing remains intact and unambiguous.

## Locked Decisions Applied Here

- Routing decides WebSocket endpoint intent, not low-level handshake acceptance.
- The dedicated WebSocket upgrade handler owns protocol validation and handshake response construction.
- Register WebSocket routes through a dedicated `.websocket(path, WebSocketCallbacks)` method on `ProviderRegistryBuilder`; do not extend the `on<THandler>` template path.
- The public callback surface is a `WebSocketCallbacks` aggregate of five `std::function` fields: `onOpen`, `onText`, `onBinary`, `onClose`, and `onError`.

## Unit Test Coverage Targets

- Add native tests for WebSocket route registration, path matching, and endpoint selection.
- Verify that unmatched upgrade requests do not accidentally fall into unrelated HTTP handlers or vice versa, and that matched routes hand off into the upgrade-handler path.
- Verify callback ordering for open, text message, binary message, close, and error on a minimal session fixture.
- Verify session factory creation timing so sessions are not constructed for rejected or unmatched routes.
- Verify coexistence scenarios where normal HTTP routes and WebSocket routes share nearby path patterns.

## Tasks

### Public Registration Shape

- [ ] Implement the `.websocket(path, WebSocketCallbacks)` method on `ProviderRegistryBuilder` as the sole registration entry point for WebSocket endpoints.
- [ ] Define `WebSocketCallbacks` as a struct of five `std::function` fields: `onOpen`, `onText`, `onBinary`, `onClose`, and `onError`.
- [ ] Prefer straightforward ownership and explicit factories over compatibility shims or layered adapter types.

### Builder Integration

- [ ] Update `src/server/WebServerBuilder.h` to register WebSocket endpoints through the same overall server composition path.
- [ ] Wire `.websocket()` into the builder's route-registration path so it produces an upgrade-capable handler factory backed by the dedicated upgrade handler from Phase 2.
- [ ] Ensure the builder still initializes a unified pipeline path that can serve both HTTP and upgraded connections while routing WebSocket matches into the dedicated upgrade handler.

### Routing Behavior

- [ ] Wire upgrade-time route matching so a WebSocket route is selected before the dedicated upgrade handler validates protocol details or creates a session.
- [ ] Ensure rejected or unmatched upgrade attempts continue through deterministic HTTP behavior.
- [ ] Keep route matching rules explicit enough that path intent remains routing-owned while handshake acceptance remains handler-owned.

## Decision Follow-Through

- Item 4 in the pre-implementation decision backlog fixes the routing-to-handshake split this phase must respect; public API design must not leak handshake-validation policy back into the route matcher.
- Item 10 in the pre-implementation decision backlog fixes the registration entry point to `.websocket()` on `ProviderRegistryBuilder`.
- Item 11 in the pre-implementation decision backlog fixes the callback surface to a `WebSocketCallbacks` struct of five `std::function` fields.

## Owner

TBD

## Priority

High

## References

- `docs/plans/websocket-support-plan.md`
- `src/server/WebServerBuilder.h`
- `src/routing/`
- `src/core/HttpRequestHandlerFactory.h`
- `src/core/IHttpRequestHandlerFactory.h`
- `src/core/HttpRequest.h`
- `src/core/HttpRequest.cpp`
- `test/test_native/`
- `docs/backlog/task-lists/web-sockets/008-websocket-pre-implementation-decisions-backlog.md`