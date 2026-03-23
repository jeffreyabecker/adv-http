2026-03-23 - Copilot: added HttpRequest custom-method propagation coverage and clarified current llhttp-backed method support limits.
2026-03-23 - Copilot: marked initial custom-verb parser coverage complete and left request propagation follow-up open.
2026-03-23 - Copilot: extended backlog to cover custom HTTP verb parsing allowed by the HTTP specification.
2026-03-23 - Copilot: created detailed Phase 3 request parser and request model backlog.

# Unit Testing Phase 3 Request Parser And Request Model Backlog

## Summary

This phase covers the inbound HTTP request path without requiring any real transport. `RequestParser` already wraps llhttp behind a library-owned event interface, and `HttpRequest` already centralizes request lifecycle, address propagation, handler invocation, and response dispatch. The goal is to drive both components entirely from in-memory byte fixtures and fake handlers so parser correctness and request-phase transitions become deterministic native tests. Coverage should also explicitly verify that llhttp-supported extension methods are accepted and preserved. Truly arbitrary project-defined verbs remain a separate follow-up because the current vendored llhttp parser only recognizes a fixed method table.

## Goal / Acceptance Criteria

- `RequestParser` behavior is validated against valid, malformed, boundary-sized, and split-input request fixtures.
- llhttp-supported extension HTTP methods are accepted, surfaced unchanged to pipeline callbacks, and propagated into `HttpRequest` state without normalization.
- The current request-method length limit is explicit, tested, and aligned with the llhttp-backed method surface currently supported by the library.
- `HttpRequest` lifecycle behavior is validated without sockets by invoking pipeline callbacks directly.
- Parser events, error mapping, and request-phase transitions can be asserted in order with reusable fixtures.

## Tasks

### RequestParser Fixture Coverage

- [ ] Add parser tests for simple GET requests, POST requests with bodies, and header-only requests.
- [x] Add parser tests for llhttp-supported extension methods such as `PURGE`, `MKCOL`, and `UNSUBSCRIBE`, including verification that method text is preserved exactly and remains case-sensitive.
- [ ] Add parser tests that split request bytes across multiple `execute()` calls at request-line, header, and body boundaries.
- [x] Add parser tests that split custom-method bytes across multiple `execute()` calls so method buffering is covered independently from URL and header buffering.
- [x] Add malformed-request tests for invalid method tokens, malformed request lines, invalid headers, and unexpected EOF handling.
- [ ] Add oversized-URI, oversized-header-field, oversized-header-value, and total-buffer-limit tests.
- [x] Add method-boundary tests around the parser's current method-buffer limit so extension methods near the supported maximum length are either accepted or rejected in a deterministic, documented way.
- [ ] Evaluate parser changes if arbitrary project-defined method tokens must be supported beyond the current vendored llhttp method table.
- [ ] Verify parser completion, keep-alive decisions, and repeated `execute(nullptr, 0)` or post-finish behavior.

### Event Recording And Error Mapping

- [ ] Add a reusable `IPipelineHandler` recorder that captures message-begin, header, body, completion, and error callbacks in order.
- [x] Verify that recorded `onMessageBegin(...)` callbacks receive custom methods byte-for-byte unchanged rather than mapped to a fixed verb enum or normalized casing.
- [ ] Verify llhttp error-code mapping to `PipelineErrorCode` for each library-level path currently exposed.
- [ ] Freeze current body-chunk delivery semantics, including multiple body callbacks for one request.

### HttpRequest Lifecycle Coverage

- [x] Add tests that drive `HttpRequest` through starting-line completion, header completion, body delivery, and message completion.
- [x] Verify that `HttpRequest` exposes custom request methods unchanged after parser-driven request construction and through handler invocation.
- [ ] Verify address propagation through `setAddresses(...)` and request accessors.
- [ ] Verify item storage, URI-view caching behavior, and handler creation timing.
- [ ] Verify when `handleStep()` is triggered relative to completed phases and response-writing callbacks.
- [ ] Verify `onError(...)`, `onResponseStarted()`, `onResponseCompleted()`, and disconnect behavior where currently defined.

### Response Dispatch Interactions

- [ ] Add tests for requests whose handlers return no response, immediate responses, and body-dependent responses.
- [ ] Verify that the response-stream callback is populated exactly when a response body is available.
- [ ] Verify that response dispatch does not require a concrete socket implementation.

## Owner

TBD

## Priority

High

## References

- `src/pipeline/RequestParser.h`
- `src/pipeline/RequestParser.cpp`
- `src/pipeline/IPipelineHandler.h`
- `src/pipeline/PipelineError.h`
- `src/core/HttpRequest.h`
- `src/core/HttpRequest.cpp`
- `src/core/HttpRequestPhase.h`
- `src/core/IHttpRequestHandlerFactory.h`
- `src/llhttp/include/llhttp.h`
- RFC 9110 Section 9.1