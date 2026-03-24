2026-03-24 - Copilot: clarified that runtime callback ownership belongs directly to `WebSocketContext`, not to a separate event-dispatch wrapper.
2026-03-24 - Copilot: defined the first concrete WebSocketContext send API shape, including callback signatures, result enums, queue semantics, and the recommended v1 control-frame policy.
2026-03-24 - Copilot: aligned the send API backlog with the peer-context handoff by documenting which HTTP activation-context data is carried into `WebSocketContext` during upgrade.
2026-03-24 - Copilot: documented the concrete gap between receive-only callbacks and the private outbound queue in WebSocketSessionRuntime.
2026-03-24 - Copilot: created backlog item for designing the WebSocket callback context and outbound send API.

# Design WebSocket Context And Send API Backlog

## Summary

The current `WebSocketCallbacks` surface is receive-only. User callbacks can observe `onOpen`, `onText`, `onBinary`, `onClose`, and `onError`, but they are not given a context object or any supported way to queue outbound text, binary, ping, pong, or close frames. This backlog item captures the design work needed to introduce a proper callback context and outbound send model without leaking transport details or locking the public API into an inflexible first draft.

## Goal / Acceptance Criteria

- The public WebSocket event model includes a context object or equivalent sender handle that gives callbacks an intentional way to send data back to the connected client.
- The design makes explicit which outbound operations are supported from callbacks, including text, binary, close, and any control-frame operations that should be public.
- The outbound API defines deterministic behavior when writes cannot be queued or flushed immediately.
- The callback/context design composes cleanly with the planned `WebSocketBuilder` surface instead of forcing users back through raw `WebSocketCallbacks` forever.
- The runtime ownership model is clear about callback lifetime, re-entrancy expectations, buffering limits, and what state remains valid after close begins.
- The design explicitly identifies which HTTP activation-context data is transferred into `WebSocketContext` during upgrade so callbacks do not depend on a stale HTTP object after handoff.
- The chosen design is documented well enough to guide both implementation and native test coverage.

## Current Design Pressure

- `WebSocketCallbacks` currently carries only receive-side function signatures.
- `WebSocketSessionRuntime` owns the only outbound frame queue and serializer entry points today.
- The builder-surface backlog already points toward callback signatures that accept a future `WebSocketContext &`.
- Outbound behavior cannot be treated as an afterthought because send semantics affect queue depth, close sequencing, callback re-entrancy, and error reporting.

## Current Gap

- `WebSocketCallbacks` exposes only observation hooks: `onOpen`, `onText`, `onBinary`, `onClose`, and `onError`. None of those callbacks receives a runtime context, sender handle, or session object.
- All outbound behavior is owned privately by `WebSocketSessionRuntime` through `pendingWrite_`, `flushPendingWrite(...)`, `queueSerializedFrame(...)`, and `queueCloseFrame(...)`.
- The handshake response is also injected into that same private outbound buffer during `WebSocketSessionRuntime` construction, so the runtime already models HTTP-upgrade output and post-upgrade frame output as one write queue that user code cannot access.
- The runtime currently allows only a single pending outbound buffer at a time. `queueSerializedFrame(...)` returns `false` immediately when `pendingWrite_` is non-empty, which means queue saturation and partial-write continuation already exist internally but have no public API contract.
- Control-frame behavior is runtime-owned today. Incoming `Ping` frames trigger an automatic queued `Pong`, incoming `Close` frames trigger queued close-handshake behavior, and protocol/message-size failures can force internal close queuing without any callback-level send participation.
- User callbacks can observe received messages, but they cannot intentionally send text, binary, close, ping, or pong frames in response. The only outbound effects available today are the runtime's built-in handshake, pong, and close paths.
- Because callbacks have no sender/context, they also have no way to observe whether an outbound frame was queued, deferred by an existing pending write, rejected because close has started, or dropped because serialization failed.
- This creates a design mismatch with the planned `WebSocketBuilder` direction: richer route/event registration can be expressed at the builder surface, but the runtime still lacks the callback-time context needed to make those event handlers useful for interactive protocols.

## Design Questions To Resolve

- Should callbacks receive a full `WebSocketContext &`, a narrower sender interface, or different context types for route setup versus runtime events?
- Which methods belong on the public context: `sendText`, `sendBinary`, `close`, `sendPing`, `sendPong`, `isOpen`, `closeCode`, or fewer?
- Should send methods return `bool`, a richer status enum, or schedule work without immediate success reporting?
- What happens if a callback tries to send while another outbound frame is already pending?
- Should the API support only complete-message sends at first, or expose fragmentation controls later?
- How should callbacks observe send failure: immediate return status, `onError`, automatic close, or a combination?

