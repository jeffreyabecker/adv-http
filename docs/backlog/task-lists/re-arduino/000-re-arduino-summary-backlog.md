2026-03-23 - Copilot: reframed the backlog as Arduino-friendliness and compatibility-boundary work rather than a porting or internals-retargeting effort.
2026-03-23 - Copilot: created summary re-arduino compatibility layer backlog.

# Re-Arduino Compatibility Layer Summary Backlog

## Summary

This backlog defines the phased plan for making the library explicitly Arduino-friendly through a clear compatibility boundary instead of relying on diffuse framework assumptions spread through the codebase. The current codebase already has meaningful progress in this direction: transport is abstracted behind `IClient` and `IServer`, `Clock` is isolated in `src/compat/Clock.h`, and host-side filesystem support exists through `IFileSystem` and `PosixFileAdapter`. Current APIs may still look Arduino-shaped in places or retain Arduino-oriented naming, but those shapes are intentional compatibility surfaces and do not presently expose Arduino-specific seams. The remaining work is to complete the missing compatibility seams around the core, keep those sketch-facing surfaces backed by explicit library-owned boundaries, and add enforcement so Arduino-only dependencies do not drift back into the core.

This summary item is intentionally phase-oriented. Follow-up backlog items under this folder should break out each phase into implementation-sized tasks once work starts.

## Goal / Acceptance Criteria

- The library has a clearly defined Arduino-friendliness boundary with one canonical compatibility home under `src/compat`.
- Core and host-safe headers depend on compatibility types or standard-library types rather than Arduino framework headers.
- Arduino-shaped convenience APIs or naming remain available as intentional sketch-compatibility surfaces that forward into standard or compatibility-backed internals, without exposing Arduino-specific types.
- Filesystem, stream, clock, and any remaining sketch integration seams have explicit library-owned behavior for Arduino and non-Arduino environments.
- Native validation and Arduino build validation catch regressions that would reintroduce direct Arduino coupling into the core.
- Each implementation phase can be tracked independently through follow-up backlog items in `docs/backlog/task-lists/re-arduino`.

## Tasks

### Phase 1: Boundary And Surface Inventory

- [ ] Freeze the compatibility-layer scope and document which seams are canonical versus transitional.
- [ ] Inventory remaining Arduino-shaped dependencies and naming across core, routing, response, static-file, and adapter code.
- [ ] Document the intentional sketch-facing overloads and Arduino-shaped names that define the compatibility surface, while confirming that no current API exposes Arduino-specific types.
- [ ] Define the exit criteria for each follow-up phase backlog before implementation begins.

### Phase 2: Complete Core Compatibility Types

- [ ] Finalize the compatibility stream abstraction and preserve current `available` / `read` / `peek` / `write` / `flush` semantics.
- [ ] Complete the filesystem and file-handle compatibility contract so Arduino and host implementations expose the same library-owned behavior expectations.
- [ ] Confirm whether any IP-address or other platform value types still need an explicit compatibility seam, or document that current transport abstractions already close that gap.
- [ ] Keep `Clock` and other existing compatibility primitives aligned with the same boundary rules.

### Phase 3: Preserve Arduino-Friendly Surfaces

- [ ] Keep sketch-facing APIs Arduino-friendly while ensuring their implementations continue to route through compatibility or standard-library-backed internals.
- [ ] Convert any remaining internal owned text and routing state to standard-library storage where that work still improves the compatibility boundary.
- [ ] Keep Arduino `String` convenience overloads as intentional thin compatibility wrappers where source compatibility is worth preserving.
- [ ] Remove or simplify only those public APIs that still leak obsolete framework assumptions into the core, while preserving intentional Arduino-like compatibility surfaces.

### Phase 4: Isolate Arduino Adapters And Optional Features

- [ ] Keep Arduino-specific transport, filesystem, and sketch helper wiring in adapter-oriented code rather than the HTTP core.
- [ ] Split optional integrations such as ArduinoJson so they remain opt-in feature layers instead of baseline dependencies.
- [ ] Verify static-file serving and response-streaming paths use the compatibility boundary rather than framework headers directly.
- [ ] Ensure any remaining Arduino-oriented defaults, strings, or metadata are intentional, sketch-friendly, and easy to override.

### Phase 5: Validation, Enforcement, And Cleanup

- [ ] Add or expand native tests that lock down compatibility-layer behavior independently from board-specific transports.
- [ ] Add Arduino-side compile validation using meaningful sketches or supported build lanes rather than relying on the current root compile task.
- [ ] Add boundary checks that fail when core code starts including Arduino framework headers directly again.
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