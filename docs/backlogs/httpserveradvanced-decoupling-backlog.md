# HttpServerAdvanced Decoupling Backlog

## Purpose

This backlog turns the Arduino decoupling plan into implementation-sized tasks.

Ordering is intentional:

- compatibility seams come first
- internal migrations happen only after the seams compile
- Arduino adapters stay working while core types are moved behind those seams

## Status Model

- `todo`: not started
- `in-progress`: actively being implemented
- `blocked`: waiting on a prerequisite or design decision
- `done`: implemented and validated

## Workstream 1: Compatibility Type Foundations

This workstream establishes the alias-or-shim layer before any broad call-site migration.

### COMPAT-001: Create compatibility namespace and header layout

- Status: `in-progress`
- Goal: introduce a single place for library-owned compatibility types so later migrations do not scatter `#ifdef ARDUINO` throughout the codebase.
- Scope:
  - create a dedicated header area for compatibility types, likely under `src/compat/`
  - define naming conventions for aliased versus shimmed types
  - decide whether public names live directly in `HttpServerAdvanced` or under `HttpServerAdvanced::Compat`
  - document which headers are allowed to include Arduino headers and which are not
- Deliverables:
  - initial directory and header structure
  - one small umbrella header for compatibility types
  - updated include guidance in the decoupling plan if naming changes during implementation
- Acceptance criteria:
  - a non-Arduino translation unit can include the compatibility umbrella header
  - Arduino-specific includes are confined to compatibility headers or adapters
  - the namespace and include pattern are stable enough for follow-on tasks

Current implementation direction:

- canonical compatibility types live under `HttpServerAdvanced::Compat`
- `src/compat/Compat.h` is the umbrella header for compatibility seams
- future tasks should add Arduino alias paths or non-Arduino shim definitions in leaf headers under `src/compat/`
- core headers should include compatibility leaf headers instead of `Arduino.h` as they are migrated

Initial scaffold landed:

- added `src/compat/Compat.h`
- added placeholder leaf headers for stream, IP address, filesystem, and clock seams
- exposed the compatibility umbrella from the top-level public header so downstream code has a stable include path while the seams are filled in

### COMPAT-002: Introduce compatibility stream type

- Status: `in-progress`
- Goal: preserve existing stream semantics while removing direct `Arduino.h` dependency from core-facing headers.
- Scope:
  - add a compatibility `Stream` type alias under `ARDUINO`
  - add a minimal pure-abstract non-Arduino stream interface otherwise
  - preserve the methods already used by the library
  - avoid adding separate `Print` abstraction unless direct usage is discovered during implementation
- Required surface:
  - `available()`
  - `read()`
  - `peek()`
  - `write(uint8_t)`
  - `flush()`
  - `availableForWrite()` with default no-op behavior for non-Arduino shims because the in-tree memory-stream hierarchy currently overrides it
- Primary files likely touched:
  - new compatibility stream header
  - `src/pipeline/IPipelineHandler.h`
  - `src/pipeline/HttpPipeline.h`
  - `src/response/HttpResponseBodyStream.h`
  - stream helper headers that currently reference Arduino `Stream`
- Dependencies:
  - `COMPAT-001`
- Acceptance criteria:
  - core headers stop naming Arduino `Stream` directly and use the compatibility type instead
  - Arduino builds still resolve to the real Arduino `Stream`
  - native compilation can parse the updated headers without Arduino IO base classes

Current implementation direction:

- `HttpServerAdvanced::Stream` remains the public spelling for now, but resolves through `HttpServerAdvanced::Compat::Stream`
- under `ARDUINO`, the compatibility type aliases the real Arduino `Stream`
- outside Arduino, the compatibility type is a minimal abstract interface with the methods the library currently relies on
- header migration should prefer explicit `compat/Stream.h` includes instead of relying on transitive Arduino includes

### COMPAT-003: Introduce compatibility IP address type

