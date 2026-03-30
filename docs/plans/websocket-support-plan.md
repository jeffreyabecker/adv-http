# WebSocket Support Plan

## Summary

Adding WebSocket support will require more than a new handler type. The current server architecture is built around a short-lived HTTP request parser feeding a single HTTP response stream per connection, while a WebSocket connection becomes a long-lived upgraded session with bidirectional framed IO. The clean implementation path is to treat protocol upgrade as a first-class pipeline transition and introduce dedicated WebSocket session primitives instead of trying to force framed socket semantics through the existing `HttpContext` response model.

## Current Architecture Observations

- `src/server/HttpServerBase.*` accepts an `IClient`, creates one `HttpPipeline` per connection, and removes that pipeline when the request or response lifecycle reaches a final state.
- `src/pipeline/HttpPipeline.*` assumes the connection lifecycle is HTTP-only: parse request bytes, build one response byte source, write it to the client, then complete, abort, or keep reading for the next HTTP request.
- `src/server/WebServerBuilder.h` currently hard-wires the pipeline handler factory to `HttpContext::createPipelineHandler(...)`, which means all accepted connections are funneled directly into the HTTP request model.
- `src/core/HttpContext.*` owns routing, body handling, and response dispatch. It has no notion of a successful protocol upgrade that hands control of the socket to another long-lived component.
- `src/pipeline/RequestParser.*` already uses `llhttp`, and the vendored `llhttp` supports upgrade detection, but that upgrade signal is not surfaced through the current library APIs.
- `src/pipeline/TransportInterfaces.h` already gives a sufficiently low-level byte channel abstraction for WebSocket frame IO, which is a good base to build on.

## Architectural Direction

### 1. Introduce A Distinct Connection-Lifecycle Layer

The pipeline should stop assuming that every accepted client is only an HTTP request/response exchange. A better shape is:

- HTTP request phase
- optional protocol upgrade phase
- upgraded session phase
- connection shutdown phase

That likely means `HttpPipeline` becomes a coordinator for connection state rather than only an HTTP parser plus response writer.

### 2. Make Protocol Upgrade Explicit

The HTTP layer should be able to produce one of two outcomes:

- a normal HTTP response
- an approved upgrade result that transfers control of the client connection into a WebSocket session

This is the main architectural break from the current model. Today `HttpContext` can only produce an `IHttpResponse` stream. For WebSockets, it needs a way to return an upgrade decision and session factory instead.

### 3. Separate WebSocket Session Semantics From HTTP Handler Semantics

WebSocket work should not be modeled as just another `IHttpHandler` body-processing variant. After the handshake completes, the connection semantics change completely:

- bidirectional message flow
- frame parsing and serialization
- ping, pong, and close control frames
- masking and payload-length rules
- potentially long-lived sessions with server-initiated sends

That points toward a dedicated `WebSocketSession` abstraction with its own lifecycle callbacks and send APIs.

## Proposed Major Changes

### Connection And Pipeline

- Refactor `HttpPipeline` so it can transition from HTTP parsing into an upgraded connection mode instead of always terminating as an HTTP transaction.
- Add an internal upgraded-session interface, for example `IUpgradedConnection` or `IConnectionSession`, that owns ongoing client IO after handshake success.
- Keep timeout and disconnect handling in the pipeline so both HTTP and WebSocket paths share the same transport supervision.

### HTTP Upgrade Handshake

- Extend request handling to detect and validate WebSocket upgrade requests:
  - method must be `GET`
  - `Connection` must include `Upgrade`
  - `Upgrade` must be `websocket`
  - `Sec-WebSocket-Version` must be supported
  - `Sec-WebSocket-Key` must be present and valid
- Generate the `101 Switching Protocols` response and `Sec-WebSocket-Accept` header.
- Decide where handshake validation lives:
  - either inside a dedicated WebSocket upgrade handler in the HTTP layer
  - or in a specialized pipeline branch that consumes an upgrade result from routing

### Public API Surface

