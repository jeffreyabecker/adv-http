2026-03-21 - Copilot: kept logging example-only via `examples/ExampleRuntime.h`, isolated shared `WifiSetup` serial or delay behavior under `examples/`, and closed the remaining Phase 3 logging-boundary tasks.
2026-03-21 - Copilot: completed the first production time-seam migration, removed core `F()` literal usage from `HttpUtility.cpp`, and added native regression coverage for the clock seam and HTML encoders.
2026-03-21 - Copilot: added explicit Phase 4 backlog items to replace `IPAddress` transport exposure with `std::string_view`-based address APIs backed by safe ownership.
2026-03-21 - Copilot: landed initial Phase 3-5 seam scaffolding for clock, transport, and byte-stream contracts while leaving call-site migration open.
2026-03-21 - Copilot: created new no-Arduino migration backlog from docs/plans/no-arduino/*.
2026-03-21 - Copilot: split Phase 0 into a separate detailed backlog and marked the umbrella phase complete.

# No-Arduino Migration Backlog

## Summary

This backlog replaces the deleted de-arduinofication task list with a single implementation-oriented program backlog derived from the current planning set under `docs/plans/no-arduino/`. The goal is to move the HTTP core, parsing, routing, response streaming, and static-file paths onto platform-neutral C++17 interfaces while preserving Arduino support through thin compatibility adapters. The work is intentionally staged so the repository remains buildable during migration, native coverage can expand incrementally, and HTTPS/TLS removal plus optional-feature isolation are handled as explicit scope rather than incidental churn. The initial text migration slice is in place, and the first runtime, transport, and stream seam headers now exist in-tree, but pipeline, server, response, and request-model call sites still need to move onto them.

## Goal / Acceptance Criteria

- The core HTTP, parsing, routing, and response-streaming layers compile in non-Arduino translation units without direct `Arduino.h`, Arduino `String`, Arduino `Stream`, or Arduino networking headers.
- Arduino support remains available through thin adapter or compatibility layers so existing examples can continue to compile during the migration.
- Filesystem, transport, timing, and optional JSON support are expressed through library-owned seams that can be exercised in host-side tests.
- Built-in HTTPS/TLS server support is removed from the maintained library surface and documentation.
- The repository has repeatable build and test gates that catch regressions in the Arduino boundary.

## Detailed Backlogs

- Phase 0 baseline and scope lock: `docs/backlog/task-lists/no-arduino/002-baseline-and-scope-backlog.md`
- Phase 1 build and validation: `docs/backlog/task-lists/no-arduino/003-build-and-validation-backlog.md`
- Phase 2 text and utility: `docs/backlog/task-lists/no-arduino/004-text-and-utility-backlog.md`
- Phase 3 runtime and miscellaneous Arduino helpers: `docs/backlog/task-lists/no-arduino/005-runtime-and-misc-arduino-helpers-backlog.md`
- Phase 4 transport boundary: `docs/backlog/task-lists/no-arduino/006-transport-boundary-backlog.md`
- Phase 5 stream and response: `docs/backlog/task-lists/no-arduino/007-stream-and-response-backlog.md`
- Phase 6 filesystem and static files: `docs/backlog/task-lists/no-arduino/008-filesystem-and-static-files-backlog.md`
- Phase 7 optional features and removal scope: `docs/backlog/task-lists/no-arduino/009-optional-features-and-removal-scope-backlog.md`
- Phase 8 public API compatibility and cleanup: `docs/backlog/task-lists/no-arduino/010-public-api-compatibility-and-cleanup-backlog.md`
- Phase 9 native test consolidation and fixture coverage: `docs/backlog/task-lists/no-arduino/011-native-test-consolidation-and-fixture-coverage-backlog.md`
- Phase 10 enforcement, documentation, and exit criteria: `docs/backlog/task-lists/no-arduino/012-enforcement-documentation-and-exit-criteria-backlog.md`

## Tasks

### Phase 0: Baseline And Scope Lock

- [x] Rebuild the current no-Arduino inventory from source and confirm the active dependency categories.
- [x] Record the current repo baseline, including existing compat seams, PlatformIO config, and native test layout.
- [x] Classify each dependency class as `keep portable`, `wrap behind adapter`, `replace with core type`, or `remove`.
- [x] Lock HTTPS/TLS to cleanup-and-removal scope for the current repo state.
- [x] Choose a single-package migration strategy with internal layering for the refactor.
- [x] Track the detailed Phase 0 record in `docs/backlog/task-lists/no-arduino/002-baseline-and-scope-backlog.md`.

### Phase 1: Build And Validation Harness

- [ ] Audit the existing top-level PlatformIO configuration against the plan goals and document any missing library-only, example, or native validation targets.
- [ ] Add or adjust build targets so the library can be compiled in isolation without relying on the root sketch layout.
- [ ] Define a representative board matrix for migration validation, with RP2040/Pico W, ESP32, and ESP8266 support status made explicit.
- [ ] Add a documented command set for build validation using the current Arduino CLI and PlatformIO entrypoints.
- [ ] Add CI-ready notes or config placeholders for the final board matrix and native lane so migration phases can attach to a stable validation harness.

### Phase 2: Text And Utility Foundation

- [ ] Produce a file-by-file `String` inventory covering owned internal state, non-owning view state, Arduino compatibility surfaces, and Arduino-only adapter usage.
- [ ] Replace `util/StringUtility.*` with STL-backed text helpers and standard-library algorithms so no library-owned string-helper abstraction remains on the core path.
- [ ] Retire `util/StringView.h` in favor of STL view and ownership constructs, with compatibility shims only where a public transition requires them.
- [ ] Update URI and query parsing helpers, including `util/UriView.*` and related utilities, to use `std::string` plus `std::string_view` semantics instead of Arduino-owned text.
- [ ] Move core owned text models such as `HttpHeader`, `HttpHeaderCollection`, `HttpRequest`, and parser-owned request/header storage onto `std::string`.
- [ ] Keep Arduino-facing overloads only as compatibility shims, with a bias toward `const char *` for borrowed-input APIs.
- [ ] Remove `Arduino.h` from low-level utility and core model headers once replacement types are in place.

### Phase 3: Runtime And Miscellaneous Arduino Helpers

- [x] Introduce a library-owned clock/time seam in `src/compat/` and retarget timeout-sensitive code to it.
- [x] Replace direct core usage of Arduino runtime helpers such as `millis()` and any similar calls with compatibility wrappers.
- [x] Isolate logging so examples can still use `Serial`, but core code does not depend on Arduino logging APIs.
- [x] Replace PROGMEM/`F()` usage in core code with a portable wrapper or plain compile-time literals where the memory tradeoff is acceptable.
- [x] Move example-only hardware IO and similar sketch concerns out of any library-core code paths.

### Phase 4: Transport Boundary

- [ ] Reduce `pipeline/NetClient.h` to a platform-neutral transport contract using library-owned endpoint/value types and C++17 primitives.
- [ ] Ensure `HttpPipeline` and server layers consume transport interfaces only, not Arduino networking classes.
- [ ] Define the minimum transport address surface the core actually requires and route all address access through that seam.
- [ ] Replace `IPAddress` exposure in transport and request/server contracts with `std::string_view`-based address accessors, while keeping ports as integer values.
- [ ] Define the ownership model behind those `std::string_view` address accessors so views are backed by stable owned strings rather than temporary conversions.
- [ ] Keep Arduino transport support as thin adapter implementations and constructors rather than core dependencies.
- [ ] Document the implementer contract for any out-of-tree concrete transport implementation work and keep in-tree code interface-only where intended by the plan.

### Phase 5: Stream And Response Refactor

- [ ] Freeze the current response-stream behavior with tests before changing the stream contract.
- [ ] Introduce library-owned readable byte-source and byte-sink contracts alongside temporary bridge adapters.
- [ ] Migrate read-only stream and response-body types first, including response wrappers, concatenation, chunking, and lazy body composition.
- [ ] Review duplex memory-stream utilities separately so they do not dictate the base abstraction for the whole response stack.
- [ ] Restrict Arduino `Stream` and `Print` to adapter-only code once the read-path migration is complete.
- [ ] Verify that bounded-memory direct and chunked response behavior remains unchanged after the interface swap.

### Phase 6: Filesystem And Static Files

- [ ] Define the minimal filesystem interface required by static-file serving and file-backed response streaming.
- [ ] Retarget static-file locators and handlers onto library-owned filesystem/file abstractions.
- [ ] Provide Arduino filesystem adapters that preserve current board behavior with minimal wrapping.
- [ ] Ensure metadata-dependent behavior such as content length, directory fallback, and last-modified handling remains correct where the platform can provide it.
- [ ] Add or extend host-side adapters for filesystem-backed tests so static-file behavior can be validated without Arduino headers.

### Phase 7: Optional Features And Removal Scope

- [ ] Isolate JSON handling behind a feature seam so JSON-disabled builds remain first-class.
- [ ] Ensure JSON-enabled paths can operate on standard C++ string types rather than reintroducing Arduino-owned text.
- [ ] Remove `SecureHttpServer`, `SecureHttpServerConfig`, secure umbrella aliases, secure examples, and secure-support documentation.
- [ ] Update feature documentation so the maintained library surface matches the post-migration codebase.

### Phase 8: Public API Compatibility And Cleanup

- [ ] Preserve existing Arduino-facing convenience APIs through thin overloads or wrappers while internals migrate.
- [ ] Identify compatibility-only overloads that can later be deprecated once the new seams are proven.
- [ ] Clean up umbrella headers so core-facing includes no longer drag Arduino-specific types into non-Arduino consumers.
- [ ] Update examples incrementally to use the new adapter-aware APIs and remove assumptions that the old Arduino-coupled headers are always present.

### Phase 9: Native Test Consolidation And Fixture Coverage

- [ ] Consolidate native unit suites under a single `test/test_native/` runner with shared Unity hook routing.
- [ ] Centralize curated host-compiled production sources so suites stop owning direct `.cpp` inclusion independently.
- [ ] Add shared fixture infrastructure for HTTP parser and pipeline cases, including fixture loading, a fake client, and event/response assertions.
- [ ] Add parser fixture coverage for valid requests, malformed requests, oversized headers, oversized URIs, and body chunk delivery.
- [ ] Add pipeline transport-loop tests for chunked reads, partial writes, disconnects, and keep-alive behavior.
- [ ] Add deterministic timeout coverage after the clock seam and response seam are host-safe.

### Phase 10: Enforcement, Documentation, And Exit Criteria

- [ ] Add guardrails that fail fast if `Arduino.h` or other Arduino-only headers leak back into the platform-neutral core.
- [ ] Document the final architecture, migration constraints, and build/test commands in the docs set.
- [ ] Update example and library documentation to reflect removed HTTPS scope, adapter boundaries, and any changed include guidance.
- [ ] Define the exit checklist for declaring the no-Arduino migration complete, including build matrix, native test coverage, and dependency-boundary enforcement.

## Owner

TBD

## Priority

High

## References

- `docs/plans/no-arduino/arduino-decoupling-plan.md`
- `docs/plans/no-arduino/arduino-other-dependencies.md`
- `docs/plans/no-arduino/filesystem-interface-plan.md`
- `docs/plans/no-arduino/in-tree-transport-plan.md`
- `docs/plans/no-arduino/stream-replacement-plan.md`
- `docs/plans/no-arduino/testing-refactor-plan.md`
- `platformio.ini`