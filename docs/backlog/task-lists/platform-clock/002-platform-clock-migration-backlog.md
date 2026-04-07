# 002 — Migrate to `lumalink-platform` Clock Abstraction

**Status:** Ready  
**Priority:** Medium  
**Owner:** —

---

## Summary

The library currently owns its own clock abstraction in
`src/lumalink/http/util/Clock.h` — a `Clock` interface (`nowMillis()`),
`SystemClock`, `ManualClock`, and a `DefaultClock()` factory.  The
`lumalink-platform` dependency (already in `lib_deps`) now ships a richer
monotonic clock layer under `lumalink::platform::time` (`IMonotonicClock`,
`MonotonicMillis`, `ManualClock`, `ClockFactory.h`).

This item removes the built-in clock entirely and migrates all call sites —
production code and tests — to the platform types, so that the library no
longer maintains a duplicate clock interface.

---

## Goal / Acceptance Criteria

- `src/lumalink/http/util/Clock.h` is deleted.
- All production code that previously referenced `lumalink::http::util::Clock`,
  `ClockMillis`, `SystemClock`, `ManualClock`, or `DefaultClock()` uses the
  corresponding `lumalink::platform::time` / `lumalink::platform` types instead.
- The pipeline timeout logic and `IConnectionSession`/`IProtocolExecution`
  `handle()` signatures use `const lumalink::platform::time::IMonotonicClock &`.
- The default clock in `HttpServerBase` is a `lumalink::platform::Clock`
  instance (from `ClockFactory.h`) rather than the removed `SystemClock`.
- Tests that used `lumalink::http::util::ManualClock` use
  `lumalink::platform::time::ManualClock` and its updated API.
- The project compiles cleanly and all native tests pass.

---

## Tasks

### Production Code

- [ ] Delete `src/lumalink/http/util/Clock.h`.
- [ ] Update `src/lumalink/http/core/HttpTimeouts.h`:
  - Remove `#include "../util/Clock.h"` and `using ClockMillis`.
  - Change the three timeout fields and their getters/setters from `ClockMillis`
    to `lumalink::platform::time::MonotonicMillis`.
  - Update default field values to use aggregate init (`{PIPELINE_*_MS}`).
- [ ] Update `src/lumalink/http/core/Defines.h`:
  - Timeout constants (`PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS`, etc.) remain
    `uint32_t` scalars; no change needed here.  The consuming code (`HttpTimeouts`
    and `HttpPipeline`) converts to `MonotonicMillis` at the boundary.
- [ ] Update `src/lumalink/http/pipeline/ConnectionSession.h`:
  - Replace `#include "../util/Clock.h"` with platform header.
  - Change `handle(IClient&, const Clock&)` to
    `handle(IClient&, const lumalink::platform::time::IMonotonicClock&)`.
- [ ] Update `src/lumalink/http/pipeline/IProtocolExecution.h`:
  - Same include and signature change as `ConnectionSession.h`.
- [ ] Update `src/lumalink/http/pipeline/HttpProtocolExecution.h/cpp`:
  - Update `handle()` declaration and definition signature.
- [ ] Update `src/lumalink/http/pipeline/HttpPipeline.h`:
  - Replace `#include "../util/Clock.h"` with platform header.
  - Change `const Clock &clock_` member, `ClockMillis lastActivityMillis_` and
    `startMillis_` fields, and `currentMillis()` return type to platform types.
  - Update the constructor parameter type.
- [ ] Update `src/lumalink/http/pipeline/HttpPipeline.cpp`:
  - Update `currentMillis()` to call `clock_.monotonicNow()` (returns
    `MonotonicMillis`).
  - Update all local `ClockMillis` variables to `MonotonicMillis`.
  - Where `MonotonicMillis` is passed to `IClient::setTimeout(uint32_t)`, add a
    `static_cast<uint32_t>` on `.value`.
- [ ] Update `src/lumalink/http/server/HttpServerBase.h`:
  - Replace `#include "../util/Clock.h"` / `using Clock` with platform types.
  - Change `setClock(const Clock&)`, `clock()`, and the `clock_` member to use
    `const lumalink::platform::time::IMonotonicClock`.
- [ ] Update `src/lumalink/http/server/HttpServerBase.cpp`:
  - Add `#include <lumalink/platform/ClockFactory.h>` (or equivalent path).
  - Replace `DefaultClock()` default with a `static const lumalink::platform::Clock`.
- [ ] Update `src/lumalink/http/websocket/WebSocketProtocolExecution.h`:
  - Change `handle()` signature to use `IMonotonicClock`.
- [ ] Update `src/lumalink/http/websocket/WebSocketProtocolExecution.cpp`:
  - Update definition signature to match.

### Tests

- [ ] Update `test/test_native/test_clock.cpp`:
  - Replace include with `<lumalink/platform/time/ManualClock.h>`.
  - Update test logic: `monotonicNow().value` instead of `nowMillis()`,
    `setMonotonicMillis()` instead of `setNowMillis()`, constructor no longer
    accepts an initial value (call `setMonotonicMillis()` after default
    construction).
- [ ] Update `test/test_native/test_pipeline.cpp`:
  - Replace `#include "../../src/lumalink/http/util/Clock.h"` with platform header.
  - Update `ManualClock` namespace/usage; replace `ManualClock clock(1000)` with
    default construction + `setMonotonicMillis(1000)`.
  - Stub `handle(IClient&, const IMonotonicClock&)` implementations.
  - Update `clock.advanceMillis(x)` calls (name unchanged, argument type is now
    `uint64_t`).
- [ ] Update `test/test_native/test_http_context.cpp`:
  - Same `ManualClock` construction change (`ManualClock clock(1000)` →
    default-construct + `setMonotonicMillis(1000)`).
- [ ] Update `test/README.md`:
  - Fix the reference to `src/compat/Clock.h` and the `setClock(...)` example to
    reflect the new platform type and header.

---

## References

- Platform clock headers: `C:/ode/lumalink-platform/src/lumalink/platform/time/`
- Platform clock factory: `C:/ode/lumalink-platform/src/lumalink/platform/ClockFactory.h`
- Built-in clock being removed: `src/lumalink/http/util/Clock.h`
