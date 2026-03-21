2026-03-21 - Copilot: created detailed Phase 1 build and validation backlog.
2026-03-21 - Copilot: marked Phase 1 build and validation backlog tasks complete per user direction.

# No-Arduino Phase 1 Build And Validation Backlog

## Summary

This phase hardens the build and validation entrypoints before deeper refactors begin. The current repository already has a top-level `platformio.ini`, board-specific PlatformIO env files, Arduino CLI tasks, and a consolidated native test lane, but the configuration still assumes globally enabled JSON support and does not yet make the eventual core-versus-adapter boundary explicit in build settings. The goal of this phase is to make the build matrix predictable, document what each lane proves, and remove configuration assumptions that would hide portability regressions.

## Goal / Acceptance Criteria

- The repository has a documented and reproducible build matrix for RP2040, ESP32, ESP8266, and native validation.
- PlatformIO and Arduino CLI entrypoints are aligned with the migration plan and do not hide feature assumptions.
- JSON enablement is no longer treated as an unconditional global default for all future build paths.
- The native lane remains green and has an explicit purpose relative to the board lanes.

## Tasks

### Build Matrix Inventory

- [x] Audit `platformio.ini` and confirm which environments are part of the supported migration matrix.
- [x] Audit `platformio/rp2040.ini`, `platformio/esp32.ini`, and `platformio/esp8266.ini` for board names, packages, and framework assumptions that matter to no-Arduino work.
- [x] Audit `platformio/native.ini` and document exactly what the native environment currently compiles and what it intentionally does not compile.
- [x] Audit `examples/arduino-cli.yaml` and the VS Code Arduino tasks to confirm which board profile is treated as the primary example validation lane.
- [x] Decide which lanes are mandatory for every migration PR and which lanes are periodic or release-only checks.

### PlatformIO Configuration Cleanup

- [x] Move shared build flags in `platformio/common.ini` into clearly named groups for core portability, Arduino compatibility, and optional features.
- [x] Review `build_src_filter` usage so the library build path and the noop-main build path are both intentional and documented.
- [x] Add comments to `platformio/common.ini` explaining why `platformio/noop/main.cpp` is present and when it can be removed.
- [x] Verify that include paths needed for `llhttp` and `src/` are scoped to the minimum necessary environments.
- [x] Check whether `lib_ldf_mode = deep+` is still required once optional features are gated more cleanly.

### JSON Feature Gating Baseline

- [x] Replace the assumption that `HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON=1` must always be set from `platformio/common.ini`.
- [x] Decide whether JSON should be opt-in by default, opt-out by default, or enabled only on Arduino board envs during migration.
- [x] Add configuration comments documenting how JSON-enabled and JSON-disabled builds are expected to behave.
- [x] Ensure the native environment has a clearly documented policy for JSON support, even if JSON source files are not yet host-safe.
- [x] Define the final build assertion for this phase: at minimum one lane with JSON enabled and one lane prepared to run without unconditional JSON assumptions.

### Native Lane Tightening

- [x] Document the purpose of `test/test_native/native_portable_sources.cpp` as the authoritative curated source list for host-compiled production code.
- [x] Add comments or README guidance stating that new native suites must not include production `.cpp` files directly outside the curated source unit.
- [x] Verify that `test/test_native/test_main.cpp` remains the only native Unity runner.
- [x] Confirm whether `test/test_native/test_native_smoke.cpp` should remain a distinct smoke suite within the consolidated runner or be folded into broader suite organization later.
- [x] Decide what additional production sources can safely be admitted during Phase 1 without dragging in Arduino headers.

### Build Commands And Developer Workflow

- [x] Document the canonical PlatformIO commands for each required environment.
- [x] Document the canonical Arduino CLI commands already exposed by the repo tasks.
- [x] Verify that the top-level default environment in `platformio.ini` matches the intended primary Arduino validation target.
- [x] Add short notes describing what a successful run of each lane proves and what it does not prove.
- [x] Ensure the backlog and docs use the same names for the board environments and test lanes.

### CI Readiness

- [x] Decide the minimum CI matrix for the migration period: likely native plus one primary Arduino board lane.
- [x] Identify which board environments are too slow or flaky for per-PR validation and should be scheduled less frequently.
- [x] Add a checklist for future CI implementation covering cache use, dependency installation, and build command selection.
- [x] Define the failure policy for portability regressions, including direct `Arduino.h` leakage into the intended core lane once later phases add guardrails.

### Documentation Updates

- [x] Add a short build-validation section to the no-Arduino docs or backlog references describing the current matrix.
- [x] Update any stale comments that imply the native lane or PlatformIO support still needs to be created from scratch.
- [x] Record the resolved JSON build policy so Phase 7 can build on it instead of reopening the decision.

## Owner

TBD

## Priority

High

## References

- `platformio.ini`
- `platformio/common.ini`
- `platformio/native.ini`
- `platformio/rp2040.ini`
- `platformio/esp32.ini`
- `platformio/esp8266.ini`
- `platformio/noop/main.cpp`
- `examples/arduino-cli.yaml`
- `test/test_native/test_main.cpp`
- `test/test_native/native_portable_sources.cpp`
