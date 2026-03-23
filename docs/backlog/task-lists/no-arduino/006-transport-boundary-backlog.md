2026-03-23 - Copilot: removed the stale reference to the retired transport planning doc after the backlog became the canonical migration record.
2026-03-22 - Copilot: removed the remaining compatibility-layer network address shim and deleted the old Arduino-style address type from the repo entirely.
2026-03-22 - Copilot: documented that `connected()` is a boolean liveness gate that must remain true while unread buffered request bytes can still be drained, and updated the core interface return type from `std::uint8_t` to `bool`.
2026-03-22 - Copilot: removed `IClient::available()`, promoted result-based transport availability to the sole pure-abstract ingress readiness API, and updated the in-tree caller to consume it directly.
2026-03-22 - Copilot: documented that `IClient` timeout setters/getters remain in the transport seam as implementation hints rather than moving behind a separate clock-driven timeout control surface.
2026-03-22 - Copilot: removed the unused `IServer::status()` requirement and deleted `ConnectionStatus` from the core transport seam because neither had in-tree consumers.
2026-03-22 - Copilot: removed the unused `IClient::status()` requirement from the core transport interface because it had no in-tree consumers.
2026-03-22 - Copilot: documented that `accept()` must return `nullptr` immediately when no client is pending rather than blocking for a new connection.
2026-03-21 - Copilot: added explicit backlog items to replace legacy transport address-value exposure with `std::string_view` address APIs and adapter-owned string backing.
2026-03-21 - Copilot: dropped the `TransportEndpoint` detour and pivoted the seam toward a library-owned IP value type while preserving old-style IP-plus-port APIs.
2026-03-21 - Copilot: split transport interfaces from Arduino-shaped wrappers and introduced a dedicated endpoint value type without migrating pipeline call sites yet.
2026-03-21 - Copilot: created detailed Phase 4 transport boundary backlog.
2026-03-21 - Copilot: moved the core transport seam to textual address views, pushed that through pipeline/request/server contracts, and validated the slice in the native test lane.

# No-Arduino Phase 4 Transport Boundary Backlog

## Summary

This phase narrows the transport seam to the actual contracts the HTTP pipeline needs and removes Arduino-shaped wrapper behavior from the core-facing boundary. The repository already has `IClient` and `IServer`, but `NetClient.h` previously mixed interface definitions with template wrappers over Arduino server/client classes and exposed an Arduino-shaped network address type directly in the contract. The goal of this phase is to keep the useful seam, remove the accidental Arduino-shaped design choices from it, and make the server and pipeline layers depend only on transport interfaces plus library-owned textual address state. The seam split is now in place, and the next address correction has landed: the core contracts now expose `std::string_view` address accessors backed by owned adapter or request storage, and the old compatibility-layer network address type has been removed entirely.

## Goal / Acceptance Criteria

- `HttpPipeline`, `HttpRequest`, and server layers depend on transport interfaces and `std::string_view`-based address APIs backed by library-owned text state rather than Arduino-shaped address value types.
- Generic Arduino wrapper behavior is removed from the core-facing transport contract.
- Arduino transport support remains possible through adapter code without shaping the core interface.
- Transport semantics needed by the pipeline are explicitly documented and testable.

## Tasks

### Contract Inventory And Semantic Freeze

- [x] Inventory the methods currently exposed on `IClient` and `IServer` in `src/pipeline/NetClient.h`.
- [x] Record which methods are actually consumed by `HttpPipeline`, `HttpServerBase`, `StandardHttpServer`, and related code.
- [x] Freeze the current accept, read, write, connected, timeout, and endpoint semantics before signature changes begin.
- [x] Identify which parts of `NetClient.h` are true interface surface versus Arduino convenience wrappers.

#### Current Interface Inventory

- `IClient` currently exposes byte-channel `write()`, `available()`, `read()`, `peek()`, and `flush()` plus transport methods `stop()`, `connected()`, `remoteAddress()`, `remotePort()`, `localAddress()`, `localPort()`, `setTimeout()`, and `getTimeout()`.
- `IServer` currently exposes `accept()`, `begin()`, `port()`, `localAddress()`, and `end()`.
- `src/pipeline/NetClient.h` is now only a re-export header over `TransportInterfaces.h`; the actual interface surface is `TransportInterfaces.h`, while Arduino-facing transport wrappers have been pushed out of the core contract.

#### Current Consumer Inventory

- `HttpPipeline` consumes `IClient::available()`, `read()`, `write()`, `connected()`, `stop()`, `setTimeout()`, `remoteAddress()`, `remotePort()`, `localAddress()`, and `localPort()`.
- `HttpServerBase` consumes `IServer::accept()` indirectly via concrete server wrappers and only requires server-local address and port reporting for the higher-level server API.
- `StandardHttpServer` and `WiFiHttpServer` are now clearly adapter-facing wrappers that translate Arduino endpoint state into textual address views for the core-facing server contract.

#### Frozen Semantics Snapshot

