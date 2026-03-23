2026-03-23 - Copilot: completed core response object helper coverage in native tests.
2026-03-23 - Copilot: created detailed Phase 4 response construction and body streaming backlog.

# Unit Testing Phase 4 Response Construction And Body Streaming Backlog

## Summary

This phase expands response-side coverage beyond the stream-specific tests that already exist. The response layer now depends on the byte-source abstraction rather than raw Arduino `Stream`, which makes direct serialization and body-consumption tests practical in the native lane. The goal is to freeze response assembly, header emission, and chunking semantics before higher-level pipeline tests begin to depend on them.

## Goal / Acceptance Criteria

- Direct and chunked responses can be validated byte-for-byte from native tests.
- Response headers, body ownership, and iterator order are covered without using transport code.
- Temporarily unavailable and exhausted byte-source behavior remains well-defined across response composition.

## Tasks

### Core Response Objects

- [x] Expand `HttpResponse` coverage for status, headers, body ownership transfer, and empty-body cases.
- [x] Add direct tests for `StringResponse`, `FormResponse`, and `JsonResponse` creation helpers and header defaults.
- [x] Verify that response helpers preserve caller-provided headers and body content exactly.

### Serialization And Iteration

- [ ] Extend coverage for `HttpResponseIterators` ordering, separator placement, and header-body boundaries.
- [ ] Add exact-output tests for responses with no body, small bodies, and multiple headers.
- [ ] Verify repeated body access expectations where the API allows or disallows them.

### Chunked Output

- [ ] Extend `ChunkedHttpResponseBodyStream` coverage for empty chunks, final terminator behavior, and varying source chunk sizes.
- [ ] Add temporarily unavailable source cases that cross chunk boundaries.
- [ ] Verify that chunk framing remains byte-for-byte stable for existing response semantics.

### Regression Documentation

- [ ] Record any currently odd but intentional response serialization behaviors before pipeline tests lock them in more broadly.
- [ ] Flag any response-helper inconsistencies that should be corrected before adding API-facing examples or docs coverage.

## Owner

TBD

## Priority

High

## References

- `src/response/HttpResponse.h`
- `src/response/HttpResponse.cpp`
- `src/response/StringResponse.h`
- `src/response/StringResponse.cpp`
- `src/response/FormResponse.h`
- `src/response/FormResponse.cpp`
- `src/response/JsonResponse.h`
- `src/response/JsonResponse.cpp`
- `src/response/ChunkedHttpResponseBodyStream.h`
- `src/response/ChunkedHttpResponseBodyStream.cpp`
- `src/response/HttpResponseIterators.h`
- `src/response/HttpResponseIterators.cpp`
- `test/test_native/test_response_streams.cpp`