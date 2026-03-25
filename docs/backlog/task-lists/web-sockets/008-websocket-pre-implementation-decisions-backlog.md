2026-03-23 - Copilot: created a WebSocket pre-implementation decision punch list derived from phase backlogs.
2026-03-23 - Copilot: adopted recommended options for decisions 5 through 14.

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
- [x] Decide where ownership of that result lives between `HttpContext`, `HttpPipeline`, and the server.
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

RequestHandlingResult HttpContext::takeResult();
```

How it would flow:

- `HttpContext` finishes routing and builds exactly one `RequestHandlingResult`.
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

std::optional<ApprovedUpgrade> HttpContext::takeApprovedUpgrade();
std::unique_ptr<IByteSource> HttpContext::takeResponseStream();
```

How it would flow:

- Existing response behavior stays mostly intact.
- `HttpContext` separately exposes either a normal response stream or an approved upgrade.
- `HttpPipeline` checks for approved upgrade first, then falls back to normal response handling.

Why it is attractive:

- Narrower than a full pipeline result object, so it may fit the existing design with less churn.
- Keeps upgrade-specific data grouped around session creation and handshake metadata.
- Easier to land incrementally if the pipeline already knows how to manage response writing separately.

Why it may be awkward:

- It creates two output channels from `HttpContext`, which can drift into ambiguous states if both are populated.
- The contract for precedence between upgrade and response must be enforced manually.
- It may become a halfway design that later wants to become the dedicated result object anyway.

#### Option C: Request-Owned Upgrade Descriptor

The request object holds upgrade approval state internally, and the pipeline queries that state after handler execution completes.

Sketch:

```cpp
class HttpContext : private IPipelineHandler
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

- `HttpContext` mutates its own internal state during routing or handler execution.
- `HttpPipeline` asks the request whether it approved upgrade.
- If yes, the pipeline asks the request to create the session or expose a descriptor for session creation.

Why it is attractive:

- Minimal surface-area change in the short term.
- Can be layered onto the existing `HttpContext` lifecycle with fewer immediate type changes.
- Keeps upgrade state close to routing decisions.

Why it may be awkward:

- It keeps important ownership and control flow implicit inside `HttpContext`.
- The request object becomes more stateful and harder to reason about as both HTTP and upgraded semantics coexist.
- Testing becomes more coupled to `HttpContext` internals rather than a clean pipeline seam.

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
- This keeps long-lived upgraded behavior out of `IHttpResponse` and avoids baking hidden upgrade state into `HttpContext`.

Ownership rules to enforce:

- `HttpContext` may construct the result, but it must not retain ownership of the post-routing outcome once the pipeline consumes it.
- `HttpPipeline` becomes the sole owner of the consumed result and any response stream or upgrade factory it carries.
- The server continues to own connection acceptance and top-level pipeline lifetime, but not the per-request result object after handoff.

Mutual exclusivity rule:

- Response and upgrade are mutually exclusive by type, not by convention.
- A `RequestHandlingResult` instance must represent exactly one terminal request outcome.
- `NoResponse` and `Error` remain explicit result kinds so the pipeline does not infer missing state from null pointers.

Implications for Phase 1:

- Phase 1 should explicitly replace callback-only response handoff with result-object consumption at the pipeline boundary.
- The `HttpContext` backlog tasks should assume result construction and transfer semantics instead of adding request-owned upgrade state.
- Phase 2 handshake work should build an upgrade result payload rather than introducing a separate upgrade side channel.

Alternatives not chosen:

- Option B, session-factory result: rejected for now because it preserves dual output channels from `HttpContext` and leaves exclusivity enforcement too implicit.
- Option C, request-owned upgrade descriptor: rejected because it concentrates too much hidden ownership and control flow inside `HttpContext`.

Suggested decision record for item 1:

- Chosen direction: Dedicated pipeline result object.
- Why this option fits the current codebase: The request-to-pipeline seam already has to change, and a single result object keeps that change explicit.
- What ownership rules must be enforced: `HttpPipeline` owns the consumed result and its payloads after handoff from `HttpContext`.
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
	RequestHandlingResult handleUpgradeRequest(const HttpContext &request,
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

RoutedUpgradeDecision HandlerProviderRegistry::tryResolveWebSocket(const HttpContext &request);
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

std::optional<UpgradeIntent> HttpContext::takeUpgradeIntent();
RequestHandlingResult HttpPipeline::buildUpgradeResult(const HttpContext &request,
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

- [x] Decide the HTTP status codes and response shape for unsupported method, missing headers, unsupported version, and malformed key cases.
- [x] Decide how much error detail is exposed to callers versus kept internal.
- [x] Decide whether all rejection paths share one response helper or remain case-specific.

Option sketches:

#### Option A: RFC-Aligned Per-Failure Status Codes

Each validation failure returns the status code the RFC or HTTP spec recommends for that exact failure mode.

Sketch:

```cpp
enum class UpgradeFailure
{
    MethodNotGet,          // -> 405 Method Not Allowed
    MissingUpgradeHeader,  // -> 426 Upgrade Required
    InvalidConnectionHeader, // -> 400 Bad Request
    UnsupportedVersion,    // -> 400 Bad Request + Sec-WebSocket-Version: 13
    MissingOrBadKey,       // -> 400 Bad Request
};

RequestHandlingResult rejectUpgrade(UpgradeFailure reason);
```

How it would flow:

- The dedicated upgrade handler checks each required field in order.
- On the first failure it calls `rejectUpgrade(reason)`, which maps the failure kind to a status code and optional extra header.
- All failure paths funnel through the same helper with no body text beyond a short static reason phrase.

Why it is attractive:

- Matches RFC 6455 expectations exactly, making interoperability with standards-compliant clients predictable.
- The `UpgradeFailure` enum makes every rejection case addressable in tests.
- The single `rejectUpgrade` helper is the only place status-code policy lives.

Why it may be awkward:

- The 426 path requires an `Upgrade: websocket` response header to be correct, which adds a small extra detail to the helper.
- Multiple status codes mean more test cases, though each is simple.

#### Option B: Unified 400 Bad Request For All Failures

Every validation failure produces a 400 response regardless of the specific failure mode.

Sketch:

```cpp
// Every check failure returns the same rejection:
return RequestHandlingResult::rejection(HttpStatus(400));
```

How it would flow:

- The upgrade handler checks required fields and returns a 400 on any failure.
- No failure-kind enum is needed; the response shape is uniform.
- The rejection carries no body and no extra headers.

Why it is attractive:

- Simpler implementation and fewer test cases to maintain.
- No risk of an incorrect status code for a specific failure path.

Why it may be awkward:

- Loses RFC semantic distinctions; method errors and bad-key errors appear identical to callers.
- A 400 for a method error is technically incorrect according to HTTP semantics.

#### Option C: Default RFC-Aligned Codes With Injectable Rejection Policy

The handler has a default RFC-aligned mapping but accepts an external policy hook so callers can override the response shape.

Sketch:

```cpp
using UpgradeRejectionPolicy =
    std::function<RequestHandlingResult(UpgradeFailure, const HttpContext &)>;

void WebSocketUpgradeHandler::setRejectionPolicy(UpgradeRejectionPolicy policy);
```

How it would flow:

- The default policy behaves identically to Option A.
- A caller may inject a custom policy to produce a different response shape for any failure kind.

Why it is attractive:

- Maximum flexibility without changing the default behavior.

Why it may be awkward:

- The injectable hook is extra surface area that no current use case requires.
- It can produce inconsistent behavior across installations if callers override inconsistently.

Working comparison:

- RFC-aligned per-failure codes: most correct, one shared helper, modest extra test coverage.
- Unified 400: simplest, loses HTTP semantic precision.
- Injectable policy: most flexible, adds surface area with no immediate payoff.

Initial recommendation:

- Prefer Option A (RFC-aligned per-failure status codes) with all rejection paths routing through one shared `rejectUpgrade` helper.
- Expose no error-detail body text beyond a short static reason phrase on each status.
- Do not add an injectable policy hook in the first release.

Decision:

- Chosen option: Option A, RFC-aligned per-failure status codes with a single shared `rejectUpgrade` helper.
- Status: Accepted.

Why this option fits the current codebase:

- RFC 6455 specifies distinct failure semantics (405 for method errors, 426 for missing upgrade intent, 400 for malformed keys) that standards-compliant clients depend on.
- A single `rejectUpgrade(UpgradeFailure)` helper in the dedicated upgrade handler (item 4) keeps all status-code policy in one auditable place.
- No body text or injectable policy hook reduces surface area and makes rejection behavior deterministic under native tests.

Alternatives not chosen:

- Option B, unified 400: rejected because it loses RFC semantic distinctions and a 400 for a wrong method is technically incorrect.
- Option C, injectable rejection policy: rejected because no current use case requires per-installation customization and it introduces surface area with no payoff.

Decision criteria:
- Prefer consistent externally visible behavior.
- Keep the policy simple enough that handshake tests are stable.
- Do not imply support for optional RFC features that are intentionally deferred.

### 6. Frame Codec Ownership And IO Model

- [x] Decide whether the reader and writer operate on raw `IByteChannel` calls, owned buffers, or intermediate byte-source abstractions.
- [x] Decide whether the frame writer should return buffers, streamable sources, or direct write operations.
- [x] Decide whether message assembly belongs inside the codec layer or in the session runtime.

Option sketches:

#### Option A: Codec Operates Directly On Transport Calls

The frame reader pulls bytes from `IByteSource` and the frame writer pushes bytes to `IClient` within the session step.

Sketch:

```cpp
class WebSocketFrameReader
{
public:
    // Reads directly from the live transport on each call.
    ReadFrameResult readFrame(IByteSource &source);
};

