# HttpServerAdvanced Splitting Guide

Goal: split declaration and implementation across headers (.h) and source files (.cpp) where possible to reduce compile times, tighten dependencies, and avoid circular includes. If a dependency is only used in implementation, include it in the .cpp, not the .h.

## General Rules
- Keep public types, small inline accessors, and templates in headers; move non-trivial method bodies to .cpp.
- Prefer forward declarations in headers; include full headers only when required by the declaration (e.g., inheritance or by-value members).
- Resolve cycles with forward declarations + include the dependent header at the end of one file if needed.
- Avoid including heavy headers in other headers; move those includes to .cpp wherever possible.
- For Arduino libraries, .cpp files under `src/` are compiled automatically; no build script changes needed.

## Safe Workflow
1. Identify methods with bodies in headers.
2. Replace their bodies with declarations in the .h.
3. Create a corresponding .cpp: include its header first, then add any additional implementation-only includes.
4. Use forward declarations in headers; include the concrete headers in the .cpp.
5. Rebuild; iterate on missing includes, then commit.

## Circular-Dependency Patterns
- Forward declare peer types in headers (e.g., `class HttpContext;`).
- If a header needs the full type only for implementation, do not include it in the header; include it in the .cpp.
- Only if unavoidable, include the peer header at the end of the header after class declarations (minimizes cycle impact).

## Per-File Guidance
Below are file-by-file recommendations. “Split” indicates moving implementations to a .cpp; “Keep header” indicates template/inline-heavy code better left in headers.

### Core HTTP Types
- [libraries/HttpServerAdvanced/src/HttpRequest.h](../libraries/HttpServerAdvanced/src/HttpRequest.h): Split. Move non-trivial method bodies (parsing helpers, mutators) to `HttpRequest.cpp`. Keep POD types, trivial getters inline.
- [libraries/HttpServerAdvanced/src/HttpResponse.h](../libraries/HttpServerAdvanced/src/HttpResponse.h): Split. Implement factory methods and body stream plumbing in `HttpResponse.cpp`. Include `HttpResponseBodyStream.h` only in .cpp if possible.
- [libraries/HttpServerAdvanced/src/HttpHeader.h](../libraries/HttpServerAdvanced/src/HttpHeader.h): Split partially. Collection helpers currently inline; consider moving larger logic to `HttpHeader.cpp`. Keep very small one-liners inline.
- [libraries/HttpServerAdvanced/src/HttpMethod.h](../libraries/HttpServerAdvanced/src/HttpMethod.h): Keep header (constants/enums). If there are helper functions with logic, move to `HttpMethod.cpp`.
- [libraries/HttpServerAdvanced/src/HttpStatus.h](../libraries/HttpServerAdvanced/src/HttpStatus.h): Keep header by default (factory-style static inline creators). Option: convert to constexpr/const instances in `HttpStatus.cpp` if flash usage dictates.

