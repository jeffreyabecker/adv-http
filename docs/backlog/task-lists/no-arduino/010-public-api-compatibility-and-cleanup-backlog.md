2026-03-23 - Copilot: recorded the umbrella-header cleanup that removed the Arduino static-files convenience include from `HttpServerAdvanced.h`.
2026-03-23 - Copilot: removed obsolete public `String` overloads from the active core and routing surface, updated native tests to assert STL text behavior, and narrowed the remaining Phase 8 work to umbrella, builder, and example cleanup.
2026-03-21 - Copilot: created detailed Phase 8 public API compatibility and cleanup backlog.

# No-Arduino Phase 8 Public API Compatibility And Cleanup Backlog

## Summary

This phase reconciles the internal seam refactors with the public Arduino-facing API. By this point the internals should already be moving toward standard-library ownership and library-owned compatibility seams, but the umbrella headers, convenience builders, and example-facing aliases still need to present a coherent migration story for users. The latest cleanup slice removed the obsolete `String` caches and `String` overloads from the validated core-text and routing surface, so the remaining work is less about internal text ownership and more about deciding what Arduino-facing convenience remains justified at the boundary. The goal is to preserve practical Arduino ergonomics where they still matter, isolate compatibility overloads, and clean up public headers so non-Arduino consumers do not inherit unnecessary Arduino coupling.

## Goal / Acceptance Criteria

- Public headers reflect the new internal boundaries without unnecessarily breaking Arduino users.
- Compatibility overloads are intentional, documented, and limited.
- Umbrella includes no longer drag Arduino-only dependencies into consumers that only need the core.
- Examples demonstrate the supported public API shape rather than stale Arduino-coupled assumptions.

## Tasks

### Umbrella Header Audit

- [x] Audit `src/HttpServerAdvanced.h` for headers, typedefs, and aliases that should no longer be part of the always-on public surface.
- [ ] Group includes by core, compat, optional features, and Arduino convenience layers.
- [ ] Decide whether some includes should move out of the umbrella header into opt-in headers to reduce coupling.
- [ ] Ensure removed or optional features are not exported unconditionally through the umbrella surface.

Current note:
The latest umbrella cleanup removed the always-on Arduino static-files convenience include, so `HttpServerAdvanced.h` no longer re-exports `StaticFiles(fs::FS&, ...)` through an Arduino-only header. Further umbrella work is still open around grouping, optional features, and any remaining convenience aliases.

### Compatibility Overload Review

- [x] Inventory public APIs that currently accept Arduino `String` and classify which should remain as compatibility overloads.
- [ ] Prefer `const char *` for borrowed Arduino-friendly inputs where ownership is not required.
- [ ] Keep `String` overloads only where compatibility or ownership semantics materially justify them.
- [ ] Record which overloads are candidates for future deprecation once migration completes.

Current note:
The first cleanup sweep removed obsolete `String` overloads and adapters from `HttpHeader`, `HttpHeaderCollection`, `HttpRequest`, `BasicAuthentication`, `CrossOriginRequestSharing`, and multipart metadata accessors. Remaining compatibility review is now concentrated in umbrella exports, builder or helper APIs, example-facing entry points, and any Arduino-specific consumer guidance that still assumes implicit `String` support.

### Builder And Routing Surface Cleanup

- [ ] Audit `src/server/WebServer.h`, `src/server/WebServerBuilder.h`, and config/helper builder types for public Arduino coupling.
- [ ] Audit routing-facing builders such as `src/routing/HandlerBuilder.h` and `src/routing/ProviderRegistryBuilder.h` for public API cleanup opportunities.
- [ ] Ensure builder APIs remain ergonomic on Arduino while forwarding to standard-text internals.

### Example Alignment

- [ ] Audit example sketches and helper headers under `examples/` for APIs that would become stale after the seam migration.
- [ ] Update examples incrementally to prefer the cleaned public API shape.
- [ ] Remove example assumptions that only work because Arduino headers leak through the umbrella include today.

### Documentation And Migration Notes

- [ ] Update public documentation to explain the new include expectations and compatibility layers.
- [ ] Add migration notes for users who relied on older Arduino-shaped overloads or implicit includes.
- [ ] Record any deliberate public API break so it is visible outside the backlog files.

## Owner

TBD

## Priority

Medium

## References

- `src/HttpServerAdvanced.h`
- `src/server/WebServer.h`
- `src/server/WebServerBuilder.h`
- `src/server/WebServerConfig.h`
- `src/routing/HandlerBuilder.h`
- `src/routing/ProviderRegistryBuilder.h`
- `src/routing/BasicAuthentication.h`
- `src/routing/CrossOriginRequestSharing.h`
- `examples/`
- `docs/httpserveradvanced/LIBRARY_DOCUMENTATION.md`
- `docs/httpserveradvanced/EXAMPLES.md`
