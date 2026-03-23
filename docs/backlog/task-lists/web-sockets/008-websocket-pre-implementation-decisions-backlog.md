2026-03-23 - Copilot: created a WebSocket pre-implementation decision punch list derived from phase backlogs.

# WebSocket Pre-Implementation Decision Punch List Backlog

## Summary

This punch list extracts the design decisions that can be resolved before any code changes start. The goal is to turn the phase backlogs into a smaller set of concrete calls on ownership, API shape, protocol scope, limits, and validation policy so implementation work does not stall on architecture questions in the middle of refactors.

## Goal / Acceptance Criteria

- The major architectural and API choices needed for Phases 1 through 6 are decided in writing before code changes begin.
- Each decision records the chosen direction, rejected alternatives, and any follow-up constraints the implementation must honor.
- The first implementation scope remains intentionally narrow and aligned with the existing WebSocket plan.
- The decision set is specific enough that Phase 1 and Phase 2 can proceed without reopening basic ownership and lifecycle questions.

## How To Use This Punch List

- Resolve items in order, because later decisions depend on earlier ones.
- Prefer a short written decision for each item over extended discussion without an outcome.
- If a decision cannot be made yet, capture the blocker explicitly instead of leaving the item vague.
- When a decision is made, update the relevant phase backlog with the chosen direction if the acceptance criteria or tasks need to become more specific.

## Decision Punch List

### 1. Internal Upgrade Result Shape

- [x] Decide how request handling represents an approved upgrade: dedicated pipeline result object, session-factory result, or request-owned upgrade descriptor.
- [x] Decide where ownership of that result lives between `HttpRequest`, `HttpPipeline`, and the server.
- [x] Decide whether the upgrade result can coexist with a normal response result or must be mutually exclusive by type.

Option sketches:

#### Option A: Dedicated Pipeline Result Object

The request layer returns one explicit outcome type that represents all post-routing results.

Sketch:

```cpp
struct RequestHandlingResult
{
	enum class Kind
	{
		Response,
		Upgrade,
		NoResponse,
		Error
	};

	Kind kind;
	std::unique_ptr<IByteSource> responseStream;
	std::unique_ptr<IConnectionSessionFactory> upgradeFactory;
	PipelineError error;
};

RequestHandlingResult HttpRequest::takeResult();
```

How it would flow:

- `HttpRequest` finishes routing and builds exactly one `RequestHandlingResult`.
- `HttpPipeline` consumes that result after request handling advances far enough.
- If `kind == Response`, the pipeline writes the response stream.
- If `kind == Upgrade`, the pipeline creates and enters the upgraded session path.

Why it is attractive:

- One explicit type makes normal response and upgrade outcomes mutually visible and easy to test.
- Ownership is centralized at the pipeline boundary instead of hidden in callbacks.
- Later non-WebSocket upgraded protocols could reuse the same seam.

Why it may be awkward:

- It introduces a broader refactor because current response delivery is callback-oriented.
- The result type can become bloated if too many special cases are pushed into it.
- `NoResponse` and `Error` semantics need to be defined carefully so the type does not become vague.

#### Option B: Session-Factory Result

The request layer still decides whether upgrade is allowed, but instead of returning a general transaction result it returns a narrow upgrade artifact only when upgrade is approved.

Sketch:

```cpp
struct ApprovedUpgrade
{
	std::unique_ptr<IConnectionSessionFactory> sessionFactory;
	HttpHeaderCollection handshakeHeaders;
	HttpStatus handshakeStatus;
};

std::optional<ApprovedUpgrade> HttpRequest::takeApprovedUpgrade();
std::unique_ptr<IByteSource> HttpRequest::takeResponseStream();
```

How it would flow:

- Existing response behavior stays mostly intact.
- `HttpRequest` separately exposes either a normal response stream or an approved upgrade.
- `HttpPipeline` checks for approved upgrade first, then falls back to normal response handling.

Why it is attractive:

- Narrower than a full pipeline result object, so it may fit the existing design with less churn.
- Keeps upgrade-specific data grouped around session creation and handshake metadata.
- Easier to land incrementally if the pipeline already knows how to manage response writing separately.

Why it may be awkward:

- It creates two output channels from `HttpRequest`, which can drift into ambiguous states if both are populated.
- The contract for precedence between upgrade and response must be enforced manually.
- It may become a halfway design that later wants to become the dedicated result object anyway.

#### Option C: Request-Owned Upgrade Descriptor

The request object holds upgrade approval state internally, and the pipeline queries that state after handler execution completes.

Sketch:

