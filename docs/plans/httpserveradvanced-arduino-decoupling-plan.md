# HttpServerAdvanced Arduino Decoupling Plan

## Objective

Break the library in the top-level `src/` layout into layered, platform-neutral components so the core HTTP pipeline and routing stack can build without Arduino headers or Arduino runtime types.

This is a large refactor. The work should be staged so that:

- existing Arduino consumers keep working while the migration is in progress
- each phase leaves the library in a buildable state
- new abstractions are proven by builds and tests before the next dependency class is removed

## Constraints

- Prefer `std::string` internally over Arduino `String`, except at Arduino-facing adapter boundaries.
- Keep the existing public Arduino-friendly API working until a deliberate compatibility break is approved.
- Avoid a flag day rewrite. Introduce seams first, then migrate call sites behind those seams.
- Do not couple the platform-neutral core to PlatformIO itself; use PlatformIO first as a better build and validation tool.

## Current State Summary

The library currently depends directly on Arduino across most layers:

- packaging is Arduino-library oriented via `library.properties`
- many core and utility headers include `Arduino.h`
- internal types use Arduino `String`, `IPAddress`, `Stream`, and Arduino-style client/server interfaces
- optional JSON support is tied to `ArduinoJson`
- examples are sketch-first and Arduino-specific

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
- identify files that should become platform-neutral first
  - `core/`
  - most of `util/`
  - large parts of `routing/`, `handlers/`, `response/`, and `pipeline/`
- identify files that should remain platform adapters
  - Arduino server/client wrappers
  - Arduino stream wrappers
  - Arduino-specific examples and convenience aliases

#### Acceptance Criteria

- every Arduino dependency has a destination strategy: keep, wrap, replace, or move
- a target package split is agreed before implementation begins

### Phase 3: Introduce a platform-neutral core layer

#### Goals

- create a stable internal layer that does not include `Arduino.h`
- move shared HTTP logic onto standard C++ types and library-owned abstractions

#### Work

- define core compatibility types in a dedicated namespace, for example:
  - `Text` or direct `std::string`
  - `StringView` backed by standard C++ string storage rules
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
- replace internal Arduino string ownership with `std::string` where possible

#### Acceptance Criteria

- core utility and HTTP model types compile in a non-Arduino translation unit
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

#### Work

- isolate `ArduinoJson` support behind a feature adapter layer
- keep JSON-disabled builds as a first-class configuration
- review static file serving for file-system assumptions and move Arduino FS-specific behavior behind adapters
- review TLS/HTTPS server pieces and identify board-specific or framework-specific code paths

#### Acceptance Criteria

- the core library can build without `ArduinoJson`
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
6. Optional feature isolation: JSON, FS, TLS
7. Public API compatibility cleanup
8. CI and dependency-boundary enforcement

## Risks

- `String` usage is deeply embedded in public and internal APIs, so careless replacement will create wide churn.
- `NetClient.h` currently mixes transport abstraction with Arduino-specific value types, making it a high-impact file.
- response streaming and static file serving may rely on Arduino IO semantics more than the headers suggest.
- examples can mask coupling because they compile only in sketch-first flows today.

## Recommended Immediate Next Step

Implement Phase 1 only:

- create top-level `platformio.ini` and `library.json`
- add one or two representative environments
- make the library compile in isolation
- stop there and evaluate the build friction before starting Phase 2
