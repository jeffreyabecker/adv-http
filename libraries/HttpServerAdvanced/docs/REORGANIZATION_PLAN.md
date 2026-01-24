# HttpServerAdvanced Source Reorganization Plan

## Executive Summary

This document outlines a plan to reorganize the `src/` folder of the HttpServerAdvanced library into logical subfolders based on functionality. The goal is to improve code discoverability, maintainability, and organization while **not introducing new namespaces**.

---

## Current State Analysis

The `src/` folder currently contains **77 files** (headers and implementations) in a flat structure. Files cover diverse functionality including:

- Core HTTP types (status, headers, methods)
- Request/response handling
- Pipeline and connection management
- Handler registration and routing
- Body parsing (form, JSON, multipart)
- Stream utilities
- Static file serving
- Security features (authentication, CORS)
- Server configuration and builders

---

## Proposed Folder Structure

```
src/
├── HttpServerAdvanced.h          # Root include file (unchanged)
├── core/                          # Core HTTP types and fundamentals
├── handlers/                      # Request handlers and body processors
├── pipeline/                      # Connection/request processing pipeline
├── response/                      # Response types and builders
├── routing/                       # URL matching and handler registration
├── security/                      # Authentication and security middleware
├── server/                        # Server implementations and configuration
├── staticfiles/                   # Static file serving components
├── streams/                       # Stream utilities and encoding
└── util/                          # General utilities and helpers
```

---

## Comprehensive File Mapping

This table provides a complete alphabetical listing of all source files (.h and .cpp) and their destination folders, with status tracking for the migration.

**Status Legend:** ✓ = Moved | ○ = Pending

