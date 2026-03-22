# HttpServerAdvanced Stream Replacement Plan

## Objective

Define a library-owned replacement for the current Arduino `Stream` and inherited `Print` dependency that preserves the library's incremental streaming architecture without preserving Arduino's duplex base-class model as the center of the design.

This document is intentionally about the stream replacement itself:

- what behaviors must be preserved
- what roles the replacement interfaces must cover
- how Arduino `Stream` and `Print` should be contained at adapter boundaries
- how to stage the migration without destabilizing the response path

Concrete implementation work remains a later step. This document defines the target architecture and migration plan.

## Why A Separate Plan Is Needed

The stream dependency is deeper than the other Arduino seams.

Unlike transport endpoint values or filesystem handles, `Stream` is not just a parameter or value type. It is one of the library's fundamental composition primitives:

- response bodies are modeled as pull-based streams
- transform layers such as chunking, concatenation, URI encoding and decoding, and base64 wrappers are expressed as stream adapters
- file-backed responses participate in the same model
- some in-memory buffering helpers are currently both readable and writable

That makes the stream seam architectural rather than incidental. It needs its own plan so the main Arduino decoupling document stays focused on staging while this document captures the deeper contract definition.

## Goals

- preserve the current incremental streaming model for response generation and stream-to-stream transformations
- preserve bounded-memory behavior for direct and chunked responses
- remove Arduino `Stream` and `Print` from the platform-neutral core type system
- keep Arduino compatibility available through adapters rather than through core inheritance
- avoid a flag-day redesign of response construction and pipeline flow

## Non-Goals

- replacing the response model with a buffer-first or coroutine-based system
- redesigning request ingress around a new stream abstraction when `IClient` already covers the required behavior
- preserving every Arduino `Stream` or `Print` convenience API in the core
- introducing formatting helpers or text-oriented output APIs derived from `Print`

## Current Behavioral Contract

The current codebase uses `Stream` in a narrower way than Arduino defines it.

### Observed Usage Shape

- the pipeline owns a response `std::unique_ptr<Stream>` and drains it into a fixed-size stack buffer before forwarding bytes to `IClient`
- most in-tree stream classes are read-only byte producers, not general duplex channels
- direct `Print` usage is effectively absent in the current codebase
- writable behavior exists mainly in a small number of utility types such as in-memory streams
- `File` participates in the response path mainly because it can be consumed as a byte source

### Current Legacy `Stream` Semantics In This Repository

The repository currently documents a three-state legacy contract around `int available()`, but the concrete implementations are not perfectly uniform.

- `available() > 0` means a caller can pull that many bytes immediately from the current source.
- `available() == 0` is treated as terminal exhaustion for finite sources and is the value concat helpers skip past when moving to the next child.
- `available() < 0` is used by adapter-facing code and newer bridge layers to mean "temporarily unavailable" rather than exhausted.
- `read()` and `peek()` are the practical byte-production contract. They return the next byte as an unsigned `int` payload or `-1` when no byte is produced for the current call.

Important current quirks that must be preserved until the migration intentionally changes them:

- `ReadAsString()` and `ReadAsVector()` treat an initial `available() == 0` as definitely empty and return immediately, but they do not short-circuit on negative `available()` values.
- `IndefiniteConcatStream::available()` skips only children whose `available() == 0`; a negative `available()` is surfaced to the caller instead of being skipped.
- `IndefiniteConcatStream::read()` and `peek()` advance past any child whose `read()` or `peek()` returns `-1`, so temporary-unavailable behavior is not preserved consistently through concat wrappers today.
- `RefBufferedReadStreamWrapper::available()` reports only bytes already staged in its local buffer. It can return `0` even when the wrapped stream still has more data until `peek()` or `read()` fills the buffer.
- Transform wrappers such as base64 and URI encoding/decoding expose `available()` as a pass-through or approximation, not a guaranteed exact final-output byte count in every state.

### Semantics That Must Be Preserved

- pull-based byte production
- lazy response generation
- stream composition by wrapping an inner source
- end-of-stream detection without forcing full buffering
- the distinction between:
  - bytes available now
  - stream exhausted
  - temporarily unable to produce data but not yet finished, where current code relies on that state

## Architectural Problem Statement

Arduino `Stream` currently conflates three roles that should be independent in the decoupled design:

1. readable byte source
2. writable byte sink
3. Arduino compatibility object for board APIs

The library does need the first role. It only needs the second role in a much smaller set of places. The third role should stay entirely out of the core.

## Target Architecture

The replacement should preserve the streaming model while narrowing the contracts.

### Core Interfaces

The exact names can change, but the design should converge on these roles:

#### `IByteSource`

Required core behavior:

- `available()`
- `read()`
- `peek()`

Responsibilities:

- represent a pull-based readable byte source
- preserve the current response-stream semantics closely enough that existing wrappers can migrate with minimal behavioral change
- serve as the main base contract for response bodies, transform streams, and file-backed response reads

#### `IByteSink`