class WebSocketFrameWriter
{
public:
    // Writes a framed message directly to the live transport.
    WriteFrameResult writeFrame(IClient &client,
                                span<const uint8_t> payload,
                                WebSocketOpcode opcode,
                                bool fin = true);
};
```

How it would flow:

- The session step calls `readFrame` and `writeFrame` directly against the transport objects it already holds.
- No intermediate buffers are owned by the codec beyond small stack temporaries for header bytes.

Why it is attractive:

- Minimal moving parts; the codec is thin.
- Stack or transport-level buffering already exists in the pipeline.

Why it may be awkward:

- Unit tests must supply a fake `IByteSource` or `IClient`, coupling tests more tightly to transport abstractions.
- Mixing IO calls and parsing logic inside the same object makes the codec harder to reason about in isolation.

#### Option B: Pure-Transform Codec Operating On Caller-Supplied Byte Spans

The codec is transport-agnostic: it parses from a caller-supplied input span and serializes into a caller-supplied output span. The session runtime owns all actual IO.

Sketch:

```cpp
struct ParsedFrame
{
    WebSocketOpcode opcode;
    bool fin;
    bool masked;
    span<const uint8_t> payload; // points into input span
    std::size_t bytesConsumed;
};

class WebSocketFrameParser
{
public:
    // Parses one frame from the supplied input buffer.
    // Returns nullopt if more bytes are needed.
    std::optional<ParsedFrame> parse(span<const uint8_t> input);
};

class WebSocketFrameSerializer
{
public:
    // Fills the provided output buffer with a framed payload.
    // Returns number of bytes written (0 if buffer too small).
    std::size_t serialize(span<uint8_t> output,
                          span<const uint8_t> payload,
                          WebSocketOpcode opcode,
                          bool fin = true);
};
```

How it would flow:

- The session runtime reads raw bytes from the transport into a fixed-size stack buffer.
- It passes that buffer to `WebSocketFrameParser::parse`.
- For writes, it calls `serialize` into a stack or static output buffer, then writes that buffer to the transport.

Why it is attractive:

- The codec has no transport dependency and can be tested with plain `std::vector` or stack arrays.
- Partial-frame behavior is testable by passing truncated input spans without any fake transport.
- Fits cleanly with the pure-transform testing style already used in the native test lane.

Why it may be awkward:

- The session runtime must manage the read-buffer lifecycle and handle the common case where a partial frame spans two reads.
- Output buffer sizing requires the session to know the maximum frame header overhead in advance.

#### Option C: Codec Owns A Fixed-Size Internal Ring Buffer

The codec manages its own internal byte buffer; the session feeds raw bytes in and polls for parsed frames.

Sketch:

```cpp
class WebSocketFrameCodec
{
public:
    // Session feeds raw bytes from the transport into the codec.
    std::size_t feed(span<const uint8_t> raw);

    // Session polls for a complete parsed frame.
    std::optional<ParsedFrame> nextFrame();

