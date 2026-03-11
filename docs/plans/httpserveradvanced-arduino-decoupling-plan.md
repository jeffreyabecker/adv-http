# HttpServerAdvanced Arduino Decoupling Plan

## Objective

Break the library in the top-level `src/` layout into layered, platform-neutral components so the core HTTP pipeline and routing stack can build without Arduino headers or Arduino runtime types.

This is a large refactor. The work should be staged so that:

- existing Arduino consumers keep working while the migration is in progress
- each phase leaves the library in a buildable state
- new abstractions are proven by builds and tests before the next dependency class is removed

## Constraints

- Prefer `std::string` internally over Arduino `String`, except at Arduino-facing adapter boundaries.
- Prefer `std::string_view` or equivalent non-owning spans for read-only parsing/view types instead of owning Arduino `String` buffers in the core.
- For Arduino-facing ergonomics, prefer `const char *` inputs over Arduino `String` where ownership and mutation are not required.
- Keep the existing public Arduino-friendly API working until a deliberate compatibility break is approved.
- Avoid a flag day rewrite. Introduce seams first, then migrate call sites behind those seams.
- Do not couple the platform-neutral core to PlatformIO itself; use PlatformIO first as a better build and validation tool.

## Current State Summary

The library currently depends directly on Arduino across most layers:

- packaging is Arduino-library oriented via `library.properties`
- many core and utility headers include `Arduino.h`
- internal types use Arduino `String`, `IPAddress`, `Stream`, and Arduino-style client/server interfaces
- optional JSON support is currently tied to `ArduinoJson`, but `ArduinoJson` itself is not Arduino-specific and can work with standard C++ strings
- examples are sketch-first and Arduino-specific

The current codebase also still exposes HTTPS/TLS-specific surface area such as `SecureHttpServer`, `SecureHttpServerConfig`, and the `HttpsServer` example. That support should be treated as removal scope rather than migration scope.

The current `String` coupling is concentrated in a few important clusters:

- core request and header state such as `HttpRequest`, `HttpHeader`, `HttpHeaderCollection`, and `RequestParser`
- utility types that should become platform-neutral first, especially `StringUtility`, `StringView`, `UriView`, and query-string helpers
- routing and response APIs that currently expose `String` broadly for matcher configuration, auth helpers, form helpers, and response bodies
- some server/configuration surfaces where `String` is part of Arduino-facing configuration rather than core HTTP behavior

## String Migration Strategy

The `String` migration should be treated as its own staged track inside the broader decoupling work.

### Objectives

- move owned internal text data to `std::string` wherever Arduino APIs do not require `String`
- use non-owning string views for parsing and matching paths that do not need ownership
- preserve Arduino-friendly public entrypoints during the transition by converting only at adapter boundaries
- bias Arduino-facing convenience APIs toward `const char *` inputs instead of `String` when they only need borrowed string data
- avoid duplicating string algorithms by reusing the existing char-buffer-oriented helpers as the backend for both Arduino and standard-string call sites

### Classification Rules

When inventorying and migrating `String`, classify each usage before changing it:

- owned internal state: convert to `std::string` first
  - examples: request URL/version storage, header name/value storage, parsed query key/value data, matcher state
- non-owning parsed views: convert to `std::string_view` or a library wrapper around it
  - examples: `StringView`, `UriView` segments, parser slices into request buffers
- Arduino compatibility surfaces: keep `String` overloads, but make them thin adapters over standard-string internals
  - prefer `const char *` overloads first when the API only needs borrowed input, and keep `String` overloads only where needed for compatibility or ownership
  - examples: response factories, route-builder convenience methods, auth callbacks, Arduino-oriented umbrella typedefs
- Arduino-only integrations: leave on `String` until that adapter layer is split out
  - examples: FS/TLS configuration helpers and sketch-facing convenience APIs

`ArduinoJson` should be treated separately from Arduino-only dependencies. Despite the name, it does not require Arduino runtime types and should be evaluated as a portable dependency that can remain usable after internal migration to `std::string` and `std::string_view`.

HTTPS/TLS should be treated differently. For this library, HTTPS support is a niche embedded use case with high certificate-management cost and disproportionate complexity. The plan should assume the current HTTPS/TLS server code is removed rather than carried forward into the decoupled architecture.

