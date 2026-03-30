2026-03-30 - Copilot: reframed the backlog around a cross-platform compatibility boundary and removed framework-specific positioning.
2026-03-23 - Copilot: created summary compatibility-boundary backlog.

# Compatibility Boundary Summary Backlog

## Summary

This backlog defines the phased plan for making the library explicitly cross-platform through a clear compatibility boundary instead of relying on diffuse runtime assumptions spread through the codebase. The current codebase already has meaningful progress in this direction: transport is abstracted behind `IClient` and `IServer`, `Clock` is isolated in `src/compat/Clock.h`, and host-side filesystem support exists through `IFileSystem` and `PosixFileAdapter`. Some APIs may still reflect legacy framework-oriented shapes or naming, but those surfaces should remain intentional compatibility layers rather than leaks from a specific platform stack. The remaining work is to complete the missing seams around the core, keep application-facing surfaces backed by library-owned boundaries, and add enforcement so runtime-specific dependencies do not drift back into the core.

This summary item is intentionally phase-oriented. Follow-up backlog items under this folder should break out each phase into implementation-sized tasks once work starts.

## Goal / Acceptance Criteria

- The library has a clearly defined compatibility boundary with one canonical home under `src/compat`.
- Core and host-safe headers depend on compatibility types or standard-library types rather than runtime-specific framework headers.
- Convenience APIs and legacy naming remain available only as intentional boundary surfaces that forward into compatibility-backed internals without exposing platform-specific seams.
- Filesystem, stream, clock, and any remaining application bootstrap seams have explicit library-owned behavior for embedded and desktop environments.
- Native validation and supported embedded build validation catch regressions that would reintroduce direct runtime coupling into the core.
- Each implementation phase can be tracked independently through follow-up backlog items in this area.

## Tasks

### Phase 1: Boundary And Surface Inventory

- [ ] Freeze the compatibility-layer scope and document which seams are canonical versus transitional.
- [ ] Inventory remaining legacy framework-shaped dependencies and naming across core, routing, response, static-file, and adapter code.
- [ ] Document the intentional application-facing overloads and legacy names that define the compatibility surface, while confirming that no current API exposes runtime-specific types.
- [ ] Define the exit criteria for each follow-up phase backlog before implementation begins.

### Phase 2: Complete Core Compatibility Types

- [ ] Finalize the compatibility stream abstraction and preserve current `available` / `read` / `peek` / `write` / `flush` semantics.
- [ ] Complete the filesystem and file-handle compatibility contract so embedded and host implementations expose the same library-owned behavior expectations.
- [ ] Confirm whether any IP-address or other platform value types still need an explicit compatibility seam, or document that current transport abstractions already close that gap.
- [ ] Keep `Clock` and other existing compatibility primitives aligned with the same boundary rules.

### Phase 3: Preserve Compatibility Surfaces

- [ ] Keep application-facing APIs easy to use while ensuring their implementations continue to route through compatibility or standard-library-backed internals.
- [ ] Convert any remaining internal owned text and routing state to standard-library storage where that work still improves the compatibility boundary.
- [ ] Keep thin convenience overloads only where source compatibility is worth preserving.
- [ ] Remove or simplify public APIs that still leak obsolete runtime assumptions into the core.

### Phase 4: Isolate Adapters And Optional Features

- [ ] Keep transport, filesystem, and bootstrap-specific wiring in adapter-oriented code rather than the HTTP core.
- [ ] Split optional integrations such as JSON support so they remain opt-in feature layers instead of baseline dependencies.
- [ ] Verify static-file serving and response-streaming paths use the compatibility boundary rather than framework headers directly.
- [ ] Ensure remaining defaults, strings, or metadata are intentional, portable, and easy to override.

### Phase 5: Validation, Enforcement, And Cleanup

- [ ] Add or expand native tests that lock down compatibility-layer behavior independently from board-specific transports.
- [ ] Add supported embedded compile validation using meaningful build lanes rather than relying on the current root compile task.
- [ ] Add boundary checks that fail when core code starts including runtime framework headers directly again.
- [ ] Remove superseded temporary shims, compatibility debt, and obsolete planning notes as each phase closes.

## Owner

TBD

## Priority

High

## References

- `src/compat/Compat.h`
- `src/compat/Clock.h`
- `src/compat/IFileSystem.h`
- `src/compat/PosixFileAdapter.h`
- `src/pipeline/TransportInterfaces.h`
- `src/streams/ByteStream.h`
- `src/staticfiles/DefaultFileLocator.cpp`
- `src/routing/HandlerMatcher.h`
- `src/routing/BasicAuthentication.h`
- `src/routing/CrossOriginRequestSharing.h`
- `src/HttpServerAdvanced.h`
- `docs/backlogs/httpserveradvanced-decoupling-backlog.md`