## Recommended Public Callback Model

The first usable public shape should be context-first across runtime callbacks.

Recommended callback signatures:

- `onOpen(WebSocketContext &context)`
- `onText(WebSocketContext &context, std::string_view message)`
- `onBinary(WebSocketContext &context, span<const std::uint8_t> payload)`
- `onClose(WebSocketContext &context, std::uint16_t code, std::string_view reason)`
- `onError(WebSocketContext &context, std::string_view message)`

Why this should be the default:

- It matches the builder direction already implied in backlog `010`.
- It keeps all runtime events on one consistent mental model.
- It allows callbacks to observe websocket state and attempt outbound actions without exposing `WebSocketSessionRuntime` directly.
- It avoids a mixed callback surface where only some events can send and others cannot.

The design should therefore reject a narrower sender-only handle for v1. The callback-facing object should be `WebSocketContext`, with send capability included as part of that context.

Callback ownership should also stay direct:

- the runtime callback bundle should be owned directly by `WebSocketContext`
- `WebSocketProtocolExecution` should invoke callbacks through `WebSocketContext` rather than owning a separate dispatch wrapper or parallel callback aggregate
- callback ownership is part of why `WebSocketContext` is the public protocol context rather than just a send facade

## Recommended V1 `WebSocketContext` Surface

The first public `WebSocketContext` should expose only the operations needed for an actually interactive websocket endpoint, plus minimal state inspection.

Recommended v1 methods:

```cpp
enum class WebSocketSendResult
{
		Queued,
		Backpressured,
		Closing,
		Closed,
		TooLarge,
		InvalidState,
		InternalError
};

enum class WebSocketCloseResult
{
		CloseQueued,
		AlreadyClosing,
		AlreadyClosed,
		ReasonTooLarge,
		InvalidState,
		InternalError
};

class WebSocketContext
{
public:
		WebSocketSendResult sendText(std::string_view payload);
		WebSocketSendResult sendBinary(span<const std::uint8_t> payload);
		WebSocketCloseResult close(
				WebSocketCloseCode code = WebSocketCloseCode::NormalClosure,
				std::string_view reason = {});

		bool isOpen() const;
		bool isClosing() const;
		std::optional<std::uint16_t> closeCode() const;
		std::string_view closeReason() const;
};
```

Recommended carried-forward/contextual access remains additive to that shape:

- request/activation metadata
- connection endpoint information
- request/connection `items()` storage

But those data-access concerns should stay conceptually separate from the outbound send API.

## Upgrade Handoff: Data Carried From `HttpContext`

`WebSocketContext` should not be a pure send handle. At upgrade time it should also receive the HTTP activation data that may still matter after the connection leaves HTTP request execution.

Target naming note:

- the architectural target name is `HttpContext`
- current code still uses `HttpRequest` during the transition
- this section describes the target handoff model, even if the first implementation still copies data from the existing `HttpRequest` type

The carried-forward data should include at minimum:

- request/connection extension state from `items()`
- HTTP request metadata that may still matter after upgrade
	- method
	- version
	- headers
- endpoint information
	- remote address
	- remote port
	- local address
	- local port

Why this belongs on `WebSocketContext`:

- websocket callbacks may rely on authentication or routing data previously stored in `items()` during HTTP activation
- callbacks may need to inspect upgrade-request headers after activation for telemetry, subprotocol selection, or application-level policy
- callbacks may need endpoint information for diagnostics, authorization, rate limiting, or connection labeling

Recommended public-access shape:

```cpp
class WebSocketContext
{
public:
		ItemCollection &items();
		const ItemCollection &items() const;

		HttpMethod method() const;
		HttpVersion version() const;
		const HttpHeaderCollection &headers() const;

		std::string_view remoteAddress() const;
		std::uint16_t remotePort() const;
		std::string_view localAddress() const;
		std::uint16_t localPort() const;

		WebSocketSendResult sendText(std::string_view payload);
		WebSocketSendResult sendBinary(span<const std::uint8_t> payload);
		WebSocketCloseResult close(
				WebSocketCloseCode code = WebSocketCloseCode::NormalClosure,
				std::string_view reason = {});
};
```

Ownership guidance:

- the websocket side should receive its own carried-forward activation snapshot during upgrade
- `WebSocketContext` should not query a no-longer-active HTTP context lazily after handoff
- each carried field should have an intentional ownership rule: copied, moved, or shared immutably

