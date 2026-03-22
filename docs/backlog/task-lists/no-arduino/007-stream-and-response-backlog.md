2026-03-22 - Copilot: added exact-output regression coverage for direct and chunked response serialization, including a temporary-unavailability chunk boundary, and marked the serializer-behavior preservation task complete.
2026-03-22 - Copilot: completed the read-only stream migration, moved iterator-helper ownership onto `IByteSource`, and marked the remaining Phase 5 transformer tasks done.
2026-03-22 - Copilot: audited duplex stream utilities, removed unused owning memory-stream helpers, and fixed the phase decision on keeping duplex behavior isolated to a single legacy helper.
2026-03-22 - Copilot: refreshed phase status after the response and pipeline byte-source migration, marked file-backed response integration complete, and narrowed the remaining work to legacy stream utilities, adapters, and regression gaps.
2026-03-22 - Copilot: documented current legacy Stream semantics, froze native stream-semantics coverage, and classified in-tree stream roles.
2026-03-21 - Copilot: migrated response bodies and `ChunkedHttpResponseBodyStream` onto `IByteSource`-based ownership and added native response-stream coverage.
2026-03-21 - Copilot: introduced byte-source and byte-sink contracts plus legacy Stream bridge adapters while leaving response-path migration open.
2026-03-21 - Copilot: created detailed Phase 5 stream and response backlog.

# No-Arduino Phase 5 Stream And Response Backlog

## Summary

This phase is the architectural seam change for the response path. The repository originally modeled response production, transform layers, file-backed output, and pipeline response callbacks around `std::unique_ptr<Stream>`, with a compat fallback that still mirrors Arduino `Stream` semantics. The library-owned contract layer now exists in `src/streams/ByteStream.h`, and response bodies, response assembly, pipeline callbacks, and static-file bodies already flow through `IByteSource`. The remaining work is concentrated in the legacy stream utility layer, duplex stream review, adapter cleanup, and the last regression gaps around composed response behavior.

## Goal / Acceptance Criteria

- Core response and stream-composition code no longer requires Arduino `Stream` as the primary abstraction.
- Read-only response and transform types move onto a library-owned byte-source contract.
- Duplex writable behavior is isolated to the small number of types that actually need it.
- The pipeline continues to stream bounded-memory responses incrementally.

## Tasks

### Existing Semantics Freeze

- [x] Document the current `Stream` contract as used by the repository, including the meaning of `available()`, `read()`, `peek()`, and end-of-stream behavior.
- [x] Add or extend tests in `test/test_native/test_stream_available.cpp` and `test/test_native/test_stream_utilities.cpp` to lock in the current semantics before interface changes.
- [x] Confirm which in-tree stream types are read-only, which are duplex, and which exist only as adapters.

### Core Interface Introduction

- [x] Decide where the new byte-source and byte-sink contracts live, such as `src/streams/` or `src/compat/`.
- [x] Introduce the readable byte-source interface with explicit semantics for readable, exhausted, and temporarily unavailable states.
- [x] Introduce a narrow byte-sink interface only where write-side abstraction is actually needed.
- [x] Decide how `Availability.h` participates in the new contract and whether it becomes the canonical availability-result definition.
- [x] Add temporary bridge adapters between the new contracts and the existing compat `Stream` type.

### Read-Only Stream Migration

- [x] Migrate `src/streams/Streams.h` and `src/streams/Streams.cpp` read-only stream types such as `ReadStream`, `OctetsStream`, `LazyStreamAdapter`, and concat helpers onto the byte-source contract.
- [x] Update `src/streams/Base64Stream.h` and `src/streams/Base64Stream.cpp` to target the new readable contract.
- [x] Update `src/streams/UriStream.h` and `src/streams/UriStream.cpp` to target the new readable contract.
- [x] Update iterator helpers in `src/streams/Iterators.h` so ownership uses the new source abstraction rather than raw `Stream` pointers.

### Response Layer Migration

- [x] Update `src/response/HttpResponse.h` and `src/response/HttpResponse.cpp` so response bodies are typed against the new readable contract.
- [x] Update `src/response/HttpResponse.*` and `src/response/ChunkedHttpResponseBodyStream.*` so response bodies are owned as readable byte sources rather than legacy `Stream` objects.
- [x] Update `src/response/HttpResponseIterators.h` and related response composition helpers to emit the new source type.
- [x] Preserve the current direct-response and chunked-response behavior byte-for-byte where practical.

### Pipeline Integration

- [x] Update `src/pipeline/IPipelineHandler.h` and `src/pipeline/HttpPipeline.*` so response-ready callbacks carry the new source abstraction.
- [x] Keep the pipeline write loop buffer-based and incremental.
- [x] Ensure the client write path still uses the transport seam cleanly and does not depend on Arduino `Print` behavior.

### Duplex And Memory Stream Review

- [x] Audit `NonOwningMemoryStream`, `StaticMemoryStream`, and any other write-capable in-tree stream type.
- [x] Decide whether these types should implement a combined duplex interface, a separate sink interface, or split reader/writer views.
- [x] Prevent the small number of duplex types from forcing the entire response model back onto a duplex base class.

Outcome: `NonOwningMemoryStream` remains as an isolated legacy duplex helper backed by caller-owned storage. The unused owning wrappers `MemoryStream` and `StaticMemoryStream` were removed rather than promoting them into the byte-contract layer. No response-path type now needs a combined duplex base class.

### File-Backed And Adapter Interactions

- [x] Coordinate with the filesystem phase so file-backed responses can consume file data through the new readable contract without reintroducing `Stream` inheritance into the core.
- [ ] Keep Arduino `Stream` and `Print` usage confined to adapter code once the migration stabilizes.

### Validation

- [x] Extend native tests to cover the new source contract and bridge behavior.
- [ ] Add regression tests for full response assembly, temporarily-unavailable propagation through composed response sources, and any remaining concat, lazy-generation, chunk-framing, or end-of-stream edge cases.
- [ ] Record any semantic changes that would force example or response API compatibility work later.

## Owner

TBD

## Priority

High

## References

- `src/compat/Stream.h`
- `src/compat/Availability.h`
- `src/streams/ByteStream.h`
- `src/streams/Streams.h`
- `src/streams/Streams.cpp`
- `src/streams/Base64Stream.h`
- `src/streams/Base64Stream.cpp`
- `src/streams/UriStream.h`
- `src/streams/UriStream.cpp`
- `src/streams/Iterators.h`
- `src/response/HttpResponse.h`
- `src/response/HttpResponse.cpp`
- `src/response/ChunkedHttpResponseBodyStream.h`
- `src/response/ChunkedHttpResponseBodyStream.cpp`
- `src/response/HttpResponseIterators.h`
- `src/pipeline/IPipelineHandler.h`
- `src/pipeline/HttpPipeline.h`
- `src/pipeline/HttpPipeline.cpp`
- `src/staticfiles/StaticFileHandler.h`
- `src/staticfiles/StaticFileHandler.cpp`
- `test/test_native/test_response_streams.cpp`
- `test/test_native/test_stream_available.cpp`
- `test/test_native/test_stream_utilities.cpp`
- `docs/plans/no-arduino/stream-replacement-plan.md`