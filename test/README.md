# Native Test Conventions

This repository keeps host-side unit coverage under a single consolidated native lane in `test/test_native/`. The lane is intentionally socket-free and is the primary place for parser, request, response, routing, and fixture-driven transport tests.

The native lane exists to protect the library's cross-platform design: the same HTTP application logic should stay usable in desktop-hosted tests and embedded integrations, with platform or framework adapters isolated at the edges.

## Runner Rules

- `test/test_native/test_main.cpp` is the only native `main()` entrypoint in the repository.
- Suites expose `run_test_*()` functions and execute through `lumalink::http::TestSupport::RunConsolidatedSuite(...)` from `test/support/include/ConsolidatedNativeSuite.h`.
- New suites must not define their own `main()` or bypass the consolidated runner.

## Shared Support Layout

- `test/support/include/` holds reusable fixture headers shared across multiple suites.
- `ByteStreamFixtures.h` is the shared home for scripted byte sources, recording byte channels, and byte-drain helpers.
- `HttpTestFixtures.h` is the shared home for HTTP-facing support such as request-factory recorders, response capture helpers, pipeline event recorders, fake clients, and fake servers.
- `test/support/fixtures/` holds serialized request, response, and binary fixture data when tests need file-backed assets.

## Current Helper Inventory

- Shared helpers:
  - byte-source and byte-channel fixtures now live in `ByteStreamFixtures.h`
  - request, response, transport, and pipeline fixtures now live in `HttpTestFixtures.h`
- Suite-local helpers that should remain local unless reused elsewhere:
  - highly specific semantic probes such as `NegativeAvailableReadStream` in `test_stream_utilities.cpp`
  - deterministic header bundles or case-specific builders used only by one suite
  - fake filesystem implementations that exist solely to test filesystem compatibility behavior

## Native Source Admission Rule

- The native CMake-based environment now compiles production sources directly from `src/` via the top-level `CMakeLists.txt`.
- Do not include production `.cpp` files directly from individual suite files.
- If a production source is not yet host-safe, fix the guard or dependency issue in production code rather than adding ad hoc local includes.

## Request-Driving Strategy

- Default to lower-level pipeline callbacks plus shared recorders for parser and transport tests.
- Use `HttpContext` directly only when the behavior under test is request lifecycle logic, handler dispatch timing, item storage, or response dispatch.
- This keeps parser or transport tests narrow while still allowing request-model tests to exercise the real request object when needed.

## Timeout Guidance

- Timeout-oriented native tests should use `lumalink::platform::time::ManualClock` from `<lumalink/platform/time/ManualClock.h>`.
- Advance time explicitly in the test body; do not use sleeps or wall-clock delays.
- When a server-facing test needs deterministic time, inject the manual clock through `HttpServerBase::setClock(...)` before exercising the code under test.

## In-Scope vs Out-of-Scope Coverage

**In-scope for the native lane:**

- Parser and request-lifecycle behavior driven by raw in-memory byte input.
- Response construction, header emission, body streaming, and chunk framing.
- Body handler input processing (buffering, form-field parsing, JSON, multipart, raw bytes).
- Routing, provider composition, authentication header parsing, and CORS header construction.
- Static file lookup and metadata using the portable filesystem seam and POSIX-backed or fake FS objects.
- Pipeline control flow using scripted fake `IClient` objects for partial read, partial write, disconnect, reuse, and timeout scenarios under a manual clock.
- Core HTTP value types, utility functions, and any other host-portable production logic.

**Out of scope for the native lane:**

- Real socket bind, connect, listen, or accept operations.
- Board-specific or OS-specific network stacks (Wi-Fi, lwIP, etc.).
- Flash filesystem access on embedded targets.
- Hardware or timing behavior that cannot be deterministically reproduced through a fake clock and scripted byte sources.
- End-to-end integration sequences that require a living server and a real HTTP client over a real transport.

Board-specific or integration behavior belongs in dedicated hardware test sketches or live integration setups, not in this lane.

## Regression Review Rules

- Before freezing observed behavior into a test, confirm that behavior is correct, not merely existing.
- Temporary compatibility behaviors that are known to be revisited later should be marked with a `// TODO(regression):` comment in the test so they are easy to find.
- When revisiting phase priorities after significant coverage lands, update `docs/backlog/task-lists/unit-testing/000-unit-test-areas.md` to reflect current state.

## Commands

- Primary native test command: `./tools/run_native_tests.ps1`
- Direct CMake command: `cmake --build build --config Debug --target RUN_TESTS`
- The native runner also fails fast if active repo files outside approved historical notes reintroduce `HttpServerAdvanced`, `httpadv::`, or `httpadv/v1/` tokens.