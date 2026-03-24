# WebSocket Context Implementation Plan

## Summary

This plan turns the completed Phase 4 websocket design work into an implementation sequence. The target architecture replaces the current fused `WebSocketSessionRuntime` model with a split between `WebSocketContext`, which owns callback-facing state and carried-forward HTTP activation data, and `WebSocketProtocolExecution`, which owns parser progress, outbound write progression, and pipeline lifecycle behavior.

The plan assumes the design decisions already recorded in the websocket backlogs:

- `WebSocketContext` directly owns the callback bundle
- `WebSocketContext` exposes the public send API and close/state inspection API
- `WebSocketProtocolExecution` owns frame parsing, handshake/output progression, and close-handshake mechanics
- the HTTP-side activation object is moving toward `HttpContext` as the long-term name, even though current code still uses `HttpRequest`

## Implementation Goal

Deliver the websocket context/execution split incrementally without regressing the existing websocket route behavior, handshake flow, or native test coverage.

Success means:

- websocket callbacks run through `WebSocketContext &`
- outbound sends route through context methods and execution-owned queue admission
- carried-forward HTTP activation data is available after upgrade
- the existing websocket runtime logic is preserved, but reorganized behind the new ownership model
- the implementation remains compatible with the current pipeline seam during transition

## Current Starting Point

Today [src/websocket/WebSocketSessionRuntime.h](src/websocket/WebSocketSessionRuntime.h) and [src/websocket/WebSocketSessionRuntime.cpp](src/websocket/WebSocketSessionRuntime.cpp) combine:

- callback ownership
- outbound queue ownership
- parser and fragmented-message state
- close lifecycle state
- `IConnectionSession` transport progression

The backlog design now requires those concerns to be split instead of renamed in place.

## Target End State

### `WebSocketContext`

Owns:

- carried-forward activation snapshot copied or transferred from HTTP upgrade time
- callback bundle
- public send/close methods
- websocket-visible lifecycle state
- close code / reason once known
- upgraded-connection `items()` storage

Exposes:

- `sendText(...)`
- `sendBinary(...)`
- `close(...)`
- `isOpen()`
- `isClosing()`
- `closeCode()`
- `closeReason()`
- carried-forward HTTP metadata accessors

### `WebSocketProtocolExecution`

Owns:

- `IConnectionSession` implementation during transition
- websocket parser state
- pending serialized write buffer and partial flush offset
- fragmented-message accumulation state
- handshake write progression
- close-handshake stage and completion logic
- execution-to-context callback dispatch timing

### Upgrade Handoff

The upgrade path should explicitly construct a websocket-owned startup snapshot, for example:

```cpp
struct WebSocketActivationSnapshot
{
    ItemCollection items;
    HttpMethod method;
    HttpVersion version;
    HttpHeaderCollection headers;
    EndpointInfo localEndpoint;
    EndpointInfo remoteEndpoint;
};

struct WebSocketUpgradeStart
{
    WebSocketActivationSnapshot activation;
    std::string handshakeResponse;
    WebSocketCallbacks callbacks;
};
```

Exact type names can still change, but the handoff itself should be explicit.

## Proposed Implementation Slices

### Slice 1: Introduce Context And Snapshot Types

Add the basic types without changing runtime behavior yet.

Create:

- `src/websocket/WebSocketContext.h`
- `src/websocket/WebSocketContext.cpp`
- `src/websocket/WebSocketActivationSnapshot.h`

Initial responsibilities:

- define the context-owned state model
- store callback bundle directly on `WebSocketContext`
- define public accessors for carried-forward HTTP metadata
- define public send/close/state-inspection method declarations
- define internal hook-up to a narrow execution control interface

Validation:

- compile-only integration at first
- add native unit tests for pure context state and accessor behavior if feasible

### Slice 2: Introduce Execution Control Interface

Add the narrow seam that context will use to request outbound actions.

Create:

- `src/websocket/IWebSocketSessionControl.h`

Responsibilities:

- `tryQueueText(...)`
- `tryQueueBinary(...)`
- `tryStartClose(...)`
- state/query helpers needed by `WebSocketContext`

This slice should keep transport details hidden from context while making the public send API implementable.

Validation:

- native unit tests for context send methods using a fake `IWebSocketSessionControl`
- verify send-result mapping is deterministic

### Slice 3: Extract `WebSocketProtocolExecution`

Split the current runtime class into a new execution class while preserving current behavior.

Create:

- `src/websocket/WebSocketProtocolExecution.h`
- `src/websocket/WebSocketProtocolExecution.cpp`

Refactor:

- move parser, pending-write, fragmented-message, and close-handshake progression out of `WebSocketSessionRuntime`
- have `WebSocketProtocolExecution` implement `IConnectionSession` during transition
- construct and own a `WebSocketContext`
- invoke callbacks through context-owned callback storage

At the end of this slice:

- `WebSocketSessionRuntime` should either be deleted or reduced to a short-lived compatibility wrapper, depending on how much call-site churn exists

Validation:

- existing native websocket tests remain green
- no behavioral regression in handshake, text/binary receive, automatic pong, or close handling

