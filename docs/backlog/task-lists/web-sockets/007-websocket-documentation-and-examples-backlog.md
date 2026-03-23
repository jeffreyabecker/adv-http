2026-03-23 - Copilot: created detailed Phase 7 WebSocket documentation and examples backlog.

# WebSocket Phase 7 Documentation And Example Follow-Through Backlog

## Summary

This phase documents the actual shipped WebSocket surface after the internal architecture and API stabilize. It should make the supported scope, resource limits, validation path, and deferred RFC features explicit so users do not infer support that the first version does not provide.

## Goal / Acceptance Criteria

- The shipped WebSocket scope is documented precisely, including supported behaviors and deferred features.
- Developers can find the upgrade seam, session model, public registration API, and validation path from the documentation set.
- Any example added reflects the stable API rather than a transitional shape.
- Documentation matches the resource and protocol limits that the code enforces.

## Unit Test Coverage Targets

- Verify that documentation examples compile in the native lane or through a documentation-specific smoke check if an example is added.
- Ensure the documented validation path references the real native test commands and suites used during implementation.
- Add or update smoke coverage for any example code introduced to prevent immediate drift from the documented API.

## Tasks

### Scope And Behavior Documentation

- [ ] Document the first shipped scope: HTTP/1.1 upgrade only, no extensions, no compression, and no subprotocol negotiation unless added deliberately.
- [ ] Document supported frame behavior, close behavior, timeout expectations, and configured limits.
- [ ] Call out deferred or unsupported RFC features explicitly.

### Developer Documentation

- [ ] Add notes that explain the upgrade seam and why WebSocket support is not modeled as a normal `IHttpHandler` variant.
- [ ] Document the public route registration and callback model once it is stable.
- [ ] Document the expected native validation path and any `native_portable_sources.cpp` updates required for host-side coverage.

### Examples And Cross-Links

- [ ] Add a minimal WebSocket example only after the public API stops moving materially.
- [ ] Review existing docs pages and add cross-links where WebSocket discoverability matters.
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