    // Session requests serialization into the codec's own outbound buffer.
    bool prepareOutboundFrame(span<const uint8_t> payload,
                              WebSocketOpcode opcode);
    span<const uint8_t> pendingOutboundBytes() const;
    void consumeOutboundBytes(std::size_t n);
};
```

How it would flow:

- The session feeds chunks of raw bytes into the codec after each transport read.
- The codec accumulates and parses internally.
- For writes the codec holds serialized bytes until the session has drained them to the transport.

Why it is attractive:

- Partial-frame state is hidden inside the codec; the session never needs to reason about incomplete header bytes.

Why it may be awkward:

- The internal ring buffer requires a fixed allocation size known at compile time, which must be tuned for the target.
- The boundary between "codec buffer" and "session buffer" is an extra conceptual layer with limited benefit over Option B.

Message assembly placement:

- Under Option A and Option B, assembly of continuation frames should live in the session runtime rather than the codec. The codec delivers individual frames; the session decides when a complete message has arrived and fires the user callback.
- Under Option C, the codec could absorb continuation tracking, but the same argument applies: keeping assembly in the session keeps the codec simpler and continuation-limit enforcement testable at the session layer.

Working comparison:

- Pure-transform codec: best testability, session owns IO and assembly, most aligned with the native test lane.
- Transport-coupled codec: simplest call sites but poor unit-test isolation.
- Codec-owned ring buffer: hides partial-frame state but adds a buffer-management layer of limited value.

Initial recommendation:

- Prefer Option B (pure-transform codec) for the first implementation.
- Message assembly across continuation frames lives in the session runtime, not the codec.
- The session runtime manages read-buffer allocation and transport IO.

Decision:

- Chosen option: Option B, pure-transform codec operating on caller-supplied byte spans.
- Status: Accepted.

Why this option fits the current codebase:

- The native test harness models IO through fake byte buffers; a codec with no transport dependency is testable with plain `span` data and requires no fake `IByteSource` or `IClient`.
- The session runtime already owns the pipeline loop and transport access, so it is the natural place to manage read-buffer staging and write calls.
- Message assembly across continuation frames lives in the session runtime, not the codec, keeping the codec's responsibilities narrow and independently replaceable.

Alternatives not chosen:

- Option A, transport-coupled codec: rejected because it forces transport fakes into codec unit tests and mixes IO with parsing.
- Option C, codec-owned ring buffer: rejected because it adds a buffer-management layer of limited value over the session-owned buffer in Option B.

Decision criteria:
- Prefer incremental and bounded behavior over whole-message buffering.
- Keep the codec independently testable from routing and callbacks.
- Avoid a design that makes partial read or write testing awkward.

### 7. Fragmentation And Delivery Semantics

- [x] Decide whether the first public API delivers complete messages only or can expose fragments internally or publicly.
- [x] Decide whether continuation handling accumulates in memory or is streamed through bounded chunks.
- [x] Decide which fragmentation-related protocol errors are fatal immediately.

Option sketches:

#### Option A: Complete Messages Only

The session runtime assembles all continuation frames in memory before firing any user callback. The `WsMaxMessageSize` limit applies to the assembled total.

Sketch:

```cpp
// User callback always receives a complete assembled message.
void onText(WebSocketContext &ctx, std::string_view completeMessage);
void onBinary(WebSocketContext &ctx, span<const uint8_t> completeMessage);
```

How it would flow:

- The session runtime accumulates data frame payloads across continuation frames into a working buffer.
- When the final frame (`fin=true`) arrives, the runtime fires the appropriate callback with the complete span.
- If the accumulating message exceeds `WsMaxMessageSize`, the session terminates with a 1009 Message Too Big close code before the callback fires.

Why it is attractive:

- The simplest possible user API: callbacks always receive a complete, usable message.
- No fragmentation details leak into the public callback surface.
- Memory behaviour is bounded and predictable: one fixed working buffer per active session.

Why it may be awkward:

- Large messages require the entire payload to fit in memory simultaneously.
- On very constrained targets a single 8 KB message limit may still be too large for some configurations.

#### Option B: Per-Frame Delivery With Explicit `fin` Flag

The user callback receives each data frame payload individually, along with a `fin` flag indicating whether this was the final fragment.

Sketch:

```cpp
void onText(WebSocketContext &ctx, std::string_view fragment, bool isFinalFragment);
void onBinary(WebSocketContext &ctx, span<const uint8_t> fragment, bool isFinalFragment);
```

How it would flow:

- The session runtime fires the callback for every data frame as it arrives, passing `isFinalFragment` from the WebSocket `fin` bit.
- The user is responsible for accumulating if a complete message is needed.
- The runtime still enforces that no non-control frame interrupts a fragmentation sequence.

Why it is attractive:

- Zero additional assembly buffering: message size is unbounded as long as the user handles fragments.
- Useful for streaming large binary payloads on targets with enough flash but limited RAM.

Why it may be awkward:

- Exposes WebSocket framing semantics (the `fin` bit) in the public API.
- Most users will write their own assembler anyway, duplicating what the library could do once and correctly.
- Testing requires careful sequencing of fragment callbacks to cover all edge cases.

#### Option C: Library-Assembled Chunks With Bounded Working Buffer

The session runtime assembles continuation frames but delivers the message in chunks once the working buffer fills, rather than refusing oversized messages outright.

Sketch:

```cpp
void onText(WebSocketContext &ctx, std::string_view chunk, bool isLastChunk);
void onBinary(WebSocketContext &ctx, span<const uint8_t> chunk, bool isLastChunk);
```

How it would flow:

- The runtime fills a fixed working buffer. When it is full mid-message it fires the callback with `isLastChunk=false` and resets the buffer.
- When the final fragment arrives, it fires with `isLastChunk=true`.
- The working buffer size is controlled by `WsMaxFramePayloadSize`.

Why it is attractive:

- Handles arbitrarily large messages within a fixed memory budget.
- The `isLastChunk` flag is less protocol-specific than Option B's raw `fin` bit.

Why it may be awkward:

- The callback contract is more complex than Option A and harder to explain than Option B.
- `isLastChunk=false` mid-message is unusual in embedded use cases and may surprise users.

Protocol-error fatality rules:

- A non-control frame arriving while a fragmentation sequence is in progress is a fatal protocol error (close with 1002).
- A continuation frame arriving with no preceding unfragmented sequence open is a fatal protocol error (close with 1002).
- A message exceeding the configured size limit is a fatal application error (close with 1009 under Option A; the session must close immediately).

Working comparison:

- Complete messages only: best user ergonomics, bounded memory, smallest callback surface.
- Per-frame with fin flag: most flexible, exposes protocol internals, highest user burden.
- Chunked delivery: handles large messages but adds callback complexity neither extreme requires.

Initial recommendation:

- Prefer Option A (complete messages only) for the first release.
- Enforce `WsMaxMessageSize` as a hard limit with immediate close on excess.
- Protocol fragmentation errors (unexpected continuation, interrupted sequence) are fatal immediately.

Decision:

- Chosen option: Option A, complete messages only.
- Status: Accepted.

Why this option fits the current codebase:

- Complete-message delivery gives the simplest possible callback contract and avoids exposing WebSocket framing details in the public API.
- A single fixed working buffer per session satisfies the embedded memory constraint with predictable RAM usage.
- `WsMaxMessageSize` enforced as a hard limit with a 1009 close prevents unbounded accumulation.
- Protocol fragmentation violations (non-control frame mid-sequence, orphan continuation) close immediately with 1002, which is deterministic and easy to assert in native tests.

Alternatives not chosen:

- Option B, per-frame delivery with fin flag: rejected because it exposes protocol internals and places assembly burden on every user.
- Option C, chunked delivery: rejected because the `isLastChunk=false` contract adds complexity with no clear embedded use case.

Decision criteria:
- Stay within embedded memory constraints.
- Keep the initial public API small.
- Make protocol-error handling deterministic.

### 8. Outbound Send Model

- [x] Decide whether outbound sends are synchronous in the pipeline loop or mediated by a bounded queue.
- [x] Decide when server-initiated sends are legal from the eventual public API.
- [x] Decide what happens when a session exceeds outbound capacity.

Option sketches:

#### Option A: Synchronous Direct Write In The Pipeline Loop

All outbound writes happen synchronously inside the session's `handle()` step. There is no outbound queue.

Sketch:

```cpp
class WebSocketContext
{
public:
    // Legal only during a session handle() step (i.e., from within a callback invoked by handle).
    WriteResult sendText(std::string_view message);
    WriteResult sendBinary(span<const uint8_t> data);
    WriteResult sendClose(uint16_t code = 1000, std::string_view reason = "");
    WriteResult sendPong(span<const uint8_t> data = {});
};
```

How it would flow:

- During `session->handle(client, clock)`, the runtime fires user callbacks.
- User callbacks call `ctx.send*()`, which writes frames directly to `IClient` and returns a `WriteResult`.
- If a write is incomplete, the session retains pending state and returns `ConnectionSessionResult::Continue` so the pipeline retries on the next iteration.
- No frame state persists between loop iterations except partial-write resume data.

Why it is attractive:

- Zero per-session queue memory overhead.
- Ownership model is simple: the pipeline always holds the transport; sends happen inside session steps only.
- Native tests can drive the session synchronously without simulating async delivery.

Why it may be awkward:

- A slow `IClient::write` call can stall the loop iteration if the transport buffers are full.
- Sends can only originate from within handle-initiated callbacks; there is no way to enqueue a send from outside the pipeline loop.

#### Option B: Bounded FIFO Outbound Queue Drained Per Loop Iteration

The session holds a fixed-depth queue of pending outbound frames. Each `handle()` step drains up to a budget of frames before returning.

Sketch:

```cpp
#ifndef HTTPSERVER_ADVANCED_WEBSOCKET_SEND_QUEUE_DEPTH
    constexpr std::size_t WsSendQueueDepth = 4;
#else
    constexpr std::size_t WsSendQueueDepth = HTTPSERVER_ADVANCED_WEBSOCKET_SEND_QUEUE_DEPTH;
#endif

class WebSocketContext
{
public:
    // Enqueues a frame; returns false if the queue is full.
    bool sendText(std::string_view message);
    bool sendBinary(span<const uint8_t> data);
    bool sendClose(uint16_t code = 1000, std::string_view reason = "");
};
```

How it would flow:

- User callbacks enqueue frames into the session-owned bounded FIFO.
- `handle()` drains queued frames to the transport up to a per-iteration frame budget.
- If the queue is full, `send*` returns `false` immediately without blocking.

Why it is attractive:

- Decouples user send calls from transport availability.
- The per-iteration frame budget prevents one session from monopolizing the loop.

Why it may be awkward:

- Each queued frame requires a copy of the payload, adding per-session RAM for up to `WsSendQueueDepth` buffered messages.
- Queue-full behavior (return `false`) requires users to handle send failures explicitly.
- On single-client embedded targets the queue adds memory overhead with no real scheduling benefit.

#### Option C: Direct Write With Bounded Partial-Write Resume State

Sends are synchronous like Option A, but the session explicitly tracks a partial-write resume span so interrupted writes retry cleanly without re-entering user code.

Sketch:

```cpp
struct PendingWrite
{
    std::array<uint8_t, WsMaxFramePayloadSize + 10> frame; // serialized frame
    std::size_t frameLength;
    std::size_t bytesSent;