### Recommended Sequence

1. Refactor string utilities before touching higher layers.
  - make the char pointer plus length algorithms in `util/StringUtility.*` the primary backend
  - add `std::string` and `std::string_view` overloads there before removing Arduino overloads
  - avoid introducing a second parallel set of bespoke text helpers
2. Replace core view types next.
  - redesign `util/StringView.h` and `OwningStringView` around standard storage rules
  - update `util/UriView.*` and query parsing helpers so URI decomposition is backed by `std::string` ownership plus view slices
3. Convert core owned text models.
  - move `HttpHeader`, `HttpHeaderCollection`, `HttpRequest`, and parser-owned text state to `std::string`
  - update pipeline callbacks such as `IPipelineHandler::onMessageBegin` and `onHeader` to accept standard-string-based values internally
4. Migrate routing and handler internals.
  - move matcher state, extracted route parameters, and content-type/method matching off Arduino `String`
  - keep sketch-facing overloads only where they materially help Arduino ergonomics, and prefer `const char *` for borrowed inputs
5. Delay compatibility-heavy public APIs until the internals are stable.
  - prefer `const char *` entrypoints in response builders and convenience helpers when no ownership is required
  - keep `String` overloads only where existing compatibility or ownership semantics still justify them
  - have those overloads convert into `std::string` and forward to the new internals

### Likely First Conversion Targets

These files are good early candidates because they are central and mostly library-owned:

- `util/StringUtility.*`
- `util/StringView.h`
- `util/HttpUtility.*`
- `util/UriView.*`
- `core/HttpHeader.h`
- `core/HttpHeaderCollection.*`
- `pipeline/RequestParser.*`
- `core/HttpRequest.h`

These files are likely better treated as compatibility layers until later phases because they are closer to user-facing Arduino ergonomics:

- `response/StringResponse.*`
- `response/FormResponse.*`
- `routing/BasicAuthentication.h`
- `routing/CrossOriginRequestSharing.h`
- `routing/HandlerBuilder.*`
- umbrella aliases in `HttpServerAdvanced.h`

Within those compatibility-oriented surfaces, the preferred end state should still be Arduino-friendly APIs that accept `const char *` for borrowed text and reserve `String` for cases that genuinely need owned Arduino text.

## Migration Phases

### Phase 1: Convert HttpServerAdvanced to PlatformIO-based builds

This is the first step.

#### Goals

- give the library its own repeatable build entrypoint outside the top-level sketch
- compile the library against a small board matrix using PlatformIO
- make the build usable as a safety net for the later refactor phases

#### Work

- add top-level `platformio.ini`
- add top-level `library.json`
- define a minimal environment matrix matching currently relevant Arduino targets
  - RP2040 / Pico or Pico W
  - ESP32
  - ESP8266, if still intended to be supported
- wire required dependencies explicitly
  - `ArduinoJson` when JSON features are enabled
  - any remaining local dependencies needed for examples/tests
- make at least one small compile target for the library itself and one example-oriented target
- document the commands used to build the library in isolation

#### Acceptance Criteria

- `HttpServerAdvanced` builds from its own directory through PlatformIO
- the build does not require the root sketch to compile
- at least one representative example builds through PlatformIO
- the PlatformIO config is suitable for later CI use

#### Notes

This phase does not remove Arduino dependencies yet. It creates the build harness needed to remove them safely.

### Phase 2: Inventory Arduino dependencies and define migration seams

#### Goals

- enumerate exactly which Arduino concepts appear in the library
- decide which ones belong in the core and which must move to adapters

#### Work

- produce a dependency inventory grouped by category
  - string and text APIs: `String`, string helpers, URI/query parsing
  - network types: `IPAddress`, client/server/peer wrappers
  - IO types: `Stream`, `Print`, response/body streaming
  - runtime/time: `millis()`, delays, timeout behavior
  - storage/FS: static file locators and Arduino FS assumptions
  - optional integrations: `ArduinoJson`, TLS/SSL, board-specific features
