2026-03-30 — GitHub Copilot — Added planned path/namespace realignment phase with initial move manifest
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
- [ ] Add a dedicated path/namespace realignment phase so on-disk layout mirrors `httpadv::v1` namespaces under `src/`.
- [ ] Create an authoritative `source => target` move manifest for the `src/` tree and treat it as the single input for both `git mv` operations and include-rewrite patches.
- [ ] Execute the path realignment as mechanical moves first, with no opportunistic logic edits mixed into the move commit.
- [ ] Apply a follow-up include rewrite pass driven by the move manifest, then run builds/tests and fix only fallout caused by the relocation.
- [ ] Update examples, `docs/EXAMPLES.md`, and any README snippets to use `httpadv::...` symbols.
- [ ] Remove global-scope symbols, shims, and aliases; run a repo-wide check to assert none remain.
- [ ] Add CI checks: build matrix + a small check that searches for disallowed top-level declarations/usages (e.g., regex for `^class\\s+[A-Z]` at global scope or occurrences of `::HttpContext` outside `httpadv` namespace).
- [x] Run full builds/tests across supported PlatformIO profiles and native targets; iterate until green.
- [ ] Final review, sign-off, and bump library version + changelog entry.

## Planned Follow-on Phase: Path / Namespace Realignment

### Objective
Reorganize the `src/` tree so file paths reflect the namespaced ownership model already established in code: `src/` should represent `httpadv::v1`, with subdirectories underneath it matching the active namespace partitions. The preferred execution model is deliberately mechanical:

1. Freeze a complete `source => target` mapping.
2. Perform `git mv` operations from that mapping without mixing in behavior changes.
3. Use the same mapping to drive include updates and any minimal path-fallout patches.
4. Rebuild and test after each move wave.

### Execution Rules
- Treat the move manifest as the source of truth for both file relocation and include rewrites.
- Prefer whole-directory moves where the namespace already matches the intended destination.
- Keep third-party/vendor trees out of the move wave unless there is a strong reason to relocate them.
- Do not fold cleanup, API redesign, or stylistic edits into the move commit; keep the move wave mechanically reviewable.
- After the move commit lands, do a separate patch pass for include rewrites, include-path simplification, and any build-system path fixes.

### Exhaustive Source => Target Manifest Seed
Every current file under `src/` is listed below except the vendored `src/llhttp/` subtree, which remains intentionally out of scope for this phase. This table is meant to be the baseline manifest seed and should be expanded or regenerated mechanically as the tree changes.

