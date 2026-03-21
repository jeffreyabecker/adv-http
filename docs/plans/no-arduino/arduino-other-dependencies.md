# Arduino dependencies: (excluding filesystem, transport, stream)

This note inventories Arduino runtime and core helpers used by the project that are outside the three tracked migration areas (`FS`/`File`, transport, and stream). It explains how pervasive each dependency is, and the planned mitigation/seam for removing or adapting it.

Note: `String`, `Stream`, and filesystem/transport topics are covered in their own plans; this document intentionally excludes them and focuses on the remaining Arduino-specific runtime/utility usages.

## Summary (high-level)
- Most usages of these Arduino runtime helpers appear in examples and sketches (sketch-oriented glue). Core library code has a handful of remaining uses (notably `F()`/PROGMEM usage in `util` and some `String`-based helpers) that should be addressed during Phase 3.
- Primary categories observed: timing/clock, logging/Serial, PROGMEM/`F()` macro, hardware IO (pins/digital/analog), randomness, and a small set of miscellaneous helpers.

## Findings / Pervasiveness
- Timing/clock APIs (`millis()`, `delay()`): pervasive in examples and example loops. Example evidence: [examples/pico-httpserveradvanced.ino](examples/pico-httpserveradvanced.ino#L80-L107), [examples/1_GettingStarted/HelloWorld/HelloWorld.ino](examples/1_GettingStarted/HelloWorld/HelloWorld.ino#L19-L62). Core parsing/timeouts are already tracked in `core/HttpTimeouts.h` and will be routed to an abstract clock provider.
- Serial / logging (`Serial.begin`, `Serial.print`, `Serial.println`, `Serial.`): ubiquitous in examples and used for run-time feedback in sketches. Examples: [examples/1_GettingStarted/MultipleRoutes/MultipleRoutes.ino](examples/1_GettingStarted/MultipleRoutes/MultipleRoutes.ino#L16-L64).
- PROGMEM / `F()` macro: used in `src/util/HttpUtility.cpp` for HTML-escape string literals (saves RAM on Arduino). This is a core-code usage that must be handled rather than left in examples: [src/util/HttpUtility.cpp](src/util/HttpUtility.cpp#L101-L171).
- Hardware I/O (`pinMode`, `digitalWrite`, `analogRead` etc.): present in examples that drive LEDs and GPIO and in example route handlers that expose GPIO control. Examples: [examples/2_RequestData/UrlParameters/UrlParameters.ino](examples/2_RequestData/UrlParameters/UrlParameters.ino#L32-L111).
- Randomness (`random()`, `randomSeed()`): occasionally used in examples or platform-specific helpers; not pervasive in core.
- Yield/low-level concurrency helpers (`yield()`): not widely observed in core; may appear in examples or platform-specific code paths.
- Other core-only or near-core helpers: small amounts of Arduino-only convenience usage appear in example helpers and a few util functions; TLS/HTTPS support is already scoped for removal.

## Mitigation / How we'll address each category
- Timing / Clock
  - Introduce a `Clock`/`ITimeSource` compatibility shim in the core. Under `ARDUINO` it aliases to `millis()`/`delay()` semantics; in non-Arduino builds it uses `std::chrono` equivalents. Replace direct calls in `core/HttpTimeouts.*` and other core files with the shim. Examples keep calling `delay()` directly until adapter migration.
- Serial / Logging
  - Add a minimal `ILogger` / `Log` adapter used only by examples and optional diagnostics. Under `ARDUINO` the default adapter maps to `Serial` methods; for core-native builds the default is a no-op or `std::cout` logger for tests. Remove `Serial` usage from core; keep it in example code.
- PROGMEM / `F()` macro
  - Provide a small `HTTP_S` or `CORE_PROGMEM` wrapper macro/function that expands to `F()` when `ARDUINO` is defined and to plain string literals otherwise. For `util/HttpUtility.*`, migrate to `const char *` literal usage combined with the wrapper so core can compile without `avr/pgmspace` headers. Long-term: prefer storing permanent compile-time strings as `constexpr` `char[]` or `const char *` and use the wrapper only where memory savings are critical.
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
2. Replace `F()` usages in `src/util/HttpUtility.*` with the wrapper macro and migrate long-term to `constexpr` string constants.
3. Move example-only hardware IO into `examples/` helper headers that call through adapters.
4. Run a native build to verify no `Arduino.h` in core translation units.

If you'd like, I can implement the `Clock` and `ILogger` shims and update `HttpTimeouts` as a follow-up PR. Which shim should I create first?