- distinguish portable optional dependencies from Arduino-only ones
  - `ArduinoJson` can remain a candidate core or cross-platform dependency because it does not require Arduino and supports standard-string-based usage
  - TLS/SSL should be classified for removal rather than adapter migration
  - board/FS-specific pieces still need separate adapter analysis
- identify HTTPS/TLS removal scope explicitly
  - `SecureHttpServer`
  - `SecureHttpServerConfig`
  - secure-server umbrella aliases and includes
  - HTTPS example and related docs
- identify files that should become platform-neutral first
  - `core/`
  - most of `util/`
  - large parts of `routing/`, `handlers/`, `response/`, and `pipeline/`
- identify files that should remain platform adapters
  - Arduino server/client wrappers
  - Arduino stream wrappers
  - Arduino-specific examples and convenience aliases
- produce a `String` inventory grouped by migration role
  - owned internal state that can move directly to `std::string`
  - non-owning views that should become `std::string_view`-style types
  - public Arduino compatibility surfaces that need overload-based shims
  - genuinely Arduino-only text usage that should stay in adapters
- identify Arduino-facing APIs that can be simplified from `String` parameters to `const char *`
  - route configuration
  - response helpers
  - auth and CORS convenience functions
  - other borrowed-input convenience APIs
- identify high-churn signatures that should change only after compatibility shims exist
  - `IPipelineHandler`
  - `HttpRequest`
  - `HttpHeader` and `HttpHeaderCollection`
  - routing matcher and parameter extraction types

#### Acceptance Criteria

- every Arduino dependency has a destination strategy: keep, wrap, replace, or move
- every `String` use is classified as internal ownership, internal view, compatibility boundary, or Arduino-only adapter code
- HTTPS/TLS code paths are explicitly classified as removal scope instead of future core or adapter surface
- a target package split is agreed before implementation begins

### Phase 3: Introduce a platform-neutral core layer

#### Goals

- create a stable internal layer that does not include `Arduino.h`
- move shared HTTP logic onto standard C++ types and library-owned abstractions

#### Work

- define core compatibility types in a dedicated namespace, for example:
  - direct `std::string` for owned text
  - `std::string_view` or a library wrapper backed by standard C++ string storage rules
  - `IpAddress` value type owned by this library or reused from a platform-neutral dependency
  - stream interfaces that model read/write/flush semantics without Arduino base classes
  - clock/time provider abstraction for timeout logic
- update low-level headers to stop including `Arduino.h`
  - `core/Defines.h`
  - `core/HttpHeader.h`
  - `core/HttpStatus.h`
  - `util/StringUtility.*`
  - `util/StringView.h`
  - `util/HttpUtility.*`
  - `util/UriView.*`
- rework the string stack from the bottom up
  - keep the existing char-buffer algorithms as the canonical text-processing backend
  - add standard-string overloads first, then demote Arduino `String` overloads to adapter status
  - make ownership explicit so parsing code can return views without forcing `String` allocations
- replace internal Arduino string ownership with `std::string` in core models and parser state
  - request URL/version storage
  - header names and values
  - query parameter containers
  - matcher-owned URI/content-type state
- preserve Arduino-facing APIs with conversion shims until a deliberate public API cleanup phase
- prefer `const char *` at Arduino-facing boundaries where the API only consumes borrowed text and does not need `String` ownership semantics

#### Acceptance Criteria

- core utility and HTTP model types compile in a non-Arduino translation unit
- core parsing, header, and URI types no longer require Arduino `String` for internal ownership
- the core layer has no direct dependency on `Arduino.h`

### Phase 4: Separate transport abstractions from Arduino transport adapters

#### Goals

- move networking contracts out of Arduino types
- make the HTTP pipeline depend on transport interfaces, not Arduino classes

#### Work

- redesign `pipeline/NetClient.h` so the primary interfaces are platform-neutral
- remove direct use of Arduino `IPAddress` from transport contracts
- extract a smaller transport abstraction directly into HttpServerAdvanced core or a new dedicated adapter-neutral package
- provide Arduino adapter implementations that wrap Arduino client/server/UDP objects
- keep the existing Arduino-oriented constructors as thin wrappers during transition

#### Acceptance Criteria

- `HttpPipeline` and request processing compile against abstract transport interfaces only
- Arduino transport support exists as an adapter layer, not as a core dependency