    bool hasMore() const { return bytesSent < frameLength; }
    span<const uint8_t> remaining() const;
};
```

How it would flow:

- The session serializes a frame once and stores the serialized bytes in `PendingWrite`.
- `handle()` attempts to drain `remaining()` on each step.
- Only one outbound frame is in flight at a time; a new send call is rejected while one is pending.

Why it is attractive:

- No queue memory overhead beyond one serialized frame.
- Partial-write behavior is internal and transparent to user callbacks.

Why it may be awkward:

- Only one pending outbound frame at a time; if a callback tries to send while a previous write is still draining, it gets an error or must wait.
- The single-frame buffer size adds a fixed RAM cost per session regardless of activity.

Server-initiated send legality rule:

- Under all three options, outbound sends are legal only from within callbacks invoked during `handle()` in the first release.
- Sends from outside the pipeline loop (for example, from a timer or another thread) are not supported in the initial model. This constraint should be documented explicitly.

Capacity-exceeded behavior:

- Option A: partial write tracked internally; no capacity concept.
- Option B: `send*` returns `false`; the caller decides whether to retry or close.
- Option C: send rejected while a previous frame is still draining; caller receives an error result.

Working comparison:

- Synchronous direct write: simplest, zero queue RAM, easiest to test.
- Bounded FIFO queue: most flexible for bursty sends, highest RAM cost per session.
- Direct write with partial-write resume: middle ground, one-frame-in-flight constraint.

Initial recommendation:

- Prefer Option A (synchronous direct write) for the first implementation.
- Server-initiated sends are legal only from within `handle()`-invoked callbacks.
- Capacity exhaustion is handled by retaining partial-write state across loop iterations; the pipeline returns `Continue` until the transport drains.

Decision:

- Chosen option: Option A, synchronous direct write in the pipeline loop.
- Status: Accepted.

Why this option fits the current codebase:

- The pipeline already drives all work through a single loop iteration; a direct write fits naturally without introducing per-session queue memory.
- Partial-write state retained across iterations is sufficient for embedded targets where transport buffers are small but the pipeline loop runs fast.
- Sends legal only from within `handle()`-invoked callbacks gives a clear ownership rule with no concurrency complications.
- The native test harness can simulate partial-write scenarios by controlling fake transport write availability.

Server-initiated send rule:

- Outbound sends are legal only from within callbacks invoked during a `handle()` step in the first release.
- Sends from outside the pipeline loop (timers, other threads) are not supported and must be documented as unsupported.

Alternatives not chosen:

- Option B, bounded FIFO queue: rejected because it adds per-session RAM overhead with no proven scheduling benefit on single-connection embedded targets.
- Option C, direct write with partial-write resume state: rejected as a separate option because the partial-write resume mechanism is already embedded in Option A's approach.

Decision criteria:
- Avoid one connection monopolizing the main loop.
- Keep backpressure behavior explicit rather than implicit.
- Choose a model that the native test harness can simulate cleanly.

### 9. Control Frame Exposure

- [x] Decide whether ping and pong remain internal in the first release or are exposed as callbacks.
- [x] Decide whether close events expose raw status codes, simplified reasons, or both.
- [x] Decide which control-frame policy is public API versus strictly internal behavior.

Option sketches:

#### Option A: All Control Frames Fully Internal

PING, PONG, and CLOSE frames are handled entirely by the session runtime with no user callbacks.

Sketch:

```cpp
// No onPing, onPong callbacks exist.
// Session runtime sends an automatic PONG to every incoming PING.
// Session runtime performs the close handshake on CLOSE receipt without user interaction.
// User is informed only that the session ended, via no remaining handle() calls.
```

How it would flow:

- An incoming PING triggers an automatic PONG during the same `handle()` step.
- An incoming CLOSE triggers the session to complete the close handshake and transition to `Completed`.
- The user is never notified about ping or pong exchanges.
- The user is notified of session end only implicitly through no further callbacks firing.

Why it is attractive:

- Absolute minimum public API surface.
- Protocol obligations cannot be mis-implemented by users.

Why it may be awkward:

- Users have no way to know the connection closed or why; for embedded applications that log or react to disconnect, this is often insufficient.
- No way for the user to observe idle keepalive state.

#### Option B: Optional Callbacks For PING, PONG, And CLOSE

All three control frame types expose optional `std::function` hooks with sane defaults when unset.

Sketch:

```cpp
struct WebSocketCallbacks
{
    // Optional: called when client sends PING. Default: send automatic PONG.
    std::function<void(WebSocketContext &, span<const uint8_t> pingData)> onPing;
    // Optional: called when client sends PONG.
    std::function<void(WebSocketContext &, span<const uint8_t> pongData)> onPong;
    // Called when connection closes. Provides raw close code and reason.
    std::function<void(WebSocketContext &, uint16_t code, std::string_view reason)> onClose;
};
```

How it would flow:

- If `onPing` is unset, the runtime sends an automatic PONG.
- If `onPing` is set, the user decides whether to PONG (by calling `ctx.sendPong()`).
- `onClose` fires for both remote-initiated and local close-handshake completion.

Why it is attractive:

- Flexible; advanced users can observe all control frame activity.
- The `onPing` default preserves protocol correctness when the callback is omitted.

Why it may be awkward:

- `onPing` and `onPong` have very narrow embedded use cases; they add documentation and test burden for minor benefit.
- If `onPing` is set but does not send PONG, the runtime must still enforce PONG automatically or the default protocol obligation breaks.

#### Option C: Close-Only Callback With Internal PING/PONG

Only the close event is exposed to users. PING and PONG are always handled internally. The close callback carries both the raw close code and reason string.

Sketch:

```cpp
struct WebSocketCallbacks
{
    std::function<void(WebSocketContext &)> onOpen;
    std::function<void(WebSocketContext &, std::string_view text)> onText;
    std::function<void(WebSocketContext &, span<const uint8_t> data)> onBinary;
    // Fires for both graceful close and abnormal termination.
    // code: WebSocket close status code (1000, 1001, etc.).
    // reason: UTF-8 close reason from CLOSE frame, empty string if none or abnormal close.
    std::function<void(WebSocketContext &, uint16_t code, std::string_view reason)> onClose;
    std::function<void(WebSocketContext &)> onError;
};
```

How it would flow:

- PING → automatic PONG internally; user is never notified.
- PONG → discarded internally (no user-initiated pings in first release).
- Remote-initiated CLOSE → runtime completes handshake, fires `onClose` with the remote code and reason.
- Abnormal disconnect or timeout → `onClose` fires with 1006 (Abnormal Closure) and empty reason, or `onError` fires first.

Why it is attractive:

- Satisfies the dominant embedded use case: knowing when and why the connection ended.
- Keeps automatic PONG behavior fully internal without any callback interaction.
- The close code and reason together are sufficient for most application-level disconnect handling.

Why it may be awkward:

- Users who need to suppress or customize PONG behavior have no supported path in the first release.

Working comparison:

- All internal: narrowest API, insufficient for most real applications.
- Optional ping/pong and close: most complete, more surface area than first release needs.
- Close-only with internal ping/pong: useful to applications without exposing low-value control-frame hooks.

Initial recommendation:

- Prefer Option C (close-only callback with internal PING/PONG) for the first release.
- The `onClose` callback should carry the raw WebSocket close code and reason string.
- Automatic PONG behavior is documented as guaranteed when PING is received.
- PING and PONG observation hooks are deferred to a future release if a use case arises.

Decision:

- Chosen option: Option C, close-only callback with internal PING/PONG handling.
- Status: Accepted.

Why this option fits the current codebase:

- Close notification with raw code and reason satisfies the dominant embedded use case (knowing when and why a connection ended) without exposing low-value control-frame hooks.
- Automatic PONG keeps the protocol obligation guaranteed-correct and independent of user code.
- The `onError` callback provides a separate signal for unrecoverable errors, complementing `onClose` for abnormal termination paths.
- This matches the item 11 callback struct shape directly and keeps the first-release surface stable.

Public control-frame policy rule:

- PING receipt → automatic PONG sent internally; no user callback.
- PONG receipt → discarded internally; no user callback.
- CLOSE receipt → runtime completes handshake, fires `onClose(context, code, reason)`.
- Abnormal disconnect → `onClose` fires with 1006 and empty reason, or `onError` fires first if the failure is protocol-level.

Alternatives not chosen:

- Option A, all internal: rejected because applications need to know when and why a connection closed.
- Option B, optional ping/pong and close callbacks: rejected because PING and PONG observation have no clear embedded use case in the first release.

Decision criteria:
- Keep the public callback surface intentionally small.
- Expose only what has a clear embedded use case.
- Keep automatic protocol obligations internal where possible.

### 10. Public Route Registration Shape

- [x] Decide the registration entry point naming and builder style for WebSocket endpoints.
- [x] Decide whether session creation is function-based, type-based, or builder-object based.
- [x] Decide how WebSocket routes coexist with existing HTTP routes that share nearby patterns.

Option sketches:

#### Option A: WebSocket Handler Tag Type In The Existing `on<THandler>` Template Surface

WebSocket routes are registered via the same `on<THandler>` template already used for HTTP handlers.

Sketch:

```cpp
// Registration
handlers().on<WebSocket>("/chat", WebSocketCallbacks
{
    .onOpen  = [](WebSocketContext &ctx) { ... },
    .onText  = [](WebSocketContext &ctx, std::string_view msg) { ... },
    .onClose = [](WebSocketContext &ctx, uint16_t code, std::string_view) { ... }
});
```

How it would flow:

- `WebSocket` is a new handler tag type alongside `GetRequest`, `PostRequest`, etc.
- `ProviderRegistryBuilder::on<WebSocket>` specialization maps the callbacks struct into an upgrade-capable route.
- The `HandlerMatcher` for a WebSocket route implicitly restricts to GET with the required upgrade headers.

Why it is attractive:

- Consistent with existing call sites; users familiar with `on<GetRequest>` can infer the WebSocket form.
- Route coexistence with HTTP handlers is handled by the existing provider registry ordering.

Why it may be awkward:

- The `WebSocket` tag type needs a different `Invocation` signature than all other handler types, requiring template specializations that may feel forced.
- Forcing WebSocket callbacks through the `THandler` shape risks awkward abstractions in `HandlerTypes.h`.

#### Option B: Dedicated `.websocket()` Method On The Builder

WebSocket routes are registered through a new method on `ProviderRegistryBuilder` that is separate from the HTTP `on<THandler>` path.

Sketch:

```cpp
// Registration
handlers().websocket("/chat", WebSocketCallbacks
{
    .onOpen  = [](WebSocketContext &ctx) { ... },
    .onText  = [](WebSocketContext &ctx, std::string_view msg) { ... },
    .onClose = [](WebSocketContext &ctx, uint16_t code, std::string_view) { ... }
});

