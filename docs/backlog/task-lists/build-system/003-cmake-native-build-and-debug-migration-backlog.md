2026-04-10 - Copilot: created backlog for migrating native build and debug workflows from PlatformIO to CMake with MSVC-on-Windows, GCC-on-Linux, and direct library inclusion support for downstream CMake applications.

# CMake Native Build And Debug Migration Backlog

Purpose: define and sequence the work needed to replace the repository's native PlatformIO build and debug workflow with a CMake-native workflow for host development.
The target host matrix is MSVC on Windows and GCC on Linux; embedded targets are explicitly out of scope for this backlog, and the repository must be consumable as a CMake library by a final application via `add_subdirectory(...)` and target linking.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Implementation Status

Current status: not started.

Current repository state:

- native host build and test flow is owned by `platformio.ini`
- Windows-native runtime and linker behavior is patched through `scripts/native_build.py`
- consolidated native test execution currently shells through `tools/run_native_tests.ps1`
- the repository does not yet expose a top-level `CMakeLists.txt` for the HTTP library
- vendored `llhttp` already carries its own CMake entrypoint under `src/llhttp/CMakeLists.txt`

What remains open is the full host-development cutover: top-level project configuration, library target definition, dependency wiring, test integration, debugger-friendly build presets, and removal or de-prioritization of PlatformIO-native host workflows once CMake parity is proven.

## Design Intent

This backlog treats CMake migration as a host-development and consumer-integration improvement, not as an embedded-platform migration:

- CMake becomes the canonical native build and debug entrypoint for this repository
- Windows host development uses MSVC as the supported compiler toolchain
- Linux host development uses GCC as the supported compiler toolchain
- embedded and Arduino-focused build targets remain outside this backlog
- downstream applications must be able to include this repository as a library through normal CMake composition instead of custom local build glue
- test and debug workflows should be expressible through standard CMake generators, presets, and test commands rather than PlatformIO-native assumptions

## Scope

- Top-level CMake project and target graph for the HTTP library and native tests
- Native compiler/toolchain support for MSVC on Windows and GCC on Linux
- Build configuration needed to consume vendored `llhttp` from the repository's CMake graph
- Native test execution through CTest or equivalent CMake-native test integration
- Debuggable developer workflows via CMake-friendly build types, symbols, and launch configuration inputs
- Downstream library consumption through `add_subdirectory(...)` and stable target-link usage

## Non-Goals

- Do not add embedded, Arduino, Pico, or PlatformIO board-target support in this backlog
- Do not preserve PlatformIO as the canonical native host workflow once CMake parity is complete
- Do not introduce compiler support promises beyond MSVC on Windows and GCC on Linux for the initial cut
- Do not redesign library APIs solely to satisfy the build-system migration
- Do not require installation/package-manager/export workflows before direct source-tree inclusion works cleanly

## Architectural Rules

- Treat CMake as the source of truth for native host build, test, and debug configuration.
- Make the repository consumable as a normal CMake subproject with explicit library targets and public include paths.
- Keep host-specific compiler/linker conditionals isolated in CMake rather than ad hoc helper scripts where possible.
- Keep vendored third-party build wiring explicit and local to the repository.
- Preserve C++17 requirements and current public include structure.
- Keep embedded-target assumptions out of the first native CMake cut.

## Migration Milestones

These milestones define the execution gates for the native build-system cutover. A milestone is complete only when all mapped tasks are complete and its exit criteria are met.

| Milestone | Status | Goal | Mapped Tasks | Exit Criteria |
|---|---|---|---|---|
| M1 - Native CMake Skeleton | todo | Introduce a top-level CMake project that configures the library and vendored dependencies on supported host platforms | CMAKE-01, CMAKE-02, CMAKE-03, CMAKE-04 | A clean configure/generate step succeeds on Windows and Linux with the intended host compilers |
| M2 - Consumer Library Surface | todo | Expose the HTTP library as a stable CMake target that downstream applications can include and link | CMAKE-05, CMAKE-06, CMAKE-07 | A downstream sample or fixture can link the library through `add_subdirectory(...)` without custom include-path hacks |
| M3 - Native Test And Debug Parity | todo | Replace PlatformIO-native host test/debug assumptions with CMake-native equivalents | CMAKE-08, CMAKE-09, CMAKE-10, CMAKE-11 | Native tests build and run through the CMake graph, and debug-friendly configurations exist for supported hosts |
| M4 - Workflow Cutover And Cleanup | todo | Update docs, tasks, and validation so native development defaults to CMake rather than PlatformIO | CMAKE-12, CMAKE-13, CMAKE-14, CMAKE-15 | Developer guidance and automation point at CMake-native workflows, with old host-only PlatformIO glue retired or explicitly downgraded |

## Work Phases

## Phase 1 - Baseline Inventory And Build Contract

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| CMAKE-01 | todo | Inventory the current native PlatformIO build inputs, compile definitions, include paths, libraries, and host-specific patches that must be represented in CMake | `platformio.ini`, `scripts/native_build.py` | none | A complete migration inventory exists for native-only requirements, including compile definitions, include roots, linked system libraries, and runtime assumptions |
| CMAKE-02 | todo | Define the canonical top-level CMake project structure, minimum CMake version, and target naming strategy for the library and tests | Backlog | CMAKE-01 | The repository has an agreed native CMake graph design, including the primary library target name and test target organization |
| CMAKE-03 | todo | Define the supported host matrix as MSVC on Windows and GCC on Linux, with explicit out-of-scope status for embedded targets and unsupported host compilers | Backlog | CMAKE-02 | Supported and unsupported configurations are stated unambiguously in docs and backlog planning |
| CMAKE-04 | todo | Define how vendored `llhttp` is consumed from the repository CMake graph and whether it remains an internal implementation detail or a visible dependency target | `src/llhttp/CMakeLists.txt` | CMAKE-02 | `llhttp` integration strategy is explicit and does not rely on PlatformIO-only behavior |

