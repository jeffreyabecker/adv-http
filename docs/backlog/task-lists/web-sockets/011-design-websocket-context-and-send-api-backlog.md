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

## Tasks

- [x] Document the current gap between receive-only callbacks and the private outbound queue in `WebSocketSessionRuntime`.
- [ ] Define the minimum public outbound operations required for the first usable WebSocket callback context.
- [ ] Decide whether the public callback signatures should be context-first (`WebSocketContext &`) across all events or mixed by event type.
- [ ] Specify callback-time send semantics, including whether sends are synchronous, queued, best-effort, or explicitly backpressure-aware.
- [ ] Define queueing and failure behavior when outbound data is attempted while a write is already pending or the close handshake has started.
- [ ] Decide whether ping and pong should remain runtime-internal behaviors or become public context operations.
- [ ] Define the close API shape, including optional reason payload handling and post-close callback guarantees.
- [ ] Align the context/send design with the planned `WebSocketBuilder` event-registration surface so the two backlogs do not diverge.
- [ ] Produce a public API sketch plus implementation notes covering runtime ownership, callback lifetime, and test seams.
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