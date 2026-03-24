2026-03-24 - Copilot: created design backlog for externalizing HttpRequest-owned pipeline state so future protocol contexts such as WebSocketContext can be peers rather than special cases.

# Externalize Request-Owned Pipeline State For Peer Contexts Backlog

## Summary

The current pipeline is structurally split in two stages, but the HTTP stage is still strongly centered on `HttpRequest`. `HttpPipeline` owns the transport loop and state machine, yet `HttpRequest` owns request parsing callbacks, lazy handler creation, request-phase progression, response/upgraded-session translation, and the lifetime of the current `IHttpHandler`. That makes HTTP feel like the built-in first-class context while upgraded protocols are forced into the narrower `IConnectionSession` slot after `HttpRequest` has already done the interesting orchestration work. This backlog item examines how far that ownership can be externalized so a future `WebSocketContext` can be a peer of `HttpRequest` instead of a child artifact created only after HTTP has finished deciding everything.

## Goal / Acceptance Criteria

- The design clearly distinguishes transport-loop ownership from protocol-context ownership.
- The responsibilities currently bundled into `HttpRequest` are separated into: request data/context, protocol-step orchestration, and final result translation.
- The design identifies which responsibilities can move out of `HttpRequest` without regressing existing HTTP behavior.
- The resulting architecture allows a future `WebSocketContext` to be modeled as a peer protocol context rather than only as an opaque `IConnectionSession` implementation.
- The migration path is incremental and compatible with the existing pipeline/result model until a cleaner shared abstraction is ready.

## Current Ownership Model

- `HttpPipeline` owns the socket, loop timing, parser feeding, response writing, upgraded-session dispatch, and keep-alive restart logic.
- `HttpPipeline` creates one pipeline handler per HTTP request through `handlerFactory_`, stores it in `handler_`, and consumes only `RequestHandlingResult` values from it.
- `HttpRequest` is that pipeline handler today, but it also owns request-specific mutable state: method, version, URL, headers, addresses, items, body byte count, phase flags, cached URI view, current `IHttpHandler`, and current pending result.
- `HttpRequest` lazily creates the route handler through `IHttpRequestHandlerFactory`, retains it for the request lifetime, and decides when to call `handleStep(...)` and `handleBodyChunk(...)`.
- `HttpRequest` translates `HandlerResult` into `RequestHandlingResult`, triggers response-writing phase transitions, and constructs parser-error responses through `IHttpRequestHandlerFactory::createResponse(...)`.
- Upgraded protocols do not get a comparable context/orchestrator object in the pipeline. They are reduced to `IConnectionSession::handle(IClient &, const Compat::Clock &)` after `HttpRequest` yields an upgrade result.

## What Can Be Externalized

- Handler lifetime ownership can move out of `HttpRequest`. The current `handler_` cache is not inherently request-data state; it is protocol-execution state.
- Phase-driven orchestration can move out of `HttpRequest`. The logic that decides when to create the handler, when to call `handleStep(...)`, and when to finalize as `noResponse()` is execution control, not request data.
- `HandlerResult` to `RequestHandlingResult` translation can move out of `HttpRequest`. That translation is part of the pipeline contract boundary, not the request model itself.
- Parser-error response synthesis can move out of `HttpRequest` into a protocol-runner or adapter object, leaving `HttpRequest` as data plus helper methods.
- The pipeline-handler role can be separated from the request-context role so that parser callbacks mutate a context object but are implemented by an orchestrator that is free to own additional protocol state.

## What Probably Stays With HttpRequest Or A Shared Base Context

- Parsed request metadata should remain on a context object: method, version, URL, headers, route items, remote/local addresses, and URI helpers.
- Request-body accumulation bookkeeping should remain available to the HTTP context, even if body-phase control moves outward.
- Response factory convenience such as `createResponse(...)` can remain on the context if it is treated as protocol-local service access rather than orchestration ownership.
- Request-scoped extension storage in `items()` should remain attached to the per-request context, not the pipeline loop.

## Main Architectural Constraint

- `HttpPipeline` currently has exactly two runtime modes after parsing starts: continue driving an `IPipelineHandler` for HTTP, or switch to an `IConnectionSession` after upgrade.
- As long as that split remains, `WebSocketContext` cannot truly be a peer of `HttpRequest`; it can only be an internal detail behind `IConnectionSession`.
- A peer model therefore requires one of two changes:
  - introduce a shared protocol-runner/context seam above both `HttpRequest` and `WebSocketSessionRuntime`, or
  - broaden the pipeline runtime contract so upgraded protocols can expose richer context ownership than `IConnectionSession::handle(...)` alone.

