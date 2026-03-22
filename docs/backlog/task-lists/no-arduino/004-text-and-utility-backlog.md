2026-03-22 - Copilot: removed `StringUtility` from remaining core call sites, dropped Arduino-`String` overloads from `StringUtility`, and widened native validation to compile `HandlerMatcher.cpp`.
2026-03-22 - Copilot: migrated the remaining production `StringView` call sites in `HttpUtility` to `std::string_view`, reduced `StringView.h` to compatibility aliases, and validated the native test lane.
2026-03-22 - Copilot: migrated handler route params to standard-text plumbing, moved multipart metadata ownership to std::string-backed internals, audited matcher/auth/CORS borrowed-input paths, and validated the slice in the native lane.
2026-03-21 - Copilot: created detailed Phase 2 text and utility backlog.
2026-03-21 - Copilot: reordered Phase 2 so URI and query parsing lands before `StringUtility` and `StringView` cleanup to reduce churn.
2026-03-21 - Copilot: implemented the first URI/query parsing migration slice and validated it through the native PlatformIO test lane.
2026-03-21 - Copilot: admitted `HttpHeaderCollection` into the native lane and added standard-text header lookup accessors while leaving header/request ownership migration open.
2026-03-21 - Copilot: moved `HttpHeader` and `HttpRequest` internal ownership to `std::string`, widened the native portable source list to cover the new slice, and fixed host-safe response-path regressions uncovered by that validation.
2026-03-21 - Copilot: captured the remaining `String` inventory snapshot and froze the next conversion order around `StringUtility`, `StringView`, and routing/handler parameter plumbing.

# No-Arduino Phase 2 Text And Utility Backlog

## Summary

This phase attacks the deepest and widest coupling point in the repository: Arduino `String` and the text utility stack built around it. The core request model, header collection, URI parsing helpers, handler parameter plumbing, and many response helpers currently assume Arduino-owned strings. The first move in this phase should be the URI and query parsing transition so that the dominant parser-facing ownership and view semantics settle early, compile pressure drops sooner, and later `StringUtility` and `StringView` cleanup can follow the stabilized URI/query model instead of causing extra churn. That URI/query slice is now in place, `HttpHeaderCollection` has been admitted into the native lane with `std::string_view`-friendly lookup semantics, and the next core ownership slice has landed with `HttpHeader` and `HttpRequest` now storing text internally as `std::string` while keeping Arduino-facing adapters at the boundary. The remaining goal is to replace bespoke text helpers in `StringUtility` with STL algorithms or small standard-library-backed helpers, retire `StringView` in favor of STL view and ownership constructs, keep unwinding Arduino ownership from routing and handler plumbing, and leave Arduino-facing overloads only as transition or boundary adapters rather than the internal model.

## Goal / Acceptance Criteria

- Core text ownership no longer depends on Arduino `String` in the utility layer and core request/header models.
- URI and query helpers are rebuilt on STL text constructs and standard-library algorithms.
- URI and query parsing transitions land before the broader `StringUtility` and `StringView` cleanup so later refactors follow the new parsing model instead of reshaping it repeatedly.
- `StringUtility` is no longer a required core abstraction and `StringView` is no longer a required library-owned text-view type on the core path.
- Arduino-facing `String` overloads remain only where compatibility is still needed.
- The first host-safe core text and model files compile without `Arduino.h`.

## Tasks

### String Inventory And Conversion Order

- [x] Build a file-by-file inventory of `String` usage across `src/util`, `src/core`, `src/handlers`, `src/routing`, `src/response`, and `src/HttpServerAdvanced.h`.
- [x] Classify each `String` use as owned internal state, non-owning parsed view, compatibility overload, or Arduino-only boundary.
- [x] Record the preferred replacement type for each classification: `std::string`, `std::string_view`, `const char *`, or deferred adapter-only retention.
- [x] Freeze the conversion order so URI and query parsing lands first, then the remaining lower-level text utilities, then higher-level routing and response helpers.

#### Current Inventory Snapshot

