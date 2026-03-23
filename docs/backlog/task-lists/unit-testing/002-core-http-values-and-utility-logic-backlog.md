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

- [ ] Add direct coverage for `HttpStatus` helpers, including code, phrase, and common response-construction call sites.
- [ ] Add direct coverage for `HttpMethod` parsing or comparison helpers where present.
- [ ] Add direct coverage for `HttpContentTypes` lookup behavior, fallback behavior, and extension mapping normalization.
- [ ] Extend `HttpHeader` tests to cover empty names, empty values, copy or move behavior, and adapter-facing string views.

### Header Collection Semantics

- [ ] Expand `HttpHeaderCollection` coverage for insertion order, overwrite behavior, repeated header names, removal, and case-insensitive lookup.
- [ ] Add negative tests for missing headers and value mismatches.
- [ ] Freeze any comma-join or multi-value behavior that higher routing or CORS logic already depends on.

### Query, URI, And Text Utilities

- [ ] Extend query parsing tests for empty keys, empty values, repeated keys, plus-to-space behavior, percent-decoding failures, and delimiter edge cases.
- [ ] Extend `UriView` coverage for relative paths, authority-less URLs, fragments-only, query-only, and malformed but tolerated inputs.
- [ ] Add coverage for HTML encoding, attribute encoding, URL encoding or decoding, and base64 decode failure cases.
- [ ] Verify that string-view overloads and standard-string overloads stay behaviorally aligned.

### Regression Classification

- [ ] Distinguish intended legacy behavior from bugs that should be corrected before wider coverage locks them in.
- [ ] Record any known legacy edge cases that later phases must preserve temporarily for compatibility.

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