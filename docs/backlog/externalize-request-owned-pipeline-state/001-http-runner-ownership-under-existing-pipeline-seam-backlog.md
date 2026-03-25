2026-03-24 - Copilot: created Phase 1 backlog for extracting HTTP runner ownership under the existing pipeline seam.

# HTTP Runner Ownership Under Existing Pipeline Seam Backlog

## Summary

This phase removes orchestration ownership from `HttpRequest` without changing `HttpPipeline` to a new seam yet. The key move is to introduce an HTTP runner that owns handler lifetime, result staging, phase progression, and error-to-response mapping while the existing pipeline still talks to an `IPipelineHandler`-shaped surface.

## Goal / Acceptance Criteria

- `HttpRequest` no longer directly owns cached handler lifetime, pending result staging, parser-driven orchestration, or response-start/complete phase progression.
- An HTTP runner abstraction owns that orchestration under the current `IPipelineHandler` seam.
- Existing HTTP and websocket behavior stays green while ownership moves.

## Interface Options

### Option A: Mirror The Current `IPipelineHandler` Surface

This is the most direct extraction shape: define `HttpRequestRunner` with nearly the same methods as `IPipelineHandler`, but make the runner own orchestration while a thin adapter preserves the pipeline-facing seam.

Sketch:

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

Ownership in this option:

- runner owns handler lifetime
- runner owns result staging
- runner owns phase progression
- runner owns parser/pipeline error mapping
- context remains request data plus helpers

Advantages:

- smallest migration from the current code
- easiest to test against existing `IPipelineHandler` expectations
- lets `HttpPipeline` stay unchanged in this phase

Costs:

- the runner interface stays tightly coupled to the old pipeline callback shape
- some later work will still be needed when the shared `IProtocolExecution` seam lands

### Option B: Split Parser Input From Execution Output

This option separates raw parser callbacks from execution/result control so the runner has a narrower orchestration API and a smaller pipeline-facing result surface.

Sketch:

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

	virtual void onPipelineError(PipelineError error) = 0;
	virtual void onWriteStateChanged(HttpRequestPhase phase) = 0;
	virtual void onClientDisconnected() = 0;

	virtual std::optional<RequestHandlingResult> takePendingResult() = 0;
};
```

Ownership in this option is the same as Option A, but the interface begins to abstract away some of the legacy response callbacks.

Advantages:

- slightly cleaner runner API
- starts separating parser events from pipeline write-state events
- may reduce adapter thickness later

Costs:

- introduces a new shape while extraction is already moving ownership
- risks solving migration and redesign at the same time
- requires more judgment about phase projection before the code has been split

### Option C: Context Mutation Plus Explicit `advance()` Or `drive()`

This option minimizes callback count by treating parser hooks as pure context mutation and having the runner expose an explicit execution step method.

Sketch:

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
	virtual int onBody(const std::uint8_t *at, std::size_t length) = 0;

	virtual void advance(HttpRequestPhase trigger) = 0;
	virtual void onError(PipelineError error) = 0;
	virtual void onClientDisconnected() = 0;

	virtual bool hasPendingResult() const = 0;
	virtual RequestHandlingResult takeResult() = 0;
};
```

Advantages:

- conceptually closer to the eventual protocol-execution model
- makes execution stepping more explicit

Costs:

- biggest departure from the current seam
- would force more changes in `HttpRequest` and `HttpPipeline` immediately
- too much redesign pressure for the first extraction slice

## Recommendation

Use **Option C** for Phase 1.

Reasoning:

- It starts shaping the HTTP side around explicit execution stepping instead of preserving the current callback-heavy orchestration surface indefinitely.
- It makes the separation clearer between parser-driven context mutation and runner-owned execution decisions.
- It aligns more directly with the broader `012` direction toward a shared protocol-execution model instead of deepening investment in the current `IPipelineHandler` shape.
- It still allows a thin adapter to preserve the existing `IPipelineHandler` seam while the pipeline remains unchanged.

Tradeoff acknowledgment:

- Option C is a more ambitious first slice than Option A.
- That means the implementation should be disciplined about keeping the adapter thin and avoiding unrelated pipeline redesign while the ownership split lands.

## Ownership Responsibilities To Lock In

Whichever option is chosen, `HttpRequestRunner` should own at minimum:

- lazy handler creation through `IHttpRequestHandlerFactory`
- cached handler lifetime for the current request
- pending `RequestHandlingResult` staging
- parser-phase and response-phase progression state
- `HandlerResult` to `RequestHandlingResult` translation
- parser/pipeline error-to-response mapping
- no-response finalization when the handler completes without producing a response

`HttpRequest` should remain responsible for:

- parsed request metadata
- address data
- `items()`
- URI/helper access
- request-local helper access such as `server()` and any remaining response-factory convenience

## Tasks

- [ ] Define the concrete `HttpRequestRunner`-style interface and its ownership responsibilities.
- [ ] Introduce an `HttpRequestPipelineAdapter`-style adapter if needed to preserve the current `IPipelineHandler` seam.
- [ ] Move handler lifetime ownership out of `HttpRequest`.
- [ ] Move pending result staging out of `HttpRequest`.
- [ ] Move parser-phase and response-phase progression bookkeeping out of `HttpRequest`.
- [ ] Move parser/pipeline error-to-response mapping out of `HttpRequest`.
- [ ] Keep existing request/pipeline/native tests green throughout the extraction.

## References

- [docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md](docs/plans/externalize-request-owned-pipeline-state-implementation-plan.md)
- [docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md](docs/backlog/task-lists/web-sockets/012-externalize-request-owned-pipeline-state-for-peer-contexts-backlog.md)
- [src/core/HttpRequest.h](src/core/HttpRequest.h)
- [src/core/HttpRequest.cpp](src/core/HttpRequest.cpp)
- [src/pipeline/IPipelineHandler.h](src/pipeline/IPipelineHandler.h)
