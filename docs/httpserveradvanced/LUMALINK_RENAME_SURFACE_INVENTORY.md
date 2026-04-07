# LumaLink Rename Surface Inventory

## Summary

This note records the rename surface inventory after completion of the `lumalink::platform` extraction. It documents what is already integrated from the new platform library, what legacy HTTP naming remains, and what cleanup is still required for the `HttpServerAdvanced` / `httpadv` to `lumalink` migration.

## Locked Decisions

- Public C++ namespaces move to `lumalink::http` and `lumalink::platform`.
- Public namespace versioning is removed; versioning lives in package and release metadata only.
- `lumalink::platform` ships from a separate repository rooted at `c:\ode\lumalink-platform`.
- `lumalink::http` depends on `lumalink::platform`.
- `lumalink::platform` exposes only buffers, transport, and filesystem concerns.
- The migration is a direct cutover: no compatibility aliases, shim headers, wrapper APIs, or fallback include paths are introduced.

## Current Public Rename Surfaces

### Package And Repository Metadata

- `library.properties` still publishes the library name as `HttpServerAdvanced`, exposes `lumalink/http/HttpServerAdvanced.h` as the public include, and points to the current repository URL.
- `library.json` still publishes the package name as `HttpServerAdvanced`, lists `lumalink/http/HttpServerAdvanced.h` as the public header, and points to the current repository URL.
- `keywords.txt` still exposes legacy Arduino-facing symbols and does not reflect any future `lumalink` naming.

### Public Headers And Include Roots

- `src/lumalink/http/HttpServerAdvanced.h` is the current umbrella header for nearly the full HTTP surface.
- Public subsystem headers now live under `src/lumalink/http/` with these major areas:
  - `core/`
  - `handlers/`
  - `pipeline/`
  - `response/`
  - `routing/`
  - `server/`
  - `staticfiles/`
  - `streams/`
  - `transport/`
  - `util/`
  - `websocket/`

### Namespace Surfaces

- Active source and test code now declares and consumes `lumalink::http::<subsystem>` directly.
- The remaining public legacy surface is the umbrella basename and residual docs/examples, not the active include-root layout or C++ namespace declarations.
- The deleted `src/httpadv/namespace.h` shim is no longer part of the public surface.
- Legacy namespace-closing markers in active source have been cleaned; remaining rename work is centered on package metadata, final umbrella naming cleanup, macros, and maintained docs/examples.

### Macros, Configuration Constants, And User-Facing Symbols

- Compile-time override hooks are still published with the `HTTPSERVER_ADVANCED_` prefix in `src/lumalink/http/core/Defines.h`.
- `src/lumalink/http/routing/ReplaceVariables.h` still publishes `HTTPSERVER_ADVANCED_REPLACE_VARIABLES_MAX_TOKEN_BYTES`.
- JSON feature selection still uses `HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON` across the umbrella header and JSON-specific handlers/response types.
- User-facing strings and doc examples still use `HttpServerAdvanced`, including the main library documentation title, example include statements, and example string literals such as `X-Powered-By`.

## Contract And Implementation Split

The split between `lumalink::http` and `lumalink::platform` is implemented. The platform project is hosted at `c:\ode\lumalink-platform`, and this repository now consumes platform types through `lumalink/platform/...` includes.

### HTTP-Owned Contract Surface

`lumalink::http` retains only the platform-facing contracts it actually consumes:

- buffer-oriented abstractions required by the HTTP pipeline and stream helpers
- transport-facing interfaces and traits consumed by HTTP runtime code
- filesystem-facing interfaces required by static-file and file-locator behavior

The HTTP package depends on those contracts as imported types and does not define canonical concrete adapter implementations.

### Platform-Owned Implementation Surface

`lumalink::platform` owns concrete implementations and compile-time selection machinery, including:

- concrete transport factories and transport implementations
- concrete filesystem adapters
- path-mapping helpers used by those adapters
- compile-time platform-selection glue that chooses Arduino, POSIX, Windows, or in-memory/native implementations
- adapter-specific tests that validate those implementations

### Practical Separation Rule

