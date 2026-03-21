## Objective

Refactor this plan to specify an interface-first contract for the transport seam and explicitly defer all concrete implementation work to an outside implementer. The document will define the public C++17-facing transport API, behavioral invariants, acceptance criteria, and migration guidance for consumers of the interface. No in-tree implementation code or platform-specific details will be proposed here.
## Why This Change

The goal is to eliminate Arduino shims and any reliance on Arduino-specific classes inside the library. The HTTP pipeline must depend only on a minimal, well-documented C++17 interface built from standard primitives and STL types. Concrete transport implementations (lwIP, platform sockets, or Arduino wrappers) are out-of-scope for this document and should be created and maintained by an external implementer who consumes the interface defined here.
## Scope

### In scope (this document)

- Define a C++17 interface contract for the transport seam (`IClient`, `IServer`, and any small supporting types).
- Specify required method signatures, return semantics, error handling, and threading/callback expectations using STL and standard integer types (`std::unique_ptr`, `std::shared_ptr`, `std::chrono`, `std::size_t`, `std::uint8_t`, etc.).
- Enumerate behavioral invariants the implementation must preserve (accept semantics, `available()`/EOF semantics, timeouts, `connected()` behavior, `status()` mapping, `flush()` semantics).
- Provide a clear implementer contract: test vectors, required tests, acceptance criteria, and recommended ownership models.

### Out of scope (this document)

- Any in-tree or platform-specific implementation code (e.g., `TcpClientContext`, lwIP callbacks, or Arduino wrappers).
- TLS/HTTPS or secure client/server implementations.
- Low-level socket lifecycle or platform-specific memory-management choices beyond what is required by the interface contract.
## Current Contract and Required Interface Signatures

This library must depend only on a stable, implementation-agnostic interface. The minimal surface required by the HTTP pipeline is expressed below as interface signatures and behavior notes. All signatures use C++17 primitives and STL types; no Arduino-specific types are used here.

### `IClient` (interface)

Required methods and return semantics (suggested signatures):

- `virtual std::size_t write(HttpServerAdvanced::span<const std::uint8_t> data) = 0;`
   - `virtual AvailableResult available() const = 0;`  // readiness + optional count; see stream plan for semantics
- `virtual int read(HttpServerAdvanced::span<std::uint8_t> buffer) = 0;` // returns bytes read or -1 on error
- `virtual void flush() = 0;` // blocks or waits until pending bytes are acknowledged (implementation detail)
- `virtual void stop() = 0;` // orderly close
- `virtual ConnectionStatus status() const = 0;` // enum for CLOSED, LISTEN, ESTABLISHED, etc.
- `virtual bool connected() const = 0;` // true while connection is established or unread buffered data exists
- `virtual void setTimeout(std::chrono::milliseconds t) = 0;`
- `virtual std::chrono::milliseconds getTimeout() const = 0;`
- `virtual EndpointInfo remoteEndpoint() const = 0;` // small POD with IP/port primitives
- `virtual EndpointInfo localEndpoint() const = 0;`

Notes:
- Use plain POD types for IP and port (e.g., `struct EndpointInfo { std::array<std::uint8_t,4> ipv4; std::uint16_t port; }`), or `std::string` for textual IPs when appropriate. Avoid Arduino `IPAddress`.
- Document error codes and exceptional behaviors (e.g., `read()` returning 0 vs `-1`), and specify whether implementations may throw exceptions (prefer returning error codes to throwing).

### `IServer` (interface)

Required methods and return semantics (suggested signatures):

- `virtual void begin(uint16_t port) = 0;`
- `virtual std::unique_ptr<IClient> accept() = 0;` // returns nullptr when no client pending
- `virtual ConnectionStatus status() const = 0;`
- `virtual uint16_t port() const = 0;`
- `virtual void end() = 0;`

