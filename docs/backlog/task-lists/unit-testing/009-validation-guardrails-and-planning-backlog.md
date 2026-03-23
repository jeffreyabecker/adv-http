2026-03-23 - Copilot: created detailed Phase 9 validation guardrails and planning backlog.

# Unit Testing Phase 9 Validation Guardrails And Planning Backlog

## Summary

This phase keeps the unit-testing program maintainable as coverage expands. The earlier phases add new fixtures, new production-source admissions, and more behavior locked into the native lane, which creates a need for lightweight guardrails around scope, documentation, and update rules. The goal is to keep the socket-free test strategy coherent and visible while preventing the native lane from drifting into ad hoc integration behavior.

## Goal / Acceptance Criteria

- The repository has explicit rules for how socket-free unit tests are added and maintained.
- The native lane scope, fixture expectations, and source-admission rules are documented.
- The `000` backlog remains a live index of progress across the detailed phase files.

## Tasks

### Validation Commands And Scope

- [ ] Document the canonical native test commands and when each should be used.
- [ ] Document what counts as in-scope unit coverage versus out-of-scope socket or board integration testing.
- [ ] Decide whether any lightweight compile-only guardrails should accompany future unit-test work.

### Curated Source Admission Rules

- [ ] Define the rule for when new production `.cpp` files are added to `test/test_native/native_portable_sources.cpp`.
- [ ] Document how to handle feature-gated sources such as JSON-enabled paths in the native lane.
- [ ] Record any modules that are intentionally excluded until seam work or refactors are complete.

### Backlog And Documentation Maintenance

- [ ] Keep `000-unit-test-areas.md` current as detailed phases progress.
- [ ] Add brief progress notes to the detailed phase backlogs when major milestones land.
- [ ] Decide whether the `test/` area needs a dedicated README once parser and pipeline fixtures are added.

### Regression Review Discipline

- [ ] Distinguish bugs from legacy behaviors before freezing them into tests.
- [ ] Note any temporary compatibility behaviors that should be revisited after the first full unit-test pass.
- [ ] Revisit phase priorities after parser, handler, and pipeline coverage begins to land.

## Owner

TBD

## Priority

Medium

## References

- `docs/backlog/task-lists/unit-testing/000-unit-test-areas.md`
- `test/test_native/`
- `test/test_native/native_portable_sources.cpp`
- `tools/run_native_tests.ps1`
- `platformio/native.ini`