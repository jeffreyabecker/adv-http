2026-03-24 - Copilot: created backlog item from the websocket context implementation plan to track the `WebSocketContext` and `WebSocketProtocolExecution` refactor in implementation-sized slices.

# Implement WebSocket Context And Protocol Execution Backlog

## Summary

The websocket Phase 4 design work now defines a clear target architecture: `WebSocketContext` directly owns callback-facing state and carried-forward HTTP activation data, while `WebSocketProtocolExecution` owns parser progress, outbound write progression, and pipeline-facing lifecycle behavior. The current `WebSocketSessionRuntime` still combines all of those responsibilities. This backlog item turns the implementation plan into an incremental work queue so the runtime can be split without regressing handshake flow, send semantics, or existing websocket behavior.

## Goal / Acceptance Criteria

- A first-class `WebSocketContext` type exists and directly owns the websocket callback bundle, carried-forward activation data, and public send/close/state APIs.
- A `WebSocketProtocolExecution` type owns parser state, pending write state, handshake progression, close-handshake sequencing, and transport-loop execution.
- The upgrade path explicitly constructs websocket-owned activation state instead of leaving websocket callbacks dependent on the old HTTP activation object.
- Websocket callbacks receive `WebSocketContext &` consistently across open, text, binary, close, and error events.
- The documented send-result and close-result semantics are implemented against execution-owned queue admission.
- Existing websocket behavior remains correct during the transition, including handshake success/rejection, automatic pong handling, close handling, and native test coverage.

## Scope

- Introduce the new websocket context, activation snapshot, and execution-control abstractions.
- Split the current fused runtime into context-owned state and execution-owned state.
- Update callback registration and upgrade plumbing to use `WebSocketContext &`.
- Preserve the current pipeline seam during transition by letting `WebSocketProtocolExecution` satisfy `IConnectionSession` until a later protocol-execution unification lands.

## Out Of Scope

- The later pipeline-level move from `IConnectionSession` to the proposed higher-level `IProtocolExecution` seam.
- The HTTP-side rename from `HttpRequest` to `HttpContext` beyond what is required for carrying activation data during websocket upgrade.
- New public heartbeat APIs beyond the already documented v1 decision to keep ping/pong internal.
- Typed websocket message adapters beyond the callback signature changes needed for `WebSocketContext &`.

## Proposed Slices

### Slice 1: Introduce Context And Snapshot Types

- Add `WebSocketContext` with direct callback ownership, public send/close declarations, and lifecycle/metadata accessors.
- Add `WebSocketActivationSnapshot` to carry HTTP activation data into websocket-owned state.
- Keep this slice focused on type introduction and state modeling rather than runtime extraction.

### Slice 2: Introduce Execution Control Interface

- Add `IWebSocketSessionControl` so `WebSocketContext` can request sends/closes without owning transport buffers.
- Define deterministic send-result and close-result mapping against that control interface.
- Add unit tests using a fake session-control implementation.

### Slice 3: Extract `WebSocketProtocolExecution`

- Move parser, pending-write, fragmented-message, handshake, and close-progression state out of `WebSocketSessionRuntime`.
- Make `WebSocketProtocolExecution` own `WebSocketContext` and implement `IConnectionSession` during transition.
- Reduce `WebSocketSessionRuntime` to a compatibility wrapper or remove it entirely if churn is manageable.

### Slice 4: Convert Callback Surface To `WebSocketContext &`

- Update websocket callback types to pass `WebSocketContext &`.
- Update websocket builder and upgrade plumbing so callback registration targets the new context-first signature model.
- Update native fixtures and tests that currently assume receive-only callbacks.

### Slice 5: Wire Carried-Forward HTTP Activation Data

- Build the activation snapshot from the current HTTP activation object during upgrade.
- Transfer or copy `items()`, method, version, headers, and local/remote endpoint data into websocket-owned state.
- Prove that websocket callbacks can still inspect that data after the HTTP phase ends.

### Slice 6: Finalize Send/Close Result Semantics

- Implement `WebSocketSendResult` and `WebSocketCloseResult` over the execution-owned queue.
- Cover `Queued`, `Backpressured`, `Closing`, `Closed`, `TooLarge`, `CloseQueued`, `AlreadyClosing`, and related documented outcomes.
- Add tests for send rejection during pending-write and close progression.

### Slice 7: Remove Transitional Runtime Naming

- Delete `WebSocketSessionRuntime` if it remains as a compatibility shim.
- Update references, docs, and tests to the new context/execution terminology.
- Leave the codebase describing the new ownership model directly rather than through runtime-compatibility language.

## Tasks

- [ ] Slice 1: add `WebSocketContext` with direct callback ownership, send/close declarations, lifecycle state, and carried-forward metadata accessors.
- [ ] Slice 1: add `WebSocketActivationSnapshot` to represent websocket-owned upgrade handoff data.
- [ ] Slice 2: add `IWebSocketSessionControl` and unit-test `WebSocketContext` send/close result mapping against a fake control object.
- [ ] Slice 3: add `WebSocketProtocolExecution` and move parser, pending-write, fragmented-message, and close-handshake state out of `WebSocketSessionRuntime`.
- [ ] Slice 3: decide whether `WebSocketSessionRuntime` becomes a short-lived compatibility wrapper or is removed immediately.
- [ ] Slice 4: change websocket callbacks to receive `WebSocketContext &` for open, text, binary, close, and error events.
- [ ] Slice 4: update builder/upgrade plumbing and native fixtures for the new callback signature model.
- [ ] Slice 5: wire carried-forward activation data from the current HTTP activation object into `WebSocketActivationSnapshot` and `WebSocketContext`.
- [ ] Slice 5: add tests proving websocket callbacks can still inspect `items()`, headers, and local/remote endpoint data after upgrade.
- [ ] Slice 6: implement documented `WebSocketSendResult` and `WebSocketCloseResult` behavior over the execution-owned outbound queue.
- [ ] Slice 6: add tests for backpressure, closing-state rejection, closed-state rejection, and oversize payload/reason handling.
- [ ] Slice 7: remove transitional runtime naming and simplify websocket docs/tests to the final context/execution model.
- [ ] Run the native websocket-related tests after each implementation slice and the full native test lane at the end.

## Owner

- Copilot

## Priority

- High

## References

- [docs/plans/websocket-context-implementation-plan.md](docs/plans/websocket-context-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md](docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
- [src/websocket/WebSocketSessionRuntime.h](src/websocket/WebSocketSessionRuntime.h)
- [src/websocket/WebSocketSessionRuntime.cpp](src/websocket/WebSocketSessionRuntime.cpp)
- [src/websocket/WebSocketCallbacks.h](src/websocket/WebSocketCallbacks.h)