Notes:
- `accept()` must return `nullptr` when no client is pending; it must transfer ownership when a client is returned.
- Define backlog/reuse-address flags only as simple configuration types (POD) passed to `begin()` or a small `struct ServerOptions`.

### `IPeer` (datagram / connectionless transports)

Some platforms expose connectionless datagram transports (UDP-style) that do not fit neatly into the `IClient` / `IServer` stream model. For those transports add an `IPeer` interface that captures packet-oriented semantics while remaining implementation-agnostic and STL-friendly.

Suggested responsibilities and rationale:
- Represent a datagram endpoint that can send and receive discrete packets without a persistent connection context.
- Preserve packet boundaries: `parsePacket()` / `available()` indicate the next packet size / remaining bytes in the current packet.
- Provide endpoint metadata for both the packet source (remote) and the destination (useful for multicast or bound sockets).

Suggested minimal surface (C++17-friendly signatures):

- `virtual bool begin(uint16_t port) = 0;` // bind/listen on the given local port, return true on success
- `virtual void stop() = 0;` // close the socket / stop listening
- `virtual int parsePacket() = 0;` // start processing next packet, return packet size in bytes or 0 if none
   - `virtual AvailableResult available() const = 0;` // readiness + optional count for current packet; see stream plan
- `virtual int read(HttpServerAdvanced::span<std::uint8_t> buffer) = 0;` // read up to buffer.size(), return bytes read or -1 on error
- `virtual int peek() const = 0;` // peek next byte in current packet without consuming
- `virtual std::size_t write(HttpServerAdvanced::span<const std::uint8_t> data) = 0;` // write data to the last-specified destination (or to most recent remote if supported)
- `virtual int beginPacket(const EndpointInfo &dest) = 0;` // prepare to send to `dest`, return 1 on success, 0 on error
- `virtual int endPacket() = 0;` // finalize and send the current packet, return 1 on success
- `virtual EndpointInfo remoteEndpoint() const = 0;` // IP/port of the packet source for the current/last-read packet
- `virtual EndpointInfo destinationEndpoint() const = 0;` // destination the socket is bound to (useful for multicast)
- `virtual uint16_t localPort() const = 0;`

Behavioral notes and implementer guidance:
- `parsePacket()` should advance to the next available packet and return its total size; callers then use `available()` / `read()` to consume packet bytes. Returning 0 indicates no packet is currently available.
   - `available()` reports readiness for the current packet through `AvailableResult`. When `HasBytes` the `count` field indicates remaining bytes in the current packet; `Exhausted` means no current packet; `TemporarilyUnavailable` means none now but a packet may arrive later.
- `read()` returns the number of bytes read, 0 if no data read (but packet exists), and `-1` on unrecoverable error. Define and document the error model clearly.
- `beginPacket()` / `endPacket()` are the canonical two-step send path for datagram APIs that buffer outgoing data; `write()` may be called multiple times between `beginPacket()` and `endPacket()`.
- `remoteEndpoint()` must refer to the source of the currently parsed/consumed packet; for sent packets, an explicit `EndpointInfo` passed to `beginPacket()` should be used as the destination.
- Timeouts are less relevant for pure datagram IO, but implementations should still expose `setTimeout()` / `getTimeout()` when blocking semantics are provided for `parsePacket()` or `endPacket()`.

Mapping to existing platform APIs:
- The ESP8266 `WiFiUDP` public API is a useful reference for the `IPeer` contract: `begin()`, `beginPacket()`, `endPacket()`, `parsePacket()`, `available()`, `read()`, `remoteIP()`, `remotePort()`, and multicast helpers. Implementers should map their platform methods to the `IPeer` surface and document any semantic differences (e.g., return codes, buffer-size limits).

When to add `IPeer` vs reusing `IClient`/`IServer`:
- If the transport is stream-oriented (TCP), continue using `IClient` / `IServer`.
- If the transport is packet-oriented (UDP or similar), implement `IPeer`. Keep `IPeer` separate so stream semantics (ordering, partial reads) are not conflated with packet semantics (atomic packets, discrete sizes).


