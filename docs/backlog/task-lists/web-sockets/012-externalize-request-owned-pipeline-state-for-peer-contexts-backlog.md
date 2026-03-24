2026-03-24 - Copilot: completed Phase 2 by defining the slimmed-down HttpRequest shape, the HTTP runner abstraction, and the transitional IPipelineHandler adapter.
2026-03-24 - Copilot: completed Phase 1 by documenting the HttpRequest ownership inventory, target HTTP context/runner seam, and locked behavior-test coverage for the split.
2026-03-24 - Copilot: created design backlog for externalizing HttpRequest-owned pipeline state so future protocol contexts such as WebSocketContext can be peers rather than special cases.

# Externalize Request-Owned Pipeline State For Peer Contexts Backlog

## Summary

The current pipeline is structurally split in two stages, but the HTTP stage is still strongly centered on `HttpRequest`. `HttpPipeline` owns the transport loop and state machine, yet `HttpRequest` owns request parsing callbacks, lazy handler creation, request-phase progression, response/upgraded-session translation, and the lifetime of the current `IHttpHandler`. That makes HTTP feel like the built-in first-class context while upgraded protocols are forced into the narrower `IConnectionSession` slot after `HttpRequest` has already done the interesting orchestration work. This backlog item examines how far that ownership can be externalized so a future `WebSocketContext` can be a peer of `HttpRequest` instead of a child artifact created only after HTTP has finished deciding everything.

## Goal / Acceptance Criteria

- The design clearly distinguishes transport-loop ownership from protocol-context ownership.
- The responsibilities currently bundled into `HttpRequest` are separated into: request data/context, protocol-step orchestration, and final result translation.
- The design identifies which responsibilities can move out of `HttpRequest` without regressing existing HTTP behavior.
- The resulting architecture allows a future `WebSocketContext` to be modeled as a peer protocol context rather than only as an opaque `IConnectionSession` implementation.
- The migration path is incremental and compatible with the existing pipeline/result model until a cleaner shared abstraction is ready.

## Current Ownership Model

- `HttpPipeline` owns the socket, loop timing, parser feeding, response writing, upgraded-session dispatch, and keep-alive restart logic.
- `HttpPipeline` creates one pipeline handler per HTTP request through `handlerFactory_`, stores it in `handler_`, and consumes only `RequestHandlingResult` values from it.
- `HttpRequest` is that pipeline handler today, but it also owns request-specific mutable state: method, version, URL, headers, addresses, items, body byte count, phase flags, cached URI view, current `IHttpHandler`, and current pending result.
- `HttpRequest` lazily creates the route handler through `IHttpRequestHandlerFactory`, retains it for the request lifetime, and decides when to call `handleStep(...)` and `handleBodyChunk(...)`.
- `HttpRequest` translates `HandlerResult` into `RequestHandlingResult`, triggers response-writing phase transitions, and constructs parser-error responses through `IHttpRequestHandlerFactory::createResponse(...)`.
- Upgraded protocols do not get a comparable context/orchestrator object in the pipeline. They are reduced to `IConnectionSession::handle(IClient &, const Compat::Clock &)` after `HttpRequest` yields an upgrade result.

## What Can Be Externalized

- Handler lifetime ownership can move out of `HttpRequest`. The current `handler_` cache is not inherently request-data state; it is protocol-execution state.
- Phase-driven orchestration can move out of `HttpRequest`. The logic that decides when to create the handler, when to call `handleStep(...)`, and when to finalize as `noResponse()` is execution control, not request data.
- `HandlerResult` to `RequestHandlingResult` translation can move out of `HttpRequest`. That translation is part of the pipeline contract boundary, not the request model itself.
- Parser-error response synthesis can move out of `HttpRequest` into a protocol-runner or adapter object, leaving `HttpRequest` as data plus helper methods.
- The pipeline-handler role can be separated from the request-context role so that parser callbacks mutate a context object but are implemented by an orchestrator that is free to own additional protocol state.

## What Probably Stays With HttpRequest Or A Shared Base Context

- Parsed request metadata should remain on a context object: method, version, URL, headers, route items, remote/local addresses, and URI helpers.
- Request-body accumulation bookkeeping should remain available to the HTTP context, even if body-phase control moves outward.
- Response factory convenience such as `createResponse(...)` can remain on the context if it is treated as protocol-local service access rather than orchestration ownership.
- Request-scoped extension storage in `items()` should remain attached to the per-request context, not the pipeline loop.

## Main Architectural Constraint

