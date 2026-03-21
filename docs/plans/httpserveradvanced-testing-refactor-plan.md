# HttpServerAdvanced Testing Refactor Plan

## Objective

Refactor the native test layout so host-side unit tests compile under one consolidated PlatformIO test target, then add a deterministic strategy for HTTP pipeline tests driven by canned request and response fixtures stored on disk.

This plan has two explicit goals:

- consolidate the current native unit suites under a single compilation target modeled after `C:\ode\pled\test\test_native`
- define a fixture-driven testing strategy for the HTTP pipeline using serialized HTTP messages and file-backed expectations

## Current State Summary

The current native test setup is still in an early, per-suite form:

- `test/test_stream_utilities/test_main.cpp` owns its own Unity `main()`
- `test/test_filesystem_compat/test_main.cpp` owns its own Unity `main()`
- `test/test_native_smoke/test_main.cpp` owns its own Unity `main()`
- `platformio/native.ini` uses `test_build_src = no`, so tests currently compile only what each suite pulls in directly
- `test/test_stream_utilities/test_main.cpp` works around that by including production `.cpp` files directly
- there is no shared runner, no shared suite registration pattern, and no fixture convention for HTTP request or response traffic

That layout is workable for a small number of tests, but it scales poorly:

- each top-level test directory becomes its own PlatformIO compilation target
- source-under-test selection is duplicated across suites
- helper infrastructure is harder to share cleanly
- there is no obvious place to add broader host-side protocol tests for the request pipeline

## Reference Model From `C:\ode\pled\test`

The reference repo already demonstrates the target shape worth copying.

Key characteristics:

- one consolidated native test lane lives under a single top-level suite directory
- `test_main.cpp` is only a dispatcher; individual test files expose `run_test_*()` entry points instead of owning `main()`
- suite-local `setUp()` and `tearDown()` are routed through a shared support header
- one optional native smoke lane remains separate when it serves a distinct purpose

For this repository, the important part is not the exact file names. The important part is the compilation model:

- one top-level native unit-test directory
- one Unity executable
- many suite implementation files linked into that executable
- shared support code for hook routing and common fixtures

## Target Layout

Adopt the following target layout under `test/`:

- `test/test_native/`
  - consolidated host-side unit and integration lane
- `test/test_smoke_native/`
  - optional separate smoke lane, only if it remains materially different from the consolidated lane
- `test/support/include/`
  - shared support headers such as the consolidated Unity hook router
- `test/support/http_pipeline/`
  - fake client, fixture loader, fixture assertions, and other shared pipeline helpers
- `test/support/fixtures/http_pipeline/`
  - canned request and response fixtures stored as files

Planned migration of current suites:

- move `test/test_stream_utilities/test_main.cpp` into `test/test_native/` as a normal suite source file exposing `run_test_stream_utilities()`
- move `test/test_filesystem_compat/test_main.cpp` into `test/test_native/` as a normal suite source file exposing `run_test_filesystem_compat()`
- keep `test/test_native_smoke/` separate only if it continues to be a distinct smoke lane; otherwise fold it into `test/test_native/`

## Consolidated Native Runner Design

Add a shared support header similar to the PicoLED model:

- `test/support/include/ConsolidatedNativeSuite.h`

Responsibilities:

- hold the active `setUp()` and `tearDown()` function pointers for the currently running suite
- provide a helper to run a suite with suite-local hooks
- avoid each suite having to define global Unity hooks or its own `main()`

Add a single consolidated runner:

- `test/test_native/test_main.cpp`

Responsibilities:

- declare all `run_test_*()` entry points
- enumerate suites in a stable order
- print the suite name before execution
- accumulate the total failure count and return it as the process exit code

Each suite file should follow one pattern:

- keep its test bodies local to that file
- define `localSetUp()` and `localTearDown()` if needed
- define `runUnitySuite()` that wraps `UNITY_BEGIN()`, `RUN_TEST(...)`, and `UNITY_END()`
- export one `run_test_*()` function that calls the shared support helper

That keeps the structure consistent and makes adding new native suites cheap.

## Source Compilation Strategy

The current `test_build_src = no` setting is still appropriate for now because the library is not yet fully host-buildable. The consolidation refactor should not force a premature full-native compile of all `src/` files.

Instead, use a staged source compilation strategy.

### Stage 1: Single Native Test Target, Curated Production Sources

Keep `test_build_src = no`, but stop letting individual suite files include arbitrary production `.cpp` files independently.

Introduce one curated compilation unit inside `test/test_native/`, for example:

- `test/test_native/native_portable_sources.cpp`

That file should include the portable production `.cpp` files needed by the consolidated host lane exactly once.

Benefits:

- avoids repeated direct `.cpp` inclusion across individual suites
- prevents ODR problems as more native suites are added
- creates one explicit inventory of production sources that are expected to build in the host lane

Initial contents would likely include the same stream sources currently pulled into `test/test_stream_utilities/test_main.cpp`, and later the portable pipeline/parser sources as they become host-safe.

