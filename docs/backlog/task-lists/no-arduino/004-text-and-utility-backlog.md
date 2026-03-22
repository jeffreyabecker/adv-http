2026-03-21 - Copilot: created detailed Phase 2 text and utility backlog.
2026-03-21 - Copilot: reordered Phase 2 so URI and query parsing lands before `StringUtility` and `StringView` cleanup to reduce churn.
2026-03-21 - Copilot: implemented the first URI/query parsing migration slice and validated it through the native PlatformIO test lane.
2026-03-21 - Copilot: admitted `HttpHeaderCollection` into the native lane and added standard-text header lookup accessors while leaving header/request ownership migration open.
2026-03-21 - Copilot: moved `HttpHeader` and `HttpRequest` internal ownership to `std::string`, widened the native portable source list to cover the new slice, and fixed host-safe response-path regressions uncovered by that validation.

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

- [ ] Build a file-by-file inventory of `String` usage across `src/util`, `src/core`, `src/handlers`, `src/routing`, `src/response`, and `src/HttpServerAdvanced.h`.
- [ ] Classify each `String` use as owned internal state, non-owning parsed view, compatibility overload, or Arduino-only boundary.
- [ ] Record the preferred replacement type for each classification: `std::string`, `std::string_view`, `const char *`, or deferred adapter-only retention.
- [ ] Freeze the conversion order so URI and query parsing lands first, then the remaining lower-level text utilities, then higher-level routing and response helpers.

### URI And Query Parsing First

- [x] Refactor `src/util/UriView.h` and `src/util/UriView.cpp` so URI ownership is standard-library-based before broader `StringUtility` and `StringView` cleanup begins.
- [x] Replace `KeyValuePairView<String, String>` query storage with a standard-text equivalent or a compatibility typedef that no longer requires Arduino ownership in the core.
- [x] Refactor `src/util/HttpUtility.h` and `src/util/HttpUtility.cpp` query parsing and URI encoding helpers to use standard-string-based results internally and to stop depending on `StringUtility` or `StringView` as core infrastructure.
- [x] Replace `String`-returning utility implementations in URI and query paths with standard-string internal generation plus compatibility adapters only where public transitions still require them.
- [x] Keep any Arduino-facing borrowed-input overloads on `const char *` before reintroducing `String` convenience overloads.
- [x] Verify that the URI and query transition leaves the phase in a materially lower-churn state before broad `StringUtility` and `StringView` cleanup continues.

### `StringUtility` Refactor

- [ ] Inventory every function in `src/util/StringUtility.h` and classify it as `replace with STL call site`, `replace with small standard helper`, or `delete`.
- [ ] Replace uses of `StringUtility` in core code with direct STL equivalents where those already exist, such as comparisons, prefix/suffix checks, search, substring handling, and character classification, after the URI/query transition has established the target text model.
- [ ] For behaviors not covered cleanly by direct STL calls, introduce narrowly scoped standard-library-backed helpers instead of preserving the current `StringUtility` surface wholesale.
- [ ] Remove Arduino-`String`-centric overloads from the core path rather than carrying them forward as the canonical API.
- [ ] Decide whether `src/util/StringUtility.*` becomes a temporary compatibility shim, is reduced to a minimal adapter-only layer, or is deleted entirely once call sites are migrated.
- [ ] Add or extend native tests in `test/test_native/test_utilities.cpp` for the STL-equivalent behaviors that replace current `StringUtility` call sites.

### `StringView` And Owning Text Views

- [ ] Inventory every direct use of `src/util/StringView.h` and classify whether it should become `std::string_view`, `std::string`, `const char *`, or a local parser slice type.
- [ ] Replace core call sites so `StringView` is no longer required for parsing, comparison, or URI decomposition, with URI/query users already migrated first.
- [ ] Replace `OwningStringView` usage with explicit `std::string` ownership where retained ownership is actually needed.
- [ ] Migrate member-style helper behavior currently hanging off `StringView` into direct STL call sites or small free functions as appropriate.
- [ ] Decide whether `src/util/StringView.h` survives only as a short-lived compatibility header or is removed once migration call sites are updated.
- [ ] Remove `Arduino.h` from `src/util/StringView.h` and stop treating it as a core dependency.

### Core Request And Header Models

- [x] Refactor `src/core/HttpHeader.h` constructors, named factories, and storage so header name/value ownership is standard-library-based.
- [x] Update `src/core/HttpHeaderCollection.h` and `src/core/HttpHeaderCollection.cpp` to store and manipulate standard-text header data.
- [x] Refactor `src/core/HttpRequest.h` and `src/core/HttpRequest.cpp` so request URL, version, and parser-owned text state do not rely on Arduino `String` for ownership.
- [x] Audit any helper methods on `HttpRequest` that expose `String` today and split them into core-facing standard accessors plus compatibility adapters if needed.
- [ ] Remove direct `Arduino.h` includes from core text-model headers once their replacement types are in place.

### Handler And Routing Plumbing

- [ ] Refactor `src/handlers/HandlerRestrictions.h` and `src/handlers/HandlerTypes.h` so extracted route params are no longer typed as `std::vector<String>` internally.
- [ ] Update `src/handlers/FormBodyHandler.*`, `src/handlers/RawBodyHandler.*`, and `src/handlers/MultipartFormDataHandler.*` to reduce `String` dependence in internal plumbing.
- [ ] Decide how multipart metadata such as filename, content type, and part name should be represented internally once Arduino `String` is no longer the default ownership type.
- [ ] Audit `src/routing/HandlerBuilder.h`, `src/routing/HandlerMatcher.h`, `src/routing/BasicAuthentication.h`, and `src/routing/CrossOriginRequestSharing.h` for borrowed-input APIs that should prefer `const char *` or standard text internally.

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
