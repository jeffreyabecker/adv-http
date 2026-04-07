2026-04-07 - Copilot: cleaned remaining legacy namespace-closing markers in active source files and revalidated the native suite.
2026-04-07 - Copilot: started Phase 4 namespace cutover in source and test code, removed the legacy namespace shim header, and marked namespace migration tasks in progress.
2026-04-07 - Copilot: expanded cutover plan with additional implementation phases between migration and validation.
2026-04-07 - Copilot: added explicit in-repository namespace cutover tasks for this library.
2026-04-07 - Copilot: added explicit library cutover milestone phases with task mapping and exit criteria.
2026-04-07 - Copilot: refocused remaining scope on full library rename execution across metadata, namespaces, includes, docs, and tests.
2026-04-07 - Copilot: reinforced no-compatibility policy across all phases, rules, and sequencing notes.
2026-04-07 - Copilot: reformatted this backlog to the standard phase-table template used by other task lists.
2026-04-06 - Copilot: pinned the new platform project location to c:\ode\lumalink-platform.
2026-04-06 - Copilot: made the implementation sequence explicit by starting with copying platform code into the new project before dependency integration and local removal.
2026-04-06 - Copilot: documented the contract-versus-implementation split without changing source code.
2026-04-06 - Copilot: kept platform selection as a compile-time mechanism, owned by the platform library instead of the HTTP umbrella header.
2026-04-06 - Copilot: assigned adapter-specific test coverage to the new platform library.
2026-04-06 - Copilot: assigned TransportFactory, file adapters, and path-mapping helpers to the new platform library.
2026-04-06 - Copilot: completed the Phase 2 rename-surface inventory and documented the target include-root structure.
2026-04-06 - Copilot: moved versioning out of the public namespace and into package metadata only.
2026-04-06 - Copilot: recorded the platform dependency direction and narrowed the platform boundary to buffers, transport, and filesystem.
2026-04-06 - Copilot: created backlog for the lumalink HTTP rename and platform-library extraction.

# LumaLink HTTP Rename And Platform Extraction Backlog

Purpose: define and sequence the work needed to rename the library from `HttpServerAdvanced` / `httpadv::v1` to `lumalink::http` while extracting platform-specific transport, filesystem, and host-integration code into a separate `lumalink::platform` project at `c:\ode\lumalink-platform`.
The migration is a direct cutover: do not add compatibility layers, alias namespaces, shim headers, wrapper APIs, or backward-compatibility fixes.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Implementation Status

Current status: platform extraction is complete; library rename execution is underway, with active source/test namespace cutover completed and package/include rename work still pending.

Completed so far:

- namespace and ownership direction are defined as `lumalink::http` and `lumalink::platform`
- the platform extraction boundary is narrowed to buffers, transport, and filesystem concerns
- rename and platform exposure inventories are documented in `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md`
- platform contract-vs-implementation split and compile-time platform-selection ownership are documented
- platform integration through `lumalink/platform/...` usage is documented

What remains open is implementation and enforcement of the library rename: package metadata, public headers, include roots, namespaces, macros/constants, docs/examples, and validation surfaces still need to converge on the final `lumalink` naming.
All remaining implementation work must use direct replacement to new names and ownership with no compatibility bridges.

## Design Intent

This backlog treats rename and platform extraction as one architectural change:

- `lumalink::http` owns HTTP semantics, parsing, routing, response behavior, and pipeline policy
- `lumalink::platform` owns platform adapters and host-integration concerns
- the dependency direction is one-way: HTTP depends on platform contracts, platform does not depend on HTTP-domain code
- public API versioning moves to package/release metadata rather than a public namespace version segment
- all rename and extraction steps are hard-cut changes; no compatibility aliases, wrappers, or shims are allowed
- the remaining milestone is the HTTP library rename cutover, not additional platform-library decomposition

## Scope

- Package identity and namespace rename to `lumalink::http`
- Public include-root and umbrella-surface rename
- Extraction of platform adapters and related helpers into `lumalink::platform`
- Build, test, and validation boundaries that enforce the split
- Consumer migration guidance for Arduino, PlatformIO, and native users
- Direct migration of references to new namespacing without legacy bridge code

## Non-Goals

- Do not keep long-lived compatibility wrappers, alias namespaces, or shim headers by default
- Do not add short-lived compatibility wrappers, alias namespaces, or shim headers either
- Do not keep platform implementations permanently in this repository after dependency cutover
- Do not mix HTTP-core policy back into the platform package boundary
- Do not treat this as two unrelated migrations with independent release semantics
- Do not add backward-compatibility fixes for old include paths, old namespace segments, or old umbrella names

## Architectural Rules

- Keep HTTP protocol behavior in `lumalink::http`.
- Keep platform concerns in `lumalink::platform`.
- Enforce one-way dependency direction from HTTP to platform contracts.
- Keep public C++ namespace unversioned (`lumalink::http`), with versioning in package metadata.
- Perform migration sequencing by validating the external platform project first, then switching this repository.
- Do not introduce compatibility layers, aliases, shims, wrapper APIs, or fallback include paths at any stage.
- Update call sites and includes directly to the new namespace and include structure; do not wire temporary bridge code.

## Library Cutover Milestones

These milestones define the rename execution gates for the library cutover. A milestone is complete only when all mapped tasks are complete and its exit criteria are met.

| Milestone | Status | Goal | Mapped Tasks | Exit Criteria |
|---|---|---|---|---|
| M1 - Identity Lock | todo | Finalize package identity, naming, and release-cut posture for the rename | LUMA-01, LUMA-16, LUMA-20 | Package naming and breaking-change policy are final with explicit no-compatibility commitment |
| M2 - Namespace Cutover | doing | Replace legacy public namespace usage inside this repository with final `lumalink::http` namespace surface | LUMA-17, LUMA-26, LUMA-27, LUMA-28 | Public namespace and alias surfaces in this library are fully renamed with no compatibility aliases |
| M3 - Include Surface Cutover | todo | Replace legacy include-root and umbrella-header usage with final `lumalink/http/...` include structure | LUMA-18, LUMA-21 | Legacy include roots and umbrella references fail validation; final include structure is canonical |
| M4 - Consumer Cutover | todo | Publish downstream migration updates for Arduino, PlatformIO, and native consumers | LUMA-19 | Consumer docs/examples and dependency guidance are updated for direct rename adoption |
| M5 - Validation And Cleanup | todo | Prove renamed surfaces across native/embedded validation and remove residual legacy artifacts | LUMA-22, LUMA-23, LUMA-24, LUMA-25, LUMA-40 | Tests and compile validation pass on renamed surfaces; legacy names and residual paths are removed |

## Work Phases

## Phase 1 - Name, Ownership, And Boundary Decisions

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| LUMA-01 | todo | Confirm canonical branding and spelling for target libraries, including whether `lumalink::platform` is the final namespace and package name | Backlog | none | Naming and spelling decisions are explicitly finalized before implementation starts, with explicit no-compatibility policy |
| LUMA-02 | done | Use `lumalink::http` and `lumalink::platform` as public namespaces, with versioning moved to package/release metadata only | Backlog | LUMA-01 | Public namespace policy is documented without public C++ version segments and without alias namespace bridges |
| LUMA-03 | done | Use a separate repository for `lumalink::platform` at `c:\ode\lumalink-platform` rather than subtree/submodule/monorepo split | Backlog | LUMA-01 | Platform project location and repository ownership are fixed |
| LUMA-04 | done | Define dependency direction and ownership rules so HTTP depends on platform contracts and platform does not depend on HTTP-domain code | Backlog | LUMA-02, LUMA-03 | Dependency and ownership rules are written and unambiguous, including prohibition on compatibility glue across package boundaries |
| LUMA-05 | done | Freeze the platform boundary so HTTP-core owns protocol behavior and platform exposes only buffers, transport, and filesystem concerns | Backlog | LUMA-04 | Platform boundary statement is documented and accepted, with no compatibility-owned cross-layer adapters |

## Phase 2 - Surface Inventory And Rename Scope

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| LUMA-06 | done | Inventory all public rename surfaces across metadata, headers, include roots, docs, examples, tests, and URLs | `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md` | LUMA-02 | Inventory lists every public-facing legacy name surface to change directly, without compatibility wrappers |
| LUMA-07 | done | Inventory all namespace declarations and aliases rooted in `httpadv`, including obsolete comments/markers | `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md` | LUMA-06 | Namespace inventory is complete and traceable |
| LUMA-08 | done | Identify all public include paths and define target `lumalink/http/...` include structure | `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md` | LUMA-06 | Include migration target structure is fully documented with no legacy include fallback plan |
| LUMA-09 | done | Identify all user-facing symbols/macros/constants/metadata still encoding `HttpServerAdvanced` or `httpadv` names | `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md` | LUMA-06 | Rename-sensitive symbols are cataloged for direct replacement, not aliasing |
| LUMA-10 | done | Record all HTTP umbrella exposures of platform code, including `TransportFactory`, adapters, and platform-selection defaults | `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md` | LUMA-06 | Platform exposure map is complete and actionable |

## Phase 3 - Platform Library Extraction Design

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| LUMA-11 | done | Define the minimal platform contract consumed by HTTP as buffers, transport, and filesystem only | Backlog | LUMA-05 | Contract boundary is documented and constrained with no compatibility extension surface |
| LUMA-12 | done | Document interface/trait vs implementation split so HTTP consumes only the required contract surface | Backlog | LUMA-11 | Split is documented without code migration yet and without temporary compatibility abstractions |
| LUMA-13 | done | Move `TransportFactory`, file adapters, and path-mapping helpers into `lumalink::platform` as platform-owned decomposition | Backlog | LUMA-12 | Ownership target is documented for migration implementation |
| LUMA-14 | done | Place Arduino, POSIX, Windows, and in-memory/native adapters and adapter-specific tests in `lumalink::platform` | Backlog | LUMA-12, LUMA-13 | Adapter ownership and test ownership are assigned to platform |
| LUMA-15 | done | Keep platform selection compile-time but move selection ownership into `lumalink::platform` | Backlog | LUMA-11, LUMA-12 | HTTP no longer owns platform-branching policy in umbrella surface and no legacy selection compatibility hook is introduced |

## Phase 4 - Migration Sequencing And Consumer Impact

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| LUMA-16 | todo | Rename package metadata and repository-facing identity from `HttpServerAdvanced` to final `lumalink` naming across `library.properties`, `library.json`, and related package fields | Backlog | LUMA-06, LUMA-09 | Package metadata publishes only final names with no legacy-name compatibility fields |
| LUMA-17 | doing | Rename public C++ namespace usage in this repository from `httpadv::v1` and unversioned `httpadv` aliases to the final `lumalink::http` surface | Backlog | LUMA-07, LUMA-16 | Public namespace and aliases in this library are replaced directly with no compatibility namespace bridges |
| LUMA-26 | done | Rename namespace declarations and namespace-closing markers in this repository across `src/`, `test/`, and public headers from `httpadv::v1`/`httpadv` to `lumalink::http` | Backlog | LUMA-17 | All in-repo namespace declarations and comments reflect final namespace with no retained legacy namespace blocks |
| LUMA-27 | doing | Remove legacy namespace wrapper/alias surfaces in this repository, including unversioned alias exports and `src/httpadv/namespace.h` compatibility role | Backlog | LUMA-17, LUMA-26 | This library no longer exports legacy namespace aliases or wrapper namespace headers |
| LUMA-28 | doing | Update all in-repo call sites, tests, docs, and examples to use `lumalink::http` names directly and prohibit `httpadv` token reuse | Backlog | LUMA-26, LUMA-27 | No remaining in-repo usage of old namespace tokens in maintained code/docs except historical changelog references |
| LUMA-18 | todo | Execute include-root and umbrella-header rename to final `lumalink/http/...` structure, including removal or replacement of `HttpServerAdvanced.h` | Backlog | LUMA-08, LUMA-16, LUMA-17 | Public include story is final and contains no legacy include fallback headers |
| LUMA-19 | todo | Update downstream Arduino, PlatformIO, and native consumer guidance/examples for the renamed package, include, and namespace surfaces | Backlog | LUMA-16, LUMA-17, LUMA-18 | Consumer migration guidance is complete and requires direct updates without compatibility patches |
| LUMA-20 | todo | Capture release/versioning/changelog expectations for the breaking library rename across package name, include paths, and namespaces | Backlog | LUMA-17, LUMA-18, LUMA-19, LUMA-28 | Release notes define a clean rename cut with no backwards-compatibility fixes |

## Phase 5 - Namespace Migration Execution Waves

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| LUMA-29 | todo | Build a subsystem-by-subsystem namespace cutover execution matrix for this repository (`core`, `routing`, `pipeline`, `response`, `server`, `staticfiles`, `websocket`, `handlers`, `util`, `streams`) | Backlog | LUMA-17 | Namespace cutover order, file ownership, and risk points are tracked for each subsystem |
| LUMA-30 | todo | Execute namespace rename wave 1 across foundational headers and sources (`core`, `transport`, `pipeline`, `routing`) from `httpadv::v1` to `lumalink::http` | Backlog | LUMA-26, LUMA-29 | Wave 1 compiles and no public API in these subsystems exposes legacy namespace tokens |
| LUMA-31 | todo | Execute namespace rename wave 2 across higher-level subsystems (`handlers`, `response`, `server`, `staticfiles`, `websocket`, `util`, `streams`) | Backlog | LUMA-30 | Wave 2 compiles and no public API in these subsystems exposes legacy namespace tokens |
| LUMA-32 | todo | Execute namespace rename wave 3 across tests, fixtures, and test-support utilities to consume `lumalink::http` directly | Backlog | LUMA-31, LUMA-28 | All maintained tests and fixtures compile with final namespace usage only |
| LUMA-33 | todo | Add guard checks that fail the build when new `httpadv::` namespace declarations/usages are introduced outside historical changelog/backlog text | Backlog | LUMA-30, LUMA-31, LUMA-32 | CI/local validation fails on reintroduction of legacy namespace tokens |

## Phase 6 - Include Graph And Header-Surface Cutover

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| LUMA-34 | todo | Implement final public header graph under `lumalink/http/...` and move/rename public entrypoints to that structure | Backlog | LUMA-18, LUMA-30 | New public header layout is canonical and self-consistent without legacy include bridges |
| LUMA-35 | todo | Rewrite internal include directives across `src/` and `test/` to the final header graph and namespace-aligned include paths | Backlog | LUMA-34, LUMA-31 | Internal includes no longer depend on legacy `httpadv/v1/...` include paths |
| LUMA-36 | todo | Remove legacy public umbrella and namespace wrapper entrypoints (`HttpServerAdvanced.h`, legacy `httpadv` wrapper roles) once replacements are active | Backlog | LUMA-34, LUMA-35, LUMA-27 | Legacy public entrypoints are removed rather than retained as compatibility shims |
| LUMA-37 | todo | Verify HTTP headers consume platform contracts without re-exporting or mirroring platform implementation headers under HTTP include roots | Backlog | LUMA-34, LUMA-35 | HTTP include surface remains cleanly separated from platform implementation ownership |

## Phase 7 - Tooling, Documentation, And Release-Cut Readiness

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| LUMA-38 | todo | Rewrite repository docs and examples for final package, include, and namespace naming (`lumalink::http`) | Backlog | LUMA-31, LUMA-35 | Primary docs/examples contain final naming and no legacy guidance |
| LUMA-39 | todo | Update build/test/tooling configuration that references legacy names (tasks, scripts, native test harness assumptions, metadata checks) | Backlog | LUMA-35 | Tooling executes against final naming without compatibility glue |
| LUMA-40 | todo | Add repository-wide static checks for legacy identifiers (`HttpServerAdvanced`, `httpadv::`, `httpadv/v1/`) excluding approved historical docs/changelog paths | Backlog | LUMA-33, LUMA-36, LUMA-39 | Automated checks prevent legacy token regressions in active code and docs |
| LUMA-41 | todo | Prepare release-cut package notes and migration callouts for a strict breaking rename with no backward-compatibility fallback | Backlog | LUMA-20, LUMA-38, LUMA-39 | Release-readiness artifacts describe direct migration only and forbid compatibility pathways |

## Phase 8 - Validation, Enforcement, And Cleanup

| ID | Status | Task | Source | Depends On | Definition of Done |
|---|---|---|---|---|---|
| LUMA-21 | todo | Add validation that fails when legacy include roots (`httpadv/...`) or legacy public umbrellas are used after rename cutover | Backlog | LUMA-36, LUMA-40 | Guardrails enforce final include naming and reject compatibility include pathways |
| LUMA-22 | todo | Add or update native tests exercising renamed namespace/package/include surfaces through the platform boundary | Backlog | LUMA-32, LUMA-36, LUMA-40 | Native coverage verifies final names with no alias/shim pathways |
| LUMA-23 | todo | Add supported embedded compile validation proving renamed HTTP surfaces integrate correctly with `lumalink::platform` across intended targets | Backlog | LUMA-37, LUMA-39 | Embedded compile matrix validates renamed integration boundary |
| LUMA-24 | todo | Remove obsolete legacy names, stale docs, superseded metadata, and any residual in-repo legacy platform paths after completion | Backlog | LUMA-21, LUMA-22, LUMA-23, LUMA-41 | Legacy rename artifacts are fully cleaned up, including accidental compatibility hooks |
| LUMA-25 | todo | Close follow-up backlog items only when metadata, include paths, namespaces, tests, and platform extraction all match final architecture | Backlog | LUMA-24 | All closure criteria are satisfied consistently with zero compatibility layers, aliases, wrappers, or shims |

## Sequencing Notes

- Treat platform extraction as complete context and focus execution on rename cutover of library identity and public API surfaces.
- Update package metadata, namespace surfaces inside this repository, include roots, and docs/examples in one coherent rename sequence.
- Keep rename and platform boundary rules aligned as one release-managed architectural cut.
- Do not introduce compatibility layers, aliases, wrappers, or shims at any step.
- Do not add backwards-compatibility fixes for legacy namespacing or include roots.
- Update all references directly to `lumalink::http`, `lumalink::platform`, and the final include structure.

## Verified Progress

- `lumalink::http` and `lumalink::platform` are selected as the public namespace targets
- versioning policy is moved out of public C++ namespace segments into package/release metadata
- platform repository location is fixed to `c:\ode\lumalink-platform`
- dependency direction and platform-boundary ownership are documented and integrated
- Phase 2 rename-surface and platform-exposure inventories are complete in `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md`
- platform contract/implementation split and compile-time platform-selection ownership are documented
- versioned `httpadv::v1` namespace usage in active `src/` and `test/` code has been replaced with `lumalink::http`
- legacy `src/httpadv/namespace.h` wrapper participation has been removed from the umbrella surface and the shim header has been deleted
- remaining legacy `namespace HttpServerAdvanced` closing markers have been removed from active source files
- native validation still passes after namespace-marker cleanup (`192/192` native tests)
- remaining execution scope is centered on the HTTP library rename and public-surface cleanup

## Source References

- `library.properties`
- `library.json`
- `keywords.txt`
- `src/HttpServerAdvanced.h`
- `src/httpadv/namespace.h`
- `src/httpadv/v1/HttpServerAdvanced.h`
- `src/httpadv/v1/transport/TransportTraits.h`
- `src/httpadv/v1/transport/TransportInterfaces.h`
- `src/httpadv/v1/platform/TransportFactory.h`
- `src/httpadv/v1/platform/arduino/ArduinoWiFiTransport.h`
- `src/httpadv/v1/platform/arduino/ArduinoFileAdapter.h`
- `src/httpadv/v1/platform/posix/PosixSocketTransport.h`
- `src/httpadv/v1/platform/posix/PosixFileAdapter.h`
- `src/httpadv/v1/platform/windows/WindowsSocketTransport.h`
- `src/httpadv/v1/platform/windows/WindowsFileAdapter.h`
- `src/httpadv/v1/platform/memory/MemoryFileAdapter.h`
- `test/test_native/test_transport_native.cpp`
- `test/test_native/test_filesystem_posix.cpp`
- `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md`