For v1, the safest default is:

- `items()` moves or transfers into the websocket-owned context when practical
- request metadata and endpoint information are copied into websocket-owned storage or immutable snapshot fields

This keeps callback-visible state stable for the lifetime of the upgraded connection.

## Result-Send Design Decision

The send API should return a richer result enum, not `bool`.

Why `bool` is too weak:

- it cannot distinguish temporary backpressure from terminal close state
- it cannot distinguish message-size rejection from internal serializer/runtime failure
- it encourages callers to treat all failures as equivalent even though some are retryable and some are not

Why a status enum is the right v1 tradeoff:

- it is still lightweight enough for callback-time use
- it supports deterministic tests
- it leaves room to add helpers such as `isRetryable(result)` later without forcing exceptions or transport-visible state into the public API

Recommended result semantics:

- `Queued`
	- the full outbound message or close frame was accepted into the execution-owned queue
- `Backpressured`
	- a prior outbound buffer is still pending, so the operation was not queued
- `Closing`
	- the websocket close handshake has started; no new application data frame was accepted
- `Closed`
	- the websocket is already fully closed or no longer writable
- `TooLarge`
	- the requested payload exceeds configured websocket limits for the v1 complete-message API
- `InvalidState`
	- the operation is not valid in the current lifecycle state
- `InternalError`
	- serialization or internal runtime preparation failed unexpectedly

This means callback sends are neither synchronous writes nor fire-and-forget background work. They are immediate queue-attempt operations with deterministic admission results.

## Queueing And Backpressure Semantics

The v1 send API should be explicitly queue-oriented, not flush-oriented.

Recommended semantic rule:

- `sendText(...)` and `sendBinary(...)` attempt to queue exactly one complete websocket message
- they do not guarantee immediate socket write progress
- they succeed only if the execution object can accept the full message into its outbound slot/queue at call time

Because the current runtime only supports a single pending outbound buffer, the v1 public behavior should reflect that reality rather than hiding it.

Recommended v1 backpressure contract:

- if no outbound frame is currently pending and the session is open, the send is accepted and returns `Queued`
- if an outbound frame is already pending, the new send is rejected immediately and returns `Backpressured`
- the runtime should not silently replace, merge, or reorder queued messages
- the runtime should not block waiting for flush completion from inside the callback

This keeps callback re-entrancy and transport progress simple while leaving room for a later backlog item to introduce a deeper queue if that becomes necessary.

## Close API Decision

Close should be public in v1.

Recommended close behavior:

- `close(...)` requests the websocket close handshake by queueing one close frame
- if close has not started and the frame can be queued, return `CloseQueued`
- if close has already started, return `AlreadyClosing`
- if the connection is fully closed, return `AlreadyClosed`
- if the provided reason is too large for a close frame, return `ReasonTooLarge`
- if internal serialization/setup fails, return `InternalError`

The close API should accept:

- a close code
- an optional UTF-8 reason string

The v1 API should not expose fragmented close payload construction or transport-specific close flushing mechanics.

Post-close callback guarantees:

- once `close(...)` returns `CloseQueued`, application data sends should return `Closing`
- `onClose(...)` should still be delivered at most once
- `onError(...)` may still fire before final close notification if protocol/runtime errors occur during close progression

## Ping/Pong Policy

Recommended v1 rule:

- keep automatic `Pong` handling internal
- do not expose public `sendPong(...)`
- do not expose public `sendPing(...)` in the first usable API

Why:

- `Pong` is primarily protocol maintenance behavior already handled correctly by the runtime
- `Ping` is useful, but not required for the first interactive endpoint model
- excluding both from v1 keeps the public API focused on the essentials: text, binary, close, and observable state

If heartbeat control becomes an application requirement later, it can be added as a deliberate expansion rather than included prematurely.

## Recommended Public API Sketch

```cpp
enum class WebSocketSendResult
{
		Queued,
		Backpressured,
		Closing,
		Closed,
		TooLarge,
		InvalidState,
		InternalError
};

enum class WebSocketCloseResult
{
		CloseQueued,
		AlreadyClosing,
		AlreadyClosed,
		ReasonTooLarge,
		InvalidState,
		InternalError
};

struct WebSocketEvents
{
		std::function<void(WebSocketContext &)> onOpen;
		std::function<void(WebSocketContext &, std::string_view)> onText;
		std::function<void(WebSocketContext &, span<const std::uint8_t>)> onBinary;
		std::function<void(WebSocketContext &, std::uint16_t, std::string_view)> onClose;
		std::function<void(WebSocketContext &, std::string_view)> onError;
};
```