### Span compatibility

To keep public transport headers C++17-friendly while allowing modern code to use `std::span` when available, add a small compatibility header `src/compat/Span.h` that provides `HttpServerAdvanced::span<T>`:

- If the platform supplies `<span>` (C++20), `HttpServerAdvanced::span<T>` aliases `std::span<T>`.
- Otherwise the header supplies a minimal C++17 fallback `span<T>` implementation. Implementers are encouraged to replace the fallback with the upstream `tcbrindle/span` header (https://github.com/tcbrindle/span) if they require the full `std::span` feature set.

Transport implementations and the library's `IClient`/`IServer` signatures should use `HttpServerAdvanced::span<std::uint8_t>` for buffer parameters instead of raw `std::uint8_t*`/`std::size_t` pairs.
## Implementation: Deferred to External Implementer

This plan intentionally does not include concrete implementation code. Instead, it provides an implementer contract describing the expectations any concrete transport must meet. The implementer is free to use lwIP, native sockets, or adapted Arduino internals so long as they conform to the interface and invariants described in this document.

Key implementer responsibilities (contract summary):

- Implement the `IClient` and `IServer` interfaces with idiomatic C++17 code, avoiding Arduino-specific types in public headers.
- Ensure `accept()` returns `nullptr` when no pending client exists and returns a `std::unique_ptr<IClient>` transferring ownership when one is available.
- Preserve `connected()` semantics so that unread buffered data keeps `connected()` true until buffers are drained.
- Map platform-specific connection states into `ConnectionStatus` using documented rules (e.g., `CLOSE_WAIT`/`CLOSING` treated as `CLOSED` for writability).
- Implement `flush()` to await send acknowledgment semantics; document any platform-specific time bounds.
- Manage callback lifetimes so no platform callback can reference freed objects; detach callbacks before object destruction.
- Use `std::shared_ptr` or other documented ownership model as needed; document the chosen model and rationale.
## Migration and Consumer Guidance

The library will continue to expose `IClient` and `IServer` in `src/pipeline/NetClient.h` (or equivalent header). Consumers (e.g., `HttpPipeline`, `HttpServerBase`) must be updated to depend only on those interfaces and not on any concrete Arduino types. Where templates currently accept Arduino transports, adaptors or thin factory functions may be used by the application layer to produce concrete `std::unique_ptr<IServer>` instances, but those adaptors are outside the library core and outside the scope of this document.

## Acceptance Criteria for Implementer

Implementations provided by an outside implementer must meet these tests and validation points:

1. API compatibility: compile the library and examples while only depending on headers that declare `IClient` / `IServer` (no Arduino types in public headers).
2. Behavioral tests: implementer provides unit tests verifying `accept()` returns `nullptr` when empty, `available()`/`read()` semantics across buffer boundaries, `connected()` remains true while unread data exists, and `flush()` awaits acknowledgements.
3. Ownership and lifetime: implementer documents the ownership model and demonstrates callback-safety (no callbacks referencing freed objects).
4. Platform notes: implementer documents any platform-specific deviations or optional configuration flags and provides a short integration guide.
## Summary

This document now focuses purely on the interface contract and the required behavioral invariants. All concrete transport implementation work is intentionally deferred to an external implementer who will produce the platform-specific code and tests while conforming to the signatures and acceptance criteria specified here.

# HttpServerAdvanced Out-of-Tree Transport Plan

## Objective

Replace the previous in-tree implementation proposal with a policy that requires all concrete transport implementations to live out-of-tree (in separate repositories or platform-specific packages). The library will keep only an interface-only transport seam in-tree (`IClient`, `IServer`, supporting POD types and small adapters) and provide clear integration points, example adapters, and an implementer guide. No platform-specific or lwIP-dependent transport code will be added to this repository.

This work is specifically scoped to plain TCP server transport (non-secure). The plan intentionally defers TLS/HTTPS and platform-specific socket lifecycles to external implementers.

## Why This Refactor Is Needed

The current transport seam in `src/pipeline/NetClient.h` mixes two distinct concerns:

- the stable interface that the HTTP pipeline actually needs
- generic templates that adapt arbitrary Arduino networking classes by value

That second part is the problem.

### Current Issues

1. Ownership is implicit.

   `ClientImpl<T>` stores the wrapped client by value. For `WiFiClient`, that works only because the underlying Arduino implementation is itself a handle over an internal ref-counted context. The real socket lifetime is hidden from this library.

2. Server accept semantics are translated indirectly.

   `ServerImpl<T>::accept()` depends on Arduino `accept()` returning an invalid client object when no connection is available, then converts that to `nullptr`. This is a useful semantic conversion, but it is embedded inside a generic wrapper rather than expressed as the real server behavior of this library.

3. Platform details leak through `configureConnection()`.

   The current wrapper allows callers to mutate the concrete Arduino transport directly through callbacks. That makes the abstraction boundary weak and complicates future portability.

4. The implementation strategy is tied to Arduino class shape rather than transport behavior.

   The HTTP pipeline does not care whether the transport came from `WiFiClient`, `EthernetClient`, or an in-tree implementation. It cares about a stable set of stream, timeout, connection-state, and endpoint behaviors.

5. The current design makes decoupling harder.

   A concrete in-tree transport can isolate lwIP- and board-specific code in implementation files. The wrapper pattern keeps the library dependent on foreign class APIs and semantics.

## Scope

### In scope

- plain TCP accepted-client behavior for server-side HTTP processing
- in-tree implementations of `IClient` and `IServer`
- parity with plain `arduino-pico` `WiFiClient` / `WiFiServer` semantics where the current HTTP pipeline depends on them
- explicit ownership and lifecycle management for listener sockets and accepted connections
- transport-focused tests and compile validation

### Out of scope

- TLS / HTTPS server support
- secure client types
- outbound client APIs unless later required by another feature
- broad redesign of the HTTP pipeline contract
- replacing lwIP itself

## Current Contract In This Repository

The existing server and pipeline layers already define the minimal transport surface.

### `IClient`

The HTTP stack depends on these behaviors:

+ `write(HttpServerAdvanced::span<const std::uint8_t>)`
+ `available()`
+ `read(HttpServerAdvanced::span<std::uint8_t>)`
- `flush()`
- `stop()`
- `status()`
- `connected()`
- `remoteIP()` / `remotePort()`
- `localIP()` / `localPort()`
- `setTimeout()` / `getTimeout()`

### `IServer`

The HTTP stack depends on these behaviors:

- `accept()` returning `std::unique_ptr<IClient>`
- `begin()`
- `status()`
- `port()`
- `end()`

### How The Pipeline Uses These Interfaces

The pipeline and server layer impose several important semantics:

1. `accept()` returns ownership.

   `HttpServerBase::handleClient()` creates a new `HttpPipeline` only when `accept()` returns a non-null client.

2. `available()` drives request-read progress.

   `HttpPipeline::readFromClientIntoParser()` loops while `available() >= 0`, reads bytes, and currently treats `available() == 0` as request-read completion for that cycle.

3. `connected()` gates pipeline liveness.

   `HttpPipeline::_checkStateInHandleClient()` treats a disconnected client as a terminal pipeline condition.

4. `setTimeout()` is part of steady-state pipeline setup.

   `HttpPipeline::setupPipeline()` pushes the activity timeout into the transport before request processing begins.

The in-tree transport must preserve these expectations before any higher-level refactor is attempted.

## Relevant Behavior In `arduino-pico`

The plain TCP `WiFiClient` / `WiFiServer` implementation is thin at the public API level and delegates most real socket behavior to `ClientContext`.

### `WiFiServer`

Relevant observed behavior:

- `begin(port, backlog)` creates and binds a listener PCB, enables `SOF_REUSEADDR`, and registers an accept callback.
- accepted connections are queued internally as unclaimed client contexts
- `accept()` returns an invalid `WiFiClient` when no pending connection exists
- `accept()` dequeues one pending connection and applies server-level NoDelay settings to the accepted client
- `status()` returns the listener PCB state or `CLOSED` if the server is inactive

### `WiFiClient`

Relevant observed behavior:

- the public client is a lightweight handle over `ClientContext`
- destructor semantics are reference-count based, not unique socket ownership
- `available()` returns queued receive bytes from the context
- `read()` consumes bytes from the receive pbuf chain
- `flush()` waits for output to be acknowledged
- `stop()` flushes then closes
- `connected()` returns true while the connection is established or unread buffered data still exists
- `status()` intentionally reports `CLOSED` for states that should be treated as no-longer-writable

### `ClientContext`

This is the real model to learn from.

Relevant responsibilities:

- owns and detaches lwIP callbacks
- owns the receive pbuf chain and read offset
- manages write progress and retry behavior around `tcp_write()` / `tcp_output()`
- implements timeout-driven blocking for connect, write, and flush flows
- exposes endpoint metadata from `tcp_pcb`
- holds intrusive reference-counted shared lifetime between handles and queued server state

This means the correct in-tree design target is not “replace `WiFiClient` with our own wrapper.” The correct target is “replace `ClientContext` plus the thin public handle layers with our own library-owned equivalents.”


## Recommended Architecture (Out-of-Tree)

Keep the public transport seam in-tree and require all concrete implementations to be provided out-of-tree. The repository will provide:

- `src/pipeline/NetClient.h`: interface-only declarations for `IClient`, `IServer`, `ConnectionStatus`, `EndpointInfo`, and `HttpServerAdvanced::span` compatibility.
- a small `ServerOptions` POD and `AdapterHooks` header describing factory entry points and required symbol names for external transport packages.
- documentation and example adapter shims under `examples/transport-adapters/` that demonstrate how to build and expose an out-of-tree transport to consumers of the library.

Out-of-tree transport implementers are expected to provide their own repository (or platform package) containing the concrete connection context and concrete `IClient`/`IServer` implementations (e.g., `TcpClientContext`, `TcpClient`, `TcpServer`) and a thin factory function with a documented C++ ABI or header-only adaptor that the main library can call from examples or user code.

Rationale:

- avoids tying the core library to lwIP, Arduino shims, or board-specific lifecycle code
- keeps public headers STL/C++17-friendly and stable
- reduces maintenance surface and cross-platform build complexity in this repo

Integration points the library will expose:

- `std::unique_ptr<IServer> createPlatformServer(const ServerOptions &opts)` — documented as a function the platform package should provide (examples demonstrate a weak-linking or explicit registration approach)
- `AdapterHooks` describing expected behavior for `accept()` semantics, queueing, and timeout propagation

The repository will not contain any `tcp_pcb` or lwIP-dependent source files; those belong in the external implementer packages.

## Detailed Class Plan

### `TcpClientContext`

This type replaces the role of `arduino-pico`'s `ClientContext` for this library.

Suggested responsibilities:

- manage the raw lwIP PCB lifecycle
- track receive data using a pbuf chain and an offset into the current head pbuf
- expose local and remote endpoint data from the PCB
- implement `available()`, `read()`, `peek()`, and receive-buffer consumption
- implement `write()` with partial-send and retry handling
- implement `flush()` by waiting for unacknowledged bytes to drain
- implement `close()` and `abort()` with correct callback detachment
- normalize lwIP connection states into `ConnectionStatus`
- ensure callbacks cannot outlive the context

Suggested data members:

- `tcp_pcb *pcb_`
- `pbuf *rxBuffer_`
- `std::size_t rxOffset_`
- timeout fields for current operations
- write-in-progress fields for source pointer, length, bytes written, wait flags
- queue linkage or server callback state if needed

Suggested methods:

- `int available() const`
- `int read(HttpServerAdvanced::span<std::uint8_t> buffer)`
- `int peek() const`
- `std::size_t write(HttpServerAdvanced::span<const std::uint8_t> buffer)`
- `bool flush(uint32_t maxWaitMs)`
- `void setTimeout(uint32_t timeoutMs)`
- `uint32_t getTimeout() const`
- `ConnectionStatus status() const`
- `bool connected() const`
- `IPAddress remoteIP() const`
- `uint16_t remotePort() const`
- `IPAddress localIP() const`
- `uint16_t localPort() const`
- `void close()`
- `void abort()`

Callback surface:

- `static err_t onRecv(void *arg, tcp_pcb *pcb, pbuf *pb, err_t err)`
- `static err_t onSent(void *arg, tcp_pcb *pcb, uint16_t len)`
- `static void onError(void *arg, err_t err)`
- `static err_t onPoll(void *arg, tcp_pcb *pcb)`
- optional `static err_t onConnected(void *arg, tcp_pcb *pcb, err_t err)` if outbound connections are later needed

### Ownership model

There are two viable models:

1. intrusive reference counting, mirroring `arduino-pico`
2. `std::shared_ptr<TcpClientContext>` with explicit lwIP callback detachment before destruction

Recommended initial choice:

- use `std::shared_ptr<TcpClientContext>`

Reasoning:

- the library already uses standard smart pointers heavily
- it makes shared ownership between the server pending queue and an accepted client handle explicit
- it is easier to reason about than a hand-rolled intrusive ref-count in early iterations

Constraint:

- the destructor must detach lwIP callbacks before the context becomes unreachable
- callback arg pointers must never point at a freed object

If callback-lifetime constraints make `shared_ptr` awkward in practice, an intrusive `retain()` / `release()` model can be adopted later without changing the public `IClient` / `IServer` interfaces.

### `TcpClient`

`TcpClient` should be a thin interface adapter.

Suggested data:

- `std::shared_ptr<TcpClientContext> context_`

Responsibilities:

- implement `IClient`
- delegate all behavior to `context_`
- remain valid as an empty / disconnected handle when constructed without a context

Expected methods:

- `write()` delegates to context
- `available()` delegates to context or returns `0` / `-1` depending on chosen invalid-client semantics
- `read()` delegates to context
- `flush()` delegates to context
- `stop()` closes and releases the context
- `status()` delegates to context or returns `ConnectionStatus::CLOSED`
- `connected()` delegates to context or returns `0`

### `TcpServer`

`TcpServer` should own the listener socket and the queue of pending accepted clients.

Suggested data:

- `IPAddress bindAddress_`
- `uint16_t port_`
- `tcp_pcb *listenPcb_`
- `std::deque<std::shared_ptr<TcpClientContext>> pendingClients_`
- optional configuration flags such as reuse-address or backlog

Responsibilities:

- create, bind, and listen on a TCP PCB
- register the accept callback
- build a new `TcpClientContext` for each accepted connection
- queue pending accepted connections until the HTTP server claims them
- return `std::unique_ptr<IClient>` from `accept()`
- expose listener status and bound port
- cleanly close the listener on `end()`

Suggested methods:

- constructor `(const IPAddress &ip, uint16_t port)`
- `void begin() override`
- `std::unique_ptr<IClient> accept() override`
- `ConnectionStatus status() override`
- `uint16_t port() const override`
- `void end() override`

Suggested callback:

- `static err_t onAccept(void *arg, tcp_pcb *newPcb, err_t err)`

Queue model recommendation:

- use `std::deque<std::shared_ptr<TcpClientContext>>`

Reasoning:

- the queue is conceptually owned by the server
- it does not need to mirror Arduino's intrusive list implementation
- the intent is clearer and easier to test

## Behavioral Invariants To Preserve

These are the most important compatibility constraints.

### 1. `accept()` must return `nullptr` when no client is pending

The current server layer depends on this exact behavior.

### 2. `connected()` must remain true while unread buffered data exists

This matches the current `WiFiClient` behavior and prevents premature pipeline teardown when the peer has closed but unread request bytes remain buffered.

### 3. `status()` must reflect writability-oriented closed state

The Pico implementation treats `CLOSE_WAIT` and `CLOSING` effectively as `CLOSED`. The in-tree implementation should preserve that mapping unless the HTTP pipeline is changed to depend on raw TCP states.

### 4. `flush()` must wait for send acknowledgment, not just push bytes to lwIP

The pipeline assumes that a completed response write has real transport meaning. `flush()` should preserve the current ack-waiting semantics.

### 5. `setTimeout()` must apply to transport operations, not just be a stored value

Timeout changes from `HttpPipeline` must influence the actual behavior of reads, writes, and flushes.

### 6. `available()` / EOF behavior must remain compatible with current request parsing

The current pipeline logic is somewhat coarse, but it works against the existing transport. The first transport rewrite should match that behavior before any attempt is made to refine end-of-input semantics.

## Interaction With Existing Server Types

### Current state

`StandardHttpServer<TServer>` stores `ServerImpl<TServer>` and delegates to its wrapped transport.

`WiFiHttpServer` is currently a typedef-style adapter over Arduino `WiFiServer` behavior.

### Recommended end state

Move away from generic transport wrappers.

Two migration options are viable.

#### Option A: Transitional path

- keep `StandardHttpServer<TServer>` temporarily
- instantiate it with the in-tree `TcpServer`
- defer broader server-template cleanup until the transport is stable

Pros:

- smaller initial change surface
- easier rollout

Cons:

- keeps template transport shape longer than necessary

#### Option B: Cleaner final design

- change `StandardHttpServer` to own a concrete `IServer` implementation or `std::unique_ptr<IServer>`
- remove `ServerImpl<T>` and `ClientImpl<T>` entirely

Pros:

- transport ownership becomes explicit
- no remaining dependence on foreign class shape
- simpler long-term architecture

Cons:

- slightly larger initial refactor

Recommended approach:

- use Option A for first integration if needed
- target Option B as the steady state

## Implementation Phases

### Phase 1: Preserve the current interfaces and add in-tree concrete transport

Goals:

- keep `IClient` and `IServer` stable
- add concrete `TcpClientContext`, `TcpClient`, and `TcpServer`
- avoid touching higher-level HTTP behavior yet

Tasks:

- add transport implementation files
- port plain TCP behavior from `ClientContext` into `TcpClientContext`
- implement `TcpClient`
- implement `TcpServer`
- keep the old templates in place temporarily for comparison and rollback safety

Acceptance criteria:

- new transport types compile in Arduino Pico builds
- a listener can accept connections and feed an `HttpPipeline`
- request parsing and response writing still function end-to-end

### Phase 2: Bind the HTTP server layer to the new transport

Goals:

- switch the concrete server type used by `WiFiHttpServer`
- reduce dependence on Arduino `WiFiServer`

Tasks:

- update `WiFiHttpServer` to construct the in-tree transport
- either adapt `StandardHttpServer` to use `TcpServer` or refactor it toward a concrete `IServer` owner

Acceptance criteria:

- `WiFiHttpServer` no longer depends on Arduino `WiFiServer`
- existing HTTP examples still compile against Pico W target

### Phase 3: Remove generic transport wrappers

Goals:

- make `NetClient.h` an interface header rather than a wrapper implementation header

Tasks:

- remove `ClientImpl<T>`
- remove `ServerImpl<T>`
- remove or replace `configureConnection()` escape hatches where possible

Acceptance criteria:

- transport behavior is entirely owned by the library
- no remaining production dependency on Arduino client/server wrapper templates

### Phase 4: Add focused tests and validation

Goals:

- validate transport behavior directly instead of relying only on whole-sketch builds

Tasks:

- add unit tests for state mapping
- add tests for receive-buffer consumption behavior
- add tests for pending-queue accept behavior
- add tests for timeout propagation
- run Pico W compile validation through existing Arduino CLI tasks

Acceptance criteria:

- the transport has direct coverage for the behavior the HTTP pipeline depends on
- Arduino compile validation passes for representative examples or the main sketch target

## Testing Strategy

The repository currently has limited direct transport coverage, so this work should add targeted tests instead of relying only on end-to-end sketches.

### Unit-level tests

1. state mapping tests

   Verify lwIP state to `ConnectionStatus` translation, especially:

   - `LISTEN`
   - `ESTABLISHED`
   - `CLOSE_WAIT -> CLOSED`
   - `CLOSING -> CLOSED`

2. receive-buffer consumption tests

   Verify:

   - `available()` matches queued bytes
   - `read()` advances offsets correctly
   - reading across chained buffers behaves correctly
   - consuming the final bytes releases buffers correctly

3. server queue tests

   Verify:

   - accepted contexts are queued in arrival order
   - `accept()` returns `nullptr` when empty
   - accepted clients are removed from the queue exactly once

4. timeout propagation tests

   Verify:

   - `setTimeout()` updates the effective transport timeout
   - `getTimeout()` reports the current value

### Integration tests

1. pipeline compatibility tests with fake or test transport

   Verify:

   - request parsing completes as expected
   - disconnect with buffered unread data does not prematurely kill parsing
   - response writing treats partial writes and flush correctly

2. Arduino compile validation

   Use the existing Arduino CLI compile task against the Pico W profile as the real compile gate for the transport implementation.

## Open Design Questions

These questions do not block the initial implementation, but they should be answered during the rollout.

### 1. Should `StandardHttpServer` remain templated?

Recommendation:

- tolerate it during transition if it reduces churn
- remove transport template coupling in the steady state

### 2. Should `configureConnection()` survive?

Recommendation:

- do not preserve arbitrary raw transport mutation by default
- replace with named, library-owned transport configuration APIs only if concrete use cases require them

### 3. Should outbound connect support be part of `IClient`?

Recommendation:

- not now
- the current HTTP server path only needs accepted connections

### 4. Should shared ownership use `shared_ptr` or intrusive refs?

Recommendation:

- start with `shared_ptr`
- switch only if lwIP callback realities or memory-profile constraints justify it

## Risks

### Callback lifetime bugs

The highest-risk area is ensuring lwIP callbacks cannot access freed transport objects.

### Partial-write regressions

Write behavior must preserve the retry and backpressure semantics currently inherited from Pico's `ClientContext`.

### EOF and connection-state regressions

The current pipeline is sensitive to `available()`, `connected()`, and `status()` semantics. Any behavior drift there can cause truncated requests, premature disconnect handling, or hung pipelines.

### Over-generalizing too early

The first implementation should target plain TCP server transport for Pico W. It should not try to become a universal transport framework before the core behavior is proven.

## Recommended First Code Changes After This Document

1. keep `NetClient.h` interface-only from a design perspective, even if the templates remain temporarily in code during the first implementation phase
2. add `TcpClientContext`, `TcpClient`, and `TcpServer`
3. bind `WiFiHttpServer` to the in-tree transport
4. validate compile and pipeline behavior
5. remove generic wrapper templates once the new transport is proven

## Summary

The correct replacement for the current wrapper-based design is not another wrapper over Arduino classes. It is a library-owned transport subsystem that:

- preserves the existing `IClient` / `IServer` contract
- owns plain TCP socket behavior directly
- models accepted-client lifetime explicitly
- isolates lwIP details to implementation files
- keeps HTTP pipeline behavior stable while decoupling from Arduino transport classes

This gives the library a transport layer that is easier to reason about, easier to test, and aligned with the broader Arduino decoupling work already in progress.