- `accept()` returns `nullptr` when no pending client exists and transfers ownership when a client is available.
- `available()` now drives request-read progress in `HttpPipeline` through `AvailableResult`, with `HasBytes` continuing the read loop and `Exhausted` marking request completion for the current request.
- `connected()` is a boolean liveness gate checked before each processing pass and must remain `true` while unread buffered request bytes can still be drained.
- `setTimeout()` and `getTimeout()` remain part of the transport contract as implementation hints rather than being routed through a separate clock-driven timeout controller.
- `IClient::status()` has been removed from the core seam because no in-tree consumer depends on per-client TCP-state reporting.
- `IServer::status()` and `ConnectionStatus` have also been removed from the core seam because no in-tree consumer depends on listener-state reporting.
- Endpoint reporting is now textual in the core seam: IPv4 addresses are currently adapter-formatted as dotted-decimal text with owned backing stored in the adapter or request object that returns the view.

### IP Address Text Cleanup

- [x] Decide to preserve old-style IP-plus-port APIs instead of introducing a dedicated transport endpoint struct.
- [x] Define the canonical textual transport address representation and whether it preserves raw adapter formatting or normalizes IPv4 and later IPv6 text.
- [x] Replace `IClient` address exposure with `std::string_view` accessors while keeping `remotePort()` and `localPort()` as integer APIs.
- [x] Define where owned address strings live so returned `std::string_view` values remain valid across the request, pipeline, and server lifetimes.
- [x] Refactor `src/pipeline/IPipelineHandler.h` so address-setting APIs accept the new textual address form without Arduino-specific leakage.
- [x] Refactor `src/core/HttpRequest.h` to store owned textual address state and expose `std::string_view` accessors instead of Arduino-style address values.
- [x] Update any server-facing APIs in `src/server/HttpServerBase.h` and `src/server/StandardHttpServer.h` that still directly expose Arduino-specific address assumptions so they use the textual address seam instead.
- [x] Remove the remaining compatibility-layer address shim once transport and request/server contracts no longer depend on it.

#### Address Representation Decision

- The canonical core transport representation is now textual address data returned as `std::string_view`.
- For the currently supported in-tree adapters, IPv4 addresses are emitted as dotted-decimal text using adapter-owned `std::string` backing.
- No normalization beyond current adapter-produced IPv4 dotted-decimal formatting is attempted in this slice; future IPv6 support should extend the textual seam rather than reintroducing a legacy binary address shim into the core contract.
- Owned address strings currently live in transport adapters for accepted client endpoints, `HttpRequest` for per-request endpoint state, `StandardHttpServer` for configured bind address text, and `WiFiHttpServer` for dynamic local-address reporting.

### `NetClient.h` Separation

- [x] Split pure interface definitions from generic wrapper implementations in `src/pipeline/NetClient.h`.
- [x] Remove or relocate `ClientImpl<T>` and `ServerImpl<T>` so they no longer define the shape of the core transport contract.
- [x] Preserve only the minimum abstraction that the pipeline and server layers actually require.
- [x] Decide whether Arduino wrapper implementations remain in-tree as adapters or move to a distinct adapter header under `src/compat/` or `src/server/`.

### Pipeline And Server Integration

- [x] Update `src/pipeline/HttpPipeline.h` and `src/pipeline/HttpPipeline.cpp` to compile against the cleaned transport seam.
- [x] Update `src/server/HttpServerBase.h` and `src/server/HttpServerBase.cpp` so accept and pipeline creation remain correct after the seam cleanup.
- [x] Review `src/server/StandardHttpServer.h`, `src/server/WiFiHttpServer.h`, and any friendly web-server wrappers for adapter-only concerns that should move out of core contracts.
- [x] Preserve existing Arduino construction ergonomics only through thin wrapper layers.

### Timeout And Connection Semantics

- [x] Decide whether timeout setters/getters on `IClient` remain part of the transport contract or are routed differently once the clock seam exists.
- [x] Document how `connected()` should behave when buffered unread data remains.
- [x] Document how `accept()` should behave when no client is pending.
- [x] Remove unused transport status reporting from the core seam.

#### Timeout And Connection Semantics Snapshot

- `setTimeout()` and `getTimeout()` stay on `IClient` as hinting APIs to the transport implementation rather than moving behind a separate timeout-routing abstraction.
- `connected()` returns a boolean and must remain `true` while unread buffered request bytes still exist; returning `false` is a terminal disconnect signal that causes the pipeline to stop before further reads.
- `accept()` is non-blocking at the transport seam and must return `nullptr` immediately when no client is pending rather than waiting for a new connection.

### Adapter Strategy

- [x] Decide where Arduino client/server adapters live after the seam cleanup.
- [x] Define the minimum adapter responsibilities, including endpoint translation and timeout forwarding.
- [x] Ensure any remaining adapter hooks such as `configureConnection()` do not leak platform detail back into the core contract.

#### Adapter Responsibility Snapshot

- Translate Arduino client/server endpoint objects into textual addresses for the core seam.
- Forward timeout, liveness, read/write, flush, and stop behavior without reshaping pipeline semantics.
- Keep Arduino-only types and helper conversions out of `TransportInterfaces.h`, `IPipelineHandler.h`, `HttpPipeline`, `HttpRequest`, and `HttpServerBase`.
- The old `configureConnection()` escape hatch has been removed from the in-tree wrappers so adapter mutation is no longer exposed through the core-facing server surface.

### Validation And Tests

- [ ] Add or plan focused tests for accept semantics, connection liveness semantics, and endpoint propagation.
- [x] Ensure future fake-client work in the native fixture layer targets the cleaned interface, not the current wrapper-heavy header.
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
