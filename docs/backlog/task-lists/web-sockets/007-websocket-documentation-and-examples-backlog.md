2026-03-23 - Copilot: documented websocket scope/unsupported list/limits/validation path in library docs and added websocket example-status notes; marked completed documentation tasks.
2026-03-23 - Copilot: added locked decision for documentation gate (14); updated scope and example tasks to match merge requirements.
2026-03-23 - Copilot: created detailed Phase 7 WebSocket documentation and examples backlog.

# WebSocket Phase 7 Documentation And Example Follow-Through Backlog

## Summary

This phase documents the actual shipped WebSocket surface after the internal architecture and API stabilize. It should make the supported scope, resource limits, validation path, and deferred RFC features explicit so users do not infer support that the first version does not provide.

## Goal / Acceptance Criteria

- The shipped WebSocket scope is documented precisely, including supported behaviors and deferred features.
- Developers can find the upgrade seam, session model, public registration API, and validation path from the documentation set.
- Any example added reflects the stable API rather than a transitional shape.
- Documentation matches the resource and protocol limits that the code enforces.

## Locked Decisions Applied Here

- The unsupported-feature list must be present in `LIBRARY_DOCUMENTATION.md` before the implementation is merged; this is a hard merge-gate requirement.
- A minimal echo example is a follow-up backlog item filed after the Phase 5 API stabilizes; it is explicitly not a merge-gate requirement.

## Unit Test Coverage Targets

- Verify that documentation examples compile in the native lane or through a documentation-specific smoke check if an example is added.
- Ensure the documented validation path references the real native test commands and suites used during implementation.
- Add or update smoke coverage for any example code introduced to prevent immediate drift from the documented API.

## Tasks

### Scope And Behavior Documentation

- [x] Add the unsupported-feature section to `LIBRARY_DOCUMENTATION.md` as a merge gate, covering all of the following:
  - WebSocket extensions (permessage-deflate and all RFC 7692+ extensions).
  - Subprotocol negotiation (`Sec-WebSocket-Protocol` accepted but ignored).
  - Multiplexing (one WebSocket per TCP connection).
  - Server-initiated PING frames.
  - Messages exceeding `WsMaxMessageSize` (rejected with close code 1009).
  - PING and PONG observation callbacks.
  - Outbound sends from outside the pipeline loop.
- [x] Document supported frame behavior, close behavior, timeout expectations, and the four configured limit constants.
- [x] Validate documentation against the actual native test assertions and constant values rather than aspirational descriptions.

### Developer Documentation

- [x] Add notes that explain the upgrade seam and why WebSocket support is not modeled as a normal `IHttpHandler` variant.
- [x] Document the public route registration and callback model once it is stable.
- [x] Document the expected native validation path and any `native_portable_sources.cpp` updates required for host-side coverage.

### Examples And Cross-Links

- [ ] Add a minimal echo-server WebSocket example as a follow-up after the Phase 5 public API is confirmed stable; this is explicitly not a merge-gate requirement for the initial implementation PR.
- [x] Review existing docs pages and add cross-links where WebSocket discoverability matters.
- [ ] Keep examples narrow and aligned with the documented first-release scope.

## Owner

TBD

## Priority

Medium

## References

- `docs/plans/websocket-support-plan.md`
- `docs/httpserveradvanced/EXAMPLES.md`
- `docs/httpserveradvanced/LIBRARY_DOCUMENTATION.md`
- `test/README.md`
- `tools/run_native_tests.ps1`
- `test/test_native/native_portable_sources.cpp`