- `HttpPipeline` currently has exactly two runtime modes after parsing starts: continue driving an `IPipelineHandler` for HTTP, or switch to an `IConnectionSession` after upgrade.
- As long as that split remains, `WebSocketContext` cannot truly be a peer of `HttpRequest`; it can only be an internal detail behind `IConnectionSession`.
- A peer model therefore requires one of two changes:
  - introduce a shared protocol-runner/context seam above both `HttpRequest` and `WebSocketSessionRuntime`, or
  - broaden the pipeline runtime contract so upgraded protocols can expose richer context ownership than `IConnectionSession::handle(...)` alone.

## Candidate End State

- `HttpRequest` becomes primarily an HTTP context object containing parsed request data, HTTP-specific helper services, and request-local storage.
- A separate HTTP protocol runner owns handler creation, phase progression, error mapping, and `HandlerResult` to pipeline-result translation.
- `HttpPipeline` drives protocol runners or protocol contexts through one shared execution seam rather than privileging HTTP with a richer orchestration object.
- `WebSocketContext` can then own websocket-specific state such as outbound queue access, close-state observation, typed event dispatch, and per-connection items without being forced through a minimal session interface.

## Recommended Direction

- Do not try to make `WebSocketContext` a peer of `HttpRequest` by inflating `IConnectionSession` alone; that would still leave HTTP owning the orchestration seam.
- First split `HttpRequest` into two conceptual pieces:
  - an HTTP context/data object
  - an HTTP protocol runner/orchestrator
- After that split, decide whether the pipeline should move to a generalized protocol-runner abstraction or whether upgraded protocols should gain a richer post-upgrade context contract.
- Treat the new `WebSocketContext` as the websocket analogue of the future slimmed-down `HttpRequest`, not as a direct replacement for `WebSocketSessionRuntime`.

## Phased Execution Plan

### Phase 1: Isolate HttpRequest Responsibilities

- Produce an ownership inventory for `HttpRequest` that explicitly separates request data, protocol orchestration, result translation, and error mapping.
- Identify which `HttpRequest` fields belong to immutable or request-scoped context data versus runner-owned mutable execution state.
- Freeze the expected externally visible HTTP behavior with tests before moving orchestration logic.
- Define the target seam between a future HTTP context object and a future HTTP runner so later refactors do not drift.

## Phase 1 Findings

### HttpRequest Ownership Inventory

| Member / Responsibility | Current Role | Recommended Owner After Split |
| --- | --- | --- |
| `method_`, `version_`, `url_` | parsed request metadata | HTTP context |
| `headers_` | parsed request metadata | HTTP context |
| `remoteAddress_`, `remotePort_`, `localAddress_`, `localPort_` | connection/request addressing metadata | HTTP context |
| `items_` | request-scoped extension storage | HTTP context |
| `cachedUriView_` | derived request helper cache | HTTP context |
| `bodyBytesReceived_` | request-body progress bookkeeping | HTTP context, unless body-phase state is later generalized |
| `server_` | request-local service access | HTTP context |
| `handlerFactory_` | route-handler creation and response factory access | split responsibility: route-handler creation belongs to runner; response helper access may remain reachable through the context |
| `handler_` | cached route handler instance for request lifetime | HTTP runner |
| `pendingResult_` | pipeline-facing result staging | HTTP runner or pipeline adapter |
| `completedPhases_` | execution/phase progression state consumed by handlers | HTTP runner-owned progression state, with read-only phase view exposed through context if still needed |
| `tryGetHandler()` | lazy handler creation | HTTP runner |
| `appendBodyContents(...)` | orchestrates body-phase transition and forwards body data | split: body bookkeeping to context, call sequencing to runner |
| `handleStep()` | core orchestration entry point; invokes handler, translates results, finalizes no-response | HTTP runner |
| `sendResponse()` | pipeline result/write-state transition helper | HTTP runner or pipeline adapter |
| `completedStartingLine()`, `completedReadingHeaders()`, `completedReadingMessage()` | parser-phase progression hooks | HTTP runner |
| `startedWritingResponse()`, `completedWritingResponse()` | response-write phase hooks | HTTP runner or pipeline adapter |
| `onMessageBegin(...)`, `onHeader(...)`, `onBody(...)`, `onMessageComplete()` | parser callback entry points that mutate request state and advance execution | split: context mutation plus runner coordination |
| `onError(...)` | parser/pipeline error to HTTP response mapping | HTTP runner |
| `createPipelineHandler(...)` | binds `HttpRequest` to the `IPipelineHandler` seam | transitional adapter during the split; long-term likely replaced by runner creation |

### Phase 1 Classification Summary

- **HTTP context data**: request metadata, header collection, address data, URI helpers, request-scoped items, and lightweight service access helpers.
- **HTTP orchestration state**: cached route handler, phase advancement, result staging, lazy handler creation, response-write callbacks, no-response finalization, and parser-error mapping.
- **Pipeline/result adaptation**: `HandlerResult` to `RequestHandlingResult` translation, response-start/complete phase bridging, and the temporary `IPipelineHandler` adapter surface.

### Target Phase 1 Seam

The split should land on this boundary:

- `HttpRequest` becomes an HTTP context object that holds parsed request data and request-local helpers.
- A new HTTP runner owns execution state and is responsible for:
  - creating the route handler
  - advancing phases based on parser and response-write events
  - invoking `handleStep(...)` and `handleBodyChunk(...)`
  - translating `HandlerResult` into pipeline-consumable results
  - mapping parser/pipeline errors into HTTP responses or terminal outcomes
- During migration, the runner can still implement or back the existing `IPipelineHandler` seam so `HttpPipeline` does not need to be rewritten in the same slice.

### Locked Behavior Tests For The Split

These tests should be treated as the Phase 1 safety net and remain green while ownership moves out of `HttpRequest`:

- Request-level HTTP/request-lifecycle tests in [test/test_native/test_http_request.cpp](test/test_native/test_http_request.cpp)
  - `test_http_request_preserves_custom_method_through_factory_and_handler_steps`
  - `test_http_request_websocket_upgrade_accepts_split_request_and_returns_upgrade_session`
  - `test_http_request_websocket_upgrade_rejects_invalid_requests_with_deterministic_statuses`
- Pipeline integration tests in [test/test_native/test_pipeline.cpp](test/test_native/test_pipeline.cpp)
  - request/response completion, keep-alive reentry, parser-error propagation, timeout handling, response writing, upgrade transition, and unmatched-upgrade fallback coverage
- Body-handler behavior that currently depends on `HttpRequest::createPipelineHandler(...)` in [test/test_native/test_body_handlers.cpp](test/test_native/test_body_handlers.cpp)
- Routing harness coverage that exercises request-context creation through `HttpRequest::createPipelineHandler(...)` in [test/test_native/test_routing.cpp](test/test_native/test_routing.cpp)
- Request utility behavior that depends on `items()` semantics in [test/test_native/test_utilities.cpp](test/test_native/test_utilities.cpp)

Phase 1 conclusion:

- The split is feasible without first changing `HttpPipeline`.
- The immediate extraction target is not "replace `HttpRequest`" but "stop letting `HttpRequest` own orchestration state."
- The cleanest transitional move is an HTTP runner that still satisfies the current pipeline callback expectations while delegating request data storage to `HttpRequest`.

### Phase 2: Extract An HTTP Runner

- Move handler lifetime ownership, lazy creation, phase-driven `handleStep(...)` calls, and `noResponse()` finalization out of `HttpRequest` into a dedicated HTTP runner/adapter.
- Move `HandlerResult` to `RequestHandlingResult` translation and parser-error response synthesis into that same runner.
- Leave `HttpRequest` focused on parsed request data, helper services, and request-local storage.
- Preserve the existing `IPipelineHandler` surface during this extraction so `HttpPipeline` does not need to change simultaneously.

## Phase 2 Findings

### Slimmed-Down HttpRequest Shape

After the split, `HttpRequest` should be reduced to an HTTP context with these responsibilities:

- parsed request metadata and addressing data
  - method
  - version
  - URL
  - headers
  - remote/local address and port
- request-local helper/state access
  - `items()`
  - `uriView()` and URI cache
  - request-body byte count or equivalent body-progress view
- lightweight service access needed by handlers
  - `server()`
  - `createResponse(...)`
- read-only execution hints that handlers may still need
  - a phase/progress view, if the handler model continues to depend on `HttpRequestPhase`

It should no longer directly own:

- cached route handler lifetime
- pending pipeline result staging
- parser-callback driven orchestration
- `HandlerResult` to `RequestHandlingResult` translation
- no-response finalization
- parser-error to HTTP-response mapping

### Proposed HTTP Runner Responsibility

The HTTP runner becomes the execution object that owns everything currently implicit in `HttpRequest`'s private orchestration state:

- hold the current `HttpRequest` context instance
- lazily create the `IHttpHandler` through `IHttpRequestHandlerFactory`
- maintain execution/phase state consumed by handlers
- forward parser events into the context and drive handler execution at the correct seams
- forward body chunks to `handleBodyChunk(...)`
- translate `HandlerResult` into `RequestHandlingResult`
- own pending result staging until `HttpPipeline` consumes it
- synthesize parser/pipeline error responses through `IHttpRequestHandlerFactory::createResponse(...)`

### Minimal Phase 2 Interface Sketch

```cpp
class HttpRequestRunner
{
public:
    virtual ~HttpRequestRunner() = default;

    virtual HttpRequest &context() = 0;

    virtual int onMessageBegin(const char *method,
                               std::uint16_t versionMajor,
                               std::uint16_t versionMinor,
                               std::string_view url) = 0;
    virtual void setAddresses(std::string_view remoteAddress,
                              std::uint16_t remotePort,
                              std::string_view localAddress,
                              std::uint16_t localPort) = 0;
    virtual int onHeader(std::string_view field, std::string_view value) = 0;
    virtual int onHeadersComplete() = 0;
    virtual int onBody(const std::uint8_t *at, std::size_t length) = 0;
    virtual int onMessageComplete() = 0;
    virtual void onError(PipelineError error) = 0;

    virtual void onResponseStarted() = 0;
    virtual void onResponseCompleted() = 0;
    virtual void onClientDisconnected() = 0;

    virtual bool hasPendingResult() const = 0;
    virtual RequestHandlingResult takeResult() = 0;
};
```

This is intentionally close to `IPipelineHandler`, because the first extraction goal is to relocate ownership, not to redesign the pipeline callback contract at the same time.

### Transitional Adapter Shape

The safest migration move is a thin adapter that preserves the existing `IPipelineHandler` surface while delegating all behavior to the new runner.

```cpp
class HttpRequestPipelineAdapter : public IPipelineHandler
{
public:
    explicit HttpRequestPipelineAdapter(std::unique_ptr<HttpRequestRunner> runner);

    int onMessageBegin(const char *method,
                       std::uint16_t versionMajor,
                       std::uint16_t versionMinor,
                       std::string_view url) override;
    void setAddresses(std::string_view remoteAddress,
                      std::uint16_t remotePort,
                      std::string_view localAddress,
                      std::uint16_t localPort) override;
    int onHeader(std::string_view field, std::string_view value) override;
    int onHeadersComplete() override;
    int onBody(const std::uint8_t *at, std::size_t length) override;
    int onMessageComplete() override;
    void onError(PipelineError error) override;
    void onResponseStarted() override;
    void onResponseCompleted() override;
    void onClientDisconnected() override;
    bool hasPendingResult() const override;
    RequestHandlingResult takeResult() override;

private:
    std::unique_ptr<HttpRequestRunner> runner_;
};
```

With that transitional shape:

- `HttpPipeline` continues to consume an `IPipelineHandler`
- the runner owns orchestration and result staging
- `HttpRequest::createPipelineHandler(...)` can become a compatibility factory that constructs the adapter plus runner, rather than directly making `HttpRequest` implement the pipeline contract

### Phase 2 Design Constraints

- The runner should continue to use `IHttpRequestHandlerFactory::create(HttpRequest &)` initially, to avoid simultaneously changing routing/factory seams.
- `HttpRequestPhase` can remain exposed through `HttpRequest` in Phase 2, but the mutable phase state should move to the runner and be projected into the context rather than owned there directly.
- `RequestHandlingResult` remains the runner-to-pipeline currency in Phase 2; redesigning that contract belongs to Phase 3.
- Error-to-response mapping should move wholesale with the runner so HTTP failure behavior stays in one place.

### Phase 2 Conclusion

- The minimal viable split does not require `HttpPipeline` changes yet.
- The key new object is an HTTP runner that mirrors the current `IPipelineHandler` lifecycle while owning orchestration state.
- `HttpRequest` can be slimmed down immediately once the adapter/runner pair exists, because the current pipeline-facing methods do not need to stay on the context type itself.

### Phase 3: Generalize The Pipeline Execution Seam

- Reevaluate the split between `IPipelineHandler` and `IConnectionSession` once HTTP orchestration is no longer embedded in `HttpRequest`.
- Decide whether both HTTP and upgraded protocols should flow through a shared protocol-runner abstraction, or whether one existing seam should be widened to carry richer context ownership.
- Define the minimum shared lifecycle needed by peer protocol contexts: startup, inbound progression, outbound progression, completion, abort, and error signaling.
- Keep `RequestHandlingResult` only if it still cleanly represents the shared execution contract after this generalization.

### Phase 4: Introduce Peer WebSocket Context Ownership

- Replace the websocket runtime's purely internal-context model with a first-class `WebSocketContext` shaped around the generalized execution seam.
- Move websocket-specific mutable state such as outbound queue ownership, close-state inspection, callback context, and connection-local items behind that peer context.
- Keep the websocket protocol runtime logic, but make it operate on or through `WebSocketContext` rather than being the only public-facing runtime object.
- Align the `WebSocketBuilder` and callback/send-api backlogs with the new peer-context model so event handlers target the new context directly.

### Phase 5: Cleanup And Surface Consolidation

- Remove transitional adapters once both HTTP and websocket flows use the new execution architecture.
- Simplify documentation so the project describes one protocol-context model rather than one HTTP model plus websocket exceptions.
- Reclassify tests around context behavior versus runner behavior so ownership boundaries stay clear.
- Identify whether later upgraded protocols should be able to plug into the same peer-context seam without HTTP-specific assumptions.

## Incremental Migration Slices

- Slice 1: document and isolate the current HTTP orchestration responsibilities that do not intrinsically belong to request data.
- Slice 2: extract handler lifetime ownership and phase progression from `HttpRequest` into a dedicated HTTP runner/adapter while preserving the existing `RequestHandlingResult` contract.
- Slice 3: define a shared pipeline-facing abstraction for protocol execution that can represent both HTTP request handling and upgraded connection contexts.
- Slice 4: redesign websocket runtime/context ownership so a future `WebSocketContext` is surfaced through that shared execution abstraction rather than hidden behind a narrow session loop.

## Open Questions

- Should the shared peer abstraction be request-shaped, connection-shaped, or a more generic protocol-execution object?
- Does HTTP remain one-request-per-context while websocket remains one-connection-per-context, or should both be normalized at the connection layer?
- How much of `HttpRequestPhase` should survive once orchestration moves out of `HttpRequest`?
- Should parser callbacks continue to target the context directly, or should they target the orchestrator which then mutates the context?
- Can `RequestHandlingResult` remain the shared pipeline currency, or does a peer-context design want a broader execution-result model?

## Tasks

- [x] Phase 1: document the exact orchestration responsibilities currently owned by `HttpRequest` that are not inherently request-data responsibilities.
- [x] Phase 1: classify current `HttpRequest` fields and methods into context-owned versus runner-owned responsibilities.
- [x] Phase 1: identify and lock the HTTP behavior tests that must remain green through the split.
- [x] Phase 2: design a slimmed-down HTTP context shape that can survive without directly owning handler lifetime and pipeline-result translation.
- [x] Phase 2: design an HTTP runner/orchestrator abstraction that can own handler creation, phase advancement, and error-to-response mapping.
- [x] Phase 2: define the transitional adapter that preserves the existing `IPipelineHandler` integration while HTTP orchestration moves outward.
- [ ] Phase 3: decide whether the pipeline should generalize above `IPipelineHandler` and `IConnectionSession`, or whether one of those seams should absorb the peer-context design.
- [ ] Phase 3: define the shared execution lifecycle and result contract required for peer protocol contexts.
- [ ] Phase 4: define how a future `WebSocketContext` would map onto the chosen shared execution seam.
- [ ] Phase 4: update websocket context/send-api planning to align with the chosen peer-context architecture.
- [ ] Phase 4: identify which websocket runtime responsibilities migrate into `WebSocketContext` versus remaining in a protocol runner.
- [ ] Phase 5: identify which existing HTTP tests would need to move from `HttpRequest` behavior tests to protocol-runner behavior tests after the split.
- [ ] Phase 5: plan cleanup/removal of transitional adapters once both HTTP and websocket paths use the new model.

## Immediate Next Slice

- Start Phase 3 by deciding whether the long-term shared execution seam should replace `IPipelineHandler`, replace `IConnectionSession`, or sit above both.

## Owner

- Copilot

## Priority

- High

## References

- [src/core/HttpRequest.h](src/core/HttpRequest.h)
- [src/core/HttpRequest.cpp](src/core/HttpRequest.cpp)
- [src/core/IHttpRequestHandlerFactory.h](src/core/IHttpRequestHandlerFactory.h)
- [src/pipeline/HttpPipeline.h](src/pipeline/HttpPipeline.h)
- [src/pipeline/HttpPipeline.cpp](src/pipeline/HttpPipeline.cpp)
- [src/pipeline/ConnectionSession.h](src/pipeline/ConnectionSession.h)
- [src/websocket/WebSocketSessionRuntime.h](src/websocket/WebSocketSessionRuntime.h)
- [src/websocket/WebSocketSessionRuntime.cpp](src/websocket/WebSocketSessionRuntime.cpp)
- [docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md](docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md)
- [docs/backlog/task-lists/web-sockets/010-normalize-provider-registry-builder-websocket-surface-backlog.md](docs/backlog/task-lists/web-sockets/010-normalize-provider-registry-builder-websocket-surface-backlog.md)