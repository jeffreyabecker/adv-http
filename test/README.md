# Native Test Conventions

This repository keeps host-side unit coverage under a single consolidated native lane in `test/test_native/`. The lane is intentionally socket-free and is the primary place for parser, request, response, routing, and fixture-driven transport tests.

## Runner Rules

- `test/test_native/test_main.cpp` is the only native `main()` entrypoint in the repository.
- Suites expose `run_test_*()` functions and execute through `HttpServerAdvanced::TestSupport::RunConsolidatedSuite(...)` from `test/support/include/ConsolidatedNativeSuite.h`.
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

- Add a production `.cpp` file to `test/test_native/native_portable_sources.cpp` when a new native test depends on that translation unit and the production source is host-safe.
- Do not include production `.cpp` files directly from individual suite files.
- If a production source is not yet host-safe, keep the test blocked at the backlog level rather than hiding the dependency with ad hoc local includes.

## Request-Driving Strategy

- Default to lower-level pipeline callbacks plus shared recorders for parser and transport tests.
- Use `HttpRequest` directly only when the behavior under test is request lifecycle logic, handler dispatch timing, item storage, or response dispatch.
- This keeps parser or transport tests narrow while still allowing request-model tests to exercise the real request object when needed.

## Timeout Guidance

- Timeout-oriented native tests should use `Compat::ManualClock` from `src/compat/Clock.h`.
- Advance time explicitly in the test body; do not use sleeps or wall-clock delays.
- When a server-facing test needs deterministic time, inject the manual clock through `HttpServerBase::setClock(...)` before exercising the code under test.

## Commands

- Primary native test command: `./tools/run_native_tests.ps1`
- Direct PlatformIO equivalent: `pio test -e native`