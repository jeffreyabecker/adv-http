# Testing Gaps Plan

## Summary

Native testing coverage is strong in many core areas, especially routing happy paths and baseline response serialization behavior. The current suite already covers high-traffic request matching, provider composition, chunked response basics, and several pipeline error/time-out scenarios.

There are still high-impact blind spots in both routing and response streaming. This plan consolidates those gaps into one execution sequence so test additions are prioritized by risk and minimize duplicated fixture work.

## Current Coverage Snapshot

Covered well:
- `HandlerMatcher` core method, URI wildcard, content-type, and parameter extraction behavior.
- `HandlerProviderRegistry` beginning/end ordering, chained request filters, chained interceptors, response-filter composition, and body forwarding.
- `HandlerBuilder` request predicate chaining, interceptor wrapping, response filter application, method/content-type constraints.
- `BasicAuth` missing or malformed authorization failures and success paths for fixed-credential and validator overloads.
- `CrossOriginRequestSharing` default and extended header paths plus no-overwrite semantics.
- `ChunkedHttpResponseBodyStream` baseline chunk framing, empty-source terminator emission, and temporary unavailability handling.
- `CreateResponseStream` direct response serialization, header insertion order, no-body source behavior, and baseline chunked transfer serialization.
- `HttpPipeline` partial write failure handling, timeout handling, and core response lifecycle callback flow in common paths.
- `RequestParser` custom method acceptance/rejection boundaries, including split method parsing and invalid method token failure.

## High-Impact Gaps

### Routing

1. Critical API gap: `ProviderRegistryBuilder::on(HandlerMatcher&, IHttpHandler::Factory)` is declared but no definition was found.
- Declaration: `src/routing/ProviderRegistryBuilder.h`.
- Impact: consumers using this overload can hit a link-time failure; there is no test coverage on this registration path.

2. Registry insertion branch gap: indexed insertion path is not covered.
- Branch location: `HandlerProviderRegistry::add(IHandlerProvider&, AddPosition)` in `src/routing/HandlerProviderRegistry.cpp`.
- Untested branch: `position != AddAt::End` with explicit numeric index and out-of-range clamping.

3. HandlerMatcher mutator/custom-checker gap.
- Untested methods: `setMethodChecker`, `setUriPatternChecker`, `setContentTypeChecker`, `setArgsExtractor` in `src/routing/HandlerMatcher.cpp`.
- Untested behavior: runtime override wiring and effects on `canHandle` and `extractParameters`.

4. Registry and builder null-callback composition gap.
- `HandlerBuilder::apply` handles null filters; `HandlerProviderRegistry::apply/with/filterRequest` do not currently validate null callbacks.
- No tests verify null callback behavior under mixed composition scenarios.

### Response Streaming

1. Body-suppressed status gap.
- Current tests verify suppression behavior for `204`, but not equivalent coverage for additional no-body statuses (notably `1xx` and `304`).
- Risk: regressions in `responseHasNoBody(...)` behavior can silently produce protocol-invalid output.

2. Header interaction policy gap (`Content-Length` vs `Transfer-Encoding`).
- No tests lock behavior when both headers are explicitly present.
- No tests lock behavior for explicit `Transfer-Encoding: chunked` with a null body source.

3. Incremental write-loop progression gap.
- Existing tests cover partial write failures and timeout cases, but not enough multi-loop scenarios where response bytes alternate between available and temporarily unavailable.
- Risk: stalled streams and callback sequencing regressions in `WritingHttpResponse` state.

4. Response lifecycle interruption gap.
- Missing assertions around callback and state behavior after `onResponseStarted` when writes are interrupted before completion, including connectivity transitions while writing.

### Request Parser

1. Parser limit coverage gap (URI/header name/header value/header count).
- Current request-parser tests focus on method tokens and do not cover hard limits for URI length, header field length, header value length, or header count.
- Risk: regressions in defensive bounds checks can reintroduce parser instability under malformed/hostile input.

2. Malformed input mapping gap.
- Error mapping from llhttp to `PipelineErrorCode` is broad, but tests only lock `InvalidMethodError`.
- No parser-level tests lock behavior for malformed content length, invalid header token, invalid URL, or invalid version parse failures.

3. Callback propagation and ordering gap.
- Missing tests for event ordering across split headers/body chunks and multi-header requests.
- Missing tests for non-zero return propagation from callback points (`onHeadersComplete`, `onBody`, `onMessageComplete`) into parse failure behavior.

