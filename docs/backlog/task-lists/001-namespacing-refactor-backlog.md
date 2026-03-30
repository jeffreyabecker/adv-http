2026-03-30 — GitHub Copilot — Completed namespace migration across src and tests; native test suite now passes.
2026-03-30 — Jeffrey Becker — Created backlog for namespacing refactor

# Namespacing Refactor — Clean greenfield migration

## Summary
Refactor the codebase to adopt a responsibility-driven namespace hierarchy rooted at `httpadv` (with an `inline namespace v1` for versioning). Because this is a greenfield project, perform an in-place migration: move types and implementations directly into their final namespaces and remove all compatibility shims, aliases, or temporary adapters. Deliver a single, clean public API surface, update headers and examples, and ensure the project continues to build across supported targets.

## Goal / Acceptance Criteria
- All public API types are declared inside `httpadv::...` (inside `inline namespace v1`) and organized by responsibility (core, pipeline, handlers, routing, server, transport, response, websocket, staticfiles, util).
- No global-scope symbols remain (no `::HttpContext`, etc.).
- No compatibility shims, `using` aliases, or temporary adapter headers remain in the tree after completion.
- Project builds successfully for all supported targets/profiles and unit/native tests pass.
- Examples, documentation, and `library.properties`/include lists are updated to reflect the new layout.
- CI enforces the namespace/API surface (build + a simple grep/static-check to detect accidental global symbols).

## Tasks (checklist)
- [x] Design namespace map and conventions (names, casing, `v1` inline namespace, public vs internal split).
- [x] Draft this backlog and required acceptance criteria.
- [x] Create `include/httpadv/namespace.h` to declare `namespace httpadv { inline namespace v1 { } }` and to document migration rules.
- [ ] Establish header layout under `include/httpadv/` (one header per public concept/module) and add a small README describing include rules.
- [ ] For each module, relocate declarations into their final namespace and update header guards / include paths:
  - [x] `core` (e.g., `HttpContext`, `HttpHeader`, `HttpHeaderCollection`)
  - [x] `pipeline` (parsing/execution split as needed)
  - [x] `handlers` (IHttpHandler, concrete handlers)
  - [x] `routing` (HandlerMatcher, router types)
  - [x] `server` (HttpServerBase, FriendlyWebServer, ConnectionSession)
  - [x] `response` (IHttpResponse, HttpResponse)
  - [x] `transport` / `io` (IClient, IServer, byte stream types)
  - [x] `websocket` (WebSocketContext and friends)
  - [x] `staticfiles` / `fs` (FileLocator)
  - [x] `util` (Span, Clock, small portability wrappers)
- [x] Update all `.cpp` translation units to the new namespaces and fix references.
- [x] Update `library.properties` / top-level public include (`HttpServerAdvanced.h`) to expose the new headers; ensure PlatformIO/Arduino includes still work.
- [ ] Update examples, `docs/EXAMPLES.md`, and any README snippets to use `httpadv::...` symbols.
- [ ] Remove global-scope symbols, shims, and aliases; run a repo-wide check to assert none remain.
- [ ] Add CI checks: build matrix + a small check that searches for disallowed top-level declarations/usages (e.g., regex for `^class\\s+[A-Z]` at global scope or occurrences of `::HttpContext` outside `httpadv` namespace).
- [x] Run full builds/tests across supported PlatformIO profiles and native targets; iterate until green.
- [ ] Final review, sign-off, and bump library version + changelog entry.

## Owner
- Jeffrey Becker

## Priority
- High

## References
- Project docs: [docs/httpserveradvanced/LIBRARY_DOCUMENTATION.md](docs/httpserveradvanced/LIBRARY_DOCUMENTATION.md)
- Examples: [docs/httpserveradvanced/EXAMPLES.md](docs/httpserveradvanced/EXAMPLES.md)
- Package metadata: [library.properties](library.properties)

---

Notes:
- This backlog assumes a greenfield/intentional breaking change stance: no compatibility layers will be left in-place. If downstream consumers require a compatibility path later, handle that in a separate, explicitly-scoped migration backlog.
- Keep each module migration small and testable so failures are easy to revert and diagnose.
- Implementation note: the namespace root was added as `src/httpadv/namespace.h` and the current completed pass covered production sources plus the native/unit test suite. Include-layout, docs/examples, and CI enforcement tasks remain open in this backlog.
