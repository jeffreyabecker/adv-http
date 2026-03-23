2026-03-22 - Copilot: made JSON enablement explicit, removed stale secure-scope residue from public docs and metadata, and validated the JSON-disabled native lane.
2026-03-21 - Copilot: created detailed Phase 7 optional features and removal scope backlog.

# No-Arduino Phase 7 Optional Features And Removal Scope Backlog

## Summary

This phase cleans up the optional feature boundary and removes stale scope that should not be carried through the refactor. The main work is to isolate JSON integration so it behaves like an optional portable feature rather than an always-on assumption, and to finish removing secure-scope residue from examples, docs, and umbrella surfaces. The goal is to make feature boundaries honest and keep the post-migration library surface aligned with what the repository actually supports.

## Goal / Acceptance Criteria

- JSON can be enabled or disabled intentionally rather than being treated as an unconditional baseline feature in all build paths.
- JSON-enabled code paths no longer reintroduce Arduino-owned string assumptions into the core.
- Secure-scope residue is removed or explicitly rewritten to match the supported library surface.
- Public docs accurately describe optional features and removed scope.

## Tasks

### JSON Build Boundary

- [x] Define the intended default policy for JSON support across PlatformIO and Arduino CLI workflows.
- [x] Update build flags and conditional compilation so JSON-disabled builds are first-class rather than accidental breakage.
- [x] Audit `src/core/Defines.h` and any related feature guards to ensure the JSON feature flag is named, documented, and used consistently.
- [x] Verify that JSON feature gating does not silently pull ArduinoJson headers into files that should remain host-safe.

### JSON Handler And Response Cleanup

- [x] Refactor `src/handlers/JsonBodyHandler.h` and `src/handlers/JsonBodyHandler.cpp` so JSON integration sits behind a clear optional seam.
- [x] Refactor `src/response/JsonResponse.h` and `src/response/JsonResponse.cpp` so JSON responses operate cleanly with standard-text internals and optional feature guards.
- [x] Decide whether umbrella includes in `src/HttpServerAdvanced.h` should conditionally expose JSON facilities or keep them in a separate include group.
- [x] Add tests or compile checks proving that JSON-disabled builds exclude JSON facilities cleanly.

### Secure Scope Cleanup

- [x] Audit `examples/3_Security/HttpsServer/` and decide whether to delete it, repurpose it, or replace it with a note explaining removed scope.
- [x] Audit `docs/httpserveradvanced/LIBRARY_DOCUMENTATION.md` for stale references to HTTPS behavior, secure transport assumptions, or scheme descriptions that imply current support.
- [x] Audit `docs/httpserveradvanced/EXAMPLES.md` and `docs/httpserveradvanced/LIBRARY_COMPARISON.md` for stale secure-scope references.
- [x] Verify that no umbrella includes, aliases, or examples imply the existence of `SecureHttpServer` or related secure APIs.

### Optional Feature Documentation

- [x] Document JSON as an optional portable feature with clear enablement expectations.
- [x] Document HTTPS/TLS as removed scope for this repository.
- [x] Document any remaining filesystem or transport features that are adapter-driven rather than guaranteed in the core.

### Validation

- [x] Ensure at least one configured build path can exclude JSON assumptions cleanly.
- [x] Ensure docs and examples no longer advertise removed secure support.
- [x] Record any deliberate compatibility break in the changelog or migration notes if secure examples are removed.

## Owner

TBD

## Priority

Medium

## References

- `platformio/common.ini`
- `src/core/Defines.h`
- `src/handlers/JsonBodyHandler.h`
- `src/handlers/JsonBodyHandler.cpp`
- `src/response/JsonResponse.h`
- `src/response/JsonResponse.cpp`
- `src/HttpServerAdvanced.h`
- `examples/3_Security/HttpsServer/`
- `docs/httpserveradvanced/EXAMPLES.md`
- `docs/httpserveradvanced/LIBRARY_DOCUMENTATION.md`
- `docs/httpserveradvanced/LIBRARY_COMPARISON.md`