4. Parser lifecycle gap.
- `reset()` exists but has no direct coverage.
- End-of-input (`execute(nullptr, 0)`) and message completion behavior are not explicitly tested at parser-level.

## Plan

### Phase 1: Close Critical Runtime-Risk Gaps First

1. Add a native routing test that uses `ProviderRegistryBuilder::on(HandlerMatcher&, IHttpHandler::Factory)` through both builder and `WebServerConfig` style entry points.
2. If link failure occurs, implement the missing overload in the routing layer and keep behavior aligned with other `on(...)` overloads.
3. Add assertions that registered factory receives matched requests and is bypassed on non-matches.
4. Add response-stream tests for body-suppressed statuses (`1xx`, `304`) and lock expected serialized output shape.
5. Add response-stream tests for `Transfer-Encoding: chunked` with null body source and for explicit `Content-Length` + `Transfer-Encoding` conflicts.
6. Add request-parser limit tests for URI/header-name/header-value/header-count boundaries and overflow behavior.
7. Add malformed request tests to lock llhttp-to-`PipelineErrorCode` mappings for at least invalid URL, invalid header token, invalid content length, and invalid version.

Acceptance criteria:
- Native tests compile and run with explicit use of the overload.
- The overload successfully registers and dispatches handlers.
- Response serialization behavior is fixed by tests for no-body status classes and conflicting header inputs.
- Request-parser limit and malformed-input protections are asserted with stable error outcomes.

### Phase 2: Expand Branch and State-Transition Coverage

1. Add tests for `AddPosition` explicit index insertion.
2. Add tests for out-of-range index clamping to end.
3. Add tests that default 404 handler path is used when no custom default is set and no provider matches.
4. Add pipeline tests where response source availability alternates across multiple `handleClient()` loops.
5. Add pipeline tests that verify callback/state behavior when response writing is interrupted by connectivity changes.
6. Add request-parser tests for split header field/value chunks and body chunks to assert callback ordering and data stitching.
7. Add parser tests that force non-zero handler callback returns (`onHeadersComplete`, `onBody`, `onMessageComplete`) and lock expected failure behavior.

Acceptance criteria:
- All add-position branches are exercised and asserted.
- Default not-found behavior is validated without custom default factory.
- `WritingHttpResponse` progression across temporarily-unavailable cycles is deterministic and asserted.
- Response callback ordering during interrupted writes is asserted.
- Request-parser callback ordering and callback-failure propagation are deterministic and asserted.

### Phase 3: Exercise Matcher and Policy Customization Surface

1. Add tests that inject custom checker lambdas via mutator methods and verify they are invoked.
2. Add tests for custom args extractor output propagation.
3. Add tests for query-string matching semantics with wildcard paths to lock current behavior.
4. Add a short policy note describing response header precedence and body suppression expectations used by test assertions.
5. Add request-parser lifecycle tests for `reset()` and end-of-input finalization behavior.

Acceptance criteria:
- Setter-driven checker replacement is covered end-to-end.
- Parameter extraction from custom extractor is validated.
- Query/path behavior is explicitly frozen in tests.
- Response header/body policy rules referenced by tests are documented.
- Request-parser lifecycle transitions (`execute`, finish, `reset`) are covered by direct tests.

### Phase 4: Validate Null and Composition Safety

1. Add defensive tests for null interceptors and null response filters at registry and builder layers.
2. If current behavior is unsafe, harden implementation and test the expected semantics.

Acceptance criteria:
- No crashes from null callback composition.
- Callback chaining order remains deterministic and tested.

## Recommended Order Of Execution

1. Phase 1 (critical runtime-risk and protocol-sensitive gaps).
2. Phase 2 (branch and streaming state-transition coverage).
3. Phase 3 (matcher customization and policy documentation).
4. Phase 4 (null/composition hardening).

## Exit Criteria

- Routing suite includes explicit tests for each currently untested high-impact branch listed in this plan.
- Response-streaming suite includes tests for no-body statuses, header conflict policy, and incremental write-loop behavior.
- Request-parser suite includes direct coverage for limits, malformed input mappings, callback propagation, and lifecycle reset behavior.
- The missing/critical overload path is verified at compile and runtime.
- Native test run passes with routing, response-streaming, and pipeline suites covering the expanded surface.