```cpp
class HttpRequest : private IPipelineHandler
{
private:
	std::optional<UpgradeDescriptor> approvedUpgrade_;
	std::unique_ptr<IByteSource> responseStream_;

public:
	bool hasApprovedUpgrade() const;
	const UpgradeDescriptor *approvedUpgrade() const;
	std::unique_ptr<IConnectionSession> createUpgradedSession();
};
```

How it would flow:

- `HttpRequest` mutates its own internal state during routing or handler execution.
- `HttpPipeline` asks the request whether it approved upgrade.
- If yes, the pipeline asks the request to create the session or expose a descriptor for session creation.

Why it is attractive:

- Minimal surface-area change in the short term.
- Can be layered onto the existing `HttpRequest` lifecycle with fewer immediate type changes.
- Keeps upgrade state close to routing decisions.

Why it may be awkward:

- It keeps important ownership and control flow implicit inside `HttpRequest`.
- The request object becomes more stateful and harder to reason about as both HTTP and upgraded semantics coexist.
- Testing becomes more coupled to `HttpRequest` internals rather than a clean pipeline seam.

Working comparison:

- Dedicated pipeline result object: cleanest long-term seam, highest refactor cost up front.
- Session-factory result: smaller near-term change, but needs strict exclusivity rules.
- Request-owned upgrade descriptor: easiest to bolt on, highest risk of architectural drift.

Decision:

- Chosen option: Option A, dedicated pipeline result object.
- Status: Accepted for Phase 1 planning.

Why this option fits the current codebase:

- The existing request-to-pipeline boundary is the architectural seam that already needs to move for WebSocket support.
- A single explicit result type gives the pipeline one place to decide between HTTP completion and upgraded-session takeover.
- This keeps long-lived upgraded behavior out of `IHttpResponse` and avoids baking hidden upgrade state into `HttpRequest`.

Ownership rules to enforce:

- `HttpRequest` may construct the result, but it must not retain ownership of the post-routing outcome once the pipeline consumes it.
- `HttpPipeline` becomes the sole owner of the consumed result and any response stream or upgrade factory it carries.
- The server continues to own connection acceptance and top-level pipeline lifetime, but not the per-request result object after handoff.

Mutual exclusivity rule:

- Response and upgrade are mutually exclusive by type, not by convention.
- A `RequestHandlingResult` instance must represent exactly one terminal request outcome.
- `NoResponse` and `Error` remain explicit result kinds so the pipeline does not infer missing state from null pointers.

Implications for Phase 1:

- Phase 1 should explicitly replace callback-only response handoff with result-object consumption at the pipeline boundary.
- The `HttpRequest` backlog tasks should assume result construction and transfer semantics instead of adding request-owned upgrade state.
- Phase 2 handshake work should build an upgrade result payload rather than introducing a separate upgrade side channel.

Alternatives not chosen:

- Option B, session-factory result: rejected for now because it preserves dual output channels from `HttpRequest` and leaves exclusivity enforcement too implicit.
- Option C, request-owned upgrade descriptor: rejected because it concentrates too much hidden ownership and control flow inside `HttpRequest`.

Suggested decision record for item 1:

- Chosen direction: Dedicated pipeline result object.
- Why this option fits the current codebase: The request-to-pipeline seam already has to change, and a single result object keeps that change explicit.
- What ownership rules must be enforced: `HttpPipeline` owns the consumed result and its payloads after handoff from `HttpRequest`.
- Whether response and upgrade are mutually exclusive by type or by convention: By type.
- Which Phase 1 tasks should be rewritten once this is decided: Result and ownership model, pipeline refactor, and request-boundary tasks should all assume result-object transfer rather than side-channel upgrade state.

Decision criteria:
- Minimize ambiguity between normal HTTP completion and upgraded-connection takeover.
- Keep ownership explicit and C++17-friendly without compatibility shims.
- Avoid forcing WebSocket semantics into `IHttpResponse` or response-stream callbacks.

### 2. Upgraded Session Abstraction Boundary

- [x] Decide the internal interface name and responsibility split for long-lived upgraded connections.
- [x] Decide whether the pipeline drives sessions via a single `handle()` style step or separate read or write hooks.
- [x] Decide which lifecycle signals the session receives directly versus what remains pipeline-owned.

Option sketches:

#### Option A: Single-Step Session Interface

The pipeline owns scheduling and calls one session step method each loop iteration. The session decides what work it can make progress on using the supplied transport access.

Sketch:

```cpp
enum class ConnectionSessionResult
{
	Continue,
	Completed,
	AbortConnection,
	ProtocolError
};

class IConnectionSession
{
public:
	virtual ~IConnectionSession() = default;
	virtual ConnectionSessionResult handle(IClient &client, const Compat::Clock &clock) = 0;
};
```

How it would flow:

- `HttpPipeline` accepts the upgrade and owns an `IConnectionSession`.
- Each pipeline loop iteration calls `session->handle(client, clock)`.
- The session performs whatever read, parse, write, or close progress is currently possible.
- The returned status tells the pipeline whether to continue, clean up, or abort.

Why it is attractive:

- Very narrow seam that is easy to fake in tests.
- The pipeline loop stays simple because it delegates upgraded work through one call.
- Session internals can evolve without changing the pipeline interface repeatedly.

Why it may be awkward:

- A single method can become a large mixed-responsibility state machine.
- The boundary between pipeline-owned and session-owned timeout or activity logic can blur.
- It can hide read-versus-write starvation behavior unless the contract is documented carefully.

#### Option B: Split Read Or Write Session Interface

The session exposes separate hooks for inbound and outbound progress so the pipeline retains more control over scheduling and fairness.

Sketch:

```cpp
struct SessionProgress
{
	bool madeProgress;
	bool completed;
	bool shouldAbort;
};

class IConnectionSession
{
public:
	virtual ~IConnectionSession() = default;
	virtual SessionProgress handleRead(IClient &client) = 0;
	virtual SessionProgress handleWrite(IClient &client) = 0;
	virtual void onDisconnected() = 0;
};
```

How it would flow:

- `HttpPipeline` decides when to try read progress and when to try write progress.
- The session keeps protocol state, frame buffers, and outbound work queues.
- The pipeline uses the results to decide loop termination, fairness, and timeout resets.

Why it is attractive:

- Makes scheduling policy explicit at the pipeline layer.
- Better fit if backpressure and partial-write behavior must be tightly controlled.
- Easier to reason about read-heavy versus write-heavy stalls in tests.

Why it may be awkward:

- The interface is broader and more coupled to the current pipeline loop design.
- Session implementations may need duplicated state checks across read and write methods.
- If most sessions always need both phases, the split can add noise without much real benefit.

#### Option C: Event-Oriented Session Interface

The pipeline remains in charge of transport reads and writes and calls higher-level session callbacks such as inbound bytes, writable opportunity, timeout, and disconnect.

Sketch:

```cpp
class IConnectionSession
{
public:
	virtual ~IConnectionSession() = default;
	virtual void onReadable(IClient &client) = 0;
	virtual void onWritable(IClient &client) = 0;
	virtual void onTimeout() = 0;
	virtual void onDisconnected() = 0;
	virtual bool isFinished() const = 0;
	virtual bool shouldAbortConnection() const = 0;
};
```

How it would flow:

- The pipeline detects loop conditions and calls session lifecycle hooks.
- The session reacts to events but does not directly define the top-level loop contract.
- The pipeline polls session completion state after callbacks.

Why it is attractive:

- Fine-grained control over what remains pipeline-owned versus session-owned.
- Makes timeout and disconnect signals explicit.
- Potentially reusable if multiple upgraded protocol styles appear later.

Why it may be awkward:

- More interface surface than the first release likely needs.
- Completion and abort become distributed across multiple callbacks and query methods.
- It risks turning the pipeline-session contract into an implicit event framework.

Working comparison:

- Single-step session interface: narrowest seam, easiest to fake, highest risk of a chunky session state machine.
- Split read or write session interface: best scheduling visibility, more interface complexity.
- Event-oriented session interface: most explicit lifecycle signaling, most framework-like and easiest to overdesign.

Initial recommendation:

- Prefer the single-step session interface for the first implementation, with timeout and disconnect policy staying pipeline-owned.
- Prefer the split read or write interface only if Phase 1 already proves the pipeline must explicitly arbitrate fairness between inbound and outbound work.
- Avoid the event-oriented interface for the initial version unless later upgraded protocols are already a near-term goal.

Decision:

- Chosen option: Option A, single-step session interface.
- Working interface name: `IConnectionSession`.
- Status: Accepted for Phase 1 planning.

Why this option fits the current codebase:

- The existing pipeline loop already has one main scheduling point, so one session step integrates with that structure without inventing a new mini-framework.
- It gives the upgraded path a narrow seam that is easy to fake in native tests and easy to replace internally if the WebSocket runtime evolves.
- It keeps the first implementation focused on correctness and ownership rather than prematurely splitting scheduling concerns.

Pipeline-owned responsibilities:

- Connection lifetime and client ownership.
- Timeout tracking and activity timestamps.
- Disconnect detection and final cleanup.
- Deciding whether the loop should continue after each session step.

Session-owned responsibilities:

- WebSocket protocol state after upgrade.
- Frame parsing, outbound protocol writes, and close-handshake progress.
- Internal buffering or queueing needed by the chosen outbound-send model.
- Declaring whether work should continue, complete, or abort the connection.

Completion reporting rule:

- The session reports completion through its single-step return value.
- The pipeline remains the final authority on removing the connection and releasing transport resources.
- Timeout and disconnect are signaled by the pipeline into session cleanup, not delegated as primary policy decisions.

Implications for Phase 1 and Phase 4:

- Phase 1 should introduce `IConnectionSession` as an internal seam and wire `HttpPipeline` to drive it through one step call per loop iteration.
- Phase 4 should assume the WebSocket session runtime lives behind this one-step interface rather than split read or write hooks.
- If fairness or starvation issues appear later, they should first be solved inside the session runtime before widening the interface.

Alternatives not chosen:

- Option B, split read or write session interface: rejected for now because it exposes more loop policy than the first implementation has earned and couples the session too tightly to the current pipeline scheduler.
- Option C, event-oriented session interface: rejected because it pushes the design toward an event framework that is broader than the first release needs.

Suggested decision record for item 2:

- Chosen interface shape: Single-step `IConnectionSession` driven by the pipeline.
- What remains pipeline-owned: Transport ownership, timeout policy, disconnect detection, and final cleanup.
- What the session owns directly: Upgraded protocol state, frame IO progress, and session-level completion or abort decisions.
- How completion is reported: Through the single-step return result consumed by `HttpPipeline`.
- Which later concerns would force revisiting this boundary: Proven fairness problems, persistent read or write starvation, or a second upgraded protocol with materially different scheduling needs.

Decision criteria:
- Keep timeout and disconnect supervision centralized in the pipeline.
- Keep the session seam reusable for later upgraded protocols, if that remains desirable.
- Prefer a narrow surface that is easy to fake in native tests.

### 3. Pipeline State Model

- [x] Decide the explicit connection states that replace the current implicit request or response completion model.
- [x] Decide how upgraded-session activity interacts with keep-alive and finished-state reporting.
- [x] Decide what constitutes terminal completion for an upgraded connection versus a recoverable loop iteration.

Option sketches:

#### Option A: Single Explicit Connection State Enum

The pipeline owns one authoritative connection state enum that represents the current lifecycle stage of the client.

Sketch:

```cpp
enum class ConnectionState
{
	ReadingHttpRequest,
	WritingHttpResponse,
	UpgradedSessionActive,
	Closing,
	Completed,
	Aborted,
	Error
};
```

How it would flow:

- The pipeline enters `ReadingHttpRequest` when a client is accepted.
- A normal HTTP path transitions into `WritingHttpResponse` and then either back to `ReadingHttpRequest` for keep-alive or to `Completed`.
- An approved upgrade transitions into `UpgradedSessionActive`.
- Disconnect, fatal protocol failure, or explicit abort transition into `Closing`, `Aborted`, or `Error` as appropriate.

Why it is attractive:

- One authoritative state makes behavior easier to trace and test.
- It directly replaces today’s scattered completion booleans with a more auditable lifecycle.
- It aligns well with the chosen dedicated result object and single-step session interface.

Why it may be awkward:

- The enum can become overloaded if too many fine-grained sub-states are added.
- Some transient conditions may still need separate flags if they are orthogonal rather than lifecycle-defining.
- Keep-alive transitions need to be documented carefully so `Completed` does not mean two different things.

#### Option B: Top-Level State Plus Phase Flags

The pipeline owns a smaller top-level state enum and keeps additional booleans or flags for orthogonal details such as request complete, response started, or close pending.

Sketch:

```cpp
enum class ConnectionMode
{
	Http,
	Upgraded,
	Closing,
	Finished
};

struct ConnectionPhaseFlags
{
	bool requestReadComplete = false;
	bool responseWriteComplete = false;
	bool closePending = false;
	bool errored = false;
};
```

How it would flow:

- `ConnectionMode` tracks the large lifecycle bucket.
- Flags track in-flight details within each bucket.
- The pipeline derives finished or keep-alive decisions from the combination.

Why it is attractive:

- Smaller enum and less pressure to encode every nuance as a named state.
- Can be closer to the current implementation, which already uses booleans.
- Useful if some conditions truly are orthogonal to the main lifecycle.

