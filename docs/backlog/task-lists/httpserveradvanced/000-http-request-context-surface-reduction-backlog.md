2026-04-06 - Copilot: completed the first request-only helper cleanup slice in routing and websocket registration.
2026-04-06 - Copilot: created backlog for reducing non-pipeline HttpContext usage in favor of HttpRequestContext.

# Http Request Context Surface Reduction Backlog

## Summary

This backlog tracks the work to move as much non-pipeline code as possible from the concrete `HttpContext` type to the narrower `HttpRequestContext` interface. The current codebase still routes many request matching, callback, and handler-extension surfaces through `HttpContext` even when they only need request metadata, headers, URI parsing, and request-scoped items.

The goal is to reduce concrete-context coupling without weakening the places that genuinely need pipeline phase state, handler lifecycle coordination, or server-owned behavior. Work should prefer removing legacy `HttpContext` callback overloads and narrowing contracts cleanly rather than introducing compatibility shims.

This item is intentionally scoped to non-pipeline surfaces first. Code that depends on `completedPhases()`, handler factory lifecycle, or other concrete-context-only behavior should either remain concrete or be split behind a narrower dedicated interface when that provides a cleaner design.

## Goal / Acceptance Criteria

- Request-only APIs use `HttpRequestContext` wherever they do not require `HttpContext`-only behavior.
- Request matching, request predicates, request interceptors, request-owned metadata access, and route-level helper utilities no longer depend on `HttpContext` unless they require concrete lifecycle state.
- Legacy handler and builder overloads that only preserved `HttpContext`-typed callback surfaces are removed or narrowed rather than kept as permanent compatibility layers.
- Concrete `HttpContext` usage outside the pipeline is limited to surfaces that genuinely need phase tracking, concrete handler lifecycle operations, or server-owned state.
- Public and internal headers include `HttpContext.h` only where the concrete type is actually required.
- Native test coverage continues to validate routing, request matching, static files, body handlers, and websocket upgrade flows after the contract narrowing.

## Tasks

### Phase 1: Inventory And Immediate Request-Only Cleanup

1. [ ] Inventory every remaining direct `HttpContext` dependency outside `src/httpadv/v1/pipeline` and classify each as request-only, phase-aware, handler-lifecycle, or legacy-overload-driven.
2. [x] Narrow obvious request-only helpers and utilities to `HttpRequestContext`, including cleanup of stale `HttpContext` includes and aliases in routing helpers.
3. [x] Narrow websocket, auth, and other request-inspection features to `HttpRequestContext` wherever response generation and upgrade logic do not require concrete context behavior.

### Phase 2: Routing And Extraction Contract Narrowing

1. [ ] Rework request predicate, matcher, and builder surfaces so request-only predicates and callbacks operate on `HttpRequestContext` unless they require concrete-only behavior.
2. [ ] Rework route parameter extraction contracts to stop requiring `HttpContext` where extraction only depends on request URL, headers, and request-owned items.
3. [ ] Review static-file request resolution and decoration paths and narrow any request-only portions to `HttpRequestContext` while preserving concrete handling where provider or lifecycle integration still needs `HttpContext`.

### Phase 3: Handler API Refactoring

1. [ ] Evaluate `IHttpHandler` and related factory/provider contracts and split request-only callback surfaces from concrete lifecycle surfaces where that improves the design without obscuring phase-aware handlers.
2. [ ] Remove legacy callback overloads in body-handler and handler-helper types that exist only to preserve `HttpContext`-typed consumer call signatures when request-only signatures are sufficient.
3. [ ] Narrow body-handler extractor and factory call paths to use `HttpRequestContext` consistently where they do not require concrete-only behavior.

### Phase 4: Validation And Residual Concrete Usage

1. [ ] Add or update native tests that specifically freeze the intended `HttpRequestContext`-based callback surfaces for routing and request helpers.
2. [ ] Document the remaining justified `HttpContext` dependencies after the migration so future work does not accidentally widen request-only contracts again.

## Owner

TBD

## Priority

High

## References

- `src/httpadv/v1/core/HttpRequestContext.h`
- `src/httpadv/v1/core/HttpContext.h`
- `src/httpadv/v1/handlers/IHttpHandler.h`
- `src/httpadv/v1/handlers/HandlerRestrictions.h`
- `src/httpadv/v1/handlers/HttpHandler.h`
- `src/httpadv/v1/handlers/RawBodyHandler.h`
- `src/httpadv/v1/handlers/MultipartFormDataHandler.h`
- `src/httpadv/v1/handlers/FormBodyHandler.h`
- `src/httpadv/v1/handlers/JsonBodyHandler.h`
- `src/httpadv/v1/handlers/BufferedStringBodyHandler.h`
- `src/httpadv/v1/routing/HandlerBuilder.h`
- `src/httpadv/v1/routing/HandlerMatcher.h`
- `src/httpadv/v1/routing/HandlerProviderRegistry.h`
- `src/httpadv/v1/routing/ProviderRegistryBuilder.h`
- `src/httpadv/v1/routing/BasicAuthentication.h`
- `src/httpadv/v1/routing/ReplaceVariables.h`
- `src/httpadv/v1/staticfiles/StaticFileHandler.h`
- `src/httpadv/v1/websocket/WebSocketUpgradeHandler.h`
- `test/test_native/test_routing.cpp`
- `test/test_native/test_http_context.cpp`
- `test/test_native/test_pipeline.cpp`
- `test/test_native/test_static_files.cpp`