- Status: `todo`
- Goal: replace direct `IPAddress` exposure in core transport contracts with a compatibility value type.
- Scope:
  - add an `IpAddress` compatibility type alias under `ARDUINO`
  - add a small library-owned non-Arduino value type otherwise
  - preserve the endpoint-oriented semantics the library uses today
  - provide an any-address constant or factory to replace direct reliance on `IPADDR_ANY`
- Required surface:
  - value construction and copy/move semantics
  - pass-by-value and pass-by-const-reference use in transport interfaces
  - any-address default for server binding
- Primary files likely touched:
  - new compatibility IP address header
  - `src/pipeline/NetClient.h`
  - `src/core/HttpRequest.h`
  - `src/server/StandardHttpServer.h`
  - any transport adapter implementations that pass through Arduino `IPAddress`
- Dependencies:
  - `COMPAT-001`
- Acceptance criteria:
  - transport-facing headers no longer require Arduino `IPAddress`
  - Arduino builds still pass real `IPAddress` values through the alias path
  - non-Arduino code can represent endpoint addresses without Arduino networking headers

### COMPAT-004: Introduce compatibility file-system and file-handle types

- Status: `todo`
- Goal: isolate Arduino FS usage behind a narrow alias-or-shim layer so static-file serving can migrate without carrying Arduino FS headers into the core.
- Scope:
  - add compatibility `FS` and `File` types aliased under `ARDUINO`
  - add a minimal non-Arduino filesystem interface and file-handle abstraction otherwise
  - keep the exposed surface limited to the operations the current static-file code consumes
  - ensure file-backed response streaming can keep working through the stream compatibility layer
- Required surface:
  - `FS::open(path, mode)`
  - default-constructed invalid `File`
  - file validity checks
  - `isDirectory()`
  - `close()`
  - read-oriented stream behavior through the compatibility stream path
  - metadata access only where existing code needs it, such as `size()`, `fullName()`, and `getLastWrite()`
- Primary files likely touched:
  - new compatibility filesystem headers
  - `src/staticfiles/FileLocator.h`
  - `src/staticfiles/DefaultFileLocator.h`
  - `src/staticfiles/DefaultFileLocator.cpp`
  - `src/staticfiles/AggregateFileLocator.h`
  - `src/staticfiles/AggregateFileLocator.cpp`
  - static-file handlers that currently name Arduino `FS` or `File`
- Dependencies:
  - `COMPAT-001`
  - `COMPAT-002`
- Acceptance criteria:
  - static-file headers no longer expose Arduino `fs::FS` or `fs::File` directly
  - Arduino builds still use the native filesystem types through aliases
  - non-Arduino compilation has a viable file and filesystem contract for later tests and adapters

### COMPAT-005: Introduce timing abstraction without Arduino runtime coupling

- Status: `todo`
- Goal: replace direct clock-source coupling in request timeout handling with a compatibility seam.
- Scope:
  - introduce a small clock or tick-source abstraction for timeout calculations
  - keep duration values as plain integer milliseconds unless a broader chrono migration is explicitly approved
  - route `HttpPipeline` timing through the abstraction instead of calling Arduino runtime functions directly
- Primary files likely touched:
  - new compatibility clock header
  - `src/pipeline/HttpPipeline.h`
  - `src/pipeline/HttpPipeline.cpp`
  - any server setup code that provides timing dependencies
- Dependencies:
  - `COMPAT-001`
- Acceptance criteria:
  - core timeout logic can compile without direct Arduino runtime calls
  - Arduino builds still source timing from Arduino runtime through the adapter path
  - native tests can provide a deterministic clock implementation

### COMPAT-006: Retarget core headers to compatibility includes

- Status: `todo`
- Goal: make core and transport headers consume the new compatibility layer instead of Arduino headers.
- Scope:
  - replace direct Arduino includes in the highest-leverage headers first
  - include only the compatibility headers needed for text, stream, IP, filesystem, and timing seams
  - keep implementation files on Arduino includes temporarily if necessary while call sites are migrated incrementally
- Primary files likely touched:
  - `src/pipeline/IPipelineHandler.h`
  - `src/pipeline/NetClient.h`
  - `src/pipeline/HttpPipeline.h`
  - `src/core/HttpRequest.h`
  - static-file headers
  - any umbrella headers that currently leak Arduino types directly
- Dependencies:
  - `COMPAT-002`
  - `COMPAT-003`
  - `COMPAT-004`
  - `COMPAT-005`
- Acceptance criteria:
  - the target headers no longer include `Arduino.h`
  - the compatibility layer becomes the only type source for stream, IP, filesystem, and clock contracts in those headers
  - a native compile target can parse the refactored header set

### COMPAT-007: Add compile-only validation for compatibility headers

- Status: `todo`
- Goal: catch header regressions before deeper migrations begin.
- Scope:
  - add a native compile-only test or smoke-test translation unit that includes the new compatibility headers and representative core headers
  - verify Arduino environments still compile the alias path
  - add lightweight guardrails to keep new Arduino includes out of compatibility consumers
- Dependencies:
  - `COMPAT-006`
- Acceptance criteria:
  - native test or compile target passes with the new compatibility headers
  - at least one Arduino environment still compiles after the compatibility seam lands
  - the repo has a repeatable check for accidental Arduino-header regressions in core-facing headers

## Workstream 2: Internal Text Migration

This workstream should begin only after Workstream 1 has landed.

### TEXT-001: Rework string utility backend

- Status: `todo`
- Goal: make char-buffer logic the canonical backend and add standard-string overloads.

### TEXT-002: Replace `StringView` with `std::string_view`-oriented implementation

- Status: `todo`
- Goal: remove Arduino string ownership assumptions from view types.

### TEXT-003: Migrate `UriView` and query parsing to standard ownership plus views

- Status: `todo`
- Goal: make URI decomposition independent of Arduino strings.

### TEXT-004: Migrate request and header models to `std::string`

- Status: `todo`
- Goal: move owned request/header text state off Arduino `String`.

### TEXT-005: Retarget parser callbacks to standard-string internals

- Status: `todo`
- Goal: make `IPipelineHandler` and parser-owned text flow through the new core text model.

## Workstream 3: Routing and Response Compatibility Cleanup

### API-001: Move routing internals off Arduino `String`

- Status: `todo`
- Goal: migrate matcher state, route params, and content-type matching to standard text types.

### API-002: Add `const char *`-first convenience overloads

- Status: `todo`
- Goal: preserve Arduino ergonomics without keeping `String` as the preferred borrowed-input API.

### API-003: Demote `String` overloads to compatibility wrappers

- Status: `todo`
- Goal: keep source compatibility while forwarding into standard-string internals.

## Workstream 4: Feature Adapters and Enforcement

### FEAT-001: Isolate `ArduinoJson` support as an optional adapter

- Status: `todo`
- Goal: keep JSON optional without reintroducing Arduino runtime coupling.

### FEAT-002: Retarget static-file serving fully onto compatibility filesystem types

- Status: `todo`
- Goal: complete the move of file serving into adapter-backed filesystem contracts.

### FEAT-003: Add CI and boundary checks

- Status: `todo`
- Goal: prevent Arduino includes and Arduino-only types from creeping back into the core.

## Suggested Execution Order

1. `COMPAT-001` through `COMPAT-003`
2. `COMPAT-005` and `COMPAT-004`
3. `COMPAT-006` and `COMPAT-007`
4. `TEXT-001` through `TEXT-005`
5. `API-001` through `API-003`
6. `FEAT-001` through `FEAT-003`

## Notes

- `Print` remains intentionally absent from the backlog as a first-class compatibility type because current evidence suggests the library depends on it only indirectly through `Stream`.
- HTTPS/TLS removal is already complete and is therefore not included as a remaining implementation backlog item.