- Add a websocket-focused registration API alongside the existing HTTP route builder, rather than overloading the current response-centric handler APIs.
- Likely introduce a builder entry point such as a websocket route registry or `onWebSocket(...)` registration model.
- Define a lightweight session callback model that fits embedded usage, for example:
  - on open
  - on text message
  - on binary message
  - on ping or pong if exposed
  - on close
  - on error

### WebSocket Framing Layer

- Add dedicated frame reader and writer components on top of `IClient` or `IByteChannel`.
- Support the RFC 6455 minimum viable behavior first:
  - text and binary data frames
  - continuation or fragmentation handling
  - close handshake
  - ping and pong
  - client-mask enforcement
  - payload length handling for 7-bit, 16-bit, and 64-bit lengths
- Keep this framing layer independent from routing so it can be unit-tested directly.

### Session State And Backpressure

- Define how outbound messages are buffered or streamed.
- Decide whether writes remain synchronous in the pipeline loop or require an explicit outbound queue owned by the websocket session.
- Add guardrails for large frames and stalled clients so one upgraded connection does not monopolize the server loop.

### Testing Strategy

- Extend the native test lane with fake clients that can script:
  - HTTP upgrade requests
  - partial frame reads
  - partial writes
  - disconnects during close handshake
  - ping and pong timing
- Unit-test frame parsing and serialization independently from the full server.
- Add end-to-end fake-transport tests that cover HTTP handshake followed by multiple websocket frames on the same client.

## Recommended Implementation Sequence

### Phase 1: Design The Upgrade Seam

- Refactor the pipeline and request-handling boundary so an HTTP request can return either a normal response or an upgrade result.
- Keep this phase focused on architecture only, without implementing full frame parsing yet.

### Phase 2: Handshake-Only Vertical Slice

- Implement header validation and `101 Switching Protocols` generation.
- Prove that the pipeline can hand the accepted client to a stub upgraded-session object after the handshake.
- Add native tests for accepted and rejected upgrade requests.

### Phase 3: Core Frame Codec

- Implement frame decode and encode primitives.
- Add tests for masking, payload lengths, fragmentation boundaries, and control-frame validation.

### Phase 4: Session API And Routing Integration

- Add the public websocket registration API.
- Wire route matching to select a websocket endpoint and create its session object.
- Keep this API intentionally small at first.

### Phase 5: Control Frames, Shutdown, And Timeouts

- Complete close, ping, pong, disconnect, and timeout behavior.
- Ensure the server loop can reap dead or completed websocket sessions cleanly.

### Phase 6: Hardening And Documentation

- Add limits for frame size, message size, queue depth, and idle lifetime.
- Document unsupported features explicitly, especially if extensions and compression are deferred.

## Initial Scope Recommendation

The first shipped WebSocket version should stay narrow:

- HTTP/1.1 upgrade only
- no extensions
- no per-message compression
- no subprotocol negotiation unless a clear use case already exists
- no TLS-specific behavior in the library layer
- one connection equals one websocket session after successful upgrade

That scope is enough to validate the architecture without dragging in optional RFC surface area too early.

## Risks And Open Questions

- The current `HttpContext` type mixes request parsing, handler selection, and response dispatch. Supporting upgrade cleanly may require splitting those concerns instead of adding another flag.
- The current pipeline result model is transaction-oriented. WebSocket sessions are connection-oriented and may stay alive indefinitely, so timeout policy and completion semantics need to be redefined.
- If the existing handler model is stretched too far, the code may end up with awkward dual semantics where some handlers return `IHttpResponse` and others secretly seize the connection. That should be avoided.
- Large-frame buffering strategy matters on small-memory embedded targets. The implementation should prefer incremental parsing and bounded buffers over whole-message accumulation by default.
- Server-driven sends may require a session-owned outbound queue. If that is omitted initially, the API should be explicit about when sending is legal.

## Suggested Follow-Up Work

1. Refactor the pipeline result boundary so upgrade is representable as a first-class outcome.
2. Add a handshake-only prototype behind a narrow internal API.
3. Split detailed backlog items once the upgrade seam design is proven in code.