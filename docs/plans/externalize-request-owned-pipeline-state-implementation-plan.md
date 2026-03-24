# Externalize Request-Owned Pipeline State Implementation Plan

## Summary

This plan covers the full implementation path for the work described in [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md), not just the websocket-side context split. The architectural target is a staged peer-context model where HTTP activation remains request-shaped, upgraded protocols gain first-class context ownership, and long-term execution can be generalized above both the current `IPipelineHandler` and `IConnectionSession` seams.

The plan therefore spans four connected tracks:

- extracting HTTP orchestration out of `HttpRequest`
- reshaping `HttpRequest` toward the future `HttpContext`
- introducing websocket-owned context and execution types
- preparing the shared protocol-execution seam that eventually lets `HttpPipeline` drive both HTTP and websocket through one conceptual model

## Why The Previous Plan Was Incomplete

The earlier websocket-only plan captured the `WebSocketContext` and `WebSocketProtocolExecution` split, but it did not include the upstream work required by the broader `012` design:

- `HttpRequest` currently still owns handler lifetime, phase progression, and result staging
- routing/activation still depends on the HTTP activation object at the headers-complete seam
- the long-term design calls for `HttpRequest` to become `HttpContext`
- the long-term execution model calls for a higher-level abstraction above both current pipeline seams

Without those pieces, the websocket refactor would land as an isolated local cleanup rather than as part of the intended peer-context architecture.

## Implementation Goal

Deliver the peer-context architecture incrementally without regressing existing HTTP or websocket behavior.

Success means:

- HTTP activation/routing still happens against the HTTP activation context after headers complete
- `HttpRequest` no longer owns handler lifetime, result translation, and pipeline orchestration
- websocket execution no longer hides behind a fused `WebSocketSessionRuntime`
- `WebSocketContext` directly owns callback-visible state and carried-forward HTTP activation data
- the codebase is positioned to rename `HttpRequest` to `HttpContext`
- the codebase is positioned to introduce the higher-level `IProtocolExecution` seam above the current HTTP and upgraded-session seams

## Architectural Constraints From Backlog `012`

### 1. Activation Still Belongs To HTTP

- handler selection cannot happen until HTTP headers are complete
- activation predicates must still evaluate against the HTTP activation context
- websocket is therefore a post-activation peer context, not a replacement for HTTP activation

### 2. `HttpRequest` Is Overloaded Today

Today `HttpRequest` still combines:

- parsed request data and helper access
- route-handler lifetime ownership
- parser-driven phase progression
- `HandlerResult` to `RequestHandlingResult` translation
- error-to-response mapping

That ownership must be split before the broader peer-context model is complete.

### 3. Websocket Needs A First-Class Context

The websocket side needs:

- carried-forward HTTP activation data
- direct callback ownership on `WebSocketContext`
- a send/close API that delegates into execution-owned queue control
- execution-owned parser, write-progress, and close-handshake state

### 4. The Final Model Is Broader Than Websocket

The long-term recommendation in `012` is not just "split websocket runtime". It is:

- `HttpContext` plus `HttpProtocolExecution`
- `WebSocketContext` plus `WebSocketProtocolExecution`
- eventually, a shared `IProtocolExecution` seam above both

## Target End State

### HTTP Side

#### `HttpRequest` During Transition

Short-term, the existing `HttpRequest` type should become a slim HTTP context object that mainly owns:

- method, version, URL, headers
- local/remote endpoint data
- `items()`
- URI helpers and lightweight service access

It should stop owning:

- cached handler lifetime
- pending pipeline result staging
- phase progression state
- error-to-response orchestration

#### Future `HttpContext`

Once the split is stable, `HttpRequest` should be renamed to `HttpContext` so the public model reads as peer contexts:

- `HttpContext`
- `WebSocketContext`

#### `HttpRequestRunner` / `HttpProtocolExecution`

During transition, the HTTP side needs a runner that owns:

- handler creation
- phase advancement
- result staging
- `HandlerResult` translation
- parser/pipeline error mapping

This may land first as a runner plus `IPipelineHandler` adapter before the broader protocol-execution seam exists.

### Websocket Side

#### `WebSocketContext`

Owns:

