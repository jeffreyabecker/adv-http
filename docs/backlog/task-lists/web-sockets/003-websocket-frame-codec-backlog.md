2026-03-23 - Copilot: created detailed Phase 3 WebSocket frame codec backlog.

# WebSocket Phase 3 Core Protocol Types And Frame Codec Backlog

## Summary

This phase adds the protocol primitives that make an upgraded connection useful: frame metadata, frame parsing, masking, payload-length decoding, frame writing, and message-fragment handling. The codec should stay independent from routing and callback dispatch so it can be driven entirely by byte fixtures in the native lane.

## Goal / Acceptance Criteria

- The library has explicit internal types for WebSocket opcodes, close codes, frame metadata, parser results, and writer results.
- A reader can parse client frames incrementally from byte-channel input with correct masking and payload-length behavior.
- A writer can encode server frames for text, binary, continuation, ping, pong, and close operations.
- Fragmentation and control-frame validation rules are enforced consistently.

## Unit Test Coverage Targets

- Add byte-fixture tests for minimal text and binary frames, masked payload decoding, and correct server-frame serialization.
- Add boundary tests for 125-byte, 126-byte, 65535-byte, and 65536-byte payload-length encoding behavior.
- Add negative tests for unmasked client frames, malformed extended lengths, reserved-bit misuse, fragmented control frames, and unexpected continuation frames.
- Verify close-frame parsing for empty payload, valid two-byte status, and malformed close payload lengths.
- Verify incremental parse behavior when headers, masks, and payloads arrive across multiple small reads.

## Tasks

### Protocol Types

- [ ] Introduce internal enums and structs for opcode, close code, frame header metadata, and codec status.
- [ ] Keep the type set internal until the session runtime proves what must remain private versus public.
- [ ] Ensure value ranges and constants follow RFC 6455 without exposing unnecessary surface area yet.

### Frame Reader

- [ ] Add a frame reader built on `IByteChannel` or compatible byte-source semantics rather than a concrete client implementation.
- [ ] Support incremental parse state for header bytes, extended lengths, mask bytes, and payload consumption.
- [ ] Enforce masking and payload-length validation as part of parse, not as a later policy pass.

### Frame Writer

- [ ] Add a frame writer for text, binary, continuation, ping, pong, and close frames.
- [ ] Ensure the writer emits correct FIN, opcode, and payload-length fields for server frames.
- [ ] Decide whether the writer returns owned buffers, byte sources, or direct write commands for later runtime integration.

### Fragmentation And Validation

- [ ] Implement continuation tracking across fragmented messages.
- [ ] Enforce control-frame restrictions on FIN, payload length, and fragmentation.
- [ ] Keep error reporting deterministic so later phases can map codec failures into close behavior cleanly.

## Owner

TBD

## Priority

High

## References

- `docs/plans/websocket-support-plan.md`
- `src/pipeline/TransportInterfaces.h`
- `src/streams/ByteStream.h`
- `test/test_native/`
- RFC 6455 Section 5