## Candidate End State

- `HttpRequest` becomes primarily an HTTP context object containing parsed request data, HTTP-specific helper services, and request-local storage.
- A separate HTTP protocol runner owns handler creation, phase progression, error mapping, and `HandlerResult` to pipeline-result translation.
- `HttpPipeline` drives protocol runners or protocol contexts through one shared execution seam rather than privileging HTTP with a richer orchestration object.
- `WebSocketContext` can then own websocket-specific state such as outbound queue access, close-state observation, typed event dispatch, and per-connection items without being forced through a minimal session interface.

## Recommended Direction

- Do not try to make `WebSocketContext` a peer of `HttpRequest` by inflating `IConnectionSession` alone; that would still leave HTTP owning the orchestration seam.
- First split `HttpRequest` into two conceptual pieces:
  - an HTTP context/data object
  - an HTTP protocol runner/orchestrator
- After that split, decide whether the pipeline should move to a generalized protocol-runner abstraction or whether upgraded protocols should gain a richer post-upgrade context contract.
- Treat the new `WebSocketContext` as the websocket analogue of the future slimmed-down `HttpRequest`, not as a direct replacement for `WebSocketSessionRuntime`.

## Incremental Migration Slices

- Slice 1: document and isolate the current HTTP orchestration responsibilities that do not intrinsically belong to request data.
- Slice 2: extract handler lifetime ownership and phase progression from `HttpRequest` into a dedicated HTTP runner/adapter while preserving the existing `RequestHandlingResult` contract.
- Slice 3: define a shared pipeline-facing abstraction for protocol execution that can represent both HTTP request handling and upgraded connection contexts.
- Slice 4: redesign websocket runtime/context ownership so a future `WebSocketContext` is surfaced through that shared execution abstraction rather than hidden behind a narrow session loop.

## Open Questions

- Should the shared peer abstraction be request-shaped, connection-shaped, or a more generic protocol-execution object?
- Does HTTP remain one-request-per-context while websocket remains one-connection-per-context, or should both be normalized at the connection layer?
- How much of `HttpRequestPhase` should survive once orchestration moves out of `HttpRequest`?
- Should parser callbacks continue to target the context directly, or should they target the orchestrator which then mutates the context?
- Can `RequestHandlingResult` remain the shared pipeline currency, or does a peer-context design want a broader execution-result model?

## Tasks

- [ ] Document the exact orchestration responsibilities currently owned by `HttpRequest` that are not inherently request-data responsibilities.
- [ ] Design a slimmed-down HTTP context shape that can survive without directly owning handler lifetime and pipeline-result translation.
- [ ] Design an HTTP runner/orchestrator abstraction that can own handler creation, phase advancement, and error-to-response mapping.
- [ ] Decide whether the pipeline should generalize above `IPipelineHandler` and `IConnectionSession`, or whether one of those seams should absorb the peer-context design.
- [ ] Define how a future `WebSocketContext` would map onto the chosen shared execution seam.
- [ ] Identify which existing HTTP tests would need to move from `HttpRequest` behavior tests to protocol-runner behavior tests after the split.
- [ ] Update websocket context/send-api planning to align with the chosen peer-context architecture.

## Owner

- Copilot

## Priority

- High

## References

- [src/core/HttpRequest.h](src/core/HttpRequest.h)
- [src/core/HttpRequest.cpp](src/core/HttpRequest.cpp)
- [src/core/IHttpRequestHandlerFactory.h](src/core/IHttpRequestHandlerFactory.h)
- [src/pipeline/HttpPipeline.h](src/pipeline/HttpPipeline.h)
- [src/pipeline/HttpPipeline.cpp](src/pipeline/HttpPipeline.cpp)
- [src/pipeline/ConnectionSession.h](src/pipeline/ConnectionSession.h)
- [src/websocket/WebSocketSessionRuntime.h](src/websocket/WebSocketSessionRuntime.h)
- [src/websocket/WebSocketSessionRuntime.cpp](src/websocket/WebSocketSessionRuntime.cpp)
- [docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md](docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md)
- [docs/backlog/task-lists/web-sockets/010-normalize-provider-registry-builder-websocket-surface-backlog.md](docs/backlog/task-lists/web-sockets/010-normalize-provider-registry-builder-websocket-surface-backlog.md)