Why it may be awkward:

- It preserves some of the ambiguity that the refactor is supposed to remove.
- Illegal flag combinations are easier to create accidentally.
- Test assertions can become more verbose because meaning is spread across multiple fields.

#### Option C: State Objects Per Major Mode

The pipeline owns a small mode selector and delegates behavior to separate state objects or handler classes for HTTP, upgraded, and closing behavior.

Sketch:

```cpp
class IConnectionModeHandler
{
public:
	virtual ~IConnectionModeHandler() = default;
	virtual PipelineHandleClientResult handle(HttpPipelineContext &context) = 0;
};
```

How it would flow:

- `HttpPipeline` swaps between mode handlers for HTTP, upgraded session, and closing behavior.
- Each handler encapsulates its own sub-state.
- The top-level pipeline loop delegates most logic to the active handler.

Why it is attractive:

- Strong separation between HTTP and upgraded-session behavior.
- Can reduce giant conditional blocks inside `HttpPipeline`.
- Gives each mode a clearer home for internal invariants.

Why it may be awkward:

- Much heavier refactor than Phase 1 appears to require.
- Adds object-layer complexity before the upgraded path is proven.
- Can make the first implementation slower to land without actually reducing design uncertainty.

Working comparison:

- Single explicit connection state enum: best balance of clarity and refactor scope.
- Top-level state plus phase flags: lowest disruption, but keeps more ambiguity alive.
- State objects per major mode: cleanest separation on paper, highest implementation overhead.

Initial recommendation:

- Prefer the single explicit connection state enum for the first implementation.
- Use a very small number of supplemental flags only when a condition is truly orthogonal to lifecycle stage.
- Avoid full state objects until the upgraded path is proven and the pipeline is demonstrably too complex for a single enum-based coordinator.

Decision:

- Chosen option: Option A, single explicit connection state enum.
- Status: Accepted for Phase 1 planning.

Why this option fits the current codebase:

- The current pipeline already behaves as a coordinator, but its lifecycle is spread across booleans that will become harder to reason about once upgrade is added.
- One authoritative enum matches the dedicated result object from item 1 and the single-step session interface from item 2.
- It gives Phase 1 a clear refactor target without forcing the heavier abstraction of per-mode state objects.

Authoritative state rule:

- The connection state enum is the primary representation of lifecycle stage.
- Supplemental flags are allowed only for truly orthogonal details that are not themselves lifecycle stages.
- The pipeline should not derive major lifecycle meaning by combining multiple booleans where an enum value would be clearer.

Keep-alive and terminal-completion rule:

- A normal HTTP response may transition from `WritingHttpResponse` back to `ReadingHttpRequest` when keep-alive remains valid.
- An upgraded connection does not re-enter HTTP request-reading mode; `UpgradedSessionActive` is a one-way transition from the HTTP lifecycle.
- Terminal states are `Completed`, `Aborted`, and `Error`, with `Closing` used only for in-progress shutdown work.

Implications for Phase 1:

- Phase 1 should replace the current scattered completion booleans with an explicit connection-state model as part of the pipeline refactor.
- Pipeline tests should assert state transitions directly rather than inferring them from mixed field combinations.
- Any remaining boolean flags should be justified as orthogonal details, not leftover lifecycle surrogates.

Alternatives not chosen:

- Option B, top-level state plus phase flags: rejected because it preserves too much of the ambiguity the refactor is supposed to remove.
- Option C, state objects per major mode: rejected because the extra indirection is not justified before the upgraded path has been proven in code.

Suggested decision record for item 3:

- Chosen state model: Single explicit connection state enum.
- Which states are authoritative: `ReadingHttpRequest`, `WritingHttpResponse`, `UpgradedSessionActive`, `Closing`, `Completed`, `Aborted`, and `Error`.
- Which details, if any, remain supplemental flags: Only orthogonal details that do not redefine lifecycle stage.
- How keep-alive re-entry works after a normal HTTP response: The pipeline may transition from `WritingHttpResponse` back to `ReadingHttpRequest` when the HTTP connection remains reusable.
- What transitions are considered terminal: `Completed`, `Aborted`, and `Error`.

Decision criteria:
- State transitions should be auditable and hard to mis-sequence.
- The model should not overload existing booleans until the meaning becomes unclear.
- The state model should make unit tests straightforward to author.

### 4. Handshake Validation Placement

- [x] Decide whether handshake validation lives in a dedicated WebSocket upgrade handler, in routing, or in a narrow pipeline-owned branch fed by routed intent.
- [x] Decide where `Sec-WebSocket-Accept` generation lives.
- [x] Decide whether handshake rejection status codes are standardized now or left partially open.

