Changelog: 2026-03-30 — created by GitHub Copilot

# Refactor: ITransportFactory → SFINAE + static methods

Summary

This refactor replaces the runtime-polymorphic `ITransportFactory` pattern with a compile-time, SFINAE-based interface using static methods and traits. The goal is to remove virtual dispatch and reduce RAM overhead on constrained targets while keeping a clear migration path for existing transport implementations.

Goal / Acceptance Criteria

- Provide a small, well-documented `transport_traits` header that describes the required static API and SFINAE helpers.
- Update or replace `ITransportFactory` so that consumers can rely on static construction/creation methods detected via SFINAE.
- All existing transport factories compile and function after conversion (native tests pass).
- Arduino/PlatformIO builds remain compatible; provide migration notes for downstream consumers.

Tasks

- [ ] Design SFINAE/spec: define required static methods and trait detection rules.
- [ ] Create `src/httpadv/v1/transport/transport_traits.h` (SFINAE utilities + examples).
- [ ] Add a new `ITransportFactory` migration header (inline shim) to gradually support both APIs.
- [ ] Implement refactor on a single transport (prototype) and update tests.
- [ ] Convert remaining transport factories to the static API.
- [ ] Update builder/consumer call sites to use the static construction pattern or shims.
- [ ] Run native tests and Arduino verification; fix regressions.
- [ ] Add migration documentation and sample changes for consumers.
 - [ ] Refactor `ArduinoWifiTransport` implementation to expose implementation classes in headers (stop hiding implementation types inside the .cpp). Ensure the exposed types satisfy `transport_traits` detection and provide an incremental adaptor for existing consumers.

Owner: TBD

Priority: Medium

References

- Existing factory (example): [src/httpadv/v1/transport/ITransportFactory.h](src/httpadv/v1/transport/ITransportFactory.h)
- Server builder usages: search for `ITransportFactory` and factory call sites in `src/`
- Notes: see plans/websocket-support-plan.md and plans/externalize-request-owned-pipeline-state-implementation-plan.md for related design constraints.

Arduino-specific notes

- Problem: `ArduinoWifiTransport` currently hides its concrete implementation classes inside the translation unit (.cpp), which prevents compile-time detection (SFINAE) and static-construction patterns from working.
- Proposed solution: move the minimal implementation-type declarations (or an `Impl` header) into a public header, and provide a thin inline adaptor that implements the required static methods (constructor/create) and forwards to the platform-specific implementation.
- Acceptance: Arduino example builds and PlatformIO/Arduino targets compile; `transport_traits` detection works for `ArduinoWifiTransport`; no runtime regressions.
