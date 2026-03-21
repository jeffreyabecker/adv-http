2026-03-21 - Copilot: completed source-backed Phase 0 baseline, dependency classification, and scope lock for no-Arduino migration.

# No-Arduino Phase 0 Baseline And Scope Backlog

## Summary

This backlog captures the Phase 0 work needed before implementation-heavy no-Arduino refactors begin. It converts the broad planning notes into a source-backed baseline of what already exists in the repository, which Arduino dependencies are still embedded in core code, which seams are partially in place, what secure-scope cleanup remains, and which architectural direction should govern the next phases. The result is a tighter starting point for Phase 1 and Phase 2 so later backlog files can target concrete files and interfaces rather than high-level categories.

## Goal / Acceptance Criteria

- The repository has a verified, source-backed inventory of Arduino-specific dependencies and partial compatibility seams.
- Every dependency class is assigned one migration strategy: `keep portable`, `wrap behind adapter`, `replace with core type`, or `remove`.
- HTTPS/TLS removal scope is explicitly locked to examples and documentation unless new source code is introduced later.
- The migration has a documented architectural direction for package/layout strategy so later phases do not fork around incompatible assumptions.

## Current Validated Baseline

- `src/compat/` already exists and contains `Availability.h`, `Clock.h`, `Compat.h`, `FileSystem.h`, `IpAddress.h`, `PosixFileAdapter.h`, `Span.h`, `span.hpp`, and `Stream.h`.
- `src/compat/Compat.h` establishes the intended rule that core headers should depend on compatibility headers instead of directly including Arduino headers.
- `src/compat/Stream.h` already aliases Arduino `Stream` under `ARDUINO` and provides a minimal non-Arduino abstract fallback, but large parts of the core still include `Arduino.h` directly and still traffic in `std::unique_ptr<Stream>`.
- `src/compat/FileSystem.h` already aliases `fs::File` and `fs::FS` under `ARDUINO` and provides a host fallback plus `PosixFileAdapter.h`, but the fallback still models `File` as a `Stream` subclass, which couples filesystem migration to the current stream abstraction.
- `src/compat/IpAddress.h` already provides a non-Arduino `IpAddress` value type and Arduino alias, but the transport contract still exposes `IPAddress` directly.
- `src/compat/Clock.h` is only a forward declaration, so the time seam exists only as intent, not as an implemented abstraction.
- `platformio.ini` already pulls in `platformio/common.ini`, `platformio/rp2040.ini`, `platformio/esp32.ini`, `platformio/esp8266.ini`, and `platformio/native.ini`.
- `platformio/native.ini` already has a consolidated native test lane with `test_build_src = no`.
- `test/test_native/test_main.cpp` already provides a shared consolidated runner and hook routing.
- `test/test_native/native_portable_sources.cpp` currently compiles only `Streams.cpp`, `Base64Stream.cpp`, and `StringUtility.cpp`, so the native lane is still intentionally narrow.
- `platformio/common.ini` globally enables ArduinoJson through `-DHTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON=1`, so JSON is not yet isolated as an optional feature seam at the build-config level.

## Dependency Classification

### Replace With Core Type

- Internal text ownership and parsing state currently using Arduino `String` in `src/core/*`, `src/util/*`, `src/handlers/*`, and response/routing helpers should move to `std::string`.
- Non-owning parsed text views currently represented through `StringView`, `UriView`, and `KeyValuePairView<String, String>` should be rewritten around STL view or ownership constructs, with `StringView` retired rather than preserved as a long-term library abstraction.
- The bespoke helper surface in `src/util/StringUtility.*` should be replaced on the core path by standard-library string or character facilities rather than carried forward as a permanent utility layer.
- Core request/header/parser data structures should stop storing Arduino text directly.

### Wrap Behind Adapter

- `Stream` should remain a compatibility seam during migration but be pushed to adapter boundaries once byte-source and byte-sink contracts exist.
- `Print` should remain adapter-only and should not become a new core abstraction.
- `IPAddress` should be replaced in public transport/core contracts by a library-owned endpoint or compatibility value type.
- `FS` and `File` should remain compatibility seams during migration, with static-file serving eventually depending on a narrower filesystem contract.
- Timing/clock access should move behind a concrete compatibility seam and stop calling `millis()` directly in core code.
- Example logging and sketch runtime concerns should remain Arduino-facing adapters rather than core library behavior.

### Keep Portable

- `llhttp` is already a portable vendored parser dependency and remains in-scope for the core.
- `ArduinoJson` is not inherently Arduino-only and can remain as an optional portable dependency if its public and internal integration stops depending on Arduino-owned strings and always-on build flags.
- Host-side filesystem testing through `PosixFileAdapter.h` remains a valid portable seam.

### Remove

- Built-in HTTPS/TLS server support is removal scope, not migration scope.
- Any remaining secure-example or secure-documentation references should be treated as cleanup work, not as future feature obligations.

## Locked Scope Decisions

### Decision 1: Migration Package Shape

- Use one repository package and one include surface during migration.
- Keep the split architectural concept internal to the repo as `core` plus `compat` or adapter layering rather than publishing a separate core package during the refactor.
- Defer any two-package split until the core boundary is stable and examples can be migrated without churn.

Reasoning:

- The current repo is still Arduino-library-oriented through `library.properties` and `HttpServerAdvanced.h`.
- The migration already has significant internal churn across text, stream, transport, and filesystem seams.
- Introducing a packaging split before those seams stabilize would create extra user-facing churn without reducing the technical risk of the current refactor.

