2026-04-06 - Copilot: moved versioning out of the public namespace and into package metadata only.
2026-04-06 - Copilot: recorded the platform dependency direction and narrowed the platform boundary to buffers, transport, and filesystem.
2026-04-06 - Copilot: created backlog for the lumalink HTTP rename and platform-library extraction.

# LumaLink HTTP Rename And Platform Extraction Backlog

## Summary

This backlog defines the phased work to rename the library from `HttpServerAdvanced` / `httpadv::v1` to a `lumalink::http` identity and to move platform-specific transport, filesystem, and host-integration abstractions into a separate `lumalink::platform` library or codebase. Today, the public package metadata, umbrella header names, include roots, and C++ namespaces still reflect the legacy library name, while the platform adapters live under `src/httpadv/v1/platform` and are selected directly from the HTTP package.

The migration should treat the rename and platform split as one architectural change instead of two unrelated edits. The HTTP package should own HTTP semantics, routing, response generation, parsing, and pipeline policy, while platform-specific socket, filesystem, peer, and path-mapping concerns move behind a platform library boundary with a clearly defined dependency direction.

This item is intentionally phase-oriented. Follow-up backlog items should break each phase into implementation-sized work once the target names, package boundaries, and release sequencing are confirmed.

## Goal / Acceptance Criteria

- The canonical package and namespace names are finalized before implementation starts, including confirmation of the intended `lumalink` spelling across `lumalink::http` and `lumalink::platform`.
- The HTTP library publishes one coherent identity across repository metadata, package manifests, top-level headers, include roots, documentation, examples, tests, and C++ namespaces.
- Public HTTP APIs live under `lumalink::http` without a public version segment in the namespace; versioning is carried in package and release metadata instead of the C++ surface.
- Platform-specific abstractions and concrete adapters are owned by a separate `lumalink::platform` library or codebase with a clear contract consumed by the HTTP library.
- The HTTP library no longer directly owns Arduino, POSIX, Windows, or in-memory platform adapter implementations.
- The `lumalink::platform` public contract exposed to HTTP is limited to buffers, transport, and filesystem concerns.
- Include paths, build manifests, and dependency graphs make the platform split explicit and prevent HTTP-core code from drifting back into platform-specific ownership.
- Native tests and supported embedded compile validation cover the renamed surfaces and the new platform-library integration boundary.
- The migration plan does not rely on long-lived deprecated wrapper namespaces, shim headers, or compatibility aliases unless a temporary bridge is explicitly approved for release-management reasons.

## Tasks

### Phase 1: Name, Ownership, And Boundary Decisions

- [ ] Confirm the canonical branding and spelling for the target libraries, including whether `lumalink::platform` is the intended final namespace and package name.
- [x] Use `lumalink::http` and `lumalink::platform` as the public namespaces, with versioning moved to package and release metadata only rather than a public C++ namespace segment.
- [x] Use a separate repository for `lumalink::platform` rather than a subtree, submodule, or same-monorepo package split.
- [x] Define the dependency direction and ownership rules so HTTP depends on platform contracts, while platform code does not depend on HTTP pipeline, routing, or HTTP-domain code.
- [x] Freeze the platform boundary so HTTP-core owns protocol behavior and `lumalink::platform` exposes only buffers, transport, and filesystem concerns.

### Phase 2: Surface Inventory And Rename Scope

- [ ] Inventory all public rename surfaces, including `library.properties`, `library.json`, top-level umbrella headers, include roots under `src/httpadv`, documentation, examples, tests, and repository URLs.
- [ ] Inventory every namespace declaration and alias rooted in `httpadv`, including unversioned convenience aliases and any comments or closing-namespace markers that still use obsolete names.
- [ ] Identify all public include paths that consumers are expected to include directly and decide their target `lumalink/http/...` structure.
- [ ] Identify all macros, configuration constants, generated metadata, and user-facing symbols that still encode `HttpServerAdvanced` or `httpadv` naming.
- [ ] Record every place where platform code is exposed through the HTTP umbrella surface, including `TransportFactory`, adapter headers, and any platform-selected defaults.

### Phase 3: Platform Library Extraction Design

- [x] Define the minimal platform contract that HTTP consumes as buffers, transport, and filesystem only.
- [ ] Separate pure interfaces and traits from concrete implementations so only the contract required by HTTP remains in the HTTP-owned dependency surface.
- [ ] Decide where `TransportFactory`, file adapters, and path-mapping helpers belong after the split, and whether any of them should be decomposed into smaller platform-facing interfaces.
- [ ] Define package boundaries for Arduino, POSIX, Windows, and in-memory/native test adapters so they can live in `lumalink::platform` without reintroducing HTTP ownership.
- [ ] Define how `lumalink::http` selects or receives a platform implementation without using compile-time platform selection directly inside the HTTP umbrella header.

### Phase 4: Migration Sequencing And Consumer Impact

- [ ] Define the order of operations for header moves, namespace changes, package metadata updates, and repository or dependency changes so the codebase stays buildable during implementation.
- [ ] Decide whether the migration is a single breaking cut or a short staged transition, and document the exact conditions under which any temporary bridge surface would be allowed.
- [ ] Define the target top-level include story for consumers, including whether `HttpServerAdvanced.h` is removed outright or replaced by a new canonical umbrella header.
- [ ] Document the required downstream changes for Arduino, PlatformIO, and native consumers, including dependency declarations for the new platform package.
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
