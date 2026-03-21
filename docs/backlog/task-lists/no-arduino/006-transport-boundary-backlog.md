2026-03-21 - Copilot: created detailed Phase 4 transport boundary backlog.

# No-Arduino Phase 4 Transport Boundary Backlog

## Summary

This phase narrows the transport seam to the actual contracts the HTTP pipeline needs and removes Arduino-shaped wrapper behavior from the core-facing boundary. The repository already has `IClient` and `IServer`, but `NetClient.h` still mixes interface definitions with template wrappers over Arduino server/client classes and still exposes `IPAddress` directly in the contract. The goal of this phase is to keep the useful seam, remove the accidental Arduino-shaped design choices from it, and make the server and pipeline layers depend only on transport interfaces and library-owned endpoint types.

## Goal / Acceptance Criteria

- `HttpPipeline`, `HttpRequest`, and server layers depend on transport interfaces and library-owned endpoint/value types only.
- Generic Arduino wrapper behavior is removed from the core-facing transport contract.
- Arduino transport support remains possible through adapter code without shaping the core interface.
- Transport semantics needed by the pipeline are explicitly documented and testable.

## Tasks

### Contract Inventory And Semantic Freeze

- [ ] Inventory the methods currently exposed on `IClient` and `IServer` in `src/pipeline/NetClient.h`.
- [ ] Record which methods are actually consumed by `HttpPipeline`, `HttpServerBase`, `StandardHttpServer`, and related code.
- [ ] Freeze the current accept, read, write, connected, timeout, and endpoint semantics before signature changes begin.
- [ ] Identify which parts of `NetClient.h` are true interface surface versus Arduino convenience wrappers.

### Endpoint Type Cleanup

- [ ] Decide whether transport endpoints should be represented by the existing compat `IpAddress` plus port pair or a dedicated endpoint struct.
- [ ] Refactor `src/pipeline/IPipelineHandler.h` so address-setting APIs use the chosen library-owned endpoint representation.
- [ ] Refactor `src/core/HttpRequest.h` to store the chosen endpoint representation without Arduino-specific leakage.
- [ ] Update any server-facing APIs in `src/server/HttpServerBase.h` and `src/server/StandardHttpServer.h` that still directly expose `IPAddress` assumptions.

### `NetClient.h` Separation

- [ ] Split pure interface definitions from generic wrapper implementations in `src/pipeline/NetClient.h`.
- [ ] Remove or relocate `ClientImpl<T>` and `ServerImpl<T>` so they no longer define the shape of the core transport contract.
- [ ] Preserve only the minimum abstraction that the pipeline and server layers actually require.
- [ ] Decide whether Arduino wrapper implementations remain in-tree as adapters or move to a distinct adapter header under `src/compat/` or `src/server/`.

### Pipeline And Server Integration

- [ ] Update `src/pipeline/HttpPipeline.h` and `src/pipeline/HttpPipeline.cpp` to compile against the cleaned transport seam.
- [ ] Update `src/server/HttpServerBase.h` and `src/server/HttpServerBase.cpp` so accept and pipeline creation remain correct after the seam cleanup.
- [ ] Review `src/server/StandardHttpServer.h`, `src/server/WiFiHttpServer.h`, and any friendly web-server wrappers for adapter-only concerns that should move out of core contracts.
- [ ] Preserve existing Arduino construction ergonomics only through thin wrapper layers.

### Timeout And Connection Semantics

- [ ] Decide whether timeout setters/getters on `IClient` remain part of the transport contract or are routed differently once the clock seam exists.
- [ ] Document how `connected()` should behave when buffered unread data remains.
- [ ] Document how `accept()` should behave when no client is pending.
- [ ] Document the expected meaning of `status()` and how much of the existing TCP-state enum surface is still justified.

### Adapter Strategy

- [ ] Decide where Arduino client/server adapters live after the seam cleanup.
- [ ] Define the minimum adapter responsibilities, including endpoint translation and timeout forwarding.
- [ ] Ensure any remaining adapter hooks such as `configureConnection()` do not leak platform detail back into the core contract.

### Validation And Tests

- [ ] Add or plan focused tests for accept semantics, connection liveness semantics, and endpoint propagation.
- [ ] Ensure future fake-client work in the native fixture layer targets the cleaned interface, not the current wrapper-heavy header.
- [ ] Record any out-of-tree implementation contract that should remain documentation-only rather than in-tree code.

## Owner

TBD

## Priority

High

## References

- `src/pipeline/NetClient.h`
- `src/pipeline/IPipelineHandler.h`
- `src/pipeline/HttpPipeline.h`
- `src/pipeline/HttpPipeline.cpp`
- `src/core/HttpRequest.h`
- `src/server/HttpServerBase.h`
- `src/server/HttpServerBase.cpp`
- `src/server/StandardHttpServer.h`
- `src/server/WiFiHttpServer.h`
- `src/compat/IpAddress.h`
- `docs/plans/no-arduino/in-tree-transport-plan.md`
