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

## Summary

This backlog defines the phased work to rename the library from `HttpServerAdvanced` / `httpadv::v1` to a `lumalink::http` identity and to move platform-specific transport, filesystem, and host-integration abstractions into a separate `lumalink::platform` library or codebase created at `c:\ode\lumalink-platform`. Today, the public package metadata, umbrella header names, include roots, and C++ namespaces still reflect the legacy library name, while the platform adapters live under `src/httpadv/v1/platform` and are selected directly from the HTTP package.

The migration should treat the rename and platform split as one architectural change instead of two unrelated edits. The HTTP package should own HTTP semantics, routing, response generation, parsing, and pipeline policy, while platform-specific socket, filesystem, peer, and path-mapping concerns move behind a platform library boundary with a clearly defined dependency direction.

The implementation sequence should start by copying the current platform code into the new `lumalink::platform` project at `c:\ode\lumalink-platform`, updating namespaces and ownership there, and getting that project's adapter-specific tests passing before the HTTP repository begins consuming the new library. Only after the external platform project builds and validates cleanly should this repository add the dependency, switch references over to it, and remove the in-repo platform code.

This item is intentionally phase-oriented. Follow-up backlog items should break each phase into implementation-sized work once the target names, package boundaries, and release sequencing are confirmed.

## Goal / Acceptance Criteria

- The canonical package and namespace names are finalized before implementation starts, including confirmation of the intended `lumalink` spelling across `lumalink::http` and `lumalink::platform`.
- The HTTP library publishes one coherent identity across repository metadata, package manifests, top-level headers, include roots, documentation, examples, tests, and C++ namespaces.
- Public HTTP APIs live under `lumalink::http` without a public version segment in the namespace; versioning is carried in package and release metadata instead of the C++ surface.
- Platform-specific abstractions and concrete adapters are owned by a separate `lumalink::platform` library or codebase at `c:\ode\lumalink-platform`, with a clear contract consumed by the HTTP library.
- The HTTP library no longer directly owns Arduino, POSIX, Windows, or in-memory platform adapter implementations.
- The `lumalink::platform` public contract exposed to HTTP is limited to buffers, transport, and filesystem concerns.
- The migration begins by copying the existing platform code into the new platform project at `c:\ode\lumalink-platform`, renaming namespaces there, and making the platform project's tests pass before this repository removes local platform code.
- Include paths, build manifests, and dependency graphs make the platform split explicit and prevent HTTP-core code from drifting back into platform-specific ownership.
- Native tests and supported embedded compile validation cover the renamed surfaces and the new platform-library integration boundary.
- The migration plan does not rely on long-lived deprecated wrapper namespaces, shim headers, or compatibility aliases unless a temporary bridge is explicitly approved for release-management reasons.

## Tasks

### Phase 1: Name, Ownership, And Boundary Decisions

- [ ] Confirm the canonical branding and spelling for the target libraries, including whether `lumalink::platform` is the intended final namespace and package name.
- [x] Use `lumalink::http` and `lumalink::platform` as the public namespaces, with versioning moved to package and release metadata only rather than a public C++ namespace segment.
- [x] Use a separate repository for `lumalink::platform` located at `c:\ode\lumalink-platform`, rather than a subtree, submodule, or same-monorepo package split.
- [x] Define the dependency direction and ownership rules so HTTP depends on platform contracts, while platform code does not depend on HTTP pipeline, routing, or HTTP-domain code.
- [x] Freeze the platform boundary so HTTP-core owns protocol behavior and `lumalink::platform` exposes only buffers, transport, and filesystem concerns.

### Phase 2: Surface Inventory And Rename Scope

- [x] Inventory all public rename surfaces, including `library.properties`, `library.json`, top-level umbrella headers, include roots under `src/httpadv`, documentation, examples, tests, and repository URLs, in `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md`.
- [x] Inventory every namespace declaration and alias rooted in `httpadv`, including unversioned convenience aliases and any comments or closing-namespace markers that still use obsolete names, in `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md`.
- [x] Identify all public include paths that consumers are expected to include directly and define their target `lumalink/http/...` structure in `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md`.
- [x] Identify all macros, configuration constants, generated metadata, and user-facing symbols that still encode `HttpServerAdvanced` or `httpadv` naming in `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md`.
- [x] Record every place where platform code is exposed through the HTTP umbrella surface, including `TransportFactory`, adapter headers, and compile-time platform selection defaults, in `docs/httpserveradvanced/LUMALINK_RENAME_SURFACE_INVENTORY.md`.

### Phase 3: Platform Library Extraction Design

- [x] Define the minimal platform contract that HTTP consumes as buffers, transport, and filesystem only.
- [x] Document the split between pure interfaces/traits and concrete implementations so only the contract required by HTTP remains in the HTTP-owned dependency surface, without changing code yet.
- [x] Move `TransportFactory`, file adapters, and path-mapping helpers into `lumalink::platform` after the split, with any further decomposition handled inside the platform library rather than the HTTP package.
- [x] Place Arduino, POSIX, Windows, and in-memory/native test adapters in `lumalink::platform`, and move adapter-specific tests with them so platform validation lives in the new library rather than the HTTP package.
- [x] Keep platform selection as a compile-time mechanism, but move that selection into `lumalink::platform` so `lumalink::http` consumes the selected platform contract without owning platform-specific compile-time branching in its umbrella header.

### Phase 4: Migration Sequencing And Consumer Impact

- [ ] Execute the migration order as: copy the platform code into the new `lumalink::platform` project at `c:\ode\lumalink-platform`, update namespaces there, make that project's tests pass, add the platform library as a dependency here, then remove in-repo platform code and update references to use the library.
- [ ] Decide whether the migration is a single breaking cut or a short staged transition, and document the exact conditions under which any temporary bridge surface would be allowed.
- [ ] Define the target top-level include story for consumers, including whether `HttpServerAdvanced.h` is removed outright or replaced by a new canonical umbrella header.
- [ ] Document the required downstream changes for Arduino, PlatformIO, and native consumers, including dependency declarations for the new platform package and the point at which this repository stops shipping local platform implementations.
- [ ] Capture release, versioning, and changelog expectations for a breaking rename that changes both include paths and namespaces.

### Phase 5: Validation, Enforcement, And Cleanup

- [ ] Add validation that fails when HTTP-core code starts including platform-specific adapter headers directly after the split.
- [ ] Add or update native tests that exercise the renamed namespace and package surfaces through the new platform boundary.
- [ ] Add supported embedded compile validation that proves `lumalink::http` integrates correctly with the extracted platform package across intended targets.
- [ ] Remove obsolete legacy names, stale docs, superseded build metadata, and temporary migration notes once the rename is complete.
- [ ] Close follow-up backlog items only after package metadata, include paths, namespaces, tests, and platform extraction all reflect the same final architecture.

## Owner

TBD

## Priority

High

## References

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
