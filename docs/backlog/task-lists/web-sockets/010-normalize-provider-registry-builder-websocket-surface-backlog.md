2026-03-24 - Copilot: refined the design direction toward a dedicated WebSocketBuilder, matcher-based websocket entry point, and typed event registration adapters.
2026-03-24 - Copilot: created backlog item for normalizing the ProviderRegistryBuilder websocket registration surface so it reflects the helper model available on the handler path.

# Normalize ProviderRegistryBuilder WebSocket Surface Backlog

## Summary

`ProviderRegistryBuilder::websocket(path, WebSocketCallbacks)` currently registers a WebSocket endpoint as a one-shot operation, while the normal handler path routes through `on<THandler>(...)` and returns a `HandlerBuilder<THandler>` with additional composition helpers. That leaves the WebSocket path unable to express route-local interceptors, request filters, and other builder-time policy in the same way as ordinary handlers. It also freezes the public API at a raw `WebSocketCallbacks` aggregate even though richer event registration, typed payload coercion, and per-event composition are likely to matter for real endpoints. This backlog item captures the work needed to normalize that surface so WebSocket registrations follow the same mental model as the rest of the builder API without pretending WebSocket routes are just ordinary response handlers.

## Goal / Acceptance Criteria

- WebSocket registration exposes a builder surface that is structurally consistent with the existing handler registration path instead of a one-off helper.
- WebSocket registration is rooted in a dedicated `WebSocketBuilder` surface rather than the raw `WebSocketCallbacks` aggregate.
- Route-local composition helpers available on the handler path are either supported on the WebSocket path or intentionally rejected with a documented reason.
- The semantics of each helper are explicit for upgrade-capable routes, especially where successful upgrades differ from HTTP response production.
- The registry surface has a matcher-based websocket entry point, with path convenience overloads delegating to the same implementation.
- The event registration model can grow beyond raw text and binary callbacks so typed payload handlers such as JSON can be layered on coherently.
- Existing route ordering and upgrade matching behavior remain deterministic after the surface normalization.
- Native tests cover the normalized WebSocket builder surface, including helper composition and fallback behavior.

## Current Design Direction

- Introduce a dedicated `WebSocketBuilder` that is similar to but distinct from `HandlerBuilder<THandler>`.
- Prefer a matcher-rooted entry point such as `ws(const HandlerMatcher &request)` as the primary websocket registration seam, with convenience overloads for simple path-based registration delegating to that matcher form.
- Keep `WebSocketBuilder` distinct from `HandlerBuilder<THandler>` because websocket registration culminates in event registration and upgrade semantics, not a single request invocation callback.
- Share route-level composition behavior where it is semantically valid, but do not force the websocket API to inherit HTTP-only affordances just for surface symmetry.
- Treat `WebSocketCallbacks` as an assembly detail or compatibility carrier for the upgrade/session runtime, not as the preferred long-term user-facing registration API.

## Proposed Surface Sketch

```cpp
handlers()
	.ws(HandlerMatcher("/chat/*", "GET"))
	.with(authInterceptor)
	.filterRequest(requireTenantHeader)
	.apply(addCorsHeaders)
	.onOpen([](WebSocketContext &context)
	{
		context.sendText("ready");
	})
	.onText([](WebSocketContext &, std::string_view message)
	{
		// handle raw text
	})
	.onJson<MyMessage>([](WebSocketContext &, const MyMessage &message)
	{
		// handle decoded payload
	})
	.onClose([](WebSocketContext &, std::uint16_t code, std::string_view reason)
	{
		// observe close
	});
```

## Design Notes

- `with(...)` and `filterRequest(...)` should compose at route-selection and handler-execution time just as they do for ordinary handler routes.
- `apply(...)` should affect deterministic HTTP responses emitted by the websocket route, such as handshake rejection responses, but not successful upgrades once the connection leaves HTTP response semantics.
- `allowMethods(...)` should not imply arbitrary method support; the builder should either lock websocket routes to the valid handshake method set or reject contradictory configuration.
- `allowContentTypes(...)` is not automatically meaningful for websocket handshakes and may need to be omitted, constrained, or reinterpreted only for typed message adapters rather than the initial HTTP upgrade request.
- Typed event handlers should be layered above the raw text/binary event surface through adapters or codecs so the runtime remains transport-oriented while the builder exposes richer registration options.
- JSON support should be one instance of that typed-adapter model rather than a special-case design that prevents other payload coercions later.

## Open Questions To Resolve

- Should the public root method be named `ws(...)`, `websocket(...)`, or both, with one delegating to the other?
- Which route-builder helpers should live directly on `WebSocketBuilder`, and which should be factored into a shared base with `HandlerBuilder<THandler>`?
- Should typed event registration use named helpers such as `onJson<T>(...)`, generic codec-based helpers such as `onTextAs<TCodec>(...)`, or both?
- How much state and send capability should the event handlers receive through a future websocket context type?

## Tasks

- [x] Inventory the current `ProviderRegistryBuilder` surface mismatch between `.websocket(...)` and `on<THandler>(...)`, including which behavior today lives on `HandlerBuilder<THandler>` rather than the builder itself.
- [x] Decide whether WebSocket registrations should return a dedicated builder type, reuse `HandlerBuilder` through a generalized abstraction, or move shared helper composition behind a common route-builder primitive.
- [x] Define the expected WebSocket semantics for `with(...)`, `filterRequest(...)`, `apply(...)`, `allowMethods(...)`, and `allowContentTypes(...)` rather than assuming the HTTP meanings carry over unchanged.
- [ ] Specify how response filters should behave for upgrade-capable routes, including whether they apply only to deterministic HTTP rejection responses, to unmatched fallback responses, or to both.
- [ ] Specify how method and content-type helpers should interact with the fixed WebSocket handshake requirements so the API does not imply invalid route configurations.
- [ ] Design the `WebSocketBuilder` event-registration surface, including raw event methods and typed/coerced event overloads.
- [ ] Decide how typed message coercion is modeled so JSON and future payload adapters share one extensible registration mechanism.
- [ ] Refactor `ProviderRegistryBuilder` so the WebSocket path reflects the same builder-time composition model as ordinary handler registration.
- [ ] Add native tests for chained WebSocket builder helpers, including interceptor ordering, predicate composition, rejection-response filtering, and coexistence with nearby HTTP routes.
- [ ] Update WebSocket builder documentation and examples to describe the normalized surface and any intentional differences from the generic handler path.

## Owner

- Copilot

## Priority

- High

## References

- [src/routing/ProviderRegistryBuilder.h](src/routing/ProviderRegistryBuilder.h)
- [src/routing/HandlerBuilder.h](src/routing/HandlerBuilder.h)
- [src/handlers/HandlerTypes.h](src/handlers/HandlerTypes.h)
- [src/routing/HandlerProviderRegistry.h](src/routing/HandlerProviderRegistry.h)
- [src/websocket/WebSocketUpgradeHandler.h](src/websocket/WebSocketUpgradeHandler.h)
- [docs/backlog/task-lists/web-sockets/005-websocket-routing-and-builder-api-backlog.md](docs/backlog/task-lists/web-sockets/005-websocket-routing-and-builder-api-backlog.md)
- [docs/backlog/task-lists/web-sockets/009-normalize-websocket-flow-behind-ihttphandler-backlog.md](docs/backlog/task-lists/web-sockets/009-normalize-websocket-flow-behind-ihttphandler-backlog.md)