### Slice 4: Convert Callback Surface To `WebSocketContext &`

Update websocket callback types and builder plumbing so runtime callbacks receive context directly.

Likely touch:

- `src/websocket/WebSocketCallbacks.h`
- `src/websocket/WebSocketUpgradeHandler.h`
- `src/websocket/WebSocketUpgradeHandler.cpp`
- routing/builder files that currently assemble raw `WebSocketCallbacks`

Responsibilities:

- change `onOpen`, `onText`, `onBinary`, `onClose`, and `onError` to accept `WebSocketContext &`
- preserve the current raw callback aggregate temporarily if a compatibility layer is still needed internally
- ensure builder-time registration can target the new callback shape cleanly

Validation:

- native tests for callback invocation arguments
- update any test fixtures that currently expect receive-only callback signatures

### Slice 5: Wire Carried-Forward HTTP Activation Data

Update the websocket upgrade flow so the new context receives the HTTP snapshot at upgrade time.

Likely touch:

- `src/websocket/WebSocketUpgradeHandler.h`
- `src/websocket/WebSocketUpgradeHandler.cpp`
- HTTP-side upgrade result creation path
- `src/core/HttpRequest.h`
- `src/core/HttpRequest.cpp`

Responsibilities:

- build the activation snapshot from the current HTTP activation object
- transfer or copy `items()`, method, version, headers, and endpoint information
- ensure upgraded callbacks continue to see stable metadata after the HTTP phase ends

Validation:

- native tests proving carried-forward metadata is available after upgrade
- regression coverage for authentication/request-item propagation if present in tests

### Slice 6: Finalize Send Result Semantics

Implement the documented `WebSocketSendResult` and `WebSocketCloseResult` behavior over the execution-owned queue.

Likely touch:

- `src/websocket/WebSocketContext.*`
- `src/websocket/WebSocketProtocolExecution.*`

Responsibilities:

- map queue admission success to `Queued` / `CloseQueued`
- map occupied pending-write buffer to `Backpressured`
- map close-started states to `Closing` / `AlreadyClosing`
- map closed states to `Closed` / `AlreadyClosed`
- validate oversize payload and reason cases

Validation:

- unit tests around send/close result enums
- integration tests around send rejection during pending-write and close progression

### Slice 7: Remove Transitional Runtime Naming

Once execution is stable, clean up the old runtime identity.

Responsibilities:

- delete `WebSocketSessionRuntime` if it remains as a compatibility wrapper
- update references to the new execution/context types
- simplify docs to refer to the new architecture directly

Validation:

- full native test lane
- grep check that no obsolete runtime-only naming survives in public-facing docs or relevant code paths

## File Impact Map

Primary new files:

- `src/websocket/WebSocketContext.h`
- `src/websocket/WebSocketContext.cpp`
- `src/websocket/WebSocketActivationSnapshot.h`
- `src/websocket/IWebSocketSessionControl.h`
- `src/websocket/WebSocketProtocolExecution.h`
- `src/websocket/WebSocketProtocolExecution.cpp`

Primary existing files likely to change:

- `src/websocket/WebSocketSessionRuntime.h`
- `src/websocket/WebSocketSessionRuntime.cpp`
- `src/websocket/WebSocketCallbacks.h`
- `src/websocket/WebSocketUpgradeHandler.h`
- `src/websocket/WebSocketUpgradeHandler.cpp`
- `src/core/HttpRequest.h`
- `src/core/HttpRequest.cpp`
- websocket-related routing/builder files
- websocket native tests and fixtures

## Testing Plan

### Unit-Level Coverage

- `WebSocketContext` accessor/state tests
- send/close result mapping with fake session control
- activation snapshot ownership tests

### Integration-Level Coverage

- upgrade handshake still produces a functioning websocket execution object
- callbacks receive `WebSocketContext &`
- carried-forward headers/items/endpoints remain visible after upgrade
- send rejection during backpressure and closing states behaves as documented

### Regression Coverage

- existing websocket receive tests
- close-handshake behavior
- automatic pong behavior
- oversize message handling

## Key Risks

- callback signature changes may ripple into builder/routing code earlier than expected
- `items()` transfer semantics may be awkward if some HTTP-side code still expects the old object to retain ownership after upgrade
- preserving exact close/error callback ordering while moving state out of the runtime will require careful tests
- if too much compatibility scaffolding is kept, the refactor could become harder to reason about rather than easier

## Recommended First Code Slice

Start with Slice 1 plus Slice 2 together:

- add `WebSocketContext`
- add `WebSocketActivationSnapshot`
- add `IWebSocketSessionControl`
- unit-test the context/send-result mapping against a fake control object

That gives the project a stable surface to build against before the runtime split begins.

## References

- [docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md](docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
- [docs/plans/websocket-support-plan.md](docs/plans/websocket-support-plan.md)
- [src/websocket/WebSocketSessionRuntime.h](src/websocket/WebSocketSessionRuntime.h)
- [src/websocket/WebSocketSessionRuntime.cpp](src/websocket/WebSocketSessionRuntime.cpp)
- [src/websocket/WebSocketCallbacks.h](src/websocket/WebSocketCallbacks.h)