- carried-forward HTTP activation snapshot
- callback bundle directly
- send/close methods
- lifecycle/close state visible to callbacks
- upgraded-connection `items()`

#### `WebSocketProtocolExecution`

Owns:

- parser state
- pending write buffer and flush offset
- fragmented-message state
- handshake progression
- close-handshake sequencing
- execution-to-context callback timing

### Shared Seam

Long-term, the pipeline-facing model should become a shared execution abstraction, recommended in `012` as:

```cpp
class IProtocolExecution
{
public:
    virtual ~IProtocolExecution() = default;

    virtual void start() = 0;
    virtual void onInboundProgress() = 0;
    virtual void onOutboundProgress() = 0;
    virtual void onError(PipelineError error) = 0;
    virtual void onDisconnect() = 0;

    virtual bool hasPendingResult() const = 0;
    virtual RequestHandlingResult takeResult() = 0;
    virtual bool isFinished() const = 0;
};
```

That seam should not be forced into the earliest code slices, but the implementation sequence should move toward it rather than away from it.

## Proposed Implementation Sequence

### Slice 1: Introduce HTTP Runner Ownership Under The Existing Pipeline Seam

Goal:

- stop letting `HttpRequest` own orchestration state

Create:

- an `HttpRequestRunner`-style abstraction
- an `HttpRequestPipelineAdapter`-style adapter if needed

Responsibilities to move out of `HttpRequest` first:

- handler lifetime ownership
- pending result staging
- parser-phase advancement
- response-start/complete phase callbacks
- error-to-response mapping

Validation:

- existing HTTP request and pipeline tests remain green
- no websocket behavior changes yet

### Slice 2: Slim `HttpRequest` Toward The Future `HttpContext`

Goal:

- leave `HttpRequest` as context-shaped data plus helper access

Responsibilities retained on `HttpRequest`:

- method, version, URL, headers
- endpoint information
- `items()`
- URI helpers
- response factory convenience if still needed

Validation:

- request utility behavior stays stable
- handler code still sees the same request-context data during transition

### Slice 3: Introduce Websocket Activation Snapshot, Context, And Control Interface

Goal:

- establish websocket-owned state and API surfaces before runtime extraction

Create:

- `WebSocketActivationSnapshot`
- `WebSocketContext`
- `IWebSocketSessionControl`

`WebSocketContext` should own:

- callback bundle directly
- carried-forward activation data
- send/close declarations
- lifecycle and close metadata

Validation:

- focused unit tests for snapshot ownership and context-owned state
- send-result mapping tests using a fake session-control object

### Slice 4: Extract `WebSocketProtocolExecution` Under The Existing Upgrade Seam

Goal:

- split `WebSocketSessionRuntime` into context-owned state and execution-owned state

Create:

- `WebSocketProtocolExecution`

Move into execution:

- parser
- pending write buffer
- flush offset
- fragmented-message state
- handshake progression
- close-handshake sequencing

Validation:

- websocket pipeline tests stay green
- automatic pong and close behavior remain unchanged

### Slice 5: Convert Websocket Callback Surfaces To `WebSocketContext &`

Goal:

- align callback signatures with the context-first design

Touch:

- `WebSocketCallbacks`
- upgrade handler plumbing
- builder/routing paths that register websocket callbacks
- websocket-related fixtures/tests

Validation:

- callbacks receive `WebSocketContext &`
- context-owned callback storage is used end-to-end

### Slice 6: Wire The HTTP-To-Websocket Handoff Explicitly

Goal:

- make the activation-to-upgrade transfer explicit and websocket-owned

Responsibilities:

- build the activation snapshot from the current HTTP activation object
- transfer or copy `items()`, method, version, headers, and endpoint data
- ensure websocket callbacks can still inspect that data after HTTP activation has finished

Validation:

- tests for carried-forward metadata and items after upgrade

### Slice 7: Introduce The Shared Protocol-Execution Seam

Goal:

- begin replacing the conceptual split between `IPipelineHandler` and `IConnectionSession`

Responsibilities:

- define the concrete `IProtocolExecution` shape if it still matches the Phase 3 design
- map the HTTP runner to that seam
- map websocket execution to that seam
- keep transitional adapters where necessary so `HttpPipeline` does not have to be rewritten all at once

Validation:

- no behavior regression while both old seams and the new seam coexist temporarily

### Slice 8: Simplify `HttpPipeline` Around Staged Protocol Execution

Goal:

- make the pipeline semantically aware of staged execution rather than HTTP-vs-upgraded special casing

Responsibilities:

- keep HTTP activation/routing as the first stage
- allow execution to continue as HTTP or transition to websocket through the shared execution model
- remove duplication introduced by temporary adapters where possible

Validation:

- keep-alive, response completion, upgrade transition, and close behavior remain correct

### Slice 9: Final Naming And Adapter Cleanup

Goal:

- leave the codebase expressing the intended architecture directly

Responsibilities:

- rename `HttpRequest` to `HttpContext`
- remove transitional adapters and compatibility wrappers no longer needed
- delete `WebSocketSessionRuntime` if still present
- update docs and tests to final naming

Validation:

- full native test lane
- targeted search for stale runtime/transitional naming

## File Impact Map

Primary HTTP files likely to change:

- `src/core/HttpRequest.h`
- `src/core/HttpRequest.cpp`
- `src/core/IHttpRequestHandlerFactory.h`
- `src/core/HttpRequestHandlerFactory.h`
- `src/pipeline/HttpPipeline.h`
- `src/pipeline/HttpPipeline.cpp`
- `src/pipeline/IPipelineHandler.h`

Primary websocket files likely to change:

- `src/websocket/WebSocketCallbacks.h`
- `src/websocket/WebSocketSessionRuntime.h`
- `src/websocket/WebSocketSessionRuntime.cpp`
- `src/websocket/WebSocketUpgradeHandler.h`
- `src/websocket/WebSocketUpgradeHandler.cpp`

Primary new files likely to be introduced:

- `src/websocket/WebSocketActivationSnapshot.h`
- `src/websocket/WebSocketContext.h`
- `src/websocket/WebSocketContext.cpp`
- `src/websocket/IWebSocketSessionControl.h`
- `src/websocket/WebSocketProtocolExecution.h`
- `src/websocket/WebSocketProtocolExecution.cpp`
- HTTP runner / adapter files once named concretely
- shared protocol-execution seam files once named concretely

Test and support files likely to change:

- `test/test_native/test_http_request.cpp`
- `test/test_native/test_pipeline.cpp`
- websocket-related fixtures in `test/support/**`
- any tests that currently assume `HttpRequest` owns orchestration or websocket callbacks are receive-only

## Testing Plan

### HTTP Regression Coverage

- request lifecycle tests tied to `HttpRequest`
- pipeline integration tests for parser events, response progression, keep-alive, and error mapping
- body-handler and routing tests that currently depend on `HttpRequest::createPipelineHandler(...)`

### Websocket Regression Coverage

- handshake acceptance/rejection
- automatic pong behavior
- close-handshake behavior
- protocol error mapping
- upgrade transition from HTTP pipeline to websocket execution

### New Coverage Needed

- HTTP runner-owned orchestration state tests
- context-owned websocket callback/state tests
- activation snapshot ownership and carry-forward tests
- send/close result mapping tests
- staged execution tests once the shared protocol seam begins to land

## Key Risks

- moving HTTP orchestration and websocket ownership independently could create overlapping transitional layers if the slices are not kept disciplined
- renaming `HttpRequest` too early could add churn before the context/runner split is actually stable
- keeping `HttpPipeline` unchanged for too long could entrench adapter complexity, but changing it too early could destabilize the refactor
- callback ordering and close-handshake timing are easy to regress when state ownership moves out of the fused websocket runtime
- `items()` transfer semantics at upgrade time need to be explicit so HTTP and websocket do not both assume ownership of the same mutable state

## Recommended First Code Slice

The best actual starting point is broader than the old websocket-only plan.

Start with:

1. HTTP runner extraction under the existing `IPipelineHandler` seam.
2. Then introduce `WebSocketActivationSnapshot`, `WebSocketContext`, and `IWebSocketSessionControl` as stable websocket-side types.

That sequence matches the `012` design more closely because it starts with the root architectural overload in `HttpRequest` instead of jumping immediately to the websocket half of the model.

## References

- [docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md](docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
- [docs/plans/websocket-support-plan.md](docs/plans/websocket-support-plan.md)