| Current File | Destination Folder | Status | Purpose |
|--------------|-------------------|--------|---------|
| `AggregateFileLocator.cpp` | `staticfiles/` | ○ | Multi-source file locator implementation |
| `AggregateFileLocator.h` | `staticfiles/` | ○ | Multi-source file locator |
| `Base64Stream.cpp` | `streams/` | ○ | Base64 encoding/decoding implementation |
| `Base64Stream.h` | `streams/` | ○ | Base64 encoding/decoding streams |
| `BasicAuthentication.h` | `security/` | ○ | HTTP Basic authentication |
| `Buffer.h` | `core/` | ○ | Buffer type for request handling |
| `BufferedStringBodyHandler.cpp` | `handlers/` | ○ | Buffering handler implementation |
| `BufferedStringBodyHandler.h` | `handlers/` | ○ | Body handler buffering to string |
| `BufferingHttpHandlerBase.cpp` | `handlers/` | ○ | Buffering handler base implementation |
| `BufferingHttpHandlerBase.h` | `handlers/` | ○ | Base class for buffering handlers |
| `ChunkedHttpResponseBodyStream.cpp` | `response/` | ○ | Chunked encoding implementation |
| `ChunkedHttpResponseBodyStream.h` | `response/` | ○ | Chunked transfer encoding stream |
| `CrossOriginRequestSharing.h` | `security/` | ○ | CORS support |
| `DefaultFileLocator.cpp` | `staticfiles/` | ○ | Default file locator implementation |
| `DefaultFileLocator.h` | `staticfiles/` | ○ | LittleFS-based file locator |
| `Defines.h` | `core/` | ○ | Library-wide defines and macros |
| `FileLocator.h` | `staticfiles/` | ○ | File locator interface |
| `FormBodyHandler.cpp` | `handlers/` | ○ | Form data handler implementation |
| `FormBodyHandler.h` | `handlers/` | ○ | Form data parsing handler |
| `FormResponse.cpp` | `response/` | ○ | Form response implementation |
| `FormResponse.h` | `response/` | ○ | URL-encoded response builder |
| `HandlerBuilder.cpp` | `routing/` | ○ | Handler builder implementation (stub) |
| `HandlerBuilder.h` | `routing/` | ○ | Fluent handler configuration builder |
| `HandlerMatcher.cpp` | `routing/` | ○ | URL/method matching implementation |
| `HandlerMatcher.h` | `routing/` | ○ | URL/method matching logic |
| `HandlerProviderRegistry.cpp` | `routing/` | ○ | Handler registry implementation |
| `HandlerProviderRegistry.h` | `routing/` | ○ | Handler registration and lookup |
| `HandlerRestrictions.h` | `handlers/` | ○ | Handler access restrictions |
| `HandlerTypes.h` | `handlers/` | ○ | Handler type definitions |
| `HttpContentTypes.h` | `core/` | ○ | MIME type constants |
| `HttpHandler.h` | `handlers/` | ○ | Concrete HTTP handler |
| `HttpHeader.h` | `core/` | ○ | Header name constants and types |
| `HttpHeaderCollection.cpp` | `core/` | ○ | Header collection implementation |
| `HttpHeaderCollection.h` | `core/` | ○ | Header container |
| `HttpMethod.h` | `core/` | ○ | HTTP method enumeration |
| `HttpPipeline.cpp` | `pipeline/` | ○ | Pipeline implementation |
| `HttpPipeline.h` | `pipeline/` | ○ | Request processing pipeline |
| `HttpRequest.cpp` | `server/` | ○ | Request implementation |
| `HttpRequest.h` | `server/` | ○ | Request object |
| `HttpRequestHandlerFactory.h` | `server/` | ○ | Factory for creating handlers |
| `HttpRequestPhase.h` | `core/` | ○ | Request processing phase enum |
| `HttpResponse.cpp` | `response/` | ○ | Response implementation |
| `HttpResponse.h` | `response/` | ○ | Core response class |
| `HttpResponseBodyStream.h` | `response/` | ○ | Response body stream interface |
| `HttpResponseIterators.cpp` | `response/` | ○ | Response iteration helpers implementation |
| `HttpResponseIterators.h` | `response/` | ○ | Response iteration helpers |
| `HttpServerAdvanced.h` | *(root)* | ✓ | Main library include - stays at root |
| `HttpServerBase.cpp` | `server/` | ○ | Base server implementation |
| `HttpServerBase.h` | `server/` | ○ | Abstract server base class |
| `HttpStatus.h` | `core/` | ○ | HTTP status codes |
| `HttpTimeouts.h` | `core/` | ○ | Timeout configuration |
| `HttpUtility.cpp` | `util/` | ✓ | Utility function implementation |
| `HttpUtility.h` | `util/` | ✓ | Web encoding/decoding utilities |
| `IHandlerProvider.h` | `handlers/` | ○ | Handler provider interface |
| `IHttpHandler.h` | `handlers/` | ○ | Handler interface |
| `IHttpRequestHandlerFactory.h` | `server/` | ○ | Handler factory interface |
| `IHttpResponse.h` | `response/` | ○ | Response interface |
| `IPipelineHandler.h` | `pipeline/` | ○ | Pipeline handler interface |
| `Iterators.h` | `streams/` | ○ | Stream iterator base classes |
| `JsonBodyHandler.cpp` | `handlers/` | ○ | JSON body handler implementation |
| `JsonBodyHandler.h` | `handlers/` | ○ | JSON body parsing handler |
| `JsonResponse.cpp` | `response/` | ○ | JSON response implementation |
| `JsonResponse.h` | `response/` | ○ | JSON response builder |
| `KeyValuePairView.h` | `util/` | ✓ | Key-value pair container view |
| `MultipartFormDataHandler.cpp` | `handlers/` | ○ | Multipart form handler implementation |
| `MultipartFormDataHandler.h` | `handlers/` | ○ | Multipart form parsing |
| `NetClient.h` | `pipeline/` | ○ | Network client abstraction |
| `PipelineError.cpp` | `pipeline/` | ○ | Pipeline error implementation |
| `PipelineError.h` | `pipeline/` | ○ | Pipeline error types |
| `PipelineHandleClientResult.h` | `pipeline/` | ○ | Pipeline result enumeration |
| `ProviderRegistryBuilder.h` | `routing/` | ○ | Registry builder helper |
| `RawBodyHandler.cpp` | `handlers/` | ○ | Raw body handler implementation |
| `RawBodyHandler.h` | `handlers/` | ○ | Raw body access handler |
| `RequestParser.cpp` | `pipeline/` | ○ | HTTP request parsing implementation |
| `RequestParser.h` | `pipeline/` | ○ | HTTP request parsing |
| `SecureHttpServer.h` | `server/` | ○ | HTTPS server implementation |
| `SecureHttpServerConfig.h` | `server/` | ○ | HTTPS configuration |
| `StandardHttpServer.h` | `server/` | ○ | HTTP server implementation |
| `StaticFileHandler.cpp` | `staticfiles/` | ○ | Static file handler implementation |
| `StaticFileHandler.h` | `staticfiles/` | ○ | Handler for serving static files |
| `StaticFilesBuilder.cpp` | `staticfiles/` | ○ | Static files builder implementation |
| `StaticFilesBuilder.h` | `staticfiles/` | ○ | Fluent static file configuration |
| `Streams.cpp` | `streams/` | ○ | Stream implementations |
| `Streams.h` | `streams/` | ○ | Stream types (Memory, Concat, etc.) |
| `StringResponse.cpp` | `response/` | ○ | String response implementation |
| `StringResponse.h` | `response/` | ○ | Simple string response builder |
| `StringUtility.cpp` | `util/` | ✓ | String utility implementation |
| `StringUtility.h` | `util/` | ✓ | String comparison helpers |
| `StringView.h` | `util/` | ✓ | Non-owning string view |
| `UriStream.cpp` | `streams/` | ○ | URI encoding implementation |
| `UriStream.h` | `streams/` | ○ | URI encoding/decoding streams |
| `UriView.cpp` | `util/` | ✓ | URI parsing implementation |
| `UriView.h` | `util/` | ✓ | URI parsing and components |
| `WebServer.h` | `server/` | ○ | High-level WebServer class |
| `WebServerBuilder.h` | `server/` | ○ | WebServer fluent builder |
| `WebServerConfig.h` | `server/` | ○ | WebServer configuration |