## Phase 2 - Top-Level CMake Build Introduction

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| CMAKE-05 | todo | Add the top-level `CMakeLists.txt` and supporting CMake modules needed to configure the HTTP library from the repository root | Backlog | CMAKE-02, CMAKE-04 | The repository configures from the root with a first-class library target and no PlatformIO dependency |
| CMAKE-06 | todo | Model the library's public and private include directories, compile features, compile definitions, and source lists in target-based CMake form | Backlog | CMAKE-01, CMAKE-05 | Consumers inherit only the required public include and compile surface through target linking |
| CMAKE-07 | todo | Add a downstream-consumer validation path proving a final application can include this repository via `add_subdirectory(...)` and link the exposed target cleanly | Backlog | CMAKE-05, CMAKE-06 | A consumer fixture or documented validation sample builds without manual include-path or source-file injection |

## Phase 3 - Host Compiler And Linker Parity

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| CMAKE-08 | todo | Encode MSVC-specific Windows requirements in CMake, including any needed Winsock linkage, runtime flags, warning policy, and debug symbol behavior | `scripts/native_build.py` | CMAKE-05, CMAKE-06 | Windows builds succeed under MSVC without relying on PlatformIO-native scripting |
| CMAKE-09 | todo | Encode GCC-specific Linux native requirements in CMake, including warnings, POSIX/system library linkage, and debug-friendly defaults | Backlog | CMAKE-05, CMAKE-06 | Linux builds succeed under GCC with explicit host requirements modeled in CMake |
| CMAKE-10 | todo | Resolve host-compiler differences in source/configuration cleanly so the same library target builds under both supported host compilers | Backlog | CMAKE-08, CMAKE-09 | Native host builds pass on both supported platforms without compiler-specific ad hoc source duplication |

## Phase 4 - Native Tests, Tooling, And Debug Workflow

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| CMAKE-11 | todo | Replace PlatformIO-native host test execution with CMake-native test targets and CTest orchestration for the maintained native suite | `tools/run_native_tests.ps1`, `test/test_native/` | CMAKE-06, CMAKE-10 | Native tests build and run from the CMake graph using standard CMake test commands |
| CMAKE-12 | todo | Add CMake presets or equivalent checked-in configuration for common native configure/build/test/debug flows on supported hosts | Backlog | CMAKE-08, CMAKE-09, CMAKE-11 | Developers can configure and build standard debug and release host variants without bespoke local command recipes |
| CMAKE-13 | todo | Update VS Code tasks, launch/debug assumptions, and developer documentation to prefer the CMake-native host workflow | Backlog | CMAKE-11, CMAKE-12 | Editor-integrated build and debug flows point at CMake rather than PlatformIO for native work |

## Phase 5 - Cutover, Validation, And Cleanup

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| CMAKE-14 | todo | Add validation coverage that proves the library target, consumer-inclusion path, and native test targets remain healthy across supported host platforms | Backlog | CMAKE-07, CMAKE-10, CMAKE-11 | The repo has repeatable validation steps for Windows/MSVC and Linux/GCC host workflows |
| CMAKE-15 | todo | Retire, narrow, or clearly demote PlatformIO-native host build scripts and documentation once CMake has functional parity for native development | `platformio.ini`, `scripts/native_build.py`, `tools/run_native_tests.ps1` | CMAKE-11, CMAKE-12, CMAKE-13, CMAKE-14 | Native host guidance no longer depends on PlatformIO, and remaining PlatformIO content is explicitly limited to out-of-scope embedded follow-up work |

## Sequencing Notes

- Start by inventorying the exact PlatformIO-native behavior before introducing the top-level CMake graph.
- Establish the library target and consumer-inclusion story before optimizing editor/debug ergonomics.
- Prefer target-based CMake modeling over global compile flags and path mutation.
- Validate Windows/MSVC and Linux/GCC explicitly; do not assume parity from one host.
- Keep embedded-target reintroduction as a separate future backlog rather than muddying the native migration scope.

## Verified Progress

- `platformio.ini` defines the current native host build as a PlatformIO `native` environment with C++17, direct `src/` include roots, vendored `llhttp` include usage, and native test integration
- `scripts/native_build.py` captures current host-specific linker/runtime behavior that must be either reproduced or intentionally eliminated in the CMake migration
- `tools/run_native_tests.ps1` shows that native test orchestration currently assumes PlatformIO CLI availability
- `src/llhttp/CMakeLists.txt` confirms that vendored `llhttp` already has a CMake-aware build surface that can be integrated into a top-level native graph

## Source References

- `platformio.ini`
- `scripts/native_build.py`
- `tools/run_native_tests.ps1`
- `src/llhttp/CMakeLists.txt`
- `src/lumalink/http/LumaLinkHttp.h`
- `test/test_native/test_main.cpp`
- `test/support/include/ConsolidatedNativeSuite.h`