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

## Progress Snapshot

### Completed so far

- compatibility namespace and header scaffold has landed under `src/compat/`
- the compatibility umbrella is exposed from the public top-level header
- `COMPAT-001` is complete
- the initial stream compatibility shim is in place
- the initial IP address compatibility shim is in place
- the initial filesystem and file-handle compatibility shim is in place
- unused peer transport wrappers were removed from `NetClient.h`

### In progress

- `COMPAT-002`
- `COMPAT-003`
- `COMPAT-004`

### Not started

- `COMPAT-005`
- `COMPAT-006`
- `COMPAT-007`
- all tasks in Workstreams 2 through 4

## Workstream 1: Compatibility Type Foundations

This workstream establishes the alias-or-shim layer before any broad call-site migration.

### COMPAT-001: Create compatibility namespace and header layout

- Status: `done`
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

Completed so far:

- canonical compatibility types live under `HttpServerAdvanced::Compat`
- `src/compat/Compat.h` is the umbrella header for compatibility seams
- added placeholder leaf headers for stream, IP address, filesystem, and clock seams
- exposed the compatibility umbrella from the top-level public header so downstream code has a stable include path while the seams are filled in
- central public and compatibility entry points now use that scaffold consistently enough for follow-on seam work

Follow-on work moved to other tasks:

- broader include-boundary cleanup belongs to `COMPAT-006`
- compile-oriented validation belongs to `COMPAT-007`

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

Completed so far:

- `HttpServerAdvanced::Stream` remains the public spelling for now, but resolves through `HttpServerAdvanced::Compat::Stream`
- under `ARDUINO`, the compatibility type aliases the real Arduino `Stream`
- outside Arduino, the compatibility type is a minimal abstract interface with the methods the library currently relies on
- several central headers already include `compat/Stream.h` explicitly instead of relying on transitive Arduino includes

Remaining work:

- retarget the remaining stream-centric headers and implementations to the compatibility include path
  - `src/streams/Base64Stream.h`
  - `src/streams/UriStream.h`
  - `src/staticfiles/StaticFileHandler.h` and related implementation files that still lean on Arduino stream declarations transitively
- verify whether any other Arduino `Stream` methods are still required by real call sites beyond the current compatibility surface
- run compile validation on both Arduino and native targets once the remaining stream-heavy files are migrated
- close this item only after the remaining stream-heavy files compile cleanly through the compatibility stream path

### COMPAT-003: Introduce compatibility IP address type

- Status: `in-progress`
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

Completed so far:

- `HttpServerAdvanced::IPAddress` remains the public spelling for now, but resolves through `HttpServerAdvanced::Compat::IpAddress`
- under `ARDUINO`, the compatibility type aliases the real Arduino `IPAddress`
- outside Arduino, the compatibility type is a small IPv4 value type with only the construction and value semantics the current transport contracts require
- bind-address defaults should use a library-owned `Compat::IpAddressAny` constant instead of directly naming Arduino's `IPADDR_ANY`

Remaining work:

- confirm the non-Arduino `IpAddress` surface is sufficient for all current transport and request code paths
- remove any remaining transport headers or adapters that still rely on Arduino IP declarations transitively
  - especially around `HttpServerBase` and any server-side headers still depending on Arduino IP declarations through other includes
- validate the Arduino alias path with a real compile, not just header checks
- close this item once transport-facing headers no longer need Arduino IP declarations outside the compatibility layer and the alias path compiles successfully

### COMPAT-004: Introduce compatibility file-system and file-handle types

- Status: `in-progress`
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

Completed so far:

- `HttpServerAdvanced::FS` and `HttpServerAdvanced::File` remain the public spellings for now, but resolve through `HttpServerAdvanced::Compat`
- under `ARDUINO`, the compatibility types alias `fs::FS` and `fs::File`
- outside Arduino, `File` is a minimal stream-compatible abstract file-handle type and `FS` is a minimal file-opening interface
- static-file headers should include `compat/FileSystem.h` explicitly instead of relying on Arduino filesystem headers for type declarations
- `StaticFilesBuilder` now uses the compatibility filesystem include path instead of exposing Arduino `FS.h` directly

Remaining work:

- finish retargeting static-file implementation files and file-backed stream wrappers onto the compatibility filesystem types
  - `src/staticfiles/DefaultFileLocator.cpp`
  - `src/staticfiles/AggregateFileLocator.cpp`
  - `src/staticfiles/StaticFileHandler.cpp`
  - `src/staticfiles/StaticFilesBuilder.cpp`
- decide how invalid-file state and `open()` failure should be represented in the non-Arduino shim path
- reconcile the current non-Arduino `File` abstraction with actual call-site behavior
  - value-like default construction
  - truthiness checks
  - returned-by-value file handles from lookup and open flows
- validate that the current abstract `File` surface covers all metadata and stream behavior used by the static-file code
- close this item only after the file-handle design matches real static-file usage and the static-file implementation files are fully retargeted

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

1. finish `COMPAT-003`
2. finish `COMPAT-002`
3. finish `COMPAT-004`
4. implement `COMPAT-005`
5. complete `COMPAT-006` and `COMPAT-007`
6. move into `TEXT-001` through `TEXT-005`
7. move into `API-001` through `API-003`
8. finish `FEAT-001` through `FEAT-003`

## Notes

- `Print` remains intentionally absent from the backlog as a first-class compatibility type because current evidence suggests the library depends on it only indirectly through `Stream`.
- HTTPS/TLS removal is already complete and is therefore not included as a remaining implementation backlog item.