- `src/core/HttpHeader.*`, `src/core/HttpHeaderCollection.*`, and `src/core/HttpRequest.*`: now migrated to `std::string` internal ownership with Arduino `String` adapters retained at the boundary. Classification: owned internal state. Preferred replacement: keep `std::string` internally and leave compatibility adapters temporary.
- `src/core/HttpMethod.h` and `src/routing/HandlerMatcher.*`: `HttpMethod.h` no longer needs `StringUtility`, and `HandlerMatcher.*` now uses direct standard-library operations plus small local ASCII helpers for case-insensitive prefix checks. Classification: owned internal state plus compatibility-heavy matcher APIs. Preferred replacement: continue moving matcher configuration and extracted params to `std::string` / `std::vector<std::string>`.
- `src/util/StringUtility.*`: now reduced to a minimal standard-library-backed helper layer for compare/prefix/suffix/search operations, with `replace()` returning `std::string`; Arduino `String` overloads have been removed. Classification: reduced compatibility helper. Preferred replacement: keep only while downstream compatibility still benefits, then delete once no callers remain.
- `src/util/StringView.h`: now reduced to a short-lived compatibility header that aliases `StringView` to `std::string_view` and `OwningStringView` to `std::string`, with no `Arduino.h` dependency. Classification: compatibility-only surface. Preferred replacement: use `std::string_view` for borrowed slices and `std::string` for retained ownership directly at call sites.
- `src/util/HttpUtility.*`: query parsing, encode/decode, and Base64 helpers now take `std::string_view` on the standard-text path, leaving `String` overloads as compatibility adapters. Classification: compatibility overloads around a standard-text core. Preferred replacement: keep `const char *` and `std::string_view` front doors for the core path and continue reducing Arduino-facing wrappers later.
- `src/handlers/HandlerRestrictions.h` and `src/handlers/HandlerTypes.h`: handler plumbing still hard-wires extracted route params as `std::vector<String>`. Classification: owned internal state flowing through core-ish plumbing. Preferred replacement: `std::vector<std::string>` internally, with adapter lambdas only where Arduino-facing handler ergonomics still need `String`.
- `src/handlers/BufferedStringBodyHandler.*`, `src/handlers/FormBodyHandler.*`, `src/handlers/RawBodyHandler.*`, `src/handlers/JsonBodyHandler.*`, and `src/handlers/MultipartFormDataHandler.*`: body handlers still carry `std::vector<String>` params and, depending on handler type, Arduino `String` payloads or multipart metadata. Classification: mixed owned internal state and compatibility surfaces. Preferred replacement: `std::vector<std::string>` for params, `std::string` for buffered body and multipart metadata, leave Arduino `String` payload adapters only where a public handler contract still expects them.
- `src/routing/HandlerBuilder.h`: the builder still seeds empty params as `std::vector<String>` and depends on handler restriction types. Classification: owned internal routing state. Preferred replacement: align with `std::vector<std::string>` once handler plumbing moves.
- `src/routing/BasicAuthentication.h` and `src/routing/CrossOriginRequestSharing.h`: these remain intentionally compatibility-oriented and still manipulate Arduino `String` directly. Classification: compatibility overload / Arduino-facing boundary. Preferred replacement: prefer `const char *` or standard-text internals later, but keep the user-facing adapters until a public API cleanup phase.
- `src/response/StringResponse.h`, `src/response/FormResponse.h`, and related response helpers: still expected to be compatibility-oriented string wrappers. Classification: compatibility overload / Arduino-facing boundary. Preferred replacement: thin adapters over standard-text internals, deferred until after routing and handler plumbing stabilizes.
- `src/HttpServerAdvanced.h`: umbrella header still re-exports `StringUtility.h`, `StringView.h`, `HandlerRestrictions.h`, and `HandlerTypes.h`. Classification: umbrella compatibility surface. Preferred replacement: defer until internal utility and handler migrations finish so umbrella aliases can be reduced in one pass.

#### Frozen Conversion Order

1. Keep the already-landed URI/query and core request/header ownership slices as the baseline.
2. Reduce `src/util/StringUtility.*` to the minimum standard-library-backed helper surface needed by current call sites.
3. Replace `src/util/StringView.h` and `OwningStringView` call sites with `std::string_view` / `std::string`, then decide whether the header survives only as a compatibility shim.
4. Migrate `src/routing/HandlerMatcher.*`, `src/handlers/HandlerRestrictions.h`, `src/handlers/HandlerTypes.h`, and `src/routing/HandlerBuilder.h` from `std::vector<String>` to `std::vector<std::string>`.
5. Push the new parameter/body text model through `FormBodyHandler`, `RawBodyHandler`, `MultipartFormDataHandler`, and adjacent handler types.
6. Finish with response helpers, auth/CORS convenience wrappers, and the umbrella header once the internal model is stable.

### URI And Query Parsing First

