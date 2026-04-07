# LumaLink Rename Surface Inventory

## Summary

This note closes Phase 2 of the LumaLink rename backlog by inventorying the current public rename surfaces, documenting the namespace and include-path migration targets, and identifying where platform-specific code still leaks through the HTTP library surface. It describes the current `HttpServerAdvanced` / `httpadv` state; it does not rename code yet.

## Locked Decisions

- Public C++ namespaces move to `lumalink::http` and `lumalink::platform`.
- Public namespace versioning is removed; versioning lives in package and release metadata only.
- `lumalink::platform` ships from a separate repository rooted at `c:\ode\lumalink-platform`.
- `lumalink::http` depends on `lumalink::platform`.
- `lumalink::platform` exposes only buffers, transport, and filesystem concerns.

## Current Public Rename Surfaces

### Package And Repository Metadata

- `library.properties` still publishes the library name as `HttpServerAdvanced`, exposes `HttpServerAdvanced.h` as the public include, and points to the current repository URL.
- `library.json` still publishes the package name as `HttpServerAdvanced`, lists `HttpServerAdvanced.h` as the public header, and points to the current repository URL.
- `keywords.txt` still exposes legacy Arduino-facing symbols and does not reflect any future `lumalink` naming.

### Public Headers And Include Roots

- `src/HttpServerAdvanced.h` is the current top-level public wrapper header.
- `src/httpadv/namespace.h` establishes the unversioned `httpadv` root namespace.
- `src/httpadv/v1/HttpServerAdvanced.h` is the current umbrella header for nearly the full HTTP surface.
- Public subsystem headers currently live under `src/httpadv/v1/` with these major areas:
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

- The canonical current namespace is `httpadv::v1::<subsystem>` across the codebase.
- `src/httpadv/v1/HttpServerAdvanced.h` also exposes unversioned `httpadv` convenience aliases such as `httpadv::WebServer`, `httpadv::HttpResponse`, `httpadv::StringResponse`, and `httpadv::transport`.
- Closing namespace comments and prose throughout the codebase still use a mix of `httpadv` and legacy `HttpServerAdvanced` labels.

### Macros, Configuration Constants, And User-Facing Symbols

- Compile-time override hooks are still published with the `HTTPSERVER_ADVANCED_` prefix in `src/httpadv/v1/core/Defines.h`.
- `src/httpadv/v1/routing/ReplaceVariables.h` still publishes `HTTPSERVER_ADVANCED_REPLACE_VARIABLES_MAX_TOKEN_BYTES`.
- JSON feature selection still uses `HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON` across the umbrella header and JSON-specific handlers/response types.
- User-facing strings and doc examples still use `HttpServerAdvanced`, including the main library documentation title, example include statements, and example string literals such as `X-Powered-By`.

## Contract And Implementation Split

The split between `lumalink::http` and `lumalink::platform` is a documentation decision at this stage only. No source files are moved by this note, but the target platform project location is `c:\ode\lumalink-platform`.

### HTTP-Owned Contract Surface

`lumalink::http` should retain only the platform-facing contracts it actually consumes:

- buffer-oriented abstractions required by the HTTP pipeline and stream helpers
- transport-facing interfaces and traits consumed by HTTP runtime code
- filesystem-facing interfaces required by static-file and file-locator behavior

The HTTP package should depend on those contracts as imported types, not own concrete platform adapter implementations.

### Platform-Owned Implementation Surface

`lumalink::platform` should own the concrete implementations and compile-time selection machinery, including:

- concrete transport factories and transport implementations
- concrete filesystem adapters
- path-mapping helpers used by those adapters
- compile-time platform-selection glue that chooses Arduino, POSIX, Windows, or in-memory/native implementations
- adapter-specific tests that validate those implementations

### Practical Separation Rule

When a type exists only to define a platform contract consumed by HTTP, it belongs in the shared contract surface that `lumalink::http` depends on. When a type performs platform-specific behavior, selects a platform implementation, or exists to support a concrete adapter, it belongs in `lumalink::platform`.

## Platform Exposure Through The HTTP Library

### Current Leaks

- `src/httpadv/v1/HttpServerAdvanced.h` publicly includes `src/httpadv/v1/platform/TransportFactory.h`, which exposes a platform-owned runtime wrapper from the HTTP umbrella surface.
- `src/httpadv/v1/server/WebServer.h` directly includes Arduino, Windows, and POSIX transport headers and performs compile-time native transport selection inside the HTTP package.
- Concrete platform adapters currently live under `src/httpadv/v1/platform/`:
  - `arduino/ArduinoWiFiTransport.h`
  - `arduino/ArduinoFileAdapter.h`
  - `posix/PosixSocketTransport.h`
  - `posix/PosixFileAdapter.h`
  - `windows/WindowsSocketTransport.h`
  - `windows/WindowsFileAdapter.h`
  - `memory/MemoryFileAdapter.h`
- Native and in-memory adapter validation currently lives in the HTTP repository test tree instead of alongside the future platform package.
- Path-mapping helpers are still owned from the same platform area and are referenced by the filesystem adapters.

### Required Boundary After The Split

- `lumalink::http` may depend on buffer, transport, and filesystem contracts only.
- `lumalink::http` must not publicly re-export concrete Arduino, POSIX, Windows, or in-memory adapter headers.
- Compile-time platform selection may remain, but it must move out of the HTTP umbrella header and out of `WebServer.h` into `lumalink::platform` ownership.
- `TransportFactory`, concrete file adapters, and path-mapping helpers move into `lumalink::platform` ownership after the split.
- Adapter-specific tests, including native and in-memory validation, move with those platform implementations into `lumalink::platform`.

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

HTTP should consume those contracts rather than owning parallel platform abstractions.

The new platform library project is expected to be created in `c:\ode\lumalink-platform`.

## Phase 2 Completion Notes

- Public rename surfaces are now inventoried across package metadata, include roots, namespaces, macros, docs, and test-facing entrypoints.
- The target HTTP include tree is defined as `lumalink/http/...` with no public version segment.
- The current platform leak points are documented so Phase 3 can separate contracts from implementations cleanly.