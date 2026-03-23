2026-03-23 - Copilot: removed stale references to the retired no-Arduino plan docs after the backlog became the canonical migration record.
2026-03-21 - Copilot: decided to keep logging example-only for now and moved shared helper logging/timing behavior behind a new `examples/ExampleRuntime.h` boundary used by `examples/WifiSetup.h`.
2026-03-21 - Copilot: aligned `HttpTimeouts` with `Compat::ClockMillis`, completed the residual runtime-helper scan for `src/`, and documented the injected clock seam follow-on for native timeout fixtures.
2026-03-21 - Copilot: migrated `HttpPipeline` timekeeping onto the injected clock seam, removed core `F()` literal usage from `HttpUtility.cpp`, and added native seam regression tests.
2026-03-21 - Copilot: added a concrete header-only clock seam with system and manual clocks while leaving pipeline time migration open.
2026-03-21 - Copilot: created detailed Phase 3 runtime and miscellaneous Arduino helpers backlog.

# No-Arduino Phase 3 Runtime And Miscellaneous Arduino Helpers Backlog

## Summary

This phase cleans up the Arduino runtime dependencies that are not the main transport, stream, or filesystem seams but still prevent a clean platform-neutral core. The repository now routes pipeline timing through the library-owned clock seam, no longer uses `F()` macro literals in `HttpUtility.cpp`, and keeps shared example logging and delay behavior under `examples/ExampleRuntime.h` rather than in `src/`. The remaining goal is to verify timeout behavior more deeply and finish any example-only runtime cleanup without inventing premature core abstractions.

## Goal / Acceptance Criteria

- Timeout and activity tracking code no longer calls Arduino runtime functions directly.
- Core code no longer depends on `F()` macro literals or other Arduino-only string-storage helpers.
- Example logging and board-runtime behavior are clearly separated from core behavior.
- The repository has a concrete implemented clock seam, not just a placeholder header.

## Tasks

### Clock Seam Implementation

- [x] Replace the placeholder `src/compat/Clock.h` forward declaration with an actual compatibility interface or value-type API for time access.
- [x] Decide whether the clock seam is header-only or requires a `.cpp` implementation for host and Arduino backends.
- [x] Define the minimum operations needed by the pipeline and timeout logic, such as `nowMillis()` and any deterministic test hooks.
- [x] Ensure the chosen API is easy to fake in host-side tests without global mutable state.

### Pipeline Time Migration

- [x] Refactor `src/pipeline/HttpPipeline.cpp` to remove direct `millis()` calls.
- [x] Thread the new clock seam through pipeline construction or configuration without creating brittle global dependencies.
- [x] Review `src/core/HttpTimeouts.h` and any related timeout configuration code so naming and types align with the new seam.
- [ ] Verify that timeout state transitions and activity tracking semantics remain unchanged after the clock swap.

### Deterministic Test Support

- [x] Add a fake or test clock implementation suitable for future pipeline fixture tests.
- [x] Decide where the fake clock should live so it can be reused by later transport-loop tests.
- [x] Add at least one focused native test proving timeout arithmetic or activity-reset logic through the new seam.

### `F()` And Literal Storage Cleanup

- [x] Audit `src/util/HttpUtility.cpp` for every `F()` macro use.
- [x] Replace `F()` macro literals in core code with portable compile-time literals or a narrowly scoped compatibility helper.
- [x] Decide whether a compatibility macro is warranted at all or whether standard literals are sufficient for the current code paths.
- [x] Verify that HTML-encoding and attribute-encoding output is unchanged after the replacement.

### Residual Runtime Helper Audit

- [x] Re-scan `src/**` for any residual direct runtime helper calls such as `delay()`, `yield()`, or Arduino-specific convenience macros.
- [x] Classify any remaining runtime helper use as core bug, adapter responsibility, or example-only concern.
- [x] Remove any remaining core-side calls that should already be covered by compatibility layers.

### Logging And Example Runtime Separation

- [x] Decide whether the repository needs a formal logging adapter or whether logging remains example-only for now.
- [x] Ensure core headers and core source files do not rely on `Serial` or example-oriented debug printing.
- [x] If example helper headers need shared logging utilities, keep them under `examples/` or another explicitly non-core location.

### Documentation And Follow-On Work

- [x] Document the concrete clock seam so later backlog phases can assume it exists.
- [x] Record any follow-on requirements for timeout fixture metadata in the future HTTP pipeline test backlog.

## Owner

TBD

## Priority

High

## References

- `src/compat/Clock.h`
- `src/pipeline/HttpPipeline.cpp`
- `src/core/HttpTimeouts.h`
- `src/util/HttpUtility.cpp`