### File Count by Destination

| Destination | Total Files | .h Files | .cpp Files | Moved |
|-------------|-------------|----------|-----------|-------|
| *(root)* | 1 | 1 | 0 | ✓ |
| `core/` | 10 | 9 | 1 | ○ |
| `handlers/` | 20 | 12 | 8 | ○ |
| `pipeline/` | 12 | 7 | 5 | ○ |
| `response/` | 20 | 10 | 10 | ○ |
| `routing/` | 8 | 5 | 3 | ○ |
| `security/` | 2 | 2 | 0 | ○ |
| `server/` | 15 | 10 | 5 | ○ |
| `staticfiles/` | 9 | 5 | 4 | ○ |
| `streams/` | 8 | 5 | 3 | ○ |
| `util/` | 8 | 5 | 3 | ✓ |
| **Total** | **113** | **61** | **52** | **2/11** |

**Note:** File count increased from original plan (82 files) due to .cpp files created during split phase. Total now includes all implementation files produced by the split phase.
| `streams/` | 7 | Base64Stream.cpp/h, Iterators.h, Streams.cpp/h, UriStream.cpp/h |
| `util/` | 6 | HttpUtility.cpp/h, KeyValuePairView.h, StringUtility.h, StringView.h, UriView.h |
| **Total** | **82** | |

---


## Updated Root Include File

The `HttpServerAdvanced.h` file will need to be updated to include from the new subfolder paths:

```cpp
// HttpServerAdvanced.h - Updated includes

#include "./server/StandardHttpServer.h"
#include "./server/SecureHttpServer.h"
#include "./server/WebServerBuilder.h"
#include "./server/WebServer.h"

#include "./core/HttpContentTypes.h"
#include "./core/HttpStatus.h"
#include "./core/HttpMethod.h"
#include "./core/HttpHeader.h"

#include "./handlers/HandlerTypes.h"
#include "./routing/HandlerMatcher.h"

#include "./response/StringResponse.h"
#include "./response/JsonResponse.h"
#include "./response/FormResponse.h"

#include "./security/BasicAuthentication.h"
#include "./security/CrossOriginRequestSharing.h"

#include "./staticfiles/StaticFilesBuilder.h"

#include "./util/HttpUtility.h"
#include "./util/UriView.h"
#include "./util/KeyValuePairView.h"
```

---

## Split Plan (pre-reorganization)

The file splitting process runs before the main reorganization. See `docs/SPLIT_PLAN.md` for the full split workflow, suggested splits, dependency scanner usage, include path policy, and a per-file checklist. Perform all splits per that plan and verify compilation before continuing with the reorganization steps below.

## Migration Strategy

### Important Note: Arduino Build System
The Arduino build system **recursively includes all `.h` and `.cpp` files** from the `src/` folder and its subfolders. This means:
- Files cannot coexist in both old and new locations
- Duplicate symbol errors will occur if original files are not deleted immediately after copying
- Include path updates must happen before deletion

### Prerequisites
Ensure all required file splits have been completed and validated in-place per `docs/SPLIT_PLAN.md` before starting the migration below.

### Phase 1: Create Folder Structure
1. Create all subdirectories under `src/`

### Phase 2: Move and Update Files (with immediate deletion)
For each logical group (in dependency order):
1. Move the already-split and validated files from the flat `src/` directory into their target subfolder locations.
2. Update ALL include paths in the moved files to use correct relative paths:
    - Same folder: `#include "./FileName.h"`
    - Parent folder: `#include "../core/FileName.h"`