### Context & Handlers
- [libraries/HttpServerAdvanced/src/HttpContext.h](../libraries/HttpServerAdvanced/src/HttpContext.h): Split. Keep declarations with forward declares (`HttpRequest`, `HttpResponse`, `HttpServerBase`, `IHttpHandler`). Move phase transitions, handler wiring, and service lookups to `HttpContext.cpp`. Only include headers required by member declarations.
- [libraries/HttpServerAdvanced/src/IHttpHandler.h](../libraries/HttpServerAdvanced/src/IHttpHandler.h): Keep header (pure interface). No changes.
- [libraries/HttpServerAdvanced/src/HttpHandler.h](../libraries/HttpServerAdvanced/src/HttpHandler.h): Split. Keep class declaration; move constructors and `handleStep/handleBodyChunk` (if non-trivial) to `HttpHandler.cpp`. Use forward-declared `HttpContext` in header; include it in .cpp.
- [libraries/HttpServerAdvanced/src/HttpHandlerFactory.h](../libraries/HttpServerAdvanced/src/HttpHandlerFactory.h): Split. Move factory registration, predicates, and `createContextHandler` logic to `HttpHandlerFactory.cpp`. Use forward declarations in header; include `HttpHandler.h`, `HttpResponse.h` in .cpp.
- [libraries/HttpServerAdvanced/src/HandlerBuilder.h](../libraries/HttpServerAdvanced/src/HandlerBuilder.h): Split. If it provides a fluent builder, move method bodies to `HandlerBuilder.cpp`. Keep only the fluent API declarations in header.
- [libraries/HttpServerAdvanced/src/HandlersBuilder.h](../libraries/HttpServerAdvanced/src/HandlersBuilder.h): Split similarly; heavy includes (e.g., specific handlers) should move to .cpp.
- [libraries/HttpServerAdvanced/src/HandlerTypes.h](../libraries/HttpServerAdvanced/src/HandlerTypes.h): Keep header (type aliases, traits). If it contains logic, move that logic to .cpp and keep traits/aliases here.
- [libraries/HttpServerAdvanced/src/HandlerRestrictions.h](../libraries/HttpServerAdvanced/src/HandlerRestrictions.h): Keep header (traits/enable_if). Ensure it only includes minimal interfaces (`IHttpHandler.h`).
- [libraries/HttpServerAdvanced/src/HandlerImplementations.h](../libraries/HttpServerAdvanced/src/HandlerImplementations.h): Consider splitting by class into separate headers and .cpp files (e.g., `StaticFileHandler`, `BasicAuthentication`, etc.). Avoid a monolithic header.
- [libraries/HttpServerAdvanced/src/HandlerMatcher.h](../libraries/HttpServerAdvanced/src/HandlerMatcher.h): Split. Move default checkers and `HandlerMatcher` method bodies to `HandlerMatcher.cpp`. Keep forward declaration for `HttpContext`.

### Server & Pipeline
- [libraries/HttpServerAdvanced/src/HttpServerBase.h](../libraries/HttpServerAdvanced/src/HttpServerBase.h): Split. Keep public API; move accept loop, service registry implementation, and pipeline wiring to `HttpServerBase.cpp`.
- [libraries/HttpServerAdvanced/src/HttpServerAdvanced.h](../libraries/HttpServerAdvanced/src/HttpServerAdvanced.h): Split. Implementation-specific behavior to `HttpServerAdvanced.cpp`.
- [libraries/HttpServerAdvanced/src/HttpPipeline.h](../libraries/HttpServerAdvanced/src/HttpPipeline.h): Split. Keep interfaces and small adapters inline; move pipeline step implementations to `HttpPipeline.cpp`.
- [libraries/HttpServerAdvanced/src/PipelineHandleClientResult.h](../libraries/HttpServerAdvanced/src/PipelineHandleClientResult.h): Keep header (enum/flag helpers). If functions have bodies, move to `PipelineHandleClientResult.cpp`.
- [libraries/HttpServerAdvanced/src/IPipelineHandler.h](../libraries/HttpServerAdvanced/src/IPipelineHandler.h): Keep header (interface).

### Streams & Iterators
- [libraries/HttpServerAdvanced/src/Streams.h](../libraries/HttpServerAdvanced/src/Streams.h): Keep header for templates (`IndefiniteConcatStream`, `ConcatStream`) and tiny stream adapters. Move non-template, non-inline classes (e.g., `NonOwningMemoryStream`) to `Streams.cpp` if they grow. Consider `.inl` for lengthy template bodies if preferred.
- [libraries/HttpServerAdvanced/src/Iterators.h](../libraries/HttpServerAdvanced/src/Iterators.h): Keep header (templates `BoundedStreamIterable`, `FixedStreamIterable`).
- [libraries/HttpServerAdvanced/src/HttpResponseIterators.h](../libraries/HttpServerAdvanced/src/HttpResponseIterators.h): Split. Iterator classes are non-template here; move bodies to `HttpResponseIterators.cpp`. Keep `EnsureRequiredHeaders` helper in .cpp; expose only the declaration if needed.
- [libraries/HttpServerAdvanced/src/HttpResponseBodyStream.h](../libraries/HttpServerAdvanced/src/HttpResponseBodyStream.h): Split. Keep the abstract stream interface in header; move concrete implementations (chunked, etc.) into their own `.cpp` (e.g., `ChunkedHttpResponseBodyStream.cpp`).