### Decision 2: HTTPS/TLS Removal Scope

- No new secure transport abstraction will be designed as part of the no-Arduino work.
- Secure support cleanup is limited to example, include-surface, and documentation scope unless secure source code reappears later.
- The current `examples/3_Security/HttpsServer/` directory should be treated as stale scope that must either be removed or rewritten to match the documented post-migration surface.

Reasoning:

- Source scans found no active `SecureHttpServer` or `SecureHttpServerConfig` symbols in `src/`.
- Secure-scope residue is currently documentation and example debt, not a core implementation dependency.

### Decision 3: JSON Strategy

- Keep JSON support as an optional portable feature.
- Do not treat `ArduinoJson` as removal scope.
- Remove the assumption that JSON support is globally on by default at all build entrypoints.

Reasoning:

- `JsonBodyHandler` and `JsonResponse` still include `ArduinoJson.h` directly.
- `platformio/common.ini` globally enables ArduinoJson today, which hides the optional-feature boundary the later phases need.

## Tasks

### Inventory And Baseline

- [x] Inventory the existing compatibility seam files under `src/compat/`.
- [x] Verify that the clock seam is incomplete and currently only forward declared.
- [x] Verify that the filesystem seam exists and that a POSIX adapter already exists for host builds.
- [x] Verify that the stream seam exists but still mirrors Arduino `Stream` semantics directly.
- [x] Verify that the transport seam already exposes `IClient` and `IServer` but still mixes in Arduino-oriented wrappers and `IPAddress`-typed endpoints.
- [x] Verify the current PlatformIO environment baseline for RP2040, ESP32, ESP8266, and native.
- [x] Verify that the native lane is already consolidated at the runner level.
- [x] Verify that the curated native production-source list is still limited and therefore does not yet exercise parser, pipeline, response, or routing layers.

### Source-Backed Dependency Scan

- [x] Scan `src/**` for direct `Arduino.h` inclusion and confirm that core, util, handler, routing, response, static-file, and server headers still include it broadly.
- [x] Scan `src/**` for deep Arduino `String` usage and confirm that the coupling is embedded in utility helpers, URI parsing, request/response models, route parameter plumbing, and body handlers.
- [x] Scan `src/**` for `Stream` and `Print` usage and confirm that response streaming, file serving, iterator composition, and pipeline callbacks still revolve around `std::unique_ptr<Stream>`.
- [x] Scan `src/**` for filesystem usage and confirm that static-file serving still depends on `FS`, `File`, `fullName()`, `size()`, `getLastWrite()`, directory checks, and stream-style reads.
- [x] Scan `src/**` for runtime helper usage and confirm direct `millis()` calls remain in `HttpPipeline.cpp` and `F()` macro usage remains in `HttpUtility.cpp`.
- [x] Scan `src/**` for JSON coupling and confirm that `JsonBodyHandler` and `JsonResponse` still include `ArduinoJson.h` directly.
- [x] Scan `src/**` for secure server symbols and confirm that active HTTPS/TLS server code is not present in the current `src/` tree.

### Classification And Scope Lock

- [x] Classify `String`-backed internal ownership as replacement scope.
- [x] Classify compatibility values such as stream, IP, filesystem, and timing as adapter scope.
- [x] Classify `llhttp` and host-side POSIX support as portable retained scope.
- [x] Classify `ArduinoJson` as portable optional scope rather than removal scope.
- [x] Classify HTTPS/TLS support as cleanup-and-removal scope, primarily in docs and examples.

### Architecture Decision

- [x] Choose a single-package migration strategy with internal layering instead of an early core-package split.
- [x] Record the rationale so later phase backlogs can assume stable packaging expectations.

### Follow-On Backlog Preparation

- [x] Identify that Phase 1 should focus on build/config cleanup and optional-feature gating rather than creating a new test harness from scratch.
- [x] Identify that Phase 2 should start by replacing `util/StringUtility.*` and retiring `util/StringView.h`, then continue through `util/UriView.*`, `core/HttpHeader*`, `core/HttpRequest*`, and parser-owned text state.
- [x] Identify that stream and filesystem work must be coordinated because the current fallback `File` type inherits from `Stream`.
- [x] Identify that transport cleanup should preserve the existing `IClient`/`IServer` seam while removing the generic Arduino wrapper behavior from the core-facing contract.

## Owner

TBD

## Priority

High

## References

- `src/compat/Compat.h`
- `src/compat/Clock.h`
- `src/compat/Stream.h`
- `src/compat/FileSystem.h`
- `src/compat/IpAddress.h`
- `src/compat/PosixFileAdapter.h`
- `src/pipeline/NetClient.h`
- `src/pipeline/HttpPipeline.cpp`
- `src/util/StringUtility.h`
- `src/util/StringView.h`
- `src/util/UriView.h`
- `src/util/HttpUtility.cpp`
- `src/handlers/JsonBodyHandler.h`
- `src/response/JsonResponse.h`
- `platformio.ini`
- `platformio/common.ini`
- `platformio/native.ini`
- `test/test_native/test_main.cpp`
- `test/test_native/native_portable_sources.cpp`
- `examples/3_Security/HttpsServer/`
- `docs/httpserveradvanced/LIBRARY_DOCUMENTATION.md`
- `docs/httpserveradvanced/LIBRARY_COMPARISON.md`