3. **Immediately delete original flat `src/` files** after the move to avoid duplicate-symbol issues with the Arduino build system.
4. Run the compile check after each move. If compile fails, restore the affected files from `src.bak`, revert the move, fix include paths, and retry.

**Dependency order (process in this sequence):**
1. `util/` - No internal dependencies
2. `core/` - Depends on util/
3. `streams/` - Depends on core/, util/
4. `handlers/` - Depends on core/, response/
5. `response/` - Depends on core/, streams/
6. `routing/` - Depends on handlers/, core/
7. `pipeline/` - Depends on core/, handlers/
8. `security/` - Depends on handlers/, response/
9. `staticfiles/` - Depends on handlers/, core/
10. `server/` - Depends on pipeline/, routing/, handlers/

### Phase 3: Update Root Include File
1. Update `HttpServerAdvanced.h` to use new paths
2. Final comprehensive compilation test

---

## Include Path Conventions

After reorganization, include paths should follow these patterns:

**From within a subfolder (e.g., `handlers/FormBodyHandler.h`):**
```cpp
#include "../core/HttpHeader.h"
#include "../util/HttpUtility.h"
#include "./BufferingHttpHandlerBase.h"  // same folder
```

**From root include file:**
```cpp
#include "./server/WebServer.h"
#include "./handlers/HandlerTypes.h"
```

**Include Path Policy (After Moving Files)**

When files are moved into the new subfolder layout, all project header includes must use explicit relative paths and quote-includes (double quotes) so the Arduino build system and IDE resolve them predictably.

Rules:
- **Always use relative paths** to target files (never absolute or workspace-root-based paths).
- **Use quote-includes** for project headers: `#include "../core/HttpHeader.h"` or `#include "./MyLocalHeader.h"`.
- **Same folder:** prefer `#include "MyHeader.h"` or `#include "./MyHeader.h"`.
- **Parent or sibling folders:** use `..` to traverse up: `#include "../core/HttpHeader.h"`, `#include "../util/KeyValuePairView.h"`.
- **From root include (`HttpServerAdvanced.h`)** use `./` prefixes to reference subfolders: `#include "./server/WebServer.h"`.
- **Do not use `<...>`** for project headers; reserve angle-bracket includes for external/system libraries.

Examples:
```cpp
// In handlers/FormBodyHandler.h (same folder)
#include "BufferingHttpHandlerBase.h"

// In handlers/FormBodyHandler.cpp (needs core headers)
#include "../core/HttpHeader.h"

// In src/HttpServerAdvanced.h (root include)
#include "./server/StandardHttpServer.h"
```

Enforcement:
- Update include paths immediately after moving files.
- Use a single grep/regex pass to validate no remaining old-path includes exist before deleting originals.
- Run `libraries/HttpServerAdvanced/scripts/generate_deps.ps1 -SrcDir .\src` after splitting in-place and after moving files to detect unresolved includes and confirm project-file resolution.
- If the build system or compile step reports missing headers, check relative paths first before reverting moves.

---



## Summary Table

| Folder | File Count | Primary Purpose |
|--------|------------|-----------------|
| `core/` | 9 | HTTP fundamentals (status, headers, methods) |
| `handlers/` | 12 | Request/body processing handlers |
| `pipeline/` | 7 | Connection and request processing |
| `response/` | 13 | Response building and streaming |
| `routing/` | 5 | URL matching and handler registration |
| `security/` | 2 | Authentication and CORS |
| `server/` | 11 | Server implementations and config |
| `staticfiles/` | 9 | Static file serving |
| `streams/` | 7 | Stream utilities and encoding |
| `util/` | 6 | General utilities |
| **Total** | **81** | (includes some suggested splits) |

---

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| Duplicate symbols if old files left in place | Delete originals immediately after copying and updating paths |
| Breaking external includes | Update public-facing `HttpServerAdvanced.h` to new paths |
| Circular dependencies | Process folders in dependency order; validate includes |
| IDE/editor confusion | Clear folder naming; update `.vscode/` settings if needed |
| Incomplete include path updates | Use find-and-replace with regex to catch all includes before deletion |

---

## Next Steps

1. Review and approve this plan
2. Create a backup of current `src/` folder
3. Execute Phase 1-4 of migration strategy
4. Test with example sketches
5. Optionally execute Phase 5 (file splits)
6. Update documentation to reflect new structure