// With route parameters:
handlers().websocket(ParameterizedUri("/room/:id"), WebSocketCallbacks
{
    .onOpen = [](WebSocketContext &ctx) { ... }
});
```

How it would flow:

- `ProviderRegistryBuilder::websocket()` constructs a WebSocket-aware predicate (GET + Upgrade headers) and registers an upgrade-capable handler factory.
- The resulting handler interacts with the dedicated upgrade handler from item 4.
- Coexistence: a WebSocket route for `/chat` and an HTTP GET route for `/chat` resolve independently; the upgrade predicate matches only requests with the WebSocket upgrade intent.

Why it is attractive:

- Clean separation: WebSocket registration does not need to fit into the `THandler` template machinery.
- The method name makes the intent immediately visible at the call site.
- No compatibility friction with existing HTTP handler types.

Why it may be awkward:

- Extends the public builder API surface with a new method.
- Requires its own documentation, test coverage, and any future extension also touches this method.

#### Option C: Free-Standing Factory Function Returning A Configured Provider

WebSocket routes are registered by calling a free function that produces a pre-configured handler provider.

Sketch:

```cpp
// Registration
handlers().add(websocketAt("/chat", WebSocketCallbacks
{
    .onText  = [](WebSocketContext &ctx, std::string_view msg) { ... },
    .onClose = [](WebSocketContext &ctx, uint16_t code, std::string_view) { ... }
}));
```

How it would flow:

- `websocketAt()` is a free function that returns an `IHandlerProvider` (or predicate + factory pair).
- The call is composable with any existing mechanism that accepts a provider.
- No change to the builder API surface itself.

Why it is attractive:

- Zero builder API changes.
- Can be added to any project that already holds a `ProviderRegistryBuilder` reference.

Why it may be awkward:

- Less discoverable than a method on the builder; there is no IDE autocomplete path from the builder to `websocketAt`.
- The call site looks unlike regular HTTP handler registration, making the API feel inconsistent.

Coexistence rule (all options):

- A WebSocket route for a given path and an HTTP route for the same path can coexist in the registry.
- The WebSocket upgrade predicate (GET + `Upgrade: websocket` + `Connection: Upgrade`) is specific enough that normal GET routes for the same path will not interfere.
- Route ordering in the registry determines precedence if two applicable entries are present.

Working comparison:

- Dedicated `.websocket()` method: cleanest call site, clear separation, modest API extension.
- Handler tag type: maximally consistent pattern, awkward template-shape fit.
- Free function: no API change, low discoverability.

Initial recommendation:

- Prefer Option B (dedicated `.websocket()` method) for the first implementation.
- The method should accept a path string or `HandlerMatcher` and a `WebSocketCallbacks` struct.
- Coexistence with HTTP routes for the same path is supported by predicate specificity, not by enforced exclusion.

Decision:

- Chosen option: Option B, dedicated `.websocket()` method on `ProviderRegistryBuilder`.
- Status: Accepted.

Why this option fits the current codebase:

- The `ProviderRegistryBuilder` is the existing extension point for all route registration; adding `.websocket()` as a sibling to `on<THandler>` is the least disruptive integration path.
- WebSocket registration semantics (upgrade-intent predicate, callbacks struct, session lifetime) do not fit cleanly into the `THandler` template shape, so a dedicated method avoids forcing awkward template specializations.
- Call-site readability is high: `handlers().websocket("/path", callbacks)` communicates intent without implementation archaeology.

Coexistence rule:

- A WebSocket route and an HTTP route for the same URI pattern coexist via predicate specificity: the WebSocket predicate (GET + `Upgrade: websocket` + `Connection: Upgrade`) matches only upgrade-intent requests and does not fire for plain HTTP requests to the same path.
- No registry-level enforcement of path exclusivity is required.

Alternatives not chosen:

- Option A, `THandler` tag type for WebSocket: rejected because the `THandler`-shaped constraints do not match the WebSocket callback signature and would require awkward template specializations.
- Option C, free-standing factory function: rejected because it is less discoverable and produces call sites inconsistent with all other route registration.

Decision criteria:
- Preserve a clean separation from existing response-centric HTTP handler APIs.
- Favor direct, readable call sites over transitional compatibility layers.
- Keep route matching rules understandable without implementation archaeology.

### 11. Public Callback Surface

- [x] Decide the exact first-release callback set: open, text, binary, close, error, and any optional control-frame hooks.
- [x] Decide whether callbacks receive a session object, raw payload spans, owned buffers, or higher-level message wrappers.
- [x] Decide what callback ordering guarantees will be documented.

Option sketches:

#### Option A: Struct Of `std::function` Callbacks

All per-session events are represented as named `std::function` fields in a single aggregate.

Sketch:

```cpp
struct WebSocketCallbacks
{
    // Fired once when the upgrade handshake completes and the session is open.
    std::function<void(WebSocketContext &)> onOpen;

    // Fired for each complete incoming text message.
    std::function<void(WebSocketContext &, std::string_view text)> onText;

    // Fired for each complete incoming binary message.
    std::function<void(WebSocketContext &, span<const uint8_t> data)> onBinary;

    // Fired when the connection closes; code is the WebSocket close status code.
    // reason is the UTF-8 close reason string, or empty for abnormal closure.
    std::function<void(WebSocketContext &, uint16_t code, std::string_view reason)> onClose;

    // Fired on unrecoverable protocol or write errors before the connection closes.
    std::function<void(WebSocketContext &)> onError;
};
```

How it would flow:

- The user constructs a `WebSocketCallbacks` value at registration time.
- The session runtime stores a copy and invokes the relevant field for each event during `handle()`.
- Unset callbacks are null-checked at invocation time; a null `onOpen`, `onText`, or `onBinary` means the event is silently discarded.
- `onClose` should always be set; the runtime may log a warning if it is null and the connection closes.

Why it is attractive:

- Aggregate initialization in C++17 is readable at call sites even without designated initializers.
- All callbacks are visible together; users can see the full surface at a glance.
- No heap-allocated virtual object per session beyond the `std::function` captures.

Why it may be awkward:

- Null-checking every callback field inside the runtime is slightly verbose.
- There is no compile-time enforcement that any required callback (such as `onClose`) is actually set.

#### Option B: Virtual Interface Per Session Handler

The user implements a virtual class for each WebSocket endpoint type.

Sketch:

```cpp
class IWebSocketHandler
{
public:
    virtual ~IWebSocketHandler() = default;
    virtual void onOpen(WebSocketContext &) {}
    virtual void onText(WebSocketContext &, std::string_view) {}
    virtual void onBinary(WebSocketContext &, span<const uint8_t>) {}
    virtual void onClose(WebSocketContext &, uint16_t code, std::string_view reason) = 0;
    virtual void onError(WebSocketContext &) {}
};

// Registration supplies a factory that creates one handler per accepted connection.
handlers().websocket("/chat",
    []() -> std::unique_ptr<IWebSocketHandler> { return std::make_unique<ChatHandler>(); }
);
```

How it would flow:

- The session runtime holds a `unique_ptr<IWebSocketHandler>` created by the registered factory.
- Each event dispatches to the appropriate virtual method.
- Default no-op implementations mean uninteresting events require no override.

Why it is attractive:

- Natural object-oriented grouping of per-connection state alongside callbacks.
- `onClose` can be made pure virtual to enforce implementation.

Why it may be awkward:

- Requires a named class per WebSocket endpoint; a lambda-only call site is not possible.
- The factory indirection adds a type to define and document.
- Heap allocation per accepted connection for the handler instance.

#### Option C: Builder-Style Per-Event Lambda Registration

Events are registered individually on a builder object returned from the `.websocket()` call.

Sketch:

```cpp
handlers()
    .websocket("/chat")
    .onText([](WebSocketContext &ctx, std::string_view msg)
    {
        ctx.sendText(msg); // echo
    })
    .onClose([](WebSocketContext &ctx, uint16_t code, std::string_view)
    {
        // log or react
    });
