2026-03-21 - Copilot: introduced byte-source and byte-sink contracts plus legacy Stream bridge adapters while leaving response-path migration open.
2026-03-21 - Copilot: created detailed Phase 5 stream and response backlog.

# No-Arduino Phase 5 Stream And Response Backlog

## Summary

This phase is the architectural seam change for the response path. The repository currently models response production, transform layers, file-backed output, and pipeline response callbacks around `std::unique_ptr<Stream>`, with a compat fallback that still mirrors Arduino `Stream` semantics. The goal of this phase is to introduce library-owned read and write contracts, migrate the read-only response types first, and stop letting duplex Arduino inheritance shape the entire response pipeline. The initial contract layer now exists in `src/streams/ByteStream.h`, backed by explicit availability semantics and bridge adapters to the current compat `Stream`, but no response or pipeline type consumes it yet.

## Goal / Acceptance Criteria

- Core response and stream-composition code no longer requires Arduino `Stream` as the primary abstraction.
- Read-only response and transform types move onto a library-owned byte-source contract.
- Duplex writable behavior is isolated to the small number of types that actually need it.
- The pipeline continues to stream bounded-memory responses incrementally.

## Tasks

### Existing Semantics Freeze

- [ ] Document the current `Stream` contract as used by the repository, including the meaning of `available()`, `read()`, `peek()`, and end-of-stream behavior.
- [ ] Add or extend tests in `test/test_native/test_stream_available.cpp` and `test/test_native/test_stream_utilities.cpp` to lock in the current semantics before interface changes.
- [ ] Confirm which in-tree stream types are read-only, which are duplex, and which exist only as adapters.

### Core Interface Introduction

- [x] Decide where the new byte-source and byte-sink contracts live, such as `src/streams/` or `src/compat/`.
- [x] Introduce the readable byte-source interface with explicit semantics for readable, exhausted, and temporarily unavailable states.
- [x] Introduce a narrow byte-sink interface only where write-side abstraction is actually needed.
- [x] Decide how `Availability.h` participates in the new contract and whether it becomes the canonical availability-result definition.
- [x] Add temporary bridge adapters between the new contracts and the existing compat `Stream` type.

### Read-Only Stream Migration

- [ ] Migrate `src/streams/Streams.h` and `src/streams/Streams.cpp` read-only stream types such as `ReadStream`, `OctetsStream`, `LazyStreamAdapter`, and concat helpers onto the byte-source contract.
- [ ] Update `src/streams/Base64Stream.h` and `src/streams/Base64Stream.cpp` to target the new readable contract.
- [ ] Update `src/streams/UriStream.h` and `src/streams/UriStream.cpp` to target the new readable contract.
- [ ] Update iterator helpers in `src/streams/Iterators.h` so ownership uses the new source abstraction rather than raw `Stream` pointers.

### Response Layer Migration

- [ ] Update `src/response/HttpResponse.h` and `src/response/HttpResponse.cpp` so response bodies are typed against the new readable contract.
- [ ] Update `src/response/HttpResponseBodyStream.h` and `src/response/ChunkedHttpResponseBodyStream.*` to wrap the new readable contract rather than `Stream` directly.
- [ ] Update `src/response/HttpResponseIterators.h` and related response composition helpers to emit the new source type.
- [ ] Preserve the current direct-response and chunked-response behavior byte-for-byte where practical.

### Pipeline Integration

- [ ] Update `src/pipeline/IPipelineHandler.h` and `src/pipeline/HttpPipeline.*` so response-ready callbacks carry the new source abstraction.
- [ ] Keep the pipeline write loop buffer-based and incremental.
- [ ] Ensure the client write path still uses the transport seam cleanly and does not depend on Arduino `Print` behavior.

### Duplex And Memory Stream Review

- [ ] Audit `NonOwningMemoryStream`, `StaticMemoryStream`, and any other write-capable in-tree stream type.
- [ ] Decide whether these types should implement a combined duplex interface, a separate sink interface, or split reader/writer views.
- [ ] Prevent the small number of duplex types from forcing the entire response model back onto a duplex base class.

### File-Backed And Adapter Interactions

- [ ] Coordinate with the filesystem phase so file-backed responses can consume file data through the new readable contract without reintroducing `Stream` inheritance into the core.
- [ ] Keep Arduino `Stream` and `Print` usage confined to adapter code once the migration stabilizes.

### Validation

- [ ] Extend native tests to cover the new source contract and bridge behavior.
- [ ] Add regression tests for concat ordering, lazy generation, chunk framing, and end-of-stream signaling.
- [ ] Record any semantic changes that would force example or response API compatibility work later.

## Owner

TBD

## Priority

High

## References

- `src/compat/Stream.h`
- `src/compat/Availability.h`
- `src/streams/Streams.h`
- `src/streams/Streams.cpp`
- `src/streams/Base64Stream.h`
- `src/streams/Base64Stream.cpp`
- `src/streams/UriStream.h`
- `src/streams/UriStream.cpp`
- `src/streams/Iterators.h`
- `src/response/HttpResponse.h`
- `src/response/HttpResponse.cpp`
- `src/response/HttpResponseBodyStream.h`
- `src/response/ChunkedHttpResponseBodyStream.h`
- `src/response/ChunkedHttpResponseBodyStream.cpp`
- `src/response/HttpResponseIterators.h`
- `src/pipeline/IPipelineHandler.h`
- `src/pipeline/HttpPipeline.h`
- `src/pipeline/HttpPipeline.cpp`
- `test/test_native/test_stream_available.cpp`
- `test/test_native/test_stream_utilities.cpp`
- `docs/plans/no-arduino/stream-replacement-plan.md`