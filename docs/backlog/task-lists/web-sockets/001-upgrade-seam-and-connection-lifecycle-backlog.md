2026-03-23 - Copilot: created detailed Phase 1 WebSocket upgrade seam and connection lifecycle backlog.

# WebSocket Phase 1 Upgrade Seam And Connection Lifecycle Backlog

## Summary

This phase establishes the architectural seam that makes WebSocket support possible without distorting the existing HTTP request and response model. The main objective is to let the pipeline represent a successful protocol upgrade as a first-class connection outcome, with transport ownership and timeout supervision remaining inside the pipeline rather than leaking into handlers.

## Goal / Acceptance Criteria

- `HttpPipeline` can represent both normal HTTP completion and transfer into an upgraded connection session.
- Upgraded-session ownership is explicit and does not depend on hidden side effects in `HttpRequest` response dispatch.
- Timeout, abort, disconnect, and cleanup logic still flow through one pipeline-owned lifecycle.
- The internal seam is narrow enough that later handshake and frame work can build on it without revisiting the core ownership model.

## Unit Test Coverage Targets

- Add native tests for pipeline transitions from HTTP parsing into an upgraded-session state using scripted fake clients.
- Verify that normal HTTP response paths still complete exactly as before when no upgrade result is produced.
- Verify upgraded-session ownership transfer, pipeline-finished decisions, and cleanup on disconnect or abort.
- Add fake-session tests that assert timeout bookkeeping and response-start or response-complete notifications do not regress when the upgraded path is present.
- Cover edge cases where upgrade is requested internally but no session object is produced, or where the session is created but fails immediately.

## Tasks

### Result And Ownership Model

- [ ] Define the internal result type that allows request handling to return either a normal HTTP response path or an approved upgrade path.
- [ ] Decide whether the seam is expressed as a new pipeline result object, an explicit session factory result, or a request-owned upgrade descriptor.
- [ ] Keep the upgrade representation internal until Phase 5 public APIs are ready.
- [ ] Ensure client ownership remains pipeline-controlled even after upgrade is accepted.

### Pipeline Refactor

- [ ] Refactor `src/pipeline/HttpPipeline.h` and `src/pipeline/HttpPipeline.cpp` so the pipeline can enter an upgraded-session execution mode.
- [ ] Introduce an internal session abstraction such as `IConnectionSession` or `IUpgradedConnection` for long-lived post-handshake IO.
- [ ] Separate request-read completion from whole-connection completion so the upgraded path can continue after HTTP parsing ends.
- [ ] Keep state transitions explicit enough to avoid ambiguous combinations of completed request, completed response, and upgraded-session activity.

### Request Boundary Adjustments

- [ ] Adjust `src/core/HttpRequest.h` and `src/core/HttpRequest.cpp` so request handling can report upgrade approval without pretending it produced an `IHttpResponse` stream.
- [ ] Split request-result creation from response-stream delivery if the current `onStreamReady_` callback is too response-specific for upgraded connections.
- [ ] Review whether `HttpRequest::handleStep()` should remain the central coordination point once upgrade approval exists.

### Fake Transport Support

- [ ] Add or extend fake `IClient` fixtures for upgraded-session loop testing.
- [ ] Add a stub upgraded-session fixture that records lifecycle callbacks, read attempts, write attempts, and completion signals.
- [ ] Keep fixture behavior reusable by later handshake and runtime phases.

## Owner

TBD

## Priority

High

## References

- `docs/plans/websocket-support-plan.md`
- `src/pipeline/HttpPipeline.h`
- `src/pipeline/HttpPipeline.cpp`
- `src/core/HttpRequest.h`
- `src/core/HttpRequest.cpp`
- `src/pipeline/IPipelineHandler.h`
- `src/pipeline/PipelineHandleClientResult.h`
- `src/pipeline/TransportInterfaces.h`
- `src/server/HttpServerBase.h`
- `src/server/HttpServerBase.cpp`
- `test/test_native/`