2026-03-24 - Copilot: clarified that websocket callback ownership should stay directly on `WebSocketContext` rather than being factored into a separate dispatch object.
2026-03-24 - Copilot: aligned the peer-context naming plan so the target HTTP context class is `HttpContext`, with `HttpContext` treated as the transitional current-code name until the split lands.
2026-03-24 - Copilot: completed Phase 3 by comparing seam options, defining the shared execution lifecycle, and recommending a higher-level protocol execution abstraction above both existing pipeline seams.
2026-03-24 - Copilot: completed Phase 2 by defining the slimmed-down HttpContext shape, the HTTP runner abstraction, and the transitional IPipelineHandler adapter.
2026-03-24 - Copilot: completed Phase 1 by documenting the HttpContext ownership inventory, target HTTP context/runner seam, and locked behavior-test coverage for the split.
2026-03-24 - Copilot: created design backlog for externalizing HttpContext-owned pipeline state so future protocol contexts such as WebSocketContext can be peers rather than special cases.

# Externalize Request-Owned Pipeline State For Peer Contexts Backlog

## Summary

The current pipeline is structurally split in two stages, but the HTTP stage is still strongly centered on `HttpContext`. `HttpPipeline` owns the transport loop and state machine, yet `HttpContext` owns request parsing callbacks, lazy handler creation, request-phase progression, response/upgraded-session translation, and the lifetime of the current `IHttpHandler`. That makes HTTP feel like the built-in first-class context while upgraded protocols are forced into the narrower `IConnectionSession` slot after `HttpContext` has already done the interesting orchestration work. This backlog item examines how far that ownership can be externalized so a future `WebSocketContext` can be a peer of the HTTP activation context instead of a child artifact created only after HTTP has finished deciding everything. The target end-state name for that HTTP context should be `HttpContext`, with `HttpContext` treated as the transitional current-code name until the split is complete.

## Goal / Acceptance Criteria

- The design clearly distinguishes transport-loop ownership from protocol-context ownership.
- The responsibilities currently bundled into `HttpContext` are separated into: request data/context, protocol-step orchestration, and final result translation.
- The design identifies which responsibilities can move out of `HttpContext` without regressing existing HTTP behavior.
- The resulting architecture allows a future `WebSocketContext` to be modeled as a peer protocol context rather than only as an opaque `IConnectionSession` implementation.
- The migration path is incremental and compatible with the existing pipeline/result model until a cleaner shared abstraction is ready.
- The long-term naming plan is explicit: once orchestration is externalized, `HttpContext` should be renamed to `HttpContext` so the HTTP and websocket context types use parallel names.

## Current Ownership Model

- `HttpPipeline` owns the socket, loop timing, parser feeding, response writing, upgraded-session dispatch, and keep-alive restart logic.
- `HttpPipeline` creates one pipeline handler per HTTP request through `handlerFactory_`, stores it in `handler_`, and consumes only `RequestHandlingResult` values from it.
- `HttpContext` is that pipeline handler today, but it also owns request-specific mutable state: method, version, URL, headers, addresses, items, body byte count, phase flags, cached URI view, current `IHttpHandler`, and current pending result.
- `HttpContext` lazily creates the route handler through `IHttpRequestHandlerFactory`, retains it for the request lifetime, and decides when to call `handleStep(...)` and `handleBodyChunk(...)`.
- `HttpContext` translates `HandlerResult` into `RequestHandlingResult`, triggers response-writing phase transitions, and constructs parser-error responses through `IHttpRequestHandlerFactory::createResponse(...)`.
- Upgraded protocols do not get a comparable context/orchestrator object in the pipeline. They are reduced to `IConnectionSession::handle(IClient &, const Compat::Clock &)` after `HttpContext` yields an upgrade result.

## What Can Be Externalized

- Handler lifetime ownership can move out of `HttpContext`. The current `handler_` cache is not inherently request-data state; it is protocol-execution state.
- Phase-driven orchestration can move out of `HttpContext`. The logic that decides when to create the handler, when to call `handleStep(...)`, and when to finalize as `noResponse()` is execution control, not request data.
- `HandlerResult` to `RequestHandlingResult` translation can move out of `HttpContext`. That translation is part of the pipeline contract boundary, not the request model itself.
- Parser-error response synthesis can move out of `HttpContext` into a protocol-runner or adapter object, leaving `HttpContext` as data plus helper methods.
- The pipeline-handler role can be separated from the request-context role so that parser callbacks mutate a context object but are implemented by an orchestrator that is free to own additional protocol state.

## What Probably Stays With HttpContext / Future HttpContext Or A Shared Base Context

- Parsed request metadata should remain on a context object: method, version, URL, headers, route items, remote/local addresses, and URI helpers.
- Request-body accumulation bookkeeping should remain available to the HTTP context, even if body-phase control moves outward.
- Response factory convenience such as `createResponse(...)` can remain on the context if it is treated as protocol-local service access rather than orchestration ownership.
- Request-scoped extension storage in `items()` should remain attached to the per-request context, not the pipeline loop.

## Main Architectural Constraint

- `HttpPipeline` currently has exactly two runtime modes after parsing starts: continue driving an `IPipelineHandler` for HTTP, or switch to an `IConnectionSession` after upgrade.
- As long as that split remains, `WebSocketContext` cannot truly be a peer of `HttpContext`; it can only be an internal detail behind `IConnectionSession`.
- A peer model therefore requires one of two changes:
  - introduce a shared protocol-runner/context seam above both `HttpContext` and `WebSocketSessionRuntime`, or
  - broaden the pipeline runtime contract so upgraded protocols can expose richer context ownership than `IConnectionSession::handle(...)` alone.

## Candidate End State

- `HttpContext` becomes primarily an HTTP context object containing parsed request data, HTTP-specific helper services, and request-local storage.
- A separate HTTP protocol runner owns handler creation, phase progression, error mapping, and `HandlerResult` to pipeline-result translation.
- `HttpPipeline` drives protocol runners or protocol contexts through one shared execution seam rather than privileging HTTP with a richer orchestration object.
- `WebSocketContext` can then own websocket-specific state such as outbound queue access, close-state observation, typed event dispatch, and per-connection items without being forced through a minimal session interface.

## Naming Direction: HttpContext Should Become HttpContext

The current class name no longer matches the architecture being designed here.

- In the target model, the HTTP-side object is not primarily just the raw request; it is the callback-facing HTTP context used during activation and request handling.
- `WebSocketContext` is the peer protocol context name already implied by the websocket design.
- Keeping the HTTP name as `HttpContext` while the websocket side becomes `WebSocketContext` would preserve an unnecessary asymmetry in the public model.

Recommended naming direction:

- current code name during transition: `HttpContext`
- target architectural name after the split: `HttpContext`

Why rename instead of keeping the old name:

- It makes the HTTP and websocket sides read as peers: `HttpContext` and `WebSocketContext`.
- It better describes the post-split responsibility set, which is context/state plus helper access rather than runner/orchestration ownership.
- It reduces the chance that future API discussions accidentally treat the HTTP object as only raw request data while websocket gets the richer "context" framing.

Migration implication:

- Early slices can continue to discuss and use `HttpContext` where required by current code.
- Once the orchestration split is in place and the HTTP object is genuinely context-shaped, the class rename should be treated as part of the architecture completion work rather than as optional cleanup.

## Recommended Direction

- Do not try to make `WebSocketContext` a peer of `HttpContext` by inflating `IConnectionSession` alone; that would still leave HTTP owning the orchestration seam.
- First split `HttpContext` into two conceptual pieces:
  - an HTTP context/data object
  - an HTTP protocol runner/orchestrator
- After that split, decide whether the pipeline should move to a generalized protocol-runner abstraction or whether upgraded protocols should gain a richer post-upgrade context contract.
- Treat the new `WebSocketContext` as the websocket analogue of the future slimmed-down `HttpContext`, not as a direct replacement for `WebSocketSessionRuntime`.

## Phased Execution Plan

### Phase 1: Isolate HttpContext Responsibilities

- Produce an ownership inventory for `HttpContext` that explicitly separates request data, protocol orchestration, result translation, and error mapping.
- Identify which `HttpContext` fields belong to immutable or request-scoped context data versus runner-owned mutable execution state.
- Freeze the expected externally visible HTTP behavior with tests before moving orchestration logic.
- Define the target seam between a future HTTP context object and a future HTTP runner so later refactors do not drift.

## Phase 1 Findings

### HttpContext Ownership Inventory

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
| `createPipelineHandler(...)` | binds `HttpContext` to the `IPipelineHandler` seam | transitional adapter during the split; long-term likely replaced by runner creation |

### Phase 1 Classification Summary

- **HTTP context data**: request metadata, header collection, address data, URI helpers, request-scoped items, and lightweight service access helpers.
- **HTTP orchestration state**: cached route handler, phase advancement, result staging, lazy handler creation, response-write callbacks, no-response finalization, and parser-error mapping.
- **Pipeline/result adaptation**: `HandlerResult` to `RequestHandlingResult` translation, response-start/complete phase bridging, and the temporary `IPipelineHandler` adapter surface.

### Target Phase 1 Seam

The split should land on this boundary:

- `HttpContext` becomes an HTTP context object that holds parsed request data and request-local helpers.
- A new HTTP runner owns execution state and is responsible for:
  - creating the route handler
  - advancing phases based on parser and response-write events
  - invoking `handleStep(...)` and `handleBodyChunk(...)`
  - translating `HandlerResult` into pipeline-consumable results
  - mapping parser/pipeline errors into HTTP responses or terminal outcomes
- During migration, the runner can still implement or back the existing `IPipelineHandler` seam so `HttpPipeline` does not need to be rewritten in the same slice.

### Locked Behavior Tests For The Split

These tests should be treated as the Phase 1 safety net and remain green while ownership moves out of `HttpContext`:

- Request-level HTTP/request-lifecycle tests in [test/test_native/test_http_context.cpp](test/test_native/test_http_context.cpp)
  - `test_http_context_preserves_custom_method_through_factory_and_handler_steps`
  - `test_http_context_websocket_upgrade_accepts_split_request_and_returns_upgrade_session`
  - `test_http_context_websocket_upgrade_rejects_invalid_requests_with_deterministic_statuses`
- Pipeline integration tests in [test/test_native/test_pipeline.cpp](test/test_native/test_pipeline.cpp)
  - request/response completion, keep-alive reentry, parser-error propagation, timeout handling, response writing, upgrade transition, and unmatched-upgrade fallback coverage
- Body-handler behavior that currently depends on `HttpContext::createPipelineHandler(...)` in [test/test_native/test_body_handlers.cpp](test/test_native/test_body_handlers.cpp)
- Routing harness coverage that exercises request-context creation through `HttpContext::createPipelineHandler(...)` in [test/test_native/test_routing.cpp](test/test_native/test_routing.cpp)
- Request utility behavior that depends on `items()` semantics in [test/test_native/test_utilities.cpp](test/test_native/test_utilities.cpp)

Phase 1 conclusion:

- The split is feasible without first changing `HttpPipeline`.
- The immediate extraction target is not "replace `HttpContext`" but "stop letting `HttpContext` own orchestration state."
- The cleanest transitional move is an HTTP runner that still satisfies the current pipeline callback expectations while delegating request data storage to `HttpContext`.

### Phase 2: Extract An HTTP Runner

- Move handler lifetime ownership, lazy creation, phase-driven `handleStep(...)` calls, and `noResponse()` finalization out of `HttpContext` into a dedicated HTTP runner/adapter.
- Move `HandlerResult` to `RequestHandlingResult` translation and parser-error response synthesis into that same runner.
- Leave `HttpContext` focused on parsed request data, helper services, and request-local storage.
- Preserve the existing `IPipelineHandler` surface during this extraction so `HttpPipeline` does not need to change simultaneously.

## Phase 2 Findings

### Slimmed-Down HttpContext Shape

After the split, `HttpContext` should be reduced to an HTTP context with these responsibilities:

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

The HTTP runner becomes the execution object that owns everything currently implicit in `HttpContext`'s private orchestration state:

- hold the current `HttpContext` context instance
- lazily create the `IHttpHandler` through `IHttpRequestHandlerFactory`
- maintain execution/phase state consumed by handlers
- forward parser events into the context and drive handler execution at the correct seams
- forward body chunks to `handleBodyChunk(...)`
- translate `HandlerResult` into `RequestHandlingResult`
- own pending result staging until `HttpPipeline` consumes it
- synthesize parser/pipeline error responses through `IHttpRequestHandlerFactory::createResponse(...)`

### Minimal Phase 2 Interface Sketch

```cpp
class HttpContextRunner
{
public:
    virtual ~HttpContextRunner() = default;

    virtual HttpContext &context() = 0;

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
class HttpContextPipelineAdapter : public IPipelineHandler
{
public:
    explicit HttpContextPipelineAdapter(std::unique_ptr<HttpContextRunner> runner);

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
    std::unique_ptr<HttpContextRunner> runner_;
};
```

With that transitional shape:

- `HttpPipeline` continues to consume an `IPipelineHandler`
- the runner owns orchestration and result staging
- `HttpContext::createPipelineHandler(...)` can become a compatibility factory that constructs the adapter plus runner, rather than directly making `HttpContext` implement the pipeline contract

### Phase 2 Design Constraints

- The runner should continue to use `IHttpRequestHandlerFactory::create(HttpContext &)` initially, to avoid simultaneously changing routing/factory seams.
- `HttpRequestPhase` can remain exposed through `HttpContext` in Phase 2, but the mutable phase state should move to the runner and be projected into the context rather than owned there directly.
- `RequestHandlingResult` remains the runner-to-pipeline currency in Phase 2; redesigning that contract belongs to Phase 3.
- Error-to-response mapping should move wholesale with the runner so HTTP failure behavior stays in one place.

### Phase 2 Conclusion

- The minimal viable split does not require `HttpPipeline` changes yet.
- The key new object is an HTTP runner that mirrors the current `IPipelineHandler` lifecycle while owning orchestration state.
- `HttpContext` can be slimmed down immediately once the adapter/runner pair exists, because the current pipeline-facing methods do not need to stay on the context type itself.

### Phase 3: Generalize The Pipeline Execution Seam

- Reevaluate the split between `IPipelineHandler` and `IConnectionSession` once HTTP orchestration is no longer embedded in `HttpContext`.
- Decide whether both HTTP and upgraded protocols should flow through a shared protocol-runner abstraction, or whether one existing seam should be widened to carry richer context ownership.
- Define the minimum shared lifecycle needed by peer protocol contexts: startup, inbound progression, outbound progression, completion, abort, and error signaling.
- Keep `RequestHandlingResult` only if it still cleanly represents the shared execution contract after this generalization.

## Phase 3 Findings

### The Three Plausible Routes

#### Option A: Absorb Peer-Context Ownership Into `IPipelineHandler`

In this model, `IPipelineHandler` becomes the single long-term protocol seam. HTTP keeps using it directly, and upgraded protocols such as websocket are re-expressed as pipeline handlers rather than switching over to `IConnectionSession`.

Advantages:

- `HttpPipeline` becomes structurally simpler because it no longer switches between two unrelated execution contracts.
- Parser callbacks, response notifications, disconnect handling, and pending-result consumption stay under one interface.
- The pipeline can remain centered on one "active protocol driver" pointer rather than one handler plus one upgraded session slot.

Costs:

- `IPipelineHandler` is currently HTTP-shaped. It assumes parsed HTTP events such as `onHeader(...)`, `onHeadersComplete()`, and `onMessageComplete()`.
- A websocket or other upgraded protocol would have to carry a large amount of irrelevant surface area after the HTTP upgrade is complete.
- Either the interface becomes bloated with no-op methods, or the pipeline has to branch on protocol phase anyway, which gives back some of the simplification.

Conclusion:

- This route can make `HttpPipeline` mechanically flatter, but it does so by forcing upgraded protocols to inhabit an HTTP-centric interface. That is not a clean peer-context model.

#### Option B: Absorb Peer-Context Ownership Into `IConnectionSession`

In this model, HTTP is reshaped to look more like the upgraded-session world, and `IConnectionSession` becomes the single long-term execution seam.

Advantages:

- The upgraded-connection seam already thinks in terms of per-loop progress rather than HTTP parser callbacks, which is closer to a generic protocol execution model.
- Websocket would need less reshaping because it already lives behind `IConnectionSession`.

Costs:

- HTTP would lose the natural fit between parser callbacks and request-handling phases.
- The pipeline would have to re-encode request parsing progress, response write notifications, and request lifecycle transitions as ad hoc session-internal behavior.
- Keep-alive request boundaries become harder to express cleanly if everything is forced into one connection-session loop model.

Conclusion:

- This route makes HTTP worse in order to favor upgraded protocols. It is not a good fit for the current codebase, where request parsing and request-level routing are core concepts.

#### Option C: Introduce A Higher-Level Abstraction Above Both

In this model, `IPipelineHandler` and `IConnectionSession` are treated as transitional, lower-level seams. A new protocol-execution abstraction sits above both and represents the real peer-context architecture. HTTP and websocket each get their own protocol runner/context pairing under that shared abstraction.

Advantages:

- Neither HTTP nor websocket has to pretend to be the other.
- The shared seam can be defined in terms of lifecycle and result progression rather than HTTP parser events or websocket session loops.
- `HttpPipeline` becomes conceptually cleaner because it drives one protocol-execution object, even if transitional adapters still bridge to the old interfaces for a while.
- Later upgraded protocols can plug into the same model without inheriting HTTP-specific or websocket-specific assumptions.

Costs:

- This introduces one additional architectural layer.
- There is an incremental migration cost because adapters are needed while `HttpPipeline`, HTTP, and websocket are not all moved at once.
- The team must be disciplined about keeping the new abstraction minimal and genuinely protocol-generic.

Conclusion:

- This is the best peer-context route because it yields the cleanest long-term architecture without deforming either existing seam.

### Cleaner `HttpPipeline` Question

Short answer:

- **Long-term cleanest**: a higher-level abstraction above both seams.
- **Short-term mechanically simplest**: pushing everything into `IPipelineHandler`, but at the cost of an HTTP-biased design.

Why the higher-level abstraction is cleaner in the long run:

- `HttpPipeline` should ideally care about protocol execution state transitions, not whether the current active object is HTTP-parser-shaped or upgraded-session-shaped.
- A higher-level protocol execution object lets `HttpPipeline` manage:
  - activation/startup
  - inbound progression
  - outbound progression
  - pending result consumption
  - completion/abort/error transitions
- That means `HttpPipeline` becomes cleaner semantically, not just shorter mechanically.

Why forcing everything into one existing seam is less clean than it first appears:

- If `IPipelineHandler` absorbs the model, then upgraded protocols inherit irrelevant HTTP event methods.
- If `IConnectionSession` absorbs the model, then HTTP loses the clarity of request parsing and request lifecycle hooks.
- In both cases, `HttpPipeline` may have fewer top-level concepts, but the protocol-specific awkwardness is pushed into the wrong abstraction.

### Activation Predicate Constraint

One important constraint is fixed by the routing model:

- activation predicates still need to run against the HTTP activation context, which should ultimately be named `HttpContext` even though current code still calls it `HttpContext`
- the system cannot know which handler or upgraded protocol path is active until at least HTTP headers have been fully read
- route activation therefore remains an HTTP-context concern even in the peer-context architecture

Implications:

- A future `WebSocketContext` should **not** replace the HTTP activation context as the object used for initial activation/routing decisions.
- The HTTP runner remains responsible for driving request parsing to the header-complete seam, evaluating activation predicates against the HTTP context, and only then deciding whether execution stays in HTTP or hands off into a peer upgraded-protocol context.
- This makes the "promote `IConnectionSession` into the main seam" option even less attractive, because it would blur the boundary between HTTP activation/routing and post-activation upgraded-protocol execution.
- It also weakens the idea of making `IPipelineHandler` itself the permanent universal seam, because the HTTP activation stage and the post-activation websocket stage are meaningfully different lifecycle phases even if they share one higher-level execution model.

What this means for the recommendation:

- The recommendation still stands: prefer a higher-level abstraction above both existing seams.
- But that abstraction should model **staged protocol execution**, where:
  - HTTP activation and handler selection happen first against `HttpContext` in the target model, with `HttpContext` serving as the transitional implementation name
  - the selected execution path may then continue as HTTP or transition into a peer protocol context such as `WebSocketContext`
- In other words, `HttpContext` remains the activation context, while `WebSocketContext` becomes the post-activation peer context for websocket execution.

### Recommended Phase 3 Decision

- Do **not** choose `IPipelineHandler` or `IConnectionSession` as the permanent peer-context seam.
- Introduce a higher-level protocol execution abstraction above both existing seams.
- Use adapters during migration:
  - HTTP runner + adapter continues to satisfy `IPipelineHandler`
  - websocket runtime/context continues to satisfy `IConnectionSession`
- Once both protocols are modeled under the higher-level abstraction, simplify `HttpPipeline` to drive that abstraction directly and retire the transitional duplication.

### Shared Execution Lifecycle Sketch

The shared seam should model lifecycle, not parser shape:

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

This is intentionally abstract:

- HTTP can implement it by internally coordinating parser-driven request handling through the HTTP runner.
- Websocket can implement it by internally coordinating connection/session progress through a websocket runner/context pair.
- `RequestHandlingResult` can remain the pipeline currency for now, but the seam no longer assumes that every protocol natively speaks in HTTP callback events.

### Naming Recommendation

Recommended name:

- `IProtocolExecution`

Why this name fits best:

- It names the abstraction after what `HttpPipeline` actually needs to drive: execution/lifecycle progression for the currently selected protocol path.
- It does not bias toward HTTP request parsing (`IPipelineHandler`) or upgraded-connection looping (`IConnectionSession`).
- It leaves room for staged execution where activation begins in HTTP and then continues in another protocol context.
- It reads naturally for both implementations and owning objects, such as `HttpProtocolExecution` and `WebSocketProtocolExecution`.

Names considered and rejected:

- `IProtocolRunner`
  - acceptable, but slightly over-emphasizes the helper object rather than the active execution contract seen by `HttpPipeline`
- `IConnectionExecution`
  - too connection-centric for the HTTP activation stage, which is still request-shaped
- `IProtocolContext`
  - too state/data-oriented; this abstraction is about progression and lifecycle, not just stored context
- `IExecutionContext`
  - too generic and easy to confuse with request or websocket context objects

Recommended naming split:

- `IProtocolExecution`
  - the shared pipeline-facing lifecycle interface
- `HttpContext`
  - the HTTP activation context
- `HttpProtocolExecution`
  - the HTTP implementation of the shared execution seam
- `WebSocketContext`
  - the websocket protocol context/state object
- `WebSocketProtocolExecution`
  - the websocket implementation of the shared execution seam

Transitional note:

- current code can continue to use `HttpContext` while the split is underway
- the design target should nevertheless be documented and discussed as `HttpContext` so the final naming shape stays consistent

The shared seam should therefore be understood as spanning two stages:

- an HTTP activation stage that parses enough of the request to select the execution path using `HttpContext` in the target model, via the current `HttpContext` implementation during transition
- an execution stage that continues either as HTTP handling or as a peer upgraded-protocol context

### Phase 3 Conclusion

- The higher-level abstraction route is the best long-term design.
- It gives the cleanest `HttpPipeline` in semantic terms, because the pipeline becomes responsible for lifecycle progression rather than protocol-specific interface choice.
- The added activation-predicate constraint makes this recommendation stronger, not weaker: the HTTP activation context still owns early activation/routing context, but it no longer needs to own the entire lifetime of post-activation execution.
- The cost is one more layer plus transitional adapters, but that cost buys a genuinely protocol-generic architecture instead of entrenching HTTP bias or session-loop bias.

### Phase 4: Introduce Peer WebSocket Context Ownership

- Replace the websocket runtime's purely internal-context model with a first-class `WebSocketContext` shaped around the generalized execution seam.
- Move websocket-specific mutable state such as outbound queue ownership, close-state inspection, callback context, and connection-local items behind that peer context.
- Ensure `WebSocketContext` takes ownership of the HTTP activation data that remains relevant after upgrade, rather than forcing websocket callbacks to reach back into a no-longer-active `HttpContext` target object or the transitional `HttpContext` implementation.
- Keep the websocket protocol runtime logic, but make it operate on or through `WebSocketContext` rather than being the only public-facing runtime object.
- Align the `WebSocketBuilder` and callback/send-api backlogs with the new peer-context model so event handlers target the new context directly.

## Phase 4 Design Constraint: Carry Forward Relevant HTTP Context

If websocket execution becomes a peer post-activation context, then `WebSocketContext` should inherit or take ownership of the HTTP activation data that may still matter to websocket callbacks.

That should include at minimum:

- request-scoped extension state from `items()`
- request metadata that may affect callback behavior or logging
  - method
  - version
  - headers
- connection addressing information
  - remote address
  - remote port
  - local address
  - local port

Why this matters:

- websocket callbacks may need the authenticated/request-derived items placed on the HTTP context during activation
- callbacks may need to inspect upgrade-request headers after activation for telemetry, subprotocol decisions, or application behavior
- callbacks may need access to local/remote endpoint information for authorization, diagnostics, or connection labeling

Implication for the handoff:

- the HTTP-to-websocket transition should not merely produce a websocket runner; it should construct a `WebSocketContext` that receives the relevant carried-forward state from the HTTP activation context
- this argues for an explicit handoff object or transfer step during upgrade, rather than leaving `WebSocketContext` to query stale HTTP state indirectly
- ownership semantics should be documented clearly: copied data versus moved data versus shared references should be chosen intentionally for each category

## Phase 4 Findings

### Recommended Split: `WebSocketContext` vs `WebSocketProtocolExecution`

The current `WebSocketSessionRuntime` mixes four responsibilities that should not stay fused once websocket becomes a peer protocol context:

- carried-forward request/connection metadata
- callback-facing websocket state and outbound operations
- frame parser / serializer progression
- connection-loop lifecycle decisions such as continue, completed, or abort

Phase 4 should split those responsibilities as follows.

`WebSocketContext` should own the callback-visible protocol state:

- carried-forward HTTP activation data
  - request items
  - method
  - version
  - headers
  - local/remote address and port
- websocket session state that application callbacks may need to inspect
  - whether open notification has occurred
  - whether close has started
  - received close code / reason once known
  - connection-local items that survive after upgrade
- the public outbound API surface
  - `sendText(...)`
  - `sendBinary(...)`
  - `close(...)`
  - possibly `sendPing(...)` if Phase 4 later decides that ping is public

`WebSocketProtocolExecution` should own protocol progression and pipeline-facing lifecycle:

- websocket frame parser progression
- serializer coordination and write flushing
- handshake-write completion
- close-handshake state machine
- mapping protocol errors to close/abort behavior
- translating client I/O progression into the shared `IProtocolExecution` lifecycle

This keeps the context focused on stable state and intentional operations while leaving transport/protocol driving in the execution object.

### Do Not Expose `WebSocketSessionRuntime` Directly As The Long-Term Context

`WebSocketSessionRuntime` is already doing too much to become the public peer context unchanged:

- it owns `pendingWrite_` and `pendingWriteOffset_`, which are transport-progress details rather than callback-facing state
- it owns parser state and fragmented-message assembly, which are execution concerns rather than context concerns
- it owns `ConnectionSessionResult` decisions, which belong to the pipeline-facing execution contract rather than the callback surface

That means the Phase 4 direction should not be "rename runtime to context". It should be "split the current runtime into a context plus an execution/runner object".

### Suggested Handoff Shape

The HTTP-to-websocket upgrade boundary should explicitly construct websocket-owned state instead of letting the execution layer pull from the HTTP activation context later.

One workable sketch is:

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

Then the runtime handoff becomes conceptually:

- HTTP activation selects the websocket upgrade handler
- the upgrade handler produces the handshake response plus websocket startup inputs
- `WebSocketContext` is constructed from the carried-forward activation snapshot
- `WebSocketProtocolExecution` is constructed around that context plus the websocket codec / write machinery

The important point is not the exact type names. The important point is that the handoff data becomes explicit, testable, and owned by the websocket side after activation.

### Suggested Responsibility Boundary

The following split is the cleanest starting point.

Responsibilities that should move into `WebSocketContext`:

- callback registration or callback bundle ownership
  - owned directly by `WebSocketContext`, not by a separate event-dispatch helper object
- public send/close operations
- websocket-visible close metadata
- request-derived metadata needed after upgrade
- application-scoped items associated with the upgraded connection

Responsibilities that should remain in `WebSocketProtocolExecution`:

- `handle(...)` / `onInboundProgress()`-style loop progression
- parser incremental state
- fragmented-message assembly buffers
- pending serialized write buffer and flush offsets
- automatic pong behavior unless Phase 4 explicitly promotes it to public API
- close-handshake advancement rules and completion detection
- mapping protocol faults to `notifyError`, close, or abort decisions

Responsibilities that should be shared only through narrow methods rather than direct field access:

- enqueueing outbound frames
- starting close handshake
- surfacing open/close/error notifications
- querying whether application sends are still allowed

This argues for `WebSocketContext` depending on a narrow outbound/session-control interface provided by the execution object, rather than directly owning raw write buffers.

### Recommended Internal Interface Shape

To keep `WebSocketContext` useful without letting it mutate transport details directly, the execution layer should expose a small internal control surface.

For example:

```cpp
class IWebSocketSessionControl
{
public:
    virtual ~IWebSocketSessionControl() = default;

    virtual bool tryQueueText(std::string_view payload) = 0;
    virtual bool tryQueueBinary(span<const std::uint8_t> payload) = 0;
    virtual bool tryStartClose(WebSocketCloseCode code, span<const std::uint8_t> reason) = 0;
    virtual bool canSend() const = 0;
};
```

With that shape:

- `WebSocketContext` can expose stable public methods without owning serialization buffers
- `WebSocketProtocolExecution` remains the only object that knows about pending writes, close state transitions, and serializer failure
- callback/send-api work in backlog `011` can define public behavior in terms of this narrow control seam instead of in terms of `WebSocketSessionRuntime` internals

### Mapping To `IProtocolExecution`

Under the Phase 3 recommendation, websocket should map to the shared execution seam like this:

- `WebSocketProtocolExecution` implements `IProtocolExecution`
- `WebSocketContext` is owned by `WebSocketProtocolExecution`
- websocket callbacks receive `WebSocketContext &`
- outbound sends mutate websocket execution indirectly through the session-control seam

That means the pipeline sees one execution object, while application code sees one context object.

This is the same architectural pattern proposed for HTTP:

- the pipeline-facing object is the execution/runner
- the protocol-facing object is the context

That parallel structure is one of the main reasons this split is preferable to treating websocket runtime state as a hidden special case.

### Impact On Backlog `011`

The websocket context/send-api backlog should now be interpreted against this Phase 4 split.

Specifically:

- callback signatures should be designed around `WebSocketContext &`, not around direct access to `WebSocketSessionRuntime`
- send semantics should be defined in terms of context operations delegating into execution-owned queue/control state
- queue saturation, close-started behavior, and serializer failure should remain execution decisions surfaced through context return values or status objects
- public API design should avoid exposing implementation details such as `pendingWrite_` or partial-flush offsets

### Recommended Phase 4 Direction So Far

- Model websocket the same way HTTP is being modeled: context plus execution, not one fused runtime object.
- Make `WebSocketContext` the stable callback-facing owner of carried-forward activation data and websocket-visible session state.
- Make `WebSocketProtocolExecution` the `IProtocolExecution` implementation that owns parser progress, outbound flushing, and pipeline completion rules.
- Use a narrow internal session-control seam so context methods can request sends/closes without taking ownership of transport buffers.
- Treat backlog `011` as defining the public shape of `WebSocketContext`, while this backlog defines the ownership and execution split underneath it.

### Exact `WebSocketSessionRuntime` Member Mapping

The current runtime fields in [src/websocket/WebSocketSessionRuntime.h](src/websocket/WebSocketSessionRuntime.h) should be split as follows.

Members that should remain fully execution-owned in `WebSocketProtocolExecution`:

- `parser_`
  - websocket frame parsing progress is pure execution state
- `pendingWrite_`
  - serialized outbound buffer ownership is transport/write-progress state
- `pendingWriteOffset_`
  - partial flush offset is transport-progress state
- `handshakeWritten_`
  - handshake write completion is part of upgrade/output progression, not callback-visible context
- `assemblingMessage_`
  - fragmented-message assembly is execution-only parser state
- `assembledMessageType_`
  - current fragmented-message opcode is execution-only parser state
- `messageBuffer_`
  - fragmented payload accumulation is execution-only parser state
- `receivedCloseFrame_`
  - whether the peer close frame has arrived is part of close-handshake progression

Members that should move primarily into `WebSocketContext`:

- `callbacks_`
  - callback bundle ownership belongs directly to the callback-facing protocol context
- `closeCode_`
  - final/observed close code is callback-visible session state
- `closeReason_`
  - final/observed close reason is callback-visible session state

Members whose current meaning should be split between context-visible state and execution-only mechanics:

- `openNotified_`
  - the callback-visible fact that the websocket is open belongs on `WebSocketContext`
  - the one-time delivery sequencing for `onOpen(...)` remains execution behavior
- `closeState_`
  - the public/session-visible portion becomes context state such as `isOpen()` / `isClosing()` / closed-state inspection
  - the execution-only portion keeps the finer-grained handshake progression details currently represented by `CloseQueued` and `CloseSent`
- `closeNotified_`
  - at-most-once `onClose(...)` delivery is an execution concern
  - any user-visible "close observed" state should be projected into context state rather than exposed as a raw delivery flag

Stated differently:

- `WebSocketContext` should own durable session facts that callbacks may inspect
- `WebSocketProtocolExecution` should own parser progress, write progress, and callback-delivery sequencing
- fields that currently conflate those two concerns should be replaced by a cleaner pair of states rather than copied verbatim into one new class

### Recommended Replacement Shape For Split Members

The members that should not survive unchanged are:

- `closeState_`
  - replace with a context-visible lifecycle state and a separate execution-only handshake stage
- `openNotified_`
  - replace with context-visible open state plus execution-managed first-open callback delivery
- `closeNotified_`
  - replace with execution-managed close-callback delivery bookkeeping only

One workable shape is:

```cpp
enum class WebSocketLifecycleState
{
    Connecting,
    Open,
    Closing,
    Closed
};

enum class WebSocketCloseHandshakeStage
{
    None,
    CloseQueued,
    CloseSent,
    PeerCloseReceived,
    Complete
};
```

With that model:

- `WebSocketContext` exposes `WebSocketLifecycleState`
- `WebSocketProtocolExecution` tracks `WebSocketCloseHandshakeStage`
- `closeCode_` and `closeReason_` live on context once known
- callback delivery guards are not treated as public protocol state

### Exact Current Methods And Their Target Home

Current private methods on `WebSocketSessionRuntime` should also separate cleanly.

Methods that stay execution-owned:

- `handle(IClient &, const Compat::Clock &)`
- `flushPendingWrite(IClient &)`
- `queueSerializedFrame(...)`
- `queueCloseFrame(...)`
- `handleParsedFrame(...)`
- `appendMessageFragment(...)`
- `buildClosePayload(...)`

Methods that conceptually become execution-to-context interaction points:

- `notifyOpen()`
  - execution decides when to fire it; context directly owns the callback bundle and open state
- `notifyClose(...)`
  - execution decides when final close notification is emitted; context directly owns the callback bundle and stores observable close metadata
- `notifyError(...)`
  - execution detects and routes the error; context directly owns the callback bundle and any callback-facing state

That means the Phase 4 implementation should not migrate these helper methods wholesale into `WebSocketContext`. Instead, execution should call into callback/state surfaces owned directly by `WebSocketContext` at the appropriate lifecycle points.

### Phase 5: Cleanup And Surface Consolidation

- Remove transitional adapters once both HTTP and websocket flows use the new execution architecture.
- Simplify documentation so the project describes one protocol-context model rather than one HTTP model plus websocket exceptions.
- Reclassify tests around context behavior versus runner behavior so ownership boundaries stay clear.
- Identify whether later upgraded protocols should be able to plug into the same peer-context seam without HTTP-specific assumptions.

## Incremental Migration Slices

- Slice 1: document and isolate the current HTTP orchestration responsibilities that do not intrinsically belong to request data.
- Slice 2: extract handler lifetime ownership and phase progression from `HttpContext` into a dedicated HTTP runner/adapter while preserving the existing `RequestHandlingResult` contract.
- Slice 3: define a shared pipeline-facing abstraction for protocol execution that can represent both HTTP request handling and upgraded connection contexts.
- Slice 4: redesign websocket runtime/context ownership so a future `WebSocketContext` is surfaced through that shared execution abstraction rather than hidden behind a narrow session loop.

## Open Questions

- Should the shared peer abstraction be request-shaped, connection-shaped, or a more generic protocol-execution object?
- Does HTTP remain one-request-per-context while websocket remains one-connection-per-context, or should both be normalized at the connection layer?
- How much of `HttpRequestPhase` should survive once orchestration moves out of `HttpContext`?
- Should parser callbacks continue to target the context directly, or should they target the orchestrator which then mutates the context?
- Can `RequestHandlingResult` remain the shared pipeline currency, or does a peer-context design want a broader execution-result model?

## Tasks

- [x] Phase 1: document the exact orchestration responsibilities currently owned by `HttpContext` that are not inherently request-data responsibilities.
- [x] Phase 1: classify current `HttpContext` fields and methods into context-owned versus runner-owned responsibilities.
- [x] Phase 1: identify and lock the HTTP behavior tests that must remain green through the split.
- [x] Phase 2: design a slimmed-down HTTP context shape that can survive without directly owning handler lifetime and pipeline-result translation.
- [x] Phase 2: design an HTTP runner/orchestrator abstraction that can own handler creation, phase advancement, and error-to-response mapping.
- [x] Phase 2: define the transitional adapter that preserves the existing `IPipelineHandler` integration while HTTP orchestration moves outward.
- [x] Phase 3: decide whether the pipeline should generalize above `IPipelineHandler` and `IConnectionSession`, or whether one of those seams should absorb the peer-context design.
- [x] Phase 3: define the shared execution lifecycle and result contract required for peer protocol contexts.
- [x] Phase 4: define how a future `WebSocketContext` would map onto the chosen shared execution seam.
- [ ] Phase 4: update websocket context/send-api planning to align with the chosen peer-context architecture.
- [x] Phase 4: identify which websocket runtime responsibilities migrate into `WebSocketContext` versus remaining in a protocol runner.
- [ ] Phase 5: identify which existing HTTP tests would need to move from `HttpContext` behavior tests to protocol-runner behavior tests after the split.
- [ ] Phase 5: plan the concrete rename from `HttpContext` to `HttpContext` once the HTTP runner/context split is stable.
- [ ] Phase 5: plan cleanup/removal of transitional adapters once both HTTP and websocket paths use the new model.

## Immediate Next Slice

- Continue Phase 4 by turning the `WebSocketContext` versus `WebSocketProtocolExecution` split into concrete callback/send semantics and deciding whether public operations return `bool`, status enums, or richer send results.

## Owner

- Copilot

## Priority

- High

## References

- [src/core/HttpContext.h](src/core/HttpContext.h)
- [src/core/HttpContext.cpp](src/core/HttpContext.cpp)
- [src/core/IHttpRequestHandlerFactory.h](src/core/IHttpRequestHandlerFactory.h)
- [src/pipeline/HttpPipeline.h](src/pipeline/HttpPipeline.h)
- [src/pipeline/HttpPipeline.cpp](src/pipeline/HttpPipeline.cpp)
- [src/pipeline/ConnectionSession.h](src/pipeline/ConnectionSession.h)
- [src/websocket/WebSocketSessionRuntime.h](src/websocket/WebSocketSessionRuntime.h)
- [src/websocket/WebSocketSessionRuntime.cpp](src/websocket/WebSocketSessionRuntime.cpp)
- [docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md](docs/backlog/task-lists/web-sockets/011-design-websocket-context-and-send-api-backlog.md)
- [docs/backlog/task-lists/web-sockets/010-normalize-provider-registry-builder-websocket-surface-backlog.md](docs/backlog/task-lists/web-sockets/010-normalize-provider-registry-builder-websocket-surface-backlog.md)