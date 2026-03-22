# Arduino dependencies: (excluding filesystem, transport, stream)

This note inventories Arduino runtime and core helpers used by the project that are outside the three tracked migration areas (`FS`/`File`, transport, and stream). It explains how pervasive each dependency is, and the planned mitigation/seam for removing or adapting it.

Note: `String`, `Stream`, and filesystem/transport topics are covered in their own plans; this document intentionally excludes them and focuses on the remaining Arduino-specific runtime/utility usages.

## Summary (high-level)
- Most usages of these Arduino runtime helpers appear in examples and sketches (sketch-oriented glue). The core library no longer calls `millis()` directly in the pipeline and no longer uses `F()`/PROGMEM literals in `HttpUtility.cpp`; the only remaining direct runtime helper observed in `src/` is `millis()` inside the Arduino branch of the clock compatibility layer.
- Primary categories observed: timing/clock, logging/Serial, PROGMEM/`F()` macro, hardware IO (pins/digital/analog), randomness, and a small set of miscellaneous helpers.

## Findings / Pervasiveness
- Timing/clock APIs (`millis()`, `delay()`): pervasive in examples and example loops. Example evidence: [examples/pico-httpserveradvanced.ino](examples/pico-httpserveradvanced.ino#L80-L107), [examples/1_GettingStarted/HelloWorld/HelloWorld.ino](examples/1_GettingStarted/HelloWorld/HelloWorld.ino#L19-L62). Core parsing/timeouts now flow through `src/compat/Clock.h` and are injected into `HttpPipeline` via `HttpServerBase`; the remaining direct `millis()` call in `src/` lives inside the Arduino backend of that compatibility seam.
- Serial / logging (`Serial.begin`, `Serial.print`, `Serial.println`, `Serial.`): ubiquitous in examples and used for run-time feedback in sketches. Examples: [examples/1_GettingStarted/MultipleRoutes/MultipleRoutes.ino](examples/1_GettingStarted/MultipleRoutes/MultipleRoutes.ino#L16-L64).
- Shared example helper logging should remain under `examples/` rather than being promoted into `src/`; `examples/WifiSetup.h` now routes its shared serial and delay behavior through `examples/ExampleRuntime.h` to keep that boundary explicit.
- PROGMEM / `F()` macro: the former core-code usage in `src/util/HttpUtility.cpp` has now been replaced with portable compile-time literals. Remaining `F()` or PROGMEM work, if any, should be treated as adapter or example scope unless new core usages appear.
- Hardware I/O (`pinMode`, `digitalWrite`, `analogRead` etc.): present in examples that drive LEDs and GPIO and in example route handlers that expose GPIO control. Examples: [examples/2_RequestData/UrlParameters/UrlParameters.ino](examples/2_RequestData/UrlParameters/UrlParameters.ino#L32-L111).
- Randomness (`random()`, `randomSeed()`): occasionally used in examples or platform-specific helpers; not pervasive in core.
- Yield/low-level concurrency helpers (`yield()`): not widely observed in core; may appear in examples or platform-specific code paths.
- Other core-only or near-core helpers: small amounts of Arduino-only convenience usage appear in example helpers and a few util functions; TLS/HTTPS support is already scoped for removal.

## Mitigation / How we'll address each category
- Timing / Clock
  - Use the existing `Clock` compatibility seam in `src/compat/Clock.h`. Under `ARDUINO` it maps to `millis()` semantics; in non-Arduino builds it uses `std::chrono` equivalents. Core timeout and activity tracking should consume injected `Clock` instances rather than calling runtime helpers directly. Examples keep calling `delay()` directly until adapter migration.
- Serial / Logging
  - Keep logging example-only for now rather than introducing a core logging seam prematurely. If multiple example helpers share serial or timing behavior, keep those wrappers under `examples/` so the boundary stays explicit and no `Serial` dependency leaks into `src/`.
- PROGMEM / `F()` macro
  - Prefer plain compile-time literals such as `constexpr const char *` in core code. Reserve any wrapper macro/function for adapter-only or clearly justified memory-sensitive paths; do not reintroduce `F()` directly into core utilities.
- Hardware I/O (pins)
  - Move any pin/LED/gpio control out of core into example-only adapters. Introduce a tiny `Gpio` helper in examples that maps to Arduino `pinMode`/`digitalWrite` under `ARDUINO` and to test/no-op in host builds. Core library will not call `pinMode`/`digitalWrite` directly.
- Randomness
  - Introduce a `RandomProvider` shim. Under Arduino it maps to `random()`/`randomSeed()`; otherwise use `std::mt19937`-backed provider. Replace direct `random()` calls in examples and any core uses with the shim.
- Yield / Concurrency Helpers
  - Feature-detect uses of `yield()`; if present inside core loops, replace with `Clock::yield()` adapter which is a no-op in most host builds and maps to `yield()` on Arduino where helpful.

## Acceptance criteria (how we'll verify)
- No direct `Serial.*` or `millis()` calls in core headers (core/. and util/.), only adapter shims.
- `F()`/PROGMEM usage is limited to adapter layers or replaced with portable wrappers in the core (no AVR-only includes in core headers).
- Examples continue to function unchanged during migration; adapters provide the Arduino behavior until examples are migrated to explicit adapters.
- A short test or native build shows the core compiling without `Arduino.h` when the `ARDUINO` define is absent.

## Next steps / actions
1. Add `Clock` and `ILogger` shims in `src/compat/` and update `core/HttpTimeouts.*` to use `Clock` (Phase 3 immediate task).
2. Decide whether logging remains example-only for now or whether a formal adapter is worth adding.
3. Move example-only hardware IO into `examples/` helper headers that call through adapters.
4. Extend native fixture work so timeout-path tests drive `HttpServerBase::setClock(...)` with `ManualClock`.

If you'd like, I can take the next Phase 3 slice into example/runtime logging cleanup or start the timeout-focused native fixture work that now has an injectable clock seam.