### Stage 2: Expand the Curated Native Source Set

As compatibility work lands, extend the curated source list to cover additional subsystems:

- parser and pipeline sources
- core request and header sources
- response-stream sources that no longer depend on raw Arduino headers

### Stage 3: Consider `test_build_src = yes`

Only after the core library is genuinely host-buildable should the project consider switching the native lane to compile production sources directly through PlatformIO.

That is a later cleanup step, not a prerequisite for consolidation.

## Proposed Migration Phases

### Phase 1: Consolidate Existing Native Unit Suites

Work:

- create `test/support/include/ConsolidatedNativeSuite.h`
- create `test/test_native/test_main.cpp`
- convert the current per-suite `main()` files into `run_test_*()` files
- move or rename the existing top-level native unit suites so only one top-level consolidated native unit lane remains
- keep or rename the smoke lane separately if needed

Acceptance criteria:

- PlatformIO sees one consolidated host-side unit test target instead of one target per native unit suite
- stream utility tests still run
- filesystem compatibility tests still run
- the smoke lane remains available if intentionally preserved

### Phase 2: Centralize Host-Compiled Production Sources

Work:

- add `test/test_native/native_portable_sources.cpp`
- move existing direct `.cpp` inclusions out of individual suites into that single compilation unit
- document the rule that new native suites must not include production `.cpp` files directly unless they are added to the curated source unit

Acceptance criteria:

- no consolidated suite source file owns the source-under-test inventory by itself
- the list of host-compiled production `.cpp` files is explicit and centralized

### Phase 3: Add Shared Fixture Infrastructure

Work:

- add `test/support/http_pipeline/FixtureLoader.*` or a header-only equivalent
- add `test/support/http_pipeline/FakeClient.*`
- add `test/support/http_pipeline/PipelineEventRecorder.*`
- add `test/support/http_pipeline/ResponseAssertions.*`
- add a fixture directory layout under `test/support/fixtures/http_pipeline/`

Acceptance criteria:

- at least one suite can load request bytes and expected response bytes from files
- helper code is shared and not duplicated across tests

### Phase 4: Add Parser and Pipeline Fixture Tests

Work:

- add fixture-driven tests for parser-level behavior
- add fixture-driven tests for transport-level `HttpPipeline` behavior using a fake client and canned response stream
- add negative cases for invalid syntax, oversized headers, timeouts, and disconnects

Acceptance criteria:

- canned fixture tests cover successful request parsing, malformed requests, and error responses or error events
- test data lives in files, not embedded string blobs in test code

## HTTP Pipeline Testing Strategy

The HTTP testing strategy should be explicitly layered. Trying to jump directly to full request-routing end-to-end tests in the first step will couple the work to unrelated decoupling tasks.

### Layer 1: Request Parser Fixture Tests

Target:

- `RequestParser`
- a test-only `IPipelineHandler` recorder

What to verify:

- method, version, and URL recognition
- header parsing order and values
- body chunk delivery
- message completion
- parser error mapping for malformed or oversized input

Why this layer matters:

- it validates raw request parsing without any fake socket behavior
- it is the cheapest path to fixture-driven coverage for serialized `.http` requests

### Layer 2: `HttpPipeline` Fixture Tests With Fake Client

Target:

- `HttpPipeline`
- fake `IClient`
- test-only `IPipelineHandler` that records events and optionally supplies a canned response stream

What to verify:

- read loop behavior across chunked input delivery
- response bytes written back to the client
- completion states such as `Completed`, `Aborted`, `TimedOutUnrecoverably`, and `ErroredUnrecoverably`
- keep-alive behavior across parser state
- disconnect and partial-write handling

Why this layer matters:

- it exercises the pipeline transport loop directly
- it remains independent of routing, handler factories, and the higher response stack

### Layer 3: Full Request-to-Response Tests Through `HttpRequest`

Target:

- `HttpPipeline`
- `HttpRequest::createPipelineHandler(...)`
- fake handler factory and deterministic canned handlers

This layer should be treated as a later phase, not the entry point.

Current blockers in the repository:

- `HttpPipeline.cpp` still calls raw `millis()` directly, so deterministic timeout tests need the planned clock seam or an approved temporary shim
- `response/HttpResponse.h` still includes `Arduino.h`, so the full response stack is not yet a clean host-build target

That means the first fixture-driven pipeline work should stop at Layer 1 and Layer 2. Layer 3 should follow once the clock and response seams are cleaned up.

## Fixture Format Proposal

Use a directory-per-case layout so cases remain readable and easy to extend.

Example:

- `test/support/fixtures/http_pipeline/get_hello_world/`
  - `request.http`
  - `expected.response.http`
  - `expected.events.json`
  - `meta.json`

Recommended file meanings:

- `request.http`
  - raw serialized HTTP request bytes as sent by the client
