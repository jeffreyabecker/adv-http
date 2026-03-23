# Support Fixture Naming

Reusable native test fixtures live under `test/support/`.

## Naming

- `*Fixtures.h`: shared fixture families used by multiple suites
- `*Recorder`: event or call-capture helpers
- `Fake*`: controllable test doubles that model library interfaces
- `Captured*`: snapshots produced from live library objects for assertions

## Placement

- Put code in `test/support/include/` when at least two suites can reasonably share it.
- Keep a helper local to a suite when it exists only to probe one narrow semantic edge case.
- Put file-backed test assets in `test/support/fixtures/` rather than embedding large serialized protocol samples in source.