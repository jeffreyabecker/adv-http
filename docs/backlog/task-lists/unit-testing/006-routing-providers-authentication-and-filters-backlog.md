2026-03-23 - Copilot: created detailed Phase 6 routing, providers, authentication, and filters backlog.

# Unit Testing Phase 6 Routing, Providers, Authentication, And Filters Backlog

## Summary

This phase covers how requests are matched to handlers and how cross-cutting request or response behavior is applied around those handlers. Some matcher and CORS behavior already has partial native coverage, but the provider-registry composition path, interceptor ordering, and auth helper paths remain largely untested. The goal is to freeze routing semantics at the library level before pipeline tests rely on them indirectly.

## Goal / Acceptance Criteria

- Route matching behavior is covered for URI patterns, methods, and content-type restrictions.
- Provider ordering, default-handler fallback, request filtering, interceptors, and response filters are explicitly tested.
- Basic authentication and CORS helpers are validated from constructed request objects without network integration.

## Tasks

### Matcher Semantics

- [ ] Expand `HandlerMatcher` coverage for exact paths, wildcard paths, method normalization, and content-type normalization.
- [ ] Add tests for mismatched methods, mismatched content types, and missing request metadata.
- [ ] Verify extracted route-parameter behavior for successful wildcard matches.

### Provider Registry Composition

- [ ] Add tests for `HandlerProviderRegistry` provider ordering at beginning and end insertion points.
- [ ] Verify default-handler selection when no provider matches.
- [ ] Verify request predicates, request interceptors, and response filters when combined on the same registry.
- [ ] Verify that wrapped handlers still receive body chunks and that filters only apply when responses exist.

### Builder Helpers

- [ ] Add tests for `HandlerBuilder` helper paths that compose predicates, handlers, and filters.
- [ ] Freeze any builder convenience behavior that examples or library docs currently depend on.

### Auth And CORS

- [ ] Expand `BasicAuthentication` tests for missing headers, malformed prefixes, malformed base64 payloads, missing separators, valid credentials, and failure-response headers.
- [ ] Verify both standard-string and Arduino-string-facing overload paths where they are intentionally kept.
- [ ] Expand CORS tests for omitted optional values, repeated application, and header overwrite behavior.

## Owner

TBD

## Priority

High

## References

- `src/routing/HandlerMatcher.h`
- `src/routing/HandlerMatcher.cpp`
- `src/routing/HandlerBuilder.h`
- `src/routing/HandlerBuilder.cpp`
- `src/routing/HandlerProviderRegistry.h`
- `src/routing/HandlerProviderRegistry.cpp`
- `src/routing/BasicAuthentication.h`
- `src/routing/CrossOriginRequestSharing.h`
- `test/test_native/test_utilities.cpp`