| Source | Target |
| --- | --- |
| `src/HttpServerAdvanced.h` | `src/httpadv/v1/HttpServerAdvanced.h` |
| `src/compat/Availability.h` | `src/httpadv/v1/transport/Availability.h` |
| `src/compat/ByteStream.h` | `src/httpadv/v1/transport/ByteStream.h` |
| `src/compat/Clock.h` | `src/httpadv/v1/util/Clock.h` |
| `src/compat/Compat.h` | `src/httpadv/v1/util/Compat.h` |
| `src/compat/IFileSystem.h` | `src/httpadv/v1/transport/IFileSystem.h` |
| `src/compat/Span.h` | `src/httpadv/v1/util/Span.h` |
| `src/compat/TransportInterfaces.h` | `src/httpadv/v1/transport/TransportInterfaces.h` |
| `src/compat/platform/PathMapper.h` | `src/httpadv/v1/platform/PathMapper.h` |
| `src/compat/platform/arduino/ArduinoFileAdapter.h` | `src/httpadv/v1/platform/arduino/ArduinoFileAdapter.h` |
| `src/compat/platform/arduino/ArduinoWiFiTransport.cpp` | `src/httpadv/v1/platform/arduino/ArduinoWiFiTransport.cpp` |
| `src/compat/platform/arduino/ArduinoWiFiTransport.h` | `src/httpadv/v1/platform/arduino/ArduinoWiFiTransport.h` |
| `src/compat/platform/memory/MemoryFileAdapter.h` | `src/httpadv/v1/platform/memory/MemoryFileAdapter.h` |
| `src/compat/platform/posix/PosixFileAdapter.h` | `src/httpadv/v1/platform/posix/PosixFileAdapter.h` |
| `src/compat/platform/posix/PosixSocketTransport.h` | `src/httpadv/v1/platform/posix/PosixSocketTransport.h` |
| `src/compat/platform/windows/WindowsFileAdapter.h` | `src/httpadv/v1/platform/windows/WindowsFileAdapter.h` |
| `src/compat/platform/windows/WindowsSocketTransport.h` | `src/httpadv/v1/platform/windows/WindowsSocketTransport.h` |
| `src/compat/span.hpp` | `src/httpadv/v1/util/span.hpp` |
| `src/core/Defines.h` | `src/httpadv/v1/core/Defines.h` |
| `src/core/HttpContentTypes.h` | `src/httpadv/v1/core/HttpContentTypes.h` |
| `src/core/HttpContext.cpp` | `src/httpadv/v1/core/HttpContext.cpp` |
| `src/core/HttpContext.h` | `src/httpadv/v1/core/HttpContext.h` |
| `src/core/HttpContextHandlerFactory.h` | `src/httpadv/v1/core/HttpContextHandlerFactory.h` |
| `src/core/HttpContextPhase.h` | `src/httpadv/v1/core/HttpContextPhase.h` |
| `src/core/HttpContextPipelineAdapter.cpp` | `src/httpadv/v1/core/HttpContextPipelineAdapter.cpp` |
| `src/core/HttpContextPipelineAdapter.h` | `src/httpadv/v1/core/HttpContextPipelineAdapter.h` |
| `src/core/HttpContextRunner.h` | `src/httpadv/v1/core/HttpContextRunner.h` |
| `src/core/HttpHeader.h` | `src/httpadv/v1/core/HttpHeader.h` |
| `src/core/HttpHeaderCollection.cpp` | `src/httpadv/v1/core/HttpHeaderCollection.cpp` |
| `src/core/HttpHeaderCollection.h` | `src/httpadv/v1/core/HttpHeaderCollection.h` |
| `src/core/HttpMethod.h` | `src/httpadv/v1/core/HttpMethod.h` |
| `src/core/HttpStatus.h` | `src/httpadv/v1/core/HttpStatus.h` |
| `src/core/HttpTimeouts.h` | `src/httpadv/v1/core/HttpTimeouts.h` |
| `src/core/IHttpContextHandlerFactory.h` | `src/httpadv/v1/core/IHttpContextHandlerFactory.h` |
| `src/handlers/BufferedStringBodyHandler.cpp` | `src/httpadv/v1/handlers/BufferedStringBodyHandler.cpp` |
| `src/handlers/BufferedStringBodyHandler.h` | `src/httpadv/v1/handlers/BufferedStringBodyHandler.h` |
| `src/handlers/BufferingHttpHandlerBase.cpp` | `src/httpadv/v1/handlers/BufferingHttpHandlerBase.cpp` |
| `src/handlers/BufferingHttpHandlerBase.h` | `src/httpadv/v1/handlers/BufferingHttpHandlerBase.h` |
| `src/handlers/FormBodyHandler.cpp` | `src/httpadv/v1/handlers/FormBodyHandler.cpp` |
| `src/handlers/FormBodyHandler.h` | `src/httpadv/v1/handlers/FormBodyHandler.h` |
| `src/handlers/HandlerRestrictions.h` | `src/httpadv/v1/handlers/HandlerRestrictions.h` |
| `src/handlers/HandlerResult.h` | `src/httpadv/v1/handlers/HandlerResult.h` |
| `src/handlers/HandlerTypes.h` | `src/httpadv/v1/handlers/HandlerTypes.h` |
| `src/handlers/HttpHandler.h` | `src/httpadv/v1/handlers/HttpHandler.h` |
| `src/handlers/IHandlerProvider.h` | `src/httpadv/v1/handlers/IHandlerProvider.h` |
| `src/handlers/IHttpHandler.h` | `src/httpadv/v1/handlers/IHttpHandler.h` |
| `src/handlers/JsonBodyHandler.cpp` | `src/httpadv/v1/handlers/JsonBodyHandler.cpp` |
| `src/handlers/JsonBodyHandler.h` | `src/httpadv/v1/handlers/JsonBodyHandler.h` |
| `src/handlers/MultipartFormDataHandler.cpp` | `src/httpadv/v1/handlers/MultipartFormDataHandler.cpp` |
| `src/handlers/MultipartFormDataHandler.h` | `src/httpadv/v1/handlers/MultipartFormDataHandler.h` |
| `src/handlers/RawBodyHandler.cpp` | `src/httpadv/v1/handlers/RawBodyHandler.cpp` |
| `src/handlers/RawBodyHandler.h` | `src/httpadv/v1/handlers/RawBodyHandler.h` |
| `src/httpadv/namespace.h` | `src/httpadv/namespace.h` |
| `src/pipeline/ConnectionSession.h` | `src/httpadv/v1/pipeline/ConnectionSession.h` |
| `src/pipeline/HttpPipeline.cpp` | `src/httpadv/v1/pipeline/HttpPipeline.cpp` |
| `src/pipeline/HttpPipeline.h` | `src/httpadv/v1/pipeline/HttpPipeline.h` |
| `src/pipeline/HttpProtocolExecution.cpp` | `src/httpadv/v1/pipeline/HttpProtocolExecution.cpp` |
| `src/pipeline/HttpProtocolExecution.h` | `src/httpadv/v1/pipeline/HttpProtocolExecution.h` |
| `src/pipeline/IPipelineHandler.h` | `src/httpadv/v1/pipeline/IPipelineHandler.h` |
| `src/pipeline/IProtocolExecution.h` | `src/httpadv/v1/pipeline/IProtocolExecution.h` |
| `src/pipeline/NetClient.h` | `src/httpadv/v1/pipeline/NetClient.h` |
| `src/pipeline/PipelineError.cpp` | `src/httpadv/v1/pipeline/PipelineError.cpp` |
| `src/pipeline/PipelineError.h` | `src/httpadv/v1/pipeline/PipelineError.h` |
| `src/pipeline/PipelineHandleClientResult.h` | `src/httpadv/v1/pipeline/PipelineHandleClientResult.h` |
| `src/pipeline/RequestHandlingResult.h` | `src/httpadv/v1/pipeline/RequestHandlingResult.h` |
| `src/pipeline/RequestParser.cpp` | `src/httpadv/v1/pipeline/RequestParser.cpp` |
| `src/pipeline/RequestParser.h` | `src/httpadv/v1/pipeline/RequestParser.h` |
| `src/response/ChunkedHttpResponseBodyStream.cpp` | `src/httpadv/v1/response/ChunkedHttpResponseBodyStream.cpp` |
| `src/response/ChunkedHttpResponseBodyStream.h` | `src/httpadv/v1/response/ChunkedHttpResponseBodyStream.h` |
| `src/response/FormResponse.cpp` | `src/httpadv/v1/response/FormResponse.cpp` |
| `src/response/FormResponse.h` | `src/httpadv/v1/response/FormResponse.h` |
| `src/response/HttpResponse.cpp` | `src/httpadv/v1/response/HttpResponse.cpp` |
| `src/response/HttpResponse.h` | `src/httpadv/v1/response/HttpResponse.h` |
| `src/response/HttpResponseIterators.cpp` | `src/httpadv/v1/response/HttpResponseIterators.cpp` |
| `src/response/HttpResponseIterators.h` | `src/httpadv/v1/response/HttpResponseIterators.h` |
| `src/response/IHttpResponse.h` | `src/httpadv/v1/response/IHttpResponse.h` |
| `src/response/JsonResponse.cpp` | `src/httpadv/v1/response/JsonResponse.cpp` |
| `src/response/JsonResponse.h` | `src/httpadv/v1/response/JsonResponse.h` |
| `src/response/StringResponse.cpp` | `src/httpadv/v1/response/StringResponse.cpp` |
| `src/response/StringResponse.h` | `src/httpadv/v1/response/StringResponse.h` |
| `src/routing/BasicAuthentication.h` | `src/httpadv/v1/routing/BasicAuthentication.h` |
| `src/routing/CrossOriginRequestSharing.h` | `src/httpadv/v1/routing/CrossOriginRequestSharing.h` |
| `src/routing/HandlerBuilder.cpp` | `src/httpadv/v1/routing/HandlerBuilder.cpp` |
| `src/routing/HandlerBuilder.h` | `src/httpadv/v1/routing/HandlerBuilder.h` |
| `src/routing/HandlerMatcher.cpp` | `src/httpadv/v1/routing/HandlerMatcher.cpp` |
| `src/routing/HandlerMatcher.h` | `src/httpadv/v1/routing/HandlerMatcher.h` |
| `src/routing/HandlerProviderRegistry.cpp` | `src/httpadv/v1/routing/HandlerProviderRegistry.cpp` |
| `src/routing/HandlerProviderRegistry.h` | `src/httpadv/v1/routing/HandlerProviderRegistry.h` |
| `src/routing/ProviderRegistryBuilder.h` | `src/httpadv/v1/routing/ProviderRegistryBuilder.h` |
| `src/routing/ReplaceVariables.h` | `src/httpadv/v1/routing/ReplaceVariables.h` |
| `src/server/HttpServerBase.cpp` | `src/httpadv/v1/server/HttpServerBase.cpp` |
| `src/server/HttpServerBase.h` | `src/httpadv/v1/server/HttpServerBase.h` |
| `src/server/WebServer.h` | `src/httpadv/v1/server/WebServer.h` |
| `src/server/WebServerBuilder.h` | `src/httpadv/v1/server/WebServerBuilder.h` |
| `src/server/WebServerConfig.h` | `src/httpadv/v1/server/WebServerConfig.h` |
| `src/staticfiles/AggregateFileLocator.cpp` | `src/httpadv/v1/staticfiles/AggregateFileLocator.cpp` |
| `src/staticfiles/AggregateFileLocator.h` | `src/httpadv/v1/staticfiles/AggregateFileLocator.h` |
| `src/staticfiles/DefaultFileLocator.cpp` | `src/httpadv/v1/staticfiles/DefaultFileLocator.cpp` |
| `src/staticfiles/DefaultFileLocator.h` | `src/httpadv/v1/staticfiles/DefaultFileLocator.h` |
| `src/staticfiles/FileLocator.h` | `src/httpadv/v1/staticfiles/FileLocator.h` |
| `src/staticfiles/StaticFileHandler.cpp` | `src/httpadv/v1/staticfiles/StaticFileHandler.cpp` |
| `src/staticfiles/StaticFileHandler.h` | `src/httpadv/v1/staticfiles/StaticFileHandler.h` |
| `src/staticfiles/StaticFilesBuilder.cpp` | `src/httpadv/v1/staticfiles/StaticFilesBuilder.cpp` |
| `src/staticfiles/StaticFilesBuilder.h` | `src/httpadv/v1/staticfiles/StaticFilesBuilder.h` |
| `src/streams/Base64Stream.cpp` | `src/httpadv/v1/streams/Base64Stream.cpp` |
| `src/streams/Base64Stream.h` | `src/httpadv/v1/streams/Base64Stream.h` |
| `src/streams/Iterators.h` | `src/httpadv/v1/streams/Iterators.h` |
| `src/streams/Streams.cpp` | `src/httpadv/v1/streams/Streams.cpp` |
| `src/streams/Streams.h` | `src/httpadv/v1/streams/Streams.h` |
| `src/streams/UriStream.cpp` | `src/httpadv/v1/streams/UriStream.cpp` |
| `src/streams/UriStream.h` | `src/httpadv/v1/streams/UriStream.h` |
| `src/util/HttpUtility.cpp` | `src/httpadv/v1/util/HttpUtility.cpp` |
| `src/util/HttpUtility.h` | `src/httpadv/v1/util/HttpUtility.h` |
| `src/util/KeyValuePairView.h` | `src/httpadv/v1/util/KeyValuePairView.h` |
| `src/util/UriView.cpp` | `src/httpadv/v1/util/UriView.cpp` |
| `src/util/UriView.h` | `src/httpadv/v1/util/UriView.h` |
| `src/websocket/IWebSocketSessionControl.h` | `src/httpadv/v1/websocket/IWebSocketSessionControl.h` |
| `src/websocket/WebSocketActivationSnapshot.h` | `src/httpadv/v1/websocket/WebSocketActivationSnapshot.h` |
| `src/websocket/WebSocketCallbacks.h` | `src/httpadv/v1/websocket/WebSocketCallbacks.h` |
| `src/websocket/WebSocketContext.cpp` | `src/httpadv/v1/websocket/WebSocketContext.cpp` |
| `src/websocket/WebSocketContext.h` | `src/httpadv/v1/websocket/WebSocketContext.h` |
| `src/websocket/WebSocketErrorPolicy.h` | `src/httpadv/v1/websocket/WebSocketErrorPolicy.h` |
| `src/websocket/WebSocketFrameCodec.cpp` | `src/httpadv/v1/websocket/WebSocketFrameCodec.cpp` |
| `src/websocket/WebSocketFrameCodec.h` | `src/httpadv/v1/websocket/WebSocketFrameCodec.h` |
| `src/websocket/WebSocketProtocolExecution.cpp` | `src/httpadv/v1/websocket/WebSocketProtocolExecution.cpp` |
| `src/websocket/WebSocketProtocolExecution.h` | `src/httpadv/v1/websocket/WebSocketProtocolExecution.h` |
| `src/websocket/WebSocketUpgradeHandler.cpp` | `src/httpadv/v1/websocket/WebSocketUpgradeHandler.cpp` |
| `src/websocket/WebSocketUpgradeHandler.h` | `src/httpadv/v1/websocket/WebSocketUpgradeHandler.h` |

### Move-First Workflow
- Phase A: generate the full manifest, including every concrete file under the directory-level rules above.
- Phase B: execute only `git mv` operations from the manifest and commit that tree move independently.
- Phase C: rewrite `#include` paths using the same manifest as the translation layer from old paths to new paths.
- Phase D: patch any build scripts, task configs, or docs that still reference old physical paths.
- Phase E: run native tests and supported PlatformIO builds; fix only relocation-induced fallout.

### Review Expectations
- The move commit should be explainable almost entirely by the manifest diff.
- The include-rewrite commit should be explainable almost entirely by path substitutions derived from the same manifest.
- Any non-mechanical changes discovered during the effort should be deferred into follow-up commits unless they are required to restore the build.

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
- Path realignment should be planned as a filesystem move problem first and a code patch problem second; the move manifest should remain the authoritative input for both phases.