```

How it would flow:

- `.websocket(path)` returns a `WebSocketRouteBuilder` that captures lambdas and commits to the registry on destruction (same pattern as `HandlerBuilder`).
- Internally the builder accumulates callbacks into a `WebSocketCallbacks` struct and registers on scope exit.

Why it is attractive:

- Fluent chaining; only wire up the events relevant to each route.
- Consistent with how `HandlerBuilder` works today.

Why it may be awkward:

- An additional builder type to define, document, and maintain.
- The builder destructor-commit pattern can be surprising when the builder is stored or moved.

Callback payload rules (applies to all options):

- Text callbacks receive a `std::string_view` into a session-owned buffer valid only for the duration of the callback. The user must copy if longer-lived access is needed.
- Binary callbacks receive a `span<const uint8_t>` with the same lifetime constraint.
- Close callbacks receive `uint16_t code` by value and `std::string_view reason` into a session-owned buffer.
- No owned buffers or heap-allocated message wrappers are passed in the first release.

Callback ordering guarantees:

- `onOpen` fires exactly once, before any message or close callback.
- Message callbacks (`onText`, `onBinary`) fire in the order messages are received.
- `onClose` fires exactly once, after the last message callback and before the session is destroyed.
- `onError` may fire before `onClose`; if both fire, `onError` fires first.

Working comparison:

- Struct of callbacks: simple aggregate, easiest call sites, requires null-checking at invocation.
- Virtual interface: natural per-connection state holder, requires a named class and factory.
- Builder-style per-event: most ergonomic chaining, adds a builder type.

Initial recommendation:

- Prefer Option A (struct of `std::function` callbacks) for the first release.
- Payload lifetimes are view-only, no owned buffers.
- Ordering guarantees are documented as listed above.

Decision:

- Chosen option: Option A, struct of `std::function` callbacks (`WebSocketCallbacks`).
- Status: Accepted.

Why this option fits the current codebase:

- Aggregate initialization in C++17 is concise at call sites (`WebSocketCallbacks{ .onText = ..., .onClose = ... }`) without requiring a named class per endpoint.
- Passing a struct to `.websocket()` (item 10) is consistent with how other configuration aggregates are passed in the library.
- No heap-allocated virtual dispatch per connection beyond the `std::function` captures already present in scope-captured lambdas.
- The five-field set (onOpen, onText, onBinary, onClose, onError) is aligned with the item 9 close-only policy and item 7 complete-message delivery decision.

Callback payload rules:

- `onText` receives `std::string_view` into a session-owned buffer valid only for the duration of the call.
- `onBinary` receives `span<const uint8_t>` with identical lifetime.
- `onClose` receives `uint16_t code` by value and `std::string_view reason` into a session-owned buffer.
- No owned buffers or heap-allocated message wrappers are passed in the first release.

Ordering guarantees:

- `onOpen` fires exactly once before any message or close callback.
- Message callbacks fire in receive order.
- `onClose` fires exactly once after the last message callback.
- `onError` fires before `onClose` when both occur.

Alternatives not chosen:

- Option B, virtual interface per handler: rejected because it requires a named class and factory per endpoint and adds heap allocation per accepted connection.
- Option C, builder-style per-event registration: rejected because it introduces a new builder type purely to avoid the callbacks struct, with no call-site advantage.

Decision criteria:
- Keep the API narrow enough to stabilize quickly.
- Avoid defaulting to heap-heavy abstractions unless required.
- Keep the eventual test surface precise and deterministic.

### 12. Limits And Constant Names

- [x] Decide which WebSocket limits are compile-time constants in the first release: frame size, message size, queue depth, idle timeout, and any close timeout.
- [x] Decide the default values for those limits.
- [x] Decide the macro and `static constexpr` naming scheme before implementation lands.

Option sketches:

#### Option A: All Limits As Compile-Time `HTTPSERVER_ADVANCED_` Overrideable Constants

All WebSocket limits follow the existing project pattern in `Defines.h` exactly.

Sketch:

```cpp
// Maximum WebSocket frame payload size accepted or sent.
#ifndef HTTPSERVER_ADVANCED_WEBSOCKET_MAX_FRAME_PAYLOAD_SIZE
    constexpr std::size_t WsMaxFramePayloadSize = 4096;
#else
    constexpr std::size_t WsMaxFramePayloadSize =
        HTTPSERVER_ADVANCED_WEBSOCKET_MAX_FRAME_PAYLOAD_SIZE;
#endif

// Maximum assembled message size before a 1009 close is sent.
#ifndef HTTPSERVER_ADVANCED_WEBSOCKET_MAX_MESSAGE_SIZE
    constexpr std::size_t WsMaxMessageSize = 8192;
#else
    constexpr std::size_t WsMaxMessageSize =
        HTTPSERVER_ADVANCED_WEBSOCKET_MAX_MESSAGE_SIZE;
#endif

// Idle timeout: close with 1001 if no message or ping/pong in this window.
#ifndef HTTPSERVER_ADVANCED_WEBSOCKET_IDLE_TIMEOUT_MS
    constexpr uint32_t WsIdleTimeoutMs = 30000UL;
#else
    constexpr uint32_t WsIdleTimeoutMs =
        HTTPSERVER_ADVANCED_WEBSOCKET_IDLE_TIMEOUT_MS;
#endif

// Time allowed for the close handshake to complete before forceful teardown.
#ifndef HTTPSERVER_ADVANCED_WEBSOCKET_CLOSE_TIMEOUT_MS
    constexpr uint32_t WsCloseTimeoutMs = 2000UL;
#else
    constexpr uint32_t WsCloseTimeoutMs =
        HTTPSERVER_ADVANCED_WEBSOCKET_CLOSE_TIMEOUT_MS;
#endif
```

How it would flow:

- All limits are global compile-time constants identical in structure to `PIPELINE_STACK_BUFFER_SIZE` and friends.
- Build flags (`-DHTTPSERVER_ADVANCED_WEBSOCKET_MAX_FRAME_PAYLOAD_SIZE=2048`) override defaults at compile time.
- Code reads the `constexpr` variable name, never the raw macro.

Why it is attractive:

- Zero new patterns or mechanisms; consistent with every other limit in the library.
- Easy to override per-build for targets with tighter RAM budgets.
- Names are stable and grep-friendly.

Why it may be awkward:

- All limits are global; there is no per-route or per-connection override path.
- If a deployment needs different frame-size limits for two different WebSocket endpoints, this model cannot express that without recompilation.

#### Option B: Compile-Time Size Limits, Runtime Timeout Configuration Per Registration

Frame and message size limits are compile-time constants, but timeout values are configurable per route registration.

Sketch:

```cpp
// Compile-time:
constexpr std::size_t WsMaxFramePayloadSize = 4096;
constexpr std::size_t WsMaxMessageSize = 8192;

// Runtime, per registration:
struct WebSocketTimeouts
{
    uint32_t idleTimeoutMs  = 30000;
    uint32_t closeTimeoutMs = 2000;
};

handlers().websocket("/chat", callbacks, WebSocketTimeouts{ .idleTimeoutMs = 60000 });
```

How it would flow:

- Frame and message sizes stay compile-time because they determine buffer sizes.
- Timeouts are passed as an optional aggregate at registration; the default uses the compile-time values.

Why it is attractive:

- Allows per-endpoint timeout tuning (for example, a long-poll WebSocket versus a real-time sensor feed).

Why it may be awkward:

- Introduces a new `WebSocketTimeouts` struct and a more complex registration signature before any use case demands it.
- Inconsistent with how all other timeouts are handled in the library today.

#### Option C: Single `WebSocketLimits` Struct With Compile-Time Defaults

All limits are gathered into a single struct with compile-time-defaulted fields, passed optionally at registration.

Sketch:

```cpp
struct WebSocketLimits
{
    std::size_t maxFramePayloadSize = 4096;
    std::size_t maxMessageSize = 8192;
    uint32_t idleTimeoutMs = 30000;
    uint32_t closeTimeoutMs = 2000;
};

handlers().websocket("/chat", callbacks, WebSocketLimits{ .maxMessageSize = 16384 });
```

How it would flow:

- Users can override any limit per route by passing a `WebSocketLimits` value.
- If omitted, library defaults apply.

Why it is attractive:

- Per-route customization of all limits in one aggregate.

Why it may be awkward:

- Buffer sizes that drive stack or static allocations cannot be per-route at compile time; this model could claim to support it at runtime but would actually require dynamic buffer allocation.
- Breaks the existing `HTTPSERVER_ADVANCED_` macro override pattern.

Proposed constant names and default values (for Option A):

| constexpr name | Macro | Default | Rationale |
|---|---|---|---|
| `WsMaxFramePayloadSize` | `HTTPSERVER_ADVANCED_WEBSOCKET_MAX_FRAME_PAYLOAD_SIZE` | 4096 | One Ethernet MTU with headroom; fits a typical small JSON message |
| `WsMaxMessageSize` | `HTTPSERVER_ADVANCED_WEBSOCKET_MAX_MESSAGE_SIZE` | 8192 | Two frame payloads; conservative for RP2040 heap |
| `WsIdleTimeoutMs` | `HTTPSERVER_ADVANCED_WEBSOCKET_IDLE_TIMEOUT_MS` | 30000 | 30 s; generous for embedded keepalive |
| `WsCloseTimeoutMs` | `HTTPSERVER_ADVANCED_WEBSOCKET_CLOSE_TIMEOUT_MS` | 2000 | 2 s; enough for compliant clients to echo CLOSE |

Working comparison:

- All compile-time constants: most consistent with existing library pattern, no per-route flexibility.
- Runtime timeouts only: useful middle ground if per-endpoint timeout tuning is a real need.
- Full runtime limits struct: most flexible on paper but conflicts with compile-time buffer sizing.

Initial recommendation:

- Prefer Option A (all compile-time constants) for the first implementation, following the existing `Defines.h` pattern exactly.
- Defaults should be conservative for RP2040 and ESP-class targets as listed above.
- Per-route limit customization can be revisited once concrete use cases are identified.

Decision:

- Chosen option: Option A, all limits as compile-time `HTTPSERVER_ADVANCED_WEBSOCKET_*` overrideable constants in `Defines.h`.
- Status: Accepted.

Why this option fits the current codebase:

- The existing `Defines.h` pattern is the established project convention and requires no new mechanisms.
- Buffer sizes that drive stack allocations must be compile-time; runtime-configurable sizes would require dynamic allocation inconsistent with the embedded target constraints.
- The `HTTPSERVER_ADVANCED_` prefix and `constexpr` variable names are already familiar to users who have tuned other library limits.

Authoritative constant set and defaults:

| `constexpr` name | Macro | Default |
|---|---|---|
| `WsMaxFramePayloadSize` | `HTTPSERVER_ADVANCED_WEBSOCKET_MAX_FRAME_PAYLOAD_SIZE` | 4096 |
| `WsMaxMessageSize` | `HTTPSERVER_ADVANCED_WEBSOCKET_MAX_MESSAGE_SIZE` | 8192 |
| `WsIdleTimeoutMs` | `HTTPSERVER_ADVANCED_WEBSOCKET_IDLE_TIMEOUT_MS` | 30000 |
| `WsCloseTimeoutMs` | `HTTPSERVER_ADVANCED_WEBSOCKET_CLOSE_TIMEOUT_MS` | 2000 |

All constants must be defined in `Defines.h` following the `#ifndef / constexpr / #else / constexpr / #endif` pattern used by every other limit in that file. Code must read the `constexpr` variable name, never the raw macro.

Alternatives not chosen:

- Option B, runtime timeout configuration per registration: rejected because no current use case requires per-endpoint timeout tuning and it is inconsistent with how all other timeouts work.
- Option C, `WebSocketLimits` struct: rejected because buffer-sizing limits cannot safely be per-route at runtime without dynamic allocation.

Decision criteria:
- Follow the project constant pattern exactly.
- Default values should fit RP2040 and ESP-class targets conservatively.
- Limit names should remain stable even if the implementation evolves.

### 13. Error Mapping And Close Policy

- [x] Decide how codec failures, timeout failures, write failures, and disconnects map to close status or forced shutdown.
- [x] Decide which failures attempt a close handshake and which terminate immediately.
- [x] Decide what internal error categories need to remain distinguishable for testing and debugging.

Option sketches:

#### Option A: Centralized Policy Table In The Session

The session runtime defines an error category enum and a static mapping function that resolves each category to a close policy.

Sketch:

```cpp
enum class WsErrorCategory
{
    FrameParseError,      // Malformed frame bytes
    ProtocolViolation,    // Valid bytes, invalid protocol sequence
    MessageTooLarge,      // Assembled message exceeds WsMaxMessageSize
    WriteFailure,         // Transport write returned an error
    IdleTimeout,          // No activity within WsIdleTimeoutMs
    CloseHandshakeTimeout,// Remote did not echo CLOSE within WsCloseTimeoutMs
    RemoteDisconnect,     // Transport disconnected without a CLOSE frame
};

struct WsClosePolicy
{
    bool attemptCloseHandshake; // true: send CLOSE frame first; false: immediate teardown
    uint16_t closeCode;         // WebSocket status code to use in CLOSE frame or logging
};

// Single authoritative mapping:
WsClosePolicy policyFor(WsErrorCategory category);
```

Proposed mapping:

| Category | Attempt close | Close code | Rationale |
|---|---|---|---|
| `FrameParseError` | No | 1002 | Transport state likely corrupt; no safe CLOSE send |
| `ProtocolViolation` | Yes | 1002 | Frame bytes parse but sequence is illegal; CLOSE is safe |
| `MessageTooLarge` | Yes | 1009 | Application limit; CLOSE handshake is safe |
| `WriteFailure` | No | 1006 | Cannot write CLOSE if writes are failing |
| `IdleTimeout` | Yes | 1001 | Going Away; transport is likely still healthy |
| `CloseHandshakeTimeout` | No | 1006 | Remote non-responsive; force shutdown |
| `RemoteDisconnect` | No | 1006 | Connection already gone |

How it would flow:

- Whenever the session encounters a failure, it calls `policyFor(category)` to determine behavior.
- If `attemptCloseHandshake` is true, the session transitions to close-in-progress and sends a CLOSE frame.
- If false, the session transitions directly to `Aborted` and signals the pipeline to drop the connection.
- The `onClose` or `onError` user callback fires before the session transitions to terminal state.

Why it is attractive:

- One function covers all error-to-behavior mapping; easy to test exhaustively with a simple table assertion.
- Adding a new error category requires updating the enum and one mapping entry.
- Auditable in a single location without scanning the session state machine.

Why it may be awkward:

- A caller adding a new error category must remember to update the policy mapping; a missing entry needs a safe default.

#### Option B: Per-Category Compile-Time Policy Constants

Each error category has its own `constexpr WsClosePolicy` value declared near the relevant handling code.

Sketch:

```cpp
constexpr WsClosePolicy FrameParsePolicy     = { false, 1002 };
constexpr WsClosePolicy ProtocolViolPolicy   = { true,  1002 };
constexpr WsClosePolicy MessageTooLargePolicy = { true,  1009 };
constexpr WsClosePolicy WriteFailurePolicy   = { false, 1006 };
constexpr WsClosePolicy IdleTimeoutPolicy    = { true,  1001 };
constexpr WsClosePolicy CloseTimeoutPolicy   = { false, 1006 };
constexpr WsClosePolicy DisconnectPolicy     = { false, 1006 };
```

How it would flow:

- Each failure site in the session state machine references its corresponding constant directly.
- No mapping function call; the policy is inlined at the decision point.

Why it is attractive:

- Each policy is visible next to the code that applies it.
- No indirection through an enum or mapping function.

Why it may be awkward:

- The full policy set is distributed across the codebase rather than gathered in one place.
- Verifying exhaustive coverage requires searching for every constant rather than asserting a table.

#### Option C: Inline Per-Failure Decisions With No Shared Policy Type

Each failure path in the session state machine makes its own close or abort decision locally, with no shared type or mapping.

Sketch:

```cpp
// Inside session handle() for frame parse error:
client_.stop();
return ConnectionSessionResult::AbortConnection;

// Inside session handle() for idle timeout:
sendCloseFrame(client_, 1001);
transitionToClosing();
return ConnectionSessionResult::Continue;
```

How it would flow:

- No shared types; decisions are inline comments or one-off logic.

Why it is attractive:

- No types to define.

Why it may be awkward:

- Close and abort behavior becomes scattered and hard to audit.
- Testing requires simulating each failure path independently, with no central assertion target.
- Future behavior changes require hunting through all failure sites.

Working comparison:

- Centralized policy table: most testable, most auditable, modest type overhead.
- Per-category compile-time constants: readable at each site, distributed coverage.
- Inline decisions: minimal abstraction, worst testability and auditability.

Initial recommendation:

- Prefer Option A (centralized policy table) for the first implementation.
- The mapping table should be tested exhaustively in the native test suite.
- A safe default policy (no close handshake, 1006) should be returned for any unmapped category to avoid undefined behavior at runtime.

Decision:

- Chosen option: Option A, centralized `WsErrorCategory` enum and `policyFor()` mapping table.
- Status: Accepted.

Why this option fits the current codebase:

- A single `policyFor(WsErrorCategory)` function is the only place close and abort behavior is decided; the native test suite can assert the entire table in one focused test.
- The `WsErrorCategory` enum makes every failure path addressable by name in tests and log output.
- A safe default (no close handshake, 1006) prevents any newly added category from silently inheriting undefined behavior.

Authoritative policy table:

| Category | Attempt close handshake | Close code |
|---|---|---|
| `FrameParseError` | No | 1002 |
| `ProtocolViolation` | Yes | 1002 |
| `MessageTooLarge` | Yes | 1009 |
| `WriteFailure` | No | 1006 |
| `IdleTimeout` | Yes | 1001 |
| `CloseHandshakeTimeout` | No | 1006 |
| `RemoteDisconnect` | No | 1006 |

Alternatives not chosen:

- Option B, per-category compile-time constants: rejected because the full policy set is distributed rather than auditable in one place.
- Option C, inline per-failure decisions: rejected because scattered close/abort logic is hard to test exhaustively and easy to diverge over time.

Decision criteria:
- Make failure behavior deterministic and testable.
- Keep close policy centralized rather than scattered.
- Avoid over-promising graceful shutdown where transport state does not allow it.

### 14. Documentation Commitments For First Release

- [x] Decide the exact unsupported feature list that will be documented on day one.
- [x] Decide whether an example is required for the first merge or can wait until the API is stable.
- [x] Decide what validation commands and test suites will be part of the documented acceptance bar.

Option sketches:

#### Option A: Unsupported-Feature List Required Before Merge; Example Deferred

The merge gate requires a written unsupported-feature list in `LIBRARY_DOCUMENTATION.md` and all native tests passing, but no code example is required at merge time.

Sketched unsupported-feature list:

- WebSocket extensions (permessage-deflate and all others defined in RFC 7692 and later).
- Subprotocol negotiation (`Sec-WebSocket-Protocol` header is accepted but ignored).
- Multiplexing: the library manages one WebSocket per physical TCP connection.
- User-initiated pings: the server does not send PING frames in the first release.
- Message streaming beyond `WsMaxMessageSize`: messages exceeding the limit are rejected.
- PING and PONG observation callbacks: control-frame interception is not exposed.

How it would flow:

- Before merge, the implementation PR must add the unsupported-feature list to `LIBRARY_DOCUMENTATION.md`.
- Acceptance bar: all native tests pass; no test failures with a reference RFC 6455 client against a minimal echo server built in the test suite.
- An example in `examples/` is a follow-up item added to the backlog, not a merge blocker.

Why it is attractive:

- Documenting unsupported features sets correct expectations immediately and prevents support confusion.
- Deferring the example avoids prematurely locking in the call-site API shape before items 10 and 11 are confirmed stable.

Why it may be awkward:

- Without any example, downstream users have no concrete call-site reference until a follow-up PR lands.

#### Option B: Unsupported-Feature List And Minimal Echo Example Required Before Merge

The merge gate requires both the unsupported-feature list and a compiling echo server example.

Sketch:

```cpp
// examples/ws-echo/ws-echo.cpp
// Minimal echo server demonstrating the first-release WebSocket API.
server.handlers().websocket("/echo", WebSocketCallbacks
{
    .onText = [](WebSocketContext &ctx, std::string_view msg)
    {
        ctx.sendText(msg);
    },
    .onClose = [](WebSocketContext &ctx, uint16_t code, std::string_view)
    {
        // Connection closed.
    }
});
```

How it would flow:

- The example must compile without warnings against the merged code.
- The example does not need to run on hardware as a merge gate; it is a API-shape smoke test.
- Both the unsupported list and example are produced in the implementation PR.

Why it is attractive:

- Forces the public API to be exercise end-to-end before merge.
- Catches call-site awkwardness (items 10 and 11 surprises) while the code is still being reviewed.

Why it may be awkward:

- Freezes the example-facing API shape earlier than the post-merge stabilization period would allow.
- A compiling example without hardware verification gives limited signal beyond "it compiles".

#### Option C: All Documentation Deferred Until Post-Merge API Stabilization

The first merge is gated on test pass only; unsupported-feature list and example are scheduled as immediate follow-up work items.

How it would flow:

- The merge PR focuses entirely on the implementation.
- Follow-up items are filed in the backlog for documentation, example, and unsupported-feature list.

Why it is attractive:

- Fastest path to a merged implementation.

Why it may be awkward:

- Deferred documentation items frequently remain deferred indefinitely.
- Without an explicit unsupported-feature list, later contributors may implement conflicting partial support for deferred features.

Acceptance bar options:

| Requirement | Option A | Option B | Option C |
|---|---|---|---|
| Native tests pass | Required | Required | Required |
| Unsupported-feature list in docs | Required | Required | Deferred |
| Echo example compiles | Deferred | Required | Deferred |
| Reference client validation | Recommended | Recommended | Deferred |

Working comparison:

- Unsupported list required, example deferred: balances scope documentation with API stability.
- Both required: most thorough gate, some risk of over-specifying the API.
- All deferred: lowest merge friction, highest long-term documentation risk.

Initial recommendation:

- Prefer Option A (unsupported-feature list required, example deferred).
- The unsupported-feature list should be written as part of the implementation PR in `LIBRARY_DOCUMENTATION.md`.
- A follow-up backlog item should be created for the echo example once items 10 and 11 are confirmed stable.
- Validation guidance should reference the native test harness (`tools/run_native_tests.ps1`) as the primary acceptance command.

Decision:

- Chosen option: Option A, unsupported-feature list required before merge; echo example deferred.
- Status: Accepted.

Why this option fits the current codebase:

- Documenting the unsupported-feature list as part of the implementation PR gives reviewers and users an accurate scope boundary from day one.
- Deferring the echo example avoids prematurely locking in the items 10 and 11 API shapes before the first real integration tests surface any call-site friction.
- The native test harness (`tools/run_native_tests.ps1`) is the existing acceptance gate for all library work and requires no new tooling.

Unsupported features to document at merge:

- WebSocket extensions (permessage-deflate and all RFC 7692+ extensions).
- Subprotocol negotiation (`Sec-WebSocket-Protocol` accepted but ignored).
- Multiplexing (one WebSocket per TCP connection).
- Server-initiated PING frames.
- Messages exceeding `WsMaxMessageSize` (rejected with 1009).
- PING and PONG observation callbacks.
- Outbound sends from outside the pipeline loop.

Acceptance bar:

- All native tests pass (`tools/run_native_tests.ps1`).
- Unsupported-feature list present in `LIBRARY_DOCUMENTATION.md`.
- Echo example is a follow-up backlog item, not a merge blocker.

Alternatives not chosen:

- Option B, both list and echo example required: rejected because it risks freezing the public API shape before integration testing reveals call-site issues.
- Option C, all documentation deferred: rejected because deferred items frequently remain deferred and the unsupported-feature list prevents scope confusion for contributors.

Decision criteria:
- Documentation should match actual shipped scope, not aspirational scope.
- Examples should not lock in an API shape prematurely.
- Validation guidance should match the native test lane already used in this repo.

## Recommended Working Order

- [x] Resolve items 1 through 3 before Phase 1 implementation begins.
- [x] Resolve items 4 and 5 before any handshake code lands.
- [x] Resolve items 6 through 9 before codec and session-runtime implementation starts.
- [x] Resolve items 10 and 11 before public API code or examples are written.
- [x] Resolve items 12 through 14 before hardening and documentation work is marked complete.

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