2026-03-21 - Copilot: created detailed Phase 10 enforcement, documentation, and exit criteria backlog.

# No-Arduino Phase 10 Enforcement, Documentation, And Exit Criteria Backlog

## Summary

This phase turns the completed seam work into something maintainable. By the end of the migration, the repository needs automated guardrails against regression, clear documentation of the new architecture, and an explicit definition of what counts as done. The goal is to keep Arduino-specific dependencies from quietly creeping back into the platform-neutral core and to make the final library shape understandable to contributors and users.

## Goal / Acceptance Criteria

- The repository has automated or semi-automated guardrails against Arduino-only leakage into the intended core layer.
- Documentation reflects the final architecture, feature boundaries, and build or test workflow.
- The migration has an explicit exit checklist tied to code, tests, examples, and docs.
- Contributors can tell where platform-specific code belongs without rediscovering the entire refactor history.

## Tasks

### Dependency-Boundary Guardrails

- [ ] Define which directories count as platform-neutral core for guardrail purposes.
- [ ] Add searches, scripts, or build checks that fail when forbidden Arduino headers appear in those directories.
- [ ] Decide how to handle allowed compatibility-leaf headers so the guardrails do not create false positives.
- [ ] Add checks for accidental direct use of Arduino runtime helpers in core code once the clock and other seams land.

### Build And Test Enforcement

- [ ] Ensure the final CI or documented validation matrix includes at least one native lane and one primary Arduino lane.
- [ ] Tie the build matrix to the feature-gating expectations established earlier, especially JSON-enabled versus JSON-disabled assumptions.
- [ ] Ensure the native lane exercises the intended host-safe core surface rather than remaining permanently minimal.

### Documentation Finalization

- [ ] Update architecture docs to describe the final layering between core, compat, adapters, and optional features.
- [ ] Update developer docs with the final build and test commands.
- [ ] Update user-facing docs to reflect the current public API, removed secure scope, and any optional feature caveats.
- [ ] Add a short contributor note describing where new platform-specific adapters belong and where they do not belong.

### Example And README Consistency

- [ ] Verify that example code, example docs, and library docs all tell the same story about supported features.
- [ ] Remove stale example references that imply removed scope or old implicit includes.
- [ ] Ensure any remaining Arduino-only examples are clearly identified as adapter-layer usage, not core requirements.

### Exit Checklist Definition

- [ ] Define the exact code-level completion criteria for the no-Arduino migration.
- [ ] Define the exact build-matrix and test-matrix completion criteria.
- [ ] Define the exact documentation completion criteria.
- [ ] Define any acceptable temporary exceptions and how they are tracked if the migration closes before every possible cleanup lands.

## Owner

TBD

## Priority

Medium

## References

- `docs/backlog/task-lists/no-arduino/001-migration-backlog.md`
- `docs/backlog/task-lists/no-arduino/002-baseline-and-scope-backlog.md`
- `platformio.ini`
- `platformio/common.ini`
- `platformio/native.ini`
- `docs/httpserveradvanced/LIBRARY_DOCUMENTATION.md`
- `docs/httpserveradvanced/EXAMPLES.md`
- `docs/httpserveradvanced/LIBRARY_COMPARISON.md`
