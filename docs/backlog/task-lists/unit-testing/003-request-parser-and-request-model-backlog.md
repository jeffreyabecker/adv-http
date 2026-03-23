2026-03-23 - Copilot: created detailed Phase 3 request parser and request model backlog.

# Unit Testing Phase 3 Request Parser And Request Model Backlog

## Summary

This phase covers the inbound HTTP request path without requiring any real transport. `RequestParser` already wraps llhttp behind a library-owned event interface, and `HttpRequest` already centralizes request lifecycle, address propagation, handler invocation, and response dispatch. The goal is to drive both components entirely from in-memory byte fixtures and fake handlers so parser correctness and request-phase transitions become deterministic native tests.

## Goal / Acceptance Criteria

- `RequestParser` behavior is validated against valid, malformed, boundary-sized, and split-input request fixtures.
- `HttpRequest` lifecycle behavior is validated without sockets by invoking pipeline callbacks directly.
- Parser events, error mapping, and request-phase transitions can be asserted in order with reusable fixtures.

## Tasks

### RequestParser Fixture Coverage

- [ ] Add parser tests for simple GET requests, POST requests with bodies, and header-only requests.
- [ ] Add parser tests that split request bytes across multiple `execute()` calls at request-line, header, and body boundaries.
- [ ] Add malformed-request tests for invalid methods, malformed request lines, invalid headers, and unexpected EOF handling.
- [ ] Add oversized-URI, oversized-header-field, oversized-header-value, and total-buffer-limit tests.
- [ ] Verify parser completion, keep-alive decisions, and repeated `execute(nullptr, 0)` or post-finish behavior.

### Event Recording And Error Mapping

- [ ] Add a reusable `IPipelineHandler` recorder that captures message-begin, header, body, completion, and error callbacks in order.
- [ ] Verify llhttp error-code mapping to `PipelineErrorCode` for each library-level path currently exposed.
- [ ] Freeze current body-chunk delivery semantics, including multiple body callbacks for one request.

### HttpRequest Lifecycle Coverage

- [ ] Add tests that drive `HttpRequest` through starting-line completion, header completion, body delivery, and message completion.
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