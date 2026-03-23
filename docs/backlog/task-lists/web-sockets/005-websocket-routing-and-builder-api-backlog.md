2026-03-23 - Copilot: created detailed Phase 5 WebSocket routing and builder API backlog.

# WebSocket Phase 5 Public Routing And Builder API Backlog

## Summary

This phase exposes WebSocket support through the server builder and route-selection layers without collapsing WebSocket behavior into the existing response-centric HTTP handler model. The API should stay intentionally small at first and create sessions only after the routed endpoint has approved the handshake path.

## Goal / Acceptance Criteria

- The builder can register WebSocket routes independently from existing HTTP routes.
- Route matching can select a WebSocket endpoint and create the corresponding session object only for accepted upgrade requests.
- The first public callback surface is small, explicit, and suitable for embedded use.
- Existing non-WebSocket HTTP routing remains intact and unambiguous.

## Unit Test Coverage Targets

- Add native tests for WebSocket route registration, path matching, and endpoint selection.
- Verify that unmatched upgrade requests do not accidentally fall into unrelated HTTP handlers or vice versa.
- Verify callback ordering for open, text message, binary message, close, and error on a minimal session fixture.
- Verify session factory creation timing so sessions are not constructed for rejected or unmatched routes.
- Verify coexistence scenarios where normal HTTP routes and WebSocket routes share nearby path patterns.

## Tasks

### Public Registration Shape

- [ ] Design a WebSocket-specific registration API rather than extending normal `IHttpHandler` body-processing semantics.
- [ ] Keep the first callback surface limited to the minimum useful lifecycle and message hooks.
- [ ] Prefer straightforward ownership and explicit factories over compatibility shims or layered adapter types.

### Builder Integration

- [ ] Update `src/server/WebServerBuilder.h` to register WebSocket endpoints through the same overall server composition path.
- [ ] Introduce the registry or factory types needed to store WebSocket routes and create session instances.
- [ ] Ensure the builder still initializes a unified pipeline path that can serve both HTTP and upgraded connections.

### Routing Behavior

- [ ] Wire upgrade-time route matching so a WebSocket route is selected before session creation.
- [ ] Ensure rejected or unmatched upgrade attempts continue through deterministic HTTP behavior.
- [ ] Keep route matching rules explicit enough that method, path, and upgrade requirements are easy to reason about in tests.

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