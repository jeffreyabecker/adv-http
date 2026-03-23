2026-03-23 - Copilot: completed all validation guardrails tasks; updated test/README.md with scope, regression, and commands; updated 000 index to reflect phase completion status.
2026-03-23 - Copilot: created detailed Phase 9 validation guardrails and planning backlog.

# Unit Testing Phase 9 Validation Guardrails And Planning Backlog

## Summary

This phase keeps the unit-testing program maintainable as coverage expands. The earlier phases add new fixtures, new production-source admissions, and more behavior locked into the native lane, which creates a need for lightweight guardrails around scope, documentation, and update rules. The goal is to keep the socket-free test strategy coherent and visible while preventing the native lane from drifting into ad hoc integration behavior. Native source admission now uses `platformio/native.ini` directly rather than a translation-unit aggregation file.

## Goal / Acceptance Criteria

- The repository has explicit rules for how socket-free unit tests are added and maintained.
- The native lane scope, fixture expectations, and source-admission rules are documented.
- The `000` backlog remains a live index of progress across the detailed phase files.

## Tasks

### Validation Commands And Scope

- [x] Document the canonical native test commands and when each should be used.
- [x] Document what counts as in-scope unit coverage versus out-of-scope socket or board integration testing.
- [x] Decide whether any lightweight compile-only guardrails should accompany future unit-test work.

### Curated Source Admission Rules

- [x] Define the rule for when new production `.cpp` files are added to `test/test_native/native_portable_sources.cpp`.
- [x] Document how to handle feature-gated sources such as JSON-enabled paths in the native lane.
- [x] Record any modules that are intentionally excluded until seam work or refactors are complete.

### Backlog And Documentation Maintenance

- [x] Keep `000-unit-test-areas.md` current as detailed phases progress.
- [x] Add brief progress notes to the detailed phase backlogs when major milestones land.
- [x] Decide whether the `test/` area needs a dedicated README once parser and pipeline fixtures are added.

### Regression Review Discipline

- [x] Distinguish bugs from legacy behaviors before freezing them into tests.
- [x] Note any temporary compatibility behaviors that should be revisited after the first full unit-test pass.
- [x] Revisit phase priorities after parser, handler, and pipeline coverage begins to land.

## Owner

TBD

## Priority

Medium

## References

- `docs/backlog/task-lists/unit-testing/000-unit-test-areas.md`
- `test/README.md`
- `test/test_native/`
- `tools/run_native_tests.ps1`
- `platformio/native.ini`