### Phase 5: Decouple streaming and response writing from Arduino IO base classes

#### Goals

- remove `Stream` and `Print` from core response and body handling
- preserve efficient streaming behavior for embedded targets

#### Work

- define minimal writer and reader interfaces for response emission and request-body consumption
- refactor response body streams and helpers to target those interfaces
- adapt Arduino `Stream` and `Print` in a dedicated compatibility layer
- ensure chunked and direct response paths still work with bounded memory use

#### Acceptance Criteria

- response and body streaming compile without Arduino IO headers in the core
- Arduino stream support remains available through adapters

### Phase 6: Split optional integrations into feature adapters

#### Goals

- keep optional features optional
- avoid forcing platform-specific dependencies into the platform-neutral core
- remove HTTPS/TLS support from the library surface instead of migrating it forward

#### Work

- isolate `ArduinoJson` support behind a feature adapter layer
- treat `ArduinoJson` as a portable optional library rather than an inherently Arduino-only dependency
- keep JSON-disabled builds as a first-class configuration
- review static file serving for file-system assumptions and move Arduino FS-specific behavior behind adapters
- remove TLS/HTTPS server code and related public API surface
  - delete `SecureHttpServer` and `SecureHttpServerConfig`
  - remove secure-server aliases and includes from umbrella headers
  - remove the HTTPS example and update docs that advertise HTTPS support
  - do not replace this with a new cross-platform TLS abstraction unless a concrete supported use case is later approved

#### Acceptance Criteria

- the core library can build without `ArduinoJson`
- if JSON support remains enabled, it can operate on standard C++ string types without reintroducing Arduino type dependencies
- the library no longer exposes built-in HTTPS/TLS server support
- optional features are enabled through adapters or feature flags, not hardwired includes

### Phase 7: Preserve and then reshape the public API

#### Goals

- keep existing Arduino users functional during migration
- expose a cleaner cross-platform API after the new core is proven

#### Work

- add compatibility typedefs, wrappers, and overloads where needed for Arduino users
- remove Arduino-only aliases from core umbrella headers once replacement APIs exist
- decide whether the end state should be:
  - one package with `core` plus `arduino` adapter headers, or
  - two packages such as `HttpServerAdvancedCore` and `HttpServerAdvancedArduino`

#### Acceptance Criteria

- existing Arduino examples can still be expressed with thin compatibility wrappers
- the long-term package layout is explicit and documented

### Phase 8: Add tests, CI, and enforcement gates

#### Goals

- prevent Arduino dependencies from creeping back into the core
- verify the core on more than one toolchain

#### Work

- add CI builds for PlatformIO Arduino targets
- add at least one native or host-toolchain compile/test target for the platform-neutral core
- add guardrails such as searches or lint checks preventing `Arduino.h` includes inside the core package
- add focused tests around parsing, routing, headers, URI handling, and response formatting

#### Acceptance Criteria

- CI proves both Arduino adapter builds and core-only builds
- regressions in dependency boundaries fail fast

## Suggested PR Breakdown

1. PlatformIO support for `HttpServerAdvanced`
2. Arduino dependency inventory and target architecture document
3. Core string and utility abstractions
4. Core transport abstraction and Arduino transport adapters
5. Core stream abstraction and Arduino stream adapters
6. Optional feature isolation: JSON, FS, and remove HTTPS/TLS support
7. Public API compatibility cleanup
8. CI and dependency-boundary enforcement

## Risks

- `String` usage is deeply embedded in public and internal APIs, so careless replacement will create wide churn.
- `NetClient.h` currently mixes transport abstraction with Arduino-specific value types, making it a high-impact file.
- response streaming and static file serving may rely on Arduino IO semantics more than the headers suggest.
- removing HTTPS/TLS is a deliberate feature reduction and will break any users depending on `SecureHttpServer` or its example/documentation path.
- examples can mask coupling because they compile only in sketch-first flows today.

## Recommended Immediate Next Step

Implement Phase 1 only:

- create top-level `platformio.ini` and `library.json`
- add one or two representative environments
- make the library compile in isolation
- stop there and evaluate the build friction before starting Phase 2
