Changelog: 2026-03-30 — updated by GitHub Copilot: validated Arduino builds on rpipicow and esp8266; removed deprecated WiFiServer usage
Changelog: 2026-03-30 — updated by GitHub Copilot: implemented static transport traits, migrated native/Arduino factories, and validated native tests
Changelog: 2026-03-30 — created by GitHub Copilot

# Refactor: ITransportFactory → SFINAE + static methods

Summary

This refactor replaces the runtime-polymorphic `ITransportFactory` pattern with a compile-time, SFINAE-based interface using static methods and traits. The goal is to remove virtual dispatch and reduce RAM overhead on constrained targets while keeping a clear migration path for existing transport implementations.

Status

- Implemented: `TransportTraits.h` now defines the static-factory detection rules plus the legacy adapter shim.
- Implemented: native socket and Arduino WiFi transport factories now expose static `createServer`, `createClient`, and `createPeer` entry points.
- Implemented: `ArduinoWiFiTransport` no longer hides its implementation adapters in the `.cpp`; the concrete adapter types now live in the header.
- Verified: `tools/run_native_tests.ps1` passes (`202/202`).
- Verified: `pio run -e rpipicow` and `pio run -e esp8266` both succeed after the transport refactor.
- Note: ESP8266 emits upstream Python `SyntaxWarning` messages from the framework's `elf2bin.py`, but the build itself succeeds and no transport warnings remain.

Goal / Acceptance Criteria

- Provide a small, well-documented `transport_traits` header that describes the required static API and SFINAE helpers.
- Update or replace `ITransportFactory` so that consumers can rely on static construction/creation methods detected via SFINAE.
- All existing transport factories compile and function after conversion (native tests pass).
- Arduino/PlatformIO builds remain compatible; provide migration notes for downstream consumers.

Tasks

- [x] Design SFINAE/spec: define required static methods and trait detection rules.
- [x] Create `src/httpadv/v1/transport/TransportTraits.h` (SFINAE utilities + examples).
- [x] Add a new `ITransportFactory` migration header (inline shim) to gradually support both APIs.
- [x] Implement refactor on a single transport (prototype) and update tests.
- [x] Convert remaining transport factories to the static API.
- [x] Update builder/consumer call sites to use the static construction pattern or shims.
- [x] Run native tests and fix regressions.
- [x] Run Arduino-target verification against a sketch/example build.
- [x] Add migration documentation and sample changes for consumers.
- [x] Refactor `ArduinoWifiTransport` implementation to expose implementation classes in headers (stop hiding implementation types inside the `.cpp`). Ensure the exposed types satisfy `transport_traits` detection and provide an incremental adaptor for existing consumers.

Owner: TBD

Priority: Medium

References

- Static traits + adapter: [src/httpadv/v1/transport/TransportTraits.h](src/httpadv/v1/transport/TransportTraits.h)
- Migration include: [src/httpadv/v1/transport/ITransportFactory.h](src/httpadv/v1/transport/ITransportFactory.h)
- Native validation: [test/test_native/test_transport_native.cpp](test/test_native/test_transport_native.cpp)
- Server builder usages: search for `ITransportFactory` and factory call sites in `src/`
- Notes: see plans/websocket-support-plan.md and plans/externalize-request-owned-pipeline-state-implementation-plan.md for related design constraints.

Arduino-specific notes

- Problem: `ArduinoWifiTransport` currently hides its concrete implementation classes inside the translation unit (.cpp), which prevents compile-time detection (SFINAE) and static-construction patterns from working.
- Implemented solution: moved the adapter classes and static factory methods into [src/httpadv/v1/platform/arduino/ArduinoWiFiTransport.h](src/httpadv/v1/platform/arduino/ArduinoWiFiTransport.h), with the `.cpp` reduced to an include-only translation unit.
- Acceptance met: PlatformIO Arduino targets compile (`rpipicow`, `esp8266`); `transport_traits` detection works for `ArduinoWifiTransport`; native regression coverage remains green.
