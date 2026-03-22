# Test Fixtures

This directory will hold deterministic HTTP request and response fixtures for native tests.

Layout:
- `requests/` — raw request files used to drive parser fixtures
- `responses/` — expected response files used to assert pipeline outputs
- `binary/` — binary fixtures such as gzip-compressed files used to validate static-file fallback

See `docs/backlog/task-lists/no-arduino/011-native-test-consolidation-and-fixture-coverage-backlog.md` for related planning.