- [x] Refactor `src/util/UriView.h` and `src/util/UriView.cpp` so URI ownership is standard-library-based before broader `StringUtility` and `StringView` cleanup begins.
- [x] Replace `KeyValuePairView<String, String>` query storage with a standard-text equivalent or a compatibility typedef that no longer requires Arduino ownership in the core.
- [x] Refactor `src/util/HttpUtility.h` and `src/util/HttpUtility.cpp` query parsing and URI encoding helpers to use standard-string-based results internally and to stop depending on `StringUtility` or `StringView` as core infrastructure.
- [x] Replace `String`-returning utility implementations in URI and query paths with standard-string internal generation plus compatibility adapters only where public transitions still require them.
- [x] Keep any Arduino-facing borrowed-input overloads on `const char *` before reintroducing `String` convenience overloads.
- [x] Verify that the URI and query transition leaves the phase in a materially lower-churn state before broad `StringUtility` and `StringView` cleanup continues.

### `StringUtility` Refactor

- [x] Inventory every function in `src/util/StringUtility.h` and classify it as `replace with STL call site`, `replace with small standard helper`, or `delete`.
- [x] Replace uses of `StringUtility` in core code with direct STL equivalents where those already exist, such as comparisons, prefix/suffix checks, search, substring handling, and character classification, after the URI/query transition has established the target text model.
- [x] For behaviors not covered cleanly by direct STL calls, introduce narrowly scoped standard-library-backed helpers instead of preserving the current `StringUtility` surface wholesale.
- [x] Remove Arduino-`String`-centric overloads from the core path rather than carrying them forward as the canonical API.
- [x] Decide whether `src/util/StringUtility.*` becomes a temporary compatibility shim, is reduced to a minimal adapter-only layer, or is deleted entirely once call sites are migrated.
- [x] Add or extend native tests in `test/test_native/test_utilities.cpp` for the STL-equivalent behaviors that replace current `StringUtility` call sites.

#### `StringUtility` Function Inventory After Audit

- `compareTo(const char *, size_t, const char *, size_t, bool)`: replace with small standard helper. Reason: case-insensitive lexicographic compare still needs a compact shared implementation where direct STL calls are awkward.
- `startsWith(const char *, size_t, const char *, size_t, bool)`: replace with STL call site. Reason: core callers now use `std::string_view` size checks plus direct comparison or small file-local helpers.
- `endsWith(const char *, size_t, const char *, size_t, bool)`: replace with STL call site. Reason: direct suffix comparison is straightforward at call sites.
- `indexOf(const char *, size_t, const char *, size_t, size_t, bool)`: replace with STL call site. Reason: core callers now use `std::string_view::find()` or local parsing scans.
- `lastIndexOf(const char *, size_t, const char *, size_t, size_t, bool)`: replace with STL call site. Reason: direct reverse search is straightforward where still needed.
- `compareTo(std::string_view, std::string_view, bool)`: replace with small standard helper. Reason: it remains the smallest shared entry point for compatibility callers that still need case-insensitive compare.
- `startsWith(std::string_view, std::string_view, bool)`: replace with STL call site. Reason: core callers now use direct `std::string_view` logic or local ASCII helpers.
- `endsWith(std::string_view, std::string_view, bool)`: replace with STL call site. Reason: no core callers remain.
- `indexOf(std::string_view, std::string_view, size_t, bool)`: replace with STL call site. Reason: no core callers remain.
- `lastIndexOf(std::string_view, std::string_view, size_t, bool)`: replace with STL call site. Reason: no core callers remain.
- Former Arduino `String` overloads for compare/prefix/suffix/search: delete. Reason: no core callers remain and the no-Arduino migration should not carry them forward.
- `replace(...)`: replace with small standard helper. Reason: shared replacement logic still exists as a convenience surface, but now returns `std::string` instead of Arduino `String`.

#### Outcome

- Core code no longer depends on `src/util/StringUtility.h` for header lookup, auth prefix parsing, or URI-pattern wildcard detection.
- `src/util/StringUtility.*` is now a reduced minimal compatibility layer built only on standard-library text types and helpers.

### `StringView` And Owning Text Views

- [x] Inventory every direct use of `src/util/StringView.h` and classify whether it should become `std::string_view`, `std::string`, `const char *`, or a local parser slice type.
- [x] Replace core call sites so `StringView` is no longer required for parsing, comparison, or URI decomposition, with URI/query users already migrated first.
- [x] Replace `OwningStringView` usage with explicit `std::string` ownership where retained ownership is actually needed.
- [x] Migrate member-style helper behavior currently hanging off `StringView` into direct STL call sites or small free functions as appropriate.
- [x] Decide whether `src/util/StringView.h` survives only as a short-lived compatibility header or is removed once migration call sites are updated.
- [x] Remove `Arduino.h` from `src/util/StringView.h` and stop treating it as a core dependency.

#### Direct Use Inventory After Audit

- `src/util/HttpUtility.*`: direct parameter and implementation dependency. Replacement: `std::string_view` overloads for borrowed standard-text inputs, with existing `const char *` and Arduino `String` entry points kept as adapters.
- `src/HttpServerAdvanced.h`: umbrella compatibility include only. Replacement: keep the header present as a short-lived alias surface for now; revisit umbrella export cleanup after handler/routing migration lands.

### Core Request And Header Models

- [x] Refactor `src/core/HttpHeader.h` constructors, named factories, and storage so header name/value ownership is standard-library-based.
- [x] Update `src/core/HttpHeaderCollection.h` and `src/core/HttpHeaderCollection.cpp` to store and manipulate standard-text header data.
- [x] Refactor `src/core/HttpRequest.h` and `src/core/HttpRequest.cpp` so request URL, version, and parser-owned text state do not rely on Arduino `String` for ownership.
- [x] Audit any helper methods on `HttpRequest` that expose `String` today and split them into core-facing standard accessors plus compatibility adapters if needed.
- [ ] Remove direct `Arduino.h` includes from core text-model headers once their replacement types are in place.

### Handler And Routing Plumbing

- [x] Refactor `src/handlers/HandlerRestrictions.h` and `src/handlers/HandlerTypes.h` so extracted route params are no longer typed as `std::vector<String>` internally.
- [x] Update `src/handlers/FormBodyHandler.*`, `src/handlers/RawBodyHandler.*`, and `src/handlers/MultipartFormDataHandler.*` to reduce `String` dependence in internal plumbing.
- [x] Decide how multipart metadata such as filename, content type, and part name should be represented internally once Arduino `String` is no longer the default ownership type.
- [x] Audit `src/routing/HandlerBuilder.h`, `src/routing/HandlerMatcher.h`, `src/routing/BasicAuthentication.h`, and `src/routing/CrossOriginRequestSharing.h` for borrowed-input APIs that should prefer `const char *` or standard text internally.

### Response And Umbrella Compatibility Review

- [ ] Audit `src/response/StringResponse.h`, `src/response/FormResponse.h`, and related helpers for Arduino-facing string overloads that can become thin adapters.
- [ ] Update `src/HttpServerAdvanced.h` aliases such as `PostBodyData` so the umbrella header stops hard-wiring Arduino string types into core-facing typedefs.
- [ ] Record any compatibility overloads that must survive until a later public API cleanup phase.

### Tests And Validation

- [x] Extend native utility tests to cover URI parsing first, then replacement string helpers and view semantics once the parsing transition is in place.
- [x] Add focused tests for case-insensitive compare, prefix/suffix search, replacement, URI query parsing, and decoded/encoded output equivalence.
- [x] Verify that the curated native source list can admit the first refactored URI/query utility files without dragging in Arduino headers, then broaden that admission as later text cleanup lands.

## Owner

TBD

## Priority

High

## References

- `src/util/StringUtility.h`
- `src/util/StringUtility.cpp`
- `src/util/StringView.h`
- `src/util/UriView.h`
- `src/util/UriView.cpp`
- `src/util/HttpUtility.h`
- `src/util/HttpUtility.cpp`
- `src/util/KeyValuePairView.h`
- `src/core/HttpHeader.h`
- `src/core/HttpHeaderCollection.h`
- `src/core/HttpHeaderCollection.cpp`
- `src/core/HttpRequest.h`
- `src/core/HttpRequest.cpp`
- `src/handlers/HandlerRestrictions.h`
- `src/handlers/HandlerTypes.h`
- `src/handlers/FormBodyHandler.h`
- `src/handlers/RawBodyHandler.h`
- `src/handlers/MultipartFormDataHandler.h`
- `src/routing/HandlerBuilder.h`
- `src/routing/HandlerMatcher.h`
- `src/routing/BasicAuthentication.h`
- `src/routing/CrossOriginRequestSharing.h`
- `src/response/StringResponse.h`
- `src/response/FormResponse.h`
- `src/HttpServerAdvanced.h`
- `test/test_native/test_utilities.cpp`