### Utilities & Config
- [libraries/HttpServerAdvanced/src/HttpContentTypes.h](../libraries/HttpServerAdvanced/src/HttpContentTypes.h): Split. Keep the map/type definitions in header; move population logic to `HttpContentTypes.cpp`.
- [libraries/HttpServerAdvanced/src/HttpTimeouts.h](../libraries/HttpServerAdvanced/src/HttpTimeouts.h): Keep header (constants). If logic exists, move to `HttpTimeouts.cpp`.
- [libraries/HttpServerAdvanced/src/HttpUtility.h](../libraries/HttpServerAdvanced/src/HttpUtility.h): Split. Move parsing/formatting logic to `HttpUtility.cpp`.
- [libraries/HttpServerAdvanced/src/StringUtility.h](../libraries/HttpServerAdvanced/src/StringUtility.h): Keep header (inline helpers). If heavy logic appears, move to `StringUtility.cpp`.
- [libraries/HttpServerAdvanced/src/KeyValuePairView.h](../libraries/HttpServerAdvanced/src/KeyValuePairView.h): Keep header (small view type). If larger parsing helpers exist, move to `.cpp`.
- [libraries/HttpServerAdvanced/src/UriView.h](../libraries/HttpServerAdvanced/src/UriView.h): Keep header (small view). If heavy parsing logic, move to `UriView.cpp`.
- [libraries/HttpServerAdvanced/src/SecureHttpServerConfig.h](../libraries/HttpServerAdvanced/src/SecureHttpServerConfig.h): Keep header (POD config). Any validation logic goes to `SecureHttpServerConfig.cpp`.
- [libraries/HttpServerAdvanced/src/SecureHttpServer.h](../libraries/HttpServerAdvanced/src/SecureHttpServer.h): Split. Implementation details to `SecureHttpServer.cpp`.
- [libraries/HttpServerAdvanced/src/StandardHttpServer.h](../libraries/HttpServerAdvanced/src/StandardHttpServer.h): Split. Implementation to `StandardHttpServer.cpp`.
- [libraries/HttpServerAdvanced/src/StaticFileHandler.h](../libraries/HttpServerAdvanced/src/StaticFileHandler.h): Split. Move file-system operations to `StaticFileHandler.cpp`.
- [libraries/HttpServerAdvanced/src/StaticFilesBuilder.h](../libraries/HttpServerAdvanced/src/StaticFilesBuilder.h): Split. Keep public API in header; implementations in `StaticFilesBuilder.cpp`.
- [libraries/HttpServerAdvanced/src/FileLocator.h](../libraries/HttpServerAdvanced/src/FileLocator.h): Keep header (interface). Concrete default implementation belongs in `DefaultFileLocator.*`.
- [libraries/HttpServerAdvanced/src/RequestParser.h](../libraries/HttpServerAdvanced/src/RequestParser.h): Split. Parser logic to `RequestParser.cpp`.

### Already Split (examples)
- `Base64Encoder.*`, `BufferingHttpHandlerBase.*`, `ChunkedHttpResponseBodyStream.*`, `CoreServices.*`, `DefaultFileLocator.*` demonstrate the pattern: declarations in headers, implementations + heavy includes in .cpp.

## Include Hygiene
- In headers, include only what the declarations require (types used by value, base classes). Use forward declarations for pointer/ref members and parameter types.
- In .cpp files, include the header first, then include the concrete dependencies (`HttpContext.h`, `HttpResponse.h`, `HttpHandler.h`, `<FS.h>`, etc.).

## Verification
- Build with the existing task:
  - Arduino: Compile
- If the compiler reports missing symbols, move the corresponding include to the .cpp or add a forward declaration to the .h.
- Watch for ODR issues (multiple definitions); ensure globals/static data are defined in exactly one .cpp and declared `extern` in the header.

## Notes
- Templates and inline-only utilities should remain in headers to avoid link errors.
- When a header contains many unrelated classes (e.g., `HandlerImplementations.h`), split each class into its own `.h/.cpp` pair to minimize rebuilds and dependencies.
