2026-03-23 - Copilot: aligned Phase 1 backlog with accepted decisions for result ownership, session boundary, and pipeline state.
2026-03-23 - Copilot: created detailed Phase 1 WebSocket upgrade seam and connection lifecycle backlog.

# WebSocket Phase 1 Upgrade Seam And Connection Lifecycle Backlog

## Summary

This phase establishes the architectural seam that makes WebSocket support possible without distorting the existing HTTP request and response model. The main objective is to let the pipeline represent a successful protocol upgrade as a first-class connection outcome, with transport ownership and timeout supervision remaining inside the pipeline rather than leaking into handlers.

## Goal / Acceptance Criteria

- `HttpPipeline` can represent both normal HTTP completion and transfer into an upgraded connection session through a dedicated `RequestHandlingResult` boundary.
- Upgraded-session ownership is explicit and does not depend on hidden side effects in `HttpRequest` response dispatch.
- Timeout, abort, disconnect, and cleanup logic still flow through one pipeline-owned lifecycle coordinated by an explicit connection-state enum.
- The internal seam is narrow enough that later handshake and frame work can build on it without revisiting the core ownership model.

## Locked Decisions Applied Here

- Use a dedicated pipeline result object as the request-to-pipeline handoff.
- Use a single-step `IConnectionSession` interface for upgraded-session execution.
- Use a single explicit connection state enum as the authoritative pipeline lifecycle model.

## Unit Test Coverage Targets

- Add native tests for pipeline transitions from HTTP parsing into an upgraded-session state using scripted fake clients.
- Verify that normal HTTP response paths still complete exactly as before when no upgrade result is produced.
- Verify upgraded-session ownership transfer, pipeline-finished decisions, and cleanup on disconnect or abort.
- Add fake-session tests that assert timeout bookkeeping, enum state transitions, and response-start or response-complete notifications do not regress when the upgraded path is present.
- Cover edge cases where upgrade is requested internally but no session object is produced, or where the session is created but fails immediately.

## Tasks

### Result And Ownership Model

- [ ] Define the internal `RequestHandlingResult` type so request handling returns exactly one terminal outcome: response, upgrade, no-response, or error.
- [ ] Make response and upgrade mutually exclusive by type rather than by convention.
- [ ] Keep the upgrade representation internal until Phase 5 public APIs are ready.
- [ ] Ensure `HttpPipeline` becomes the sole owner of the consumed result payloads after handoff from `HttpRequest`.

### Pipeline Refactor

- [ ] Refactor `src/pipeline/HttpPipeline.h` and `src/pipeline/HttpPipeline.cpp` around an explicit connection-state enum rather than scattered lifecycle booleans.
- [ ] Introduce the internal single-step `IConnectionSession` abstraction for long-lived post-handshake IO.
- [ ] Separate request-read completion from whole-connection completion so the upgraded path can continue after HTTP parsing ends.
- [ ] Keep keep-alive re-entry explicit for normal HTTP responses and make `UpgradedSessionActive` a one-way transition out of the HTTP lifecycle.

### Request Boundary Adjustments

- [ ] Adjust `src/core/HttpRequest.h` and `src/core/HttpRequest.cpp` so request handling constructs a `RequestHandlingResult` instead of signaling upgrade through response-specific paths.
- [ ] Replace or narrow the current response-stream callback usage so result-object transfer becomes the authoritative boundary.
- [ ] Review whether `HttpRequest::handleStep()` should remain the central coordination point once upgrade approval exists.

### Fake Transport Support

- [ ] Add or extend fake `IClient` fixtures for upgraded-session loop testing.
- [ ] Add a stub `IConnectionSession` fixture that records loop calls, read attempts, write attempts, and completion signals.
- [ ] Keep fixture behavior reusable by later handshake and runtime phases.

## Decision Follow-Through

- Item 1 in the pre-implementation decision backlog defines the result-object ownership rules this phase must enforce.
- Item 2 defines the single-step `IConnectionSession` boundary this phase must introduce.
- Item 3 defines the explicit state model this phase must adopt.

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
- `docs/backlog/task-lists/web-sockets/008-websocket-pre-implementation-decisions-backlog.md`