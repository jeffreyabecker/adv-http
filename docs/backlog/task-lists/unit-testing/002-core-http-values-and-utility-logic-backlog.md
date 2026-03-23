2026-03-23 - Copilot: completed the native Phase 2 utility backlog, including encoder edge cases, header-collection semantics, and documented legacy URI-decoding compatibility behaviors.
2026-03-23 - Copilot: created detailed Phase 2 core HTTP values and utility logic backlog.

# Unit Testing Phase 2 Core HTTP Values And Utility Logic Backlog

## Summary

This phase expands coverage around the smallest and most reusable HTTP building blocks in the library. Several utility and header behaviors already have partial native tests, but the coverage is still centered on happy-path string handling and does not yet freeze many edge cases that higher layers rely on. The goal is to lock down these value types and utility semantics before parser, routing, and handler suites depend on them more heavily.

## Goal / Acceptance Criteria

- Core value types and utility helpers have explicit native coverage for normal, boundary, and malformed inputs.
- Header lookup and text-normalization behavior are frozen so later routing and auth suites do not silently depend on unspecified semantics.
- Utility regressions can be diagnosed at the smallest layer before they surface as parser or handler failures.

## Tasks

### HTTP Constants And Small Value Types

- [x] Add direct coverage for `HttpStatus` helpers, including code, phrase, and common response-construction call sites.
- [x] Add direct coverage for `HttpMethod` parsing or comparison helpers where present.
- [x] Add direct coverage for `HttpContentTypes` lookup behavior, fallback behavior, and extension mapping normalization.
- [x] Extend `HttpHeader` tests to cover empty names, empty values, copy or move behavior, and adapter-facing string views.

### Header Collection Semantics

- [x] Expand `HttpHeaderCollection` coverage for insertion order, overwrite behavior, repeated header names, removal, and case-insensitive lookup.
- [x] Add negative tests for missing headers and value mismatches.
- [x] Freeze any comma-join or multi-value behavior that higher routing or CORS logic already depends on.

### Query, URI, And Text Utilities

- [x] Extend query parsing tests for empty keys, empty values, repeated keys, plus-to-space behavior, percent-decoding failures, and delimiter edge cases.
- [x] Extend `UriView` coverage for relative paths, authority-less URLs, fragments-only, query-only, and malformed but tolerated inputs.
- [x] Add coverage for HTML encoding, attribute encoding, URL encoding or decoding, and base64 decode failure cases.
- [x] Verify that string-view overloads and standard-string overloads stay behaviorally aligned.

### Regression Classification

- [x] Distinguish intended legacy behavior from bugs that should be corrected before wider coverage locks them in.
- [x] Record any known legacy edge cases that later phases must preserve temporarily for compatibility.

## Regression Notes

- Compatibility-frozen for now: repeated non-`Set-Cookie` headers merge into the first insertion slot using comma-joined values, while repeated `Set-Cookie` headers remain separate entries.
- Compatibility-frozen for now: query lookup continues to return the first matching value from repeated keys, while `getAll()` preserves every occurrence.
- Compatibility-frozen for now: malformed percent-decoding currently treats `%ZZ` as a single `NUL` byte and drops trailing incomplete sequences such as `%` and `%A`; later phases should preserve this only until an intentional breaking change is approved.
- Compatibility-frozen for now: authority-less and query-only URI forms remain accepted by `UriView` and are now covered directly in the native lane.

## Owner

TBD

## Priority

High

## References

- `src/core/HttpStatus.h`
- `src/core/HttpMethod.h`
- `src/core/HttpContentTypes.h`
- `src/core/HttpHeader.h`
- `src/core/HttpHeaderCollection.h`
- `src/core/HttpHeaderCollection.cpp`
- `src/util/HttpUtility.h`
- `src/util/HttpUtility.cpp`
- `src/util/UriView.h`
- `src/util/UriView.cpp`
- `test/test_native/test_utilities.cpp`
- `test/support/include/HttpTestFixtures.h`