Option sketches:

#### Option A: Dedicated WebSocket Upgrade Handler

Routing selects a WebSocket-aware internal handler, and that handler owns validation plus handshake response construction.

Sketch:

```cpp
class WebSocketUpgradeHandler
{
public:
	RequestHandlingResult handleUpgradeRequest(const HttpRequest &request,
											   IWebSocketRoute &route);
};
```

How it would flow:

- Normal routing resolves the request path to a WebSocket route candidate.
- The upgrade handler validates HTTP method and required headers.
- The handler generates `Sec-WebSocket-Accept` and returns an upgrade result or rejection response.
- `HttpPipeline` only consumes the already-decided result object.

Why it is attractive:

- Keeps WebSocket-specific validation logic out of generic parser and pipeline code.
- Makes handshake behavior easy to test as a focused unit.
- Fits naturally with later public route registration work in Phase 5.

Why it may be awkward:

- If introduced too early, it may force route concepts into Phase 2 before the routing API is fully settled.
- There is some risk of duplicating request-selection or dispatch logic if the boundary is not kept narrow.
- The handler can become a catch-all if it also absorbs too much session-factory or rejection-policy logic.

#### Option B: Routing-Level Validation Branch

Routing itself recognizes a WebSocket endpoint and performs enough validation to decide whether the request should become an upgrade result.

Sketch:

```cpp
struct RoutedUpgradeDecision
{
	bool matched;
	RequestHandlingResult result;
};

RoutedUpgradeDecision HandlerProviderRegistry::tryResolveWebSocket(const HttpRequest &request);
```

How it would flow:

- The routing layer matches a candidate endpoint and validates upgrade requirements in the same decision path.
- If validation passes, routing returns an upgrade result.
- If validation fails, routing returns a rejection response result.
- The pipeline remains unaware of handshake details beyond the final result object.

Why it is attractive:

- Keeps route matching and route-specific validation in one place.
- Avoids adding another internal handler abstraction if routing already owns endpoint selection.
- Can make WebSocket route behavior feel consistent with other route resolution logic.

Why it may be awkward:

- It pushes protocol-specific logic into routing, which can blur the line between endpoint selection and HTTP/WebSocket semantics.
- The routing layer may become harder to reuse or reason about if it starts owning handshake rules.
- Tests may become broader and more coupled than necessary because matching and validation are fused.

#### Option C: Narrow Pipeline-Owned Handshake Branch Fed By Routed Intent

Routing only decides that a request targets a WebSocket endpoint; the pipeline-side upgrade branch performs the actual protocol validation and handshake construction.

Sketch:

```cpp
struct UpgradeIntent
{
	IWebSocketRoute *route;
};

std::optional<UpgradeIntent> HttpRequest::takeUpgradeIntent();
RequestHandlingResult HttpPipeline::buildUpgradeResult(const HttpRequest &request,
													   const UpgradeIntent &intent);
```

How it would flow:

- Routing marks the request as targeting a WebSocket-capable endpoint.
- The pipeline-side upgrade branch validates required headers and method semantics.
- The same branch generates `Sec-WebSocket-Accept`, rejection responses, and the final upgrade result.

Why it is attractive:

- Keeps route matching separate from low-level protocol validation.
- Makes handshake construction sit close to the upgrade seam chosen in items 1 and 2.
- Avoids forcing a distinct WebSocket handler abstraction too early.

Why it may be awkward:

- It puts more protocol-specific logic into pipeline-adjacent code, which can grow if not kept very narrow.
- There is a risk of the pipeline learning too much about WebSocket details rather than just consuming results.
- Handshake testing may need more pipeline scaffolding than a dedicated handler approach.

Working comparison:

- Dedicated WebSocket upgrade handler: best focused ownership of protocol validation, with modest extra abstraction.
- Routing-level validation branch: most compact if routing is already the semantic owner, but easiest to blur responsibilities.
- Narrow pipeline-owned branch fed by routed intent: close to the upgrade seam, but risks growing pipeline protocol knowledge.

Initial recommendation:

- Prefer the dedicated WebSocket upgrade handler as the cleanest place for validation and `Sec-WebSocket-Accept` generation.
- Prefer the narrow pipeline-owned branch only if Phase 2 needs a slimmer internal path before the route layer grows WebSocket-specific helper types.
- Avoid routing-level validation as the primary home unless the team explicitly wants routing to own protocol-level acceptance rules.

Decision:

- Chosen option: Option A, dedicated WebSocket upgrade handler.
- Status: Accepted for Phase 2 planning.

Why this option fits the current codebase:

- It keeps WebSocket protocol validation out of both generic parser code and the core pipeline coordinator.
- It gives Phase 2 a focused unit of behavior for request validation, handshake response construction, and upgrade-result production.
- It lines up cleanly with later route registration work, where a WebSocket route can hand off into a dedicated upgrade-specific path.

Responsibility split:

- Routing decides whether a request targets a WebSocket-capable endpoint.
- The dedicated upgrade handler validates method and headers, generates `Sec-WebSocket-Accept`, and returns either an upgrade result or a rejection response result.
- `HttpPipeline` consumes only the final `RequestHandlingResult` and does not own handshake-specific rules.

`Sec-WebSocket-Accept` placement rule:

- `Sec-WebSocket-Accept` generation lives in the dedicated WebSocket upgrade handler.
- The generation helper can be factored into a smaller protocol utility if needed, but the handler remains the semantic owner of when it is used.

Rejection-policy rule:

- Rejection status codes should be standardized during Phase 2 rather than left open-ended.
- The dedicated upgrade handler should be the place where those codes are selected consistently.
- Follow-up item 5 can refine the exact code matrix and response shape, but the ownership of that policy is now fixed.

Implications for Phase 2 and Phase 5:

- Phase 2 should introduce a narrow internal WebSocket upgrade handler rather than embedding validation into routing or pipeline code.
- Handshake-native tests should target that handler directly where possible, with pipeline tests reserved for seam integration.
- Phase 5 should assume WebSocket routing resolves to an upgrade-capable path that then hands off to the dedicated handler.

Alternatives not chosen:

- Option B, routing-level validation branch: rejected because it blurs endpoint selection with protocol validation and would make routing more protocol-aware than necessary.
- Option C, narrow pipeline-owned branch fed by routed intent: rejected because it teaches the pipeline too much about WebSocket-specific handshake rules.

Suggested decision record for item 4:

- Chosen validation home: Dedicated WebSocket upgrade handler.
- Where `Sec-WebSocket-Accept` is generated: In the upgrade handler, optionally via a small helper it owns semantically.
- Where rejection status codes are defined: In the upgrade handler, with exact response policy refined in item 5.
- What routing decides versus what handshake validation decides: Routing decides endpoint intent; the upgrade handler decides protocol acceptance or rejection.
- Which Phase 2 tasks should be updated once this is chosen: Handshake validation, response generation, parser/request integration, and rejection-behavior tasks should all assume a dedicated handler-owned handshake path.

Decision criteria:
- Keep validation logic close to the upgrade seam but not tangled into generic parser code.
- Reuse existing header parsing and lookup behavior.
- Keep accepted and rejected paths deterministic under native tests.

### 5. HTTP Rejection Policy For Invalid Upgrades

- [ ] Decide the HTTP status codes and response shape for unsupported method, missing headers, unsupported version, and malformed key cases.
- [ ] Decide how much error detail is exposed to callers versus kept internal.
- [ ] Decide whether all rejection paths share one response helper or remain case-specific.

Decision criteria:
- Prefer consistent externally visible behavior.
- Keep the policy simple enough that handshake tests are stable.
- Do not imply support for optional RFC features that are intentionally deferred.

### 6. Frame Codec Ownership And IO Model

- [ ] Decide whether the reader and writer operate on raw `IByteChannel` calls, owned buffers, or intermediate byte-source abstractions.
- [ ] Decide whether the frame writer should return buffers, streamable sources, or direct write operations.
- [ ] Decide whether message assembly belongs inside the codec layer or in the session runtime.

Decision criteria:
- Prefer incremental and bounded behavior over whole-message buffering.
- Keep the codec independently testable from routing and callbacks.
- Avoid a design that makes partial read or write testing awkward.

### 7. Fragmentation And Delivery Semantics

- [ ] Decide whether the first public API delivers complete messages only or can expose fragments internally or publicly.
- [ ] Decide whether continuation handling accumulates in memory or is streamed through bounded chunks.
- [ ] Decide which fragmentation-related protocol errors are fatal immediately.

Decision criteria:
- Stay within embedded memory constraints.
- Keep the initial public API small.
- Make protocol-error handling deterministic.

### 8. Outbound Send Model

- [ ] Decide whether outbound sends are synchronous in the pipeline loop or mediated by a bounded queue.
- [ ] Decide when server-initiated sends are legal from the eventual public API.
- [ ] Decide what happens when a session exceeds outbound capacity.

Decision criteria:
- Avoid one connection monopolizing the main loop.
- Keep backpressure behavior explicit rather than implicit.
- Choose a model that the native test harness can simulate cleanly.

### 9. Control Frame Exposure

- [ ] Decide whether ping and pong remain internal in the first release or are exposed as callbacks.
- [ ] Decide whether close events expose raw status codes, simplified reasons, or both.
- [ ] Decide which control-frame policy is public API versus strictly internal behavior.

Decision criteria:
- Keep the public callback surface intentionally small.
- Expose only what has a clear embedded use case.
- Keep automatic protocol obligations internal where possible.

### 10. Public Route Registration Shape

- [ ] Decide the registration entry point naming and builder style for WebSocket endpoints.
- [ ] Decide whether session creation is function-based, type-based, or builder-object based.
- [ ] Decide how WebSocket routes coexist with existing HTTP routes that share nearby patterns.

Decision criteria:
- Preserve a clean separation from existing response-centric HTTP handler APIs.
- Favor direct, readable call sites over transitional compatibility layers.
- Keep route matching rules understandable without implementation archaeology.

### 11. Public Callback Surface

- [ ] Decide the exact first-release callback set: open, text, binary, close, error, and any optional control-frame hooks.
- [ ] Decide whether callbacks receive a session object, raw payload spans, owned buffers, or higher-level message wrappers.
- [ ] Decide what callback ordering guarantees will be documented.

Decision criteria:
- Keep the API narrow enough to stabilize quickly.
- Avoid defaulting to heap-heavy abstractions unless required.
- Keep the eventual test surface precise and deterministic.

### 12. Limits And Constant Names

- [ ] Decide which WebSocket limits are compile-time constants in the first release: frame size, message size, queue depth, idle timeout, and any close timeout.
- [ ] Decide the default values for those limits.
- [ ] Decide the macro and `static constexpr` naming scheme before implementation lands.

Decision criteria:
- Follow the project constant pattern exactly.
- Default values should fit RP2040 and ESP-class targets conservatively.
- Limit names should remain stable even if the implementation evolves.

### 13. Error Mapping And Close Policy

- [ ] Decide how codec failures, timeout failures, write failures, and disconnects map to close status or forced shutdown.
- [ ] Decide which failures attempt a close handshake and which terminate immediately.
- [ ] Decide what internal error categories need to remain distinguishable for testing and debugging.

Decision criteria:
- Make failure behavior deterministic and testable.
- Keep close policy centralized rather than scattered.
- Avoid over-promising graceful shutdown where transport state does not allow it.

### 14. Documentation Commitments For First Release

- [ ] Decide the exact unsupported feature list that will be documented on day one.
- [ ] Decide whether an example is required for the first merge or can wait until the API is stable.
- [ ] Decide what validation commands and test suites will be part of the documented acceptance bar.

Decision criteria:
- Documentation should match actual shipped scope, not aspirational scope.
- Examples should not lock in an API shape prematurely.
- Validation guidance should match the native test lane already used in this repo.

## Recommended Working Order

- [ ] Resolve items 1 through 3 before Phase 1 implementation begins.
- [ ] Resolve items 4 and 5 before any handshake code lands.
- [ ] Resolve items 6 through 9 before codec and session-runtime implementation starts.
- [ ] Resolve items 10 and 11 before public API code or examples are written.
- [ ] Resolve items 12 through 14 before hardening and documentation work is marked complete.

## Suggested Recording Format

- Decision:
- Chosen option:
- Alternatives considered:
- Constraints introduced:
- Affected backlog files:
- Open follow-ups:

## Owner

TBD

## Priority

High

## References

- `docs/plans/websocket-support-plan.md`
- `docs/backlog/task-lists/web-sockets/000-websocket-support-implementation-backlog.md`
- `docs/backlog/task-lists/web-sockets/001-upgrade-seam-and-connection-lifecycle-backlog.md`
- `docs/backlog/task-lists/web-sockets/002-websocket-handshake-vertical-slice-backlog.md`
- `docs/backlog/task-lists/web-sockets/003-websocket-frame-codec-backlog.md`
- `docs/backlog/task-lists/web-sockets/004-websocket-session-runtime-backlog.md`
- `docs/backlog/task-lists/web-sockets/005-websocket-routing-and-builder-api-backlog.md`
- `docs/backlog/task-lists/web-sockets/006-websocket-limits-errors-and-hardening-backlog.md`
- `docs/backlog/task-lists/web-sockets/007-websocket-documentation-and-examples-backlog.md`