- `expected.response.http`
  - raw serialized HTTP response bytes expected from the pipeline or higher layer
- `expected.events.json`
  - expected parser or pipeline events when raw byte-for-byte response comparison is not the right assertion
- `meta.json`
  - per-case execution settings such as chunk sizes, timeout expectations, or disconnect behavior

### Why raw `.http` files should be the primary artifact

Raw message files make the tests realistic and easy to inspect:

- they mirror what the parser and pipeline actually consume
- they make line-ending problems visible
- they decouple test data from C++ string escaping noise
- they are reusable across parser-level and pipeline-level suites

### Recommended metadata fields

Keep metadata minimal and execution-oriented.

Suggested fields:

- `requestChunks`: array of integers controlling how the fake client exposes request bytes
- `responseWriteLimit`: optional per-write cap to simulate partial socket writes
- `disconnectAfterBytesRead`: optional failure injection point
- `advanceMillisPerLoop`: optional deterministic time progression for timeout tests
- `expectedResult`: expected `PipelineHandleClientResult`
- `expectedErrorCode`: expected `PipelineErrorCode` when failure is part of the case

Avoid putting full expected HTTP content into JSON strings when a `.http` file can hold it directly.

## Shared HTTP Test Helpers

Add the following reusable host-test helpers.

### `FakeClient`

Implements `IClient` and supports:

- deterministic request byte input
- configurable read chunking
- capture of response bytes written by the pipeline
- partial write simulation
- disconnect simulation
- timeout value capture from `setTimeout()`

### `PipelineEventRecorder`

Implements `IPipelineHandler` and records:

- message begin data
- headers
- body chunks
- message completion
- errors
- response-start and client-disconnect notifications

It should also optionally install a canned response stream through the pipeline callback so Layer 2 tests can verify output bytes without needing the full response stack.

### `FixtureLoader`

Loads fixture files from disk into:

- request bytes
- expected response bytes
- expected event structures
- metadata options

The loader should normalize as little as possible. For `.http` files, byte-for-byte fidelity is the point.

### Assertion Helpers

Provide focused helpers for:

- byte-for-byte response comparison
- ordered event comparison
- header comparison
- pipeline result comparison

## Initial Fixture Set

The first fixture batch should cover both nominal and error cases.

Recommended initial cases:

- `get_basic`
  - simple GET request with one header and no body
- `post_with_body`
  - fixed `Content-Length` request with body delivery across multiple chunks
- `invalid_method`
  - malformed request line producing a parser error
- `header_too_large`
  - oversized header name or value
- `uri_too_long`
  - oversized request target
- `client_disconnect_mid_request`
  - fake client disconnect before request completion
- `partial_response_write`
  - fake client accepts only part of the response write
- `timeout_while_reading`
  - deterministic timeout case once the clock seam exists

## Documentation Changes

The refactor should include a small testing README under `test/`.

Recommended contents:

- the purpose of `test/test_native/`
- the purpose of any separate smoke lane
- the `run_test_*()` convention
- the rule for curated native production source inclusion
- the fixture directory layout and naming rules
- how to add a new canned HTTP case

The top-level test README can stay short. The main value is documenting the conventions so the consolidated layout does not decay back into one-off suite structures.

## Risks and Constraints

### Not All Production Code Is Host-Ready Yet

The consolidated native lane should grow only over host-safe code paths. For pipeline testing in particular:

- raw `millis()` usage in `HttpPipeline.cpp` needs a clock seam before deterministic timeout tests are clean
- `response/HttpResponse.h` still includes `Arduino.h`, which blocks the first full end-to-end response layer from being a clean host target

The plan should respect those boundaries rather than hiding them behind brittle test-only hacks.

### Avoid Reintroducing Per-Suite Source Ownership

If every suite continues to decide which `.cpp` files to include directly, the consolidation will only be cosmetic. The curated native source list is a required part of the refactor.

### Preserve a Separate Smoke Lane Only If It Has a Real Role

If `test/test_native_smoke/` is just proving that Unity runs, it can be folded into the consolidated lane. If it is intended as a minimal compile-and-run sentinel distinct from the full native suite, keep it separate and rename it clearly.

## Acceptance Criteria

This plan is complete when the repository reaches the following state:

- one consolidated host-side unit test target exists under `test/test_native/`
- existing native unit coverage runs through that single target
- source-under-test selection for the native lane is centralized
- HTTP parser and pipeline tests can load canned requests from files
- response assertions can compare either raw response bytes or recorded pipeline events, depending on the layer under test
- the testing conventions are documented under `test/` and in this plan

## Recommended Implementation Order

1. Consolidate the existing native suites and add the shared Unity hook router.
2. Centralize native production-source inclusion so the consolidated lane has one authoritative source inventory.
3. Add fixture loader and fake client support.
4. Add parser-level canned-request tests.
5. Add `HttpPipeline` transport-loop tests with canned response streams.
6. Add timeout and full request-to-response cases after the clock and response seams are host-safe.