When a type exists only to define a platform contract consumed by HTTP, it belongs in the shared contract surface that `lumalink::http` depends on. When a type performs platform-specific behavior, selects a platform implementation, or exists to support a concrete adapter, it belongs in `lumalink::platform`. This rule is enforced as a hard boundary, not a compatibility bridge.

## Platform Exposure Through The HTTP Library

### Current Integration Surface (Post-Extraction)

- `src/lumalink/http/HttpServerAdvanced.h` includes `lumalink/platform/transport/TransportTraits.h` and `lumalink/platform/TransportFactory.h` from the extracted platform package.
- `src/lumalink/http/server/WebServer.h` includes platform transport headers from `lumalink/platform/...` and binds native transport factories to `lumalink::platform` types.
- Core transport and stream contracts in the HTTP package already consume `lumalink::platform::buffers` and related platform namespaces.
- A legacy in-repo platform tree still exists under `src/httpadv/v1/platform/`; it is no longer the target ownership model and should be removed as cleanup, not kept as a compatibility path.

### Required Boundary In Steady State

- `lumalink::http` may depend on buffer, transport, and filesystem contracts only.
- `lumalink::http` must not publicly re-export concrete Arduino, POSIX, Windows, or in-memory adapter headers.
- Compile-time platform selection may remain only where it resolves `lumalink::platform` contracts and implementations; HTTP must not own duplicate platform-selection codepaths.
- `TransportFactory`, concrete file adapters, and path-mapping helpers are platform-owned and must not be reintroduced under HTTP-owned include roots.
- Adapter-specific tests remain with platform implementations in `lumalink::platform`.
- No compatibility aliases, no shim headers, and no fallback include wiring are allowed when removing residual `src/httpadv/v1/platform/` content.

## Target Include Structure

### HTTP Package

The target public include root for the HTTP library is `lumalink/http/`.

Representative directory mapping:

- `httpadv/v1/core/*` -> `lumalink/http/core/*`
- `httpadv/v1/handlers/*` -> `lumalink/http/handlers/*`
- `httpadv/v1/pipeline/*` -> `lumalink/http/pipeline/*`
- `httpadv/v1/response/*` -> `lumalink/http/response/*`
- `httpadv/v1/routing/*` -> `lumalink/http/routing/*`
- `httpadv/v1/server/*` -> `lumalink/http/server/*`
- `httpadv/v1/staticfiles/*` -> `lumalink/http/staticfiles/*`
- `httpadv/v1/streams/*` -> `lumalink/http/streams/*`
- `httpadv/v1/util/*` -> `lumalink/http/util/*`
- `httpadv/v1/websocket/*` -> `lumalink/http/websocket/*`

Rules:

- No public `v1` path segment remains in the include tree.
- No `httpadv/` include root remains in the public HTTP package.
- No platform implementation headers are re-exported from `lumalink/http/`.
- The exact canonical umbrella-header filename is intentionally deferred to Phase 4, but it must live under the `lumalink/http/` include root.

### Platform Package

The platform package should publish its own include root under `lumalink/platform/` and restrict its public surface to:

- `lumalink/platform/buffers/`
- `lumalink/platform/transport/`
- `lumalink/platform/filesystem/`

HTTP consumes those contracts rather than owning parallel platform abstractions.

The platform library project is located at `c:\ode\lumalink-platform`.

## Current Completion Notes

- Public rename surfaces are inventoried across package metadata, include roots, namespaces, macros, docs, and test-facing entrypoints.
- Platform extraction is complete and integrated through `lumalink/platform/...` include usage in HTTP-facing code.
- Active source/test namespace migration to `lumalink::http` is complete, including cleanup of stale namespace-closing markers.
- Public HTTP headers now live under `src/lumalink/http/`, and the legacy top-level `src/HttpServerAdvanced.h` wrapper has been removed.
- Remaining work in this repository is package metadata rename, final umbrella naming cleanup, docs/example updates, and removal of residual legacy platform tree content.
- Cleanup must be a direct cutover to final names and boundaries with no compatibility layers or alias bridges.