Example builder-facing usage:

```cpp
handlers()
		.ws("/chat")
		.onOpen([](WebSocketContext &context)
		{
				const WebSocketSendResult result = context.sendText("ready");
				(void)result;
		})
		.onText([](WebSocketContext &context, std::string_view message)
		{
				if (message == "bye")
				{
						context.close(WebSocketCloseCode::NormalClosure, "goodbye");
						return;
				}

				context.sendText(message);
		});
```

## Alignment With Backlog `012`

This API should sit on top of the Phase 4 split already documented in backlog `012`.

- `WebSocketContext` remains the callback-facing protocol context
- `WebSocketContext` also owns the carried-forward HTTP activation snapshot needed by post-upgrade callbacks
- `WebSocketProtocolExecution` owns parser progression, write flushing, queue admission, and close-state transitions
- context send methods delegate into a narrow internal execution/control seam
- public result enums surface execution decisions without exposing transport buffers such as `pendingWrite_`

That means the send API is intentionally context-visible but execution-owned.

The callback bundle should therefore live on `WebSocketContext` itself, with execution calling into that context-owned callback surface at the appropriate lifecycle points.

## Implementation Notes And Test Seams

Implementation notes:

- keep public send methods complete-message only for v1
- validate payload size before queue admission where possible so `TooLarge` is deterministic
- keep serializer/write-buffer ownership in execution, not context
- construct `WebSocketContext` from an explicit upgrade snapshot rather than by retaining a live pointer back to `HttpContext` / `HttpRequest`
- do not allow callback-time sends to block on socket writes
- preserve at-most-once `onClose(...)` delivery across both local-close and remote-close paths

Native test seams should cover:

- text send accepted while open
- binary send accepted while open
- send rejected as `Backpressured` while a previous frame is pending
- send rejected as `Closing` after local close begins
- send rejected as `Closed` after final close completion
- close rejected as `AlreadyClosing` on repeated call
- oversize message returns `TooLarge`
- oversize close reason returns `ReasonTooLarge`
- callbacks receive `WebSocketContext &` consistently across all event types
- carried-forward `items()`, headers, and local/remote endpoint data remain available after upgrade
- builder surface can register context-first callbacks without falling back to raw `WebSocketCallbacks`

## Tasks

- [x] Document the current gap between receive-only callbacks and the private outbound queue in `WebSocketSessionRuntime`.
- [x] Define the minimum public outbound operations required for the first usable WebSocket callback context.
- [x] Decide whether the public callback signatures should be context-first (`WebSocketContext &`) across all events or mixed by event type.
- [x] Specify callback-time send semantics, including whether sends are synchronous, queued, best-effort, or explicitly backpressure-aware.
- [x] Define queueing and failure behavior when outbound data is attempted while a write is already pending or the close handshake has started.
- [x] Decide whether ping and pong should remain runtime-internal behaviors or become public context operations.
- [x] Define the close API shape, including optional reason payload handling and post-close callback guarantees.
- [x] Align the context/send design with the planned `WebSocketBuilder` event-registration surface so the two backlogs do not diverge.
- [x] Produce a public API sketch plus implementation notes covering runtime ownership, callback lifetime, and test seams.
- [ ] Add or update follow-up backlog items if the chosen design requires separate implementation slices for context plumbing, queue policy, or typed event adapters.

## Owner

- Copilot

## Priority

- High

## References

- [src/websocket/WebSocketCallbacks.h](src/websocket/WebSocketCallbacks.h)
- [src/websocket/WebSocketSessionRuntime.h](src/websocket/WebSocketSessionRuntime.h)
- [src/websocket/WebSocketSessionRuntime.cpp](src/websocket/WebSocketSessionRuntime.cpp)
- [src/routing/ProviderRegistryBuilder.h](src/routing/ProviderRegistryBuilder.h)
- [docs/backlog/task-lists/web-sockets/008-websocket-pre-implementation-decisions-backlog.md](docs/backlog/task-lists/web-sockets/008-websocket-pre-implementation-decisions-backlog.md)
- [docs/backlog/task-lists/web-sockets/010-normalize-provider-registry-builder-websocket-surface-backlog.md](docs/backlog/task-lists/web-sockets/010-normalize-provider-registry-builder-websocket-surface-backlog.md)
- [docs/plans/websocket-support-plan.md](docs/plans/websocket-support-plan.md)