Required core behavior:

- `write(const uint8_t *buffer, std::size_t size)`
- optional `write(uint8_t byte)` convenience
- `flush()`

Responsibilities:

- represent outbound byte writes without inheriting read semantics
- wrap transports, adapters, and test sinks cleanly
- stay small enough that it can be implemented by an adapter over Arduino `Print` if needed

#### Optional `IByteChannel`

This should exist only if later implementation work proves that a shared duplex abstraction is genuinely useful.

Requirements:

- it must not become the default base for all stream-like objects
- it should be used only for genuinely duplex types such as memory streams if those continue to need both directions in one object

### Design Principle

Readable byte production is the primary abstraction. Duplex behavior is a special case, not the default inheritance shape.

## Behavioral Semantics

The replacement interfaces must preserve these semantics explicitly.

### `available()`

The plan replaces the imprecise `int available()` contract with a clearer, explicit readiness result that separates state from quantity. Returning a signed integer overloaded three different meanings; that ambiguity made adapters and platform mappings error-prone. The proposed replacement is a small POD `AvailableResult` that clients can inspect safely in portable code.

Suggested `AvailableResult` design (C++17-friendly):

```cpp
enum class AvailabilityState { HasBytes, Exhausted, TemporarilyUnavailable, Error };

struct AvailableResult {
    AvailabilityState state;
    std::size_t count; // valid only when state == HasBytes
    int errorCode;      // optional platform-specific error when state == Error
};
```

Semantics:
- `HasBytes`: `count > 0` bytes are currently readable without blocking.
- `Exhausted`: the source is terminal; no more bytes will ever arrive (end-of-stream).
- `TemporarilyUnavailable`: the source has no bytes right now but may produce bytes later (non-terminal stall).
- `Error`: unrecoverable error; `errorCode` may carry a platform-specific code useful for diagnostics.

Recommended API changes:
- replace legacy integer availability reporting with `AvailableResult` on `IByteSource`-like contracts and expose the transport seam through `IClient::available()` rather than an integer-returning readiness method.
- keep `read()`/`peek()` semantics but document interactions with `AvailableResult` (e.g., `read()` may return `-1` on error or `0` if no byte was produced and `AvailableResult` reports `TemporarilyUnavailable`).
- provide helper accessors in adapters where useful, e.g., `bool hasBytes() const { return available().state == AvailabilityState::HasBytes; }` and `std::size_t peekAvailable() const { auto r = available(); return r.state == AvailabilityState::HasBytes ? r.count : 0; }`.

Migration guidance:
- Phase 2 adapters should provide a small bridge that implements `AvailableResult available()` while continuing to expose the old `int available()` internally where platform APIs require it; mapping rules are straightforward:
  - if platform `available()` > 0 → `HasBytes(count)`
  - if platform `available()` == 0 and socket/stream is closed/EOF → `Exhausted`
  - if platform `available()` == 0 and socket/stream is open but no bytes right now → `TemporarilyUnavailable`
  - if platform `available()` < 0 or an error condition observed → `Error` with code
- Update the pipeline read loops to inspect the `AvailableResult` state instead of interpreting raw integers; this makes intent explicit and reduces platform-specific conditional logic.

### `read()` and `peek()`

- `read()` consumes one byte and returns `-1` only when no byte can be produced for the current call according to the agreed contract
- `peek()` mirrors `read()` without consuming
- wrapper and transform sources must preserve these semantics when forwarding to inner sources

### Ownership

- `std::unique_ptr` should remain the default ownership model for composed byte sources
- the response path should continue to transfer ownership of body sources explicitly
- adapters may internally use shared ownership where platform constraints require it, but the core contract should remain unique-ownership-first

## Arduino Boundary Strategy

Arduino types should be adapter details, not core dependencies.

### `Stream`

- do not make Arduino `Stream` the core base contract
- provide Arduino-facing adapters that wrap `Stream` as a library byte source or duplex channel where needed
- keep Arduino headers out of platform-neutral core headers

### `Print`

- do not introduce `Print` into the core
- do not model sink APIs around Arduino formatting overloads
- if an Arduino-facing integration genuinely needs `Print`, provide a dedicated adapter such as `PrintByteSink` in adapter-only code
- if no such call site exists, keep `Print` completely out of the final decoupled architecture

## Expected Class Migration

The current codebase already suggests a natural split.

### Classes That Should Migrate To `IByteSource`

- `ReadStream`
- `EmptyReadStream`
- `OctetsStream`
- `StringStream`
- `LazyStreamAdapter`
- `ConcatStream`
- URI encoding and decoding stream wrappers
- base64 encoding and decoding stream wrappers
- `ChunkedHttpResponseBodyStream`
- static-file response wrappers

### Classes That Need Separate Review

- `NonOwningMemoryStream`
- any future helper that is truly both readable and writable

`NonOwningMemoryStream` is the only remaining in-tree duplex helper after removing the unused owning memory-stream wrappers. Any future duplex helper should justify itself explicitly rather than broadening the source-oriented response contracts.

### Filesystem Interaction

- `File` should participate in the response path primarily as a readable byte source
- Arduino `fs::File` may still be adapted as a stream-like object under `ARDUINO`, but that should remain an adapter concern

## Current In-Tree Stream Role Inventory

### Read-Only / Source-Oriented Types

- `ReadStream`
- `EmptyReadStream`
- `OctetsStream`
- `StringStream`
- `StdStringStream`
- `LazyStreamAdapter`
- `IndefiniteConcatStream`
- `ConcatStream`
- `RefBufferedReadStreamWrapper`
- `StaticBufferedReadStreamWrapper`
- `Base64DecoderStream`
- `Base64EncoderStream`
- `UriDecodingStream`
- `UriEncodingStream`
- `FormEncodingStream`
- `ChunkedHttpResponseBodyStream`
- `HttpPipelineResponseSource`

### Duplex Types

- `NonOwningMemoryStream`

### Adapter-Facing Legacy Or Bridge Types

- `Compat::File` is a legacy `Stream`-shaped readable file adapter with no-op write behavior in the non-Arduino build.
- `StaticFileHandlerFactory::FileByteSource` is a response-path adapter over `File`.
- `StreamByteSourceAdapter` and `OwningStreamByteSourceAdapter` adapt legacy `Stream` objects into `IByteSource`.
- `ByteSourceStreamAdapter` adapts an `IByteSource` back into the legacy `Stream` contract.
- `StreamByteSinkAdapter` adapts a legacy `Stream` into `IByteSink`.

## Response Path Consequences

The replacement must preserve the current response architecture.

### What Should Not Change

- handlers and response builders can still produce lazily readable bodies
- the pipeline still drains response bytes into a fixed-size stack buffer
- the pipeline still writes those bytes to `IClient`
- chunked transfer encoding can still be emitted on the fly by a wrapping source
- concatenation and layered wrappers remain the main composition technique

### What Should Change

- response bodies should eventually be typed as readable byte sources rather than full Arduino-shaped streams
- read-only wrappers should stop implementing no-op or placeholder `write()` methods just to satisfy Arduino inheritance
- sink behavior should move to the places that actually emit bytes outward

## Request Path Consequences

The request side does not need a matching stream redesign.

- `HttpPipeline` already reads incoming bytes from `IClient`
- request parsing is already incremental and chunk-based
- there is no need to force inbound parsing onto a `Stream` replacement just for symmetry

The stream replacement should therefore focus on response composition and on any utility classes that are genuinely source-oriented.

## Migration Plan

### Phase 1: Freeze Existing Semantics

- document the current source semantics in code comments and tests
- classify existing stream-like types as read-only, write-only, duplex, or adapter-facing
- add tests that lock in response composition behavior before interface changes begin

### Phase 2: Introduce New Core Contracts

- add library-owned byte-source and byte-sink interfaces alongside the current compatibility `Stream`
- avoid changing behavior in this phase
- provide temporary bridge types or adapters so existing code can migrate incrementally

### Phase 3: Migrate Read-Only Response And Utility Types

- move the `ReadStream` hierarchy onto the byte-source contract first
- migrate transform streams and response-body wrappers next
- migrate response iterator composition onto byte-source ownership

### Phase 4: Isolate Duplex Cases

- decide whether memory streams should remain duplex objects or split into distinct reader and writer roles
- ensure these cases do not force the whole response stack back onto a duplex base class

### Phase 5: Move Arduino To The Boundary

- restrict Arduino `Stream` and `Print` usage to adapter headers and implementations
- keep Arduino-facing convenience layers working through thin adapters where needed
- remove Arduino IO headers from core stream and response code

## Acceptance Criteria

The stream replacement plan is complete when the resulting implementation can satisfy these constraints:

1. Core response and stream-composition code compile without Arduino IO headers.
2. Direct and chunked response paths preserve current bounded-memory behavior.
3. Read-only response and transform types no longer inherit writable behavior by default.
4. Arduino `Stream` and `Print` appear only in adapter code or Arduino-facing compatibility layers.
5. File-backed responses still participate in the same lazy byte-source composition model.
6. The pipeline continues to drain response bytes incrementally into a fixed-size buffer and forward them to `IClient`.

## Risks And Open Questions

- the three-state `available()` contract is useful today but awkward; replacing it later would require a deliberate follow-up design rather than an incidental refactor
- memory-stream utilities may be the strongest pressure toward a duplex interface and need careful review before finalizing names and layering
- filesystem integration must not reintroduce Arduino stream inheritance into the core through `File`
- Arduino compatibility overloads in response helpers may hide additional assumptions that only become visible once body types change from `Stream` to `IByteSource`

## Recommended Immediate Follow-Up

Before implementation begins, create a file-by-file backlog for the first non-behavioral step:

- introduce the new source and sink interfaces
- add bridge adapters next to the current compatibility `Stream`
- migrate one or two clearly read-only wrappers first
