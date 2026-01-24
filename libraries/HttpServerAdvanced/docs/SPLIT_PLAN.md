# HttpServerAdvanced Split Plan

This document defines the controlled, one-at-a-time file splitting workflow that runs before the main reorganization. The goal is to extract large or inline-implemented files into smaller declaration/implementation units while keeping the repository in a compilable state at every step.

## Goals

- Split large headers with inline implementations into declaration + implementation files.
- Verify compilation after each split while files remain in the flat `src/` layout.
- Use dependency scanning to detect and correct include paths during splits.
- Keep files in the flat `src/` directory during the split phase; moving to subfolders happens later during reorganization.

---

## Suggested File Splits and Priority

The following headers have non-trivial inline method implementations that should be extracted to `.cpp` files. Files are ordered by priority and complexity.

### Files to Split

| File | Lines | Suggested Split | Notes |
|------|-------|-----------------|-------|
| `NetClient.h` | 471 | Extract `NetClient.cpp` for `ClientImpl<T>` and `ServerImpl<T>` method bodies | Template implementations must stay in header, but non-template helpers can move |
| `HandlerMatcher.h` | 337 | Extract `HandlerMatcher.cpp` for `canHandle()`, `extractArgs()`, URL matching logic | Large matching implementations inline |
| `HttpHeader.h` | 373 | Extract `HttpHeaderNames.h` for constants; `HttpHeader.cpp` for factory methods | Constants bloat every TU |
| `MultipartFormDataHandler.h` | 343 | Extract `MultipartFormDataHandler.cpp` for parsing methods (`extractBoundary`, `parsePartHeaders`, `findBoundary`, `handleBodyChunk`) | Heavy parsing logic inline |
| `RequestParser.h` | 287 | Extract `RequestParser.cpp` for `http_parser` callback implementations and parsing state machine | Complex parser logic |
| `RawBodyHandler.h` | 120 | Extract `RawBodyHandler.cpp` for `handleStep()`, `handleBodyChunk()` | Moderate inline logic |
| `HttpHeaderCollection.h` | 118 | Extract `HttpHeaderCollection.cpp` for `set()`, `find()` methods | Collection manipulation inline |
| `PipelineError.h` | 109 | Extract `PipelineError.cpp` for `PipelineErrorName()`, `PipelineErrorMessage()` switch tables | Large switch statements inline |
| `FormBodyHandler.h` | 78 | Extract `FormBodyHandler.cpp` for body parsing logic | Small but has parsing |
| `JsonBodyHandler.h` | 81 | Extract `JsonBodyHandler.cpp` for body handling | Small but has logic |
| `BufferedStringBodyHandler.h` | 79 | Extract `BufferedStringBodyHandler.cpp` for body handling | Small but has logic |
| `StringView.h` | 274 | Extract `StringView.cpp` for non-trivial methods (`indexOf`, `lastIndexOf`, `substring`, `trim`, `replace`) | Many search/manipulation methods inline |
| `StringUtility.h` | 176 | Extract `StringUtility.cpp` for `compareTo`, `indexOf`, `lastIndexOf`, `replace` implementations | All functions are inline |
| `HttpRequest.h` | 242 | Extract `HttpRequest.cpp` for `handleStep()`, `sendResponse()`, `onError()`, callback implementations | Private method implementations inline |
| `HttpResponseIterators.h` | 191 | Extract `HttpResponseIterators.cpp` for `EnsureRequiredHeaders()`, `getHeaderDateValue()`, stream construction | Helper functions inline |
| `Streams.h` / `Streams.cpp` | 214 / 325 | Consider splitting into `ReadStream.h`, `MemoryStreams.h`, `ConcatStream.h`, `LazyStream.h` | Multiple stream types in one file |
| `UriView.h` | 123 | Extract `UriView.cpp` for `parse()` method | URI parsing logic inline |
| `HandlerBuilder.h` | 162 | Extract `HandlerBuilder.cpp` for destructor and `getFactory()` | Template class but some logic extractable |


---

## Split Workflow (strict)

1. Pick the highest-priority file to split.
2. Create new split files in-place inside the flat `src/` directory (e.g., add `HandlerMatcher.cpp` next to `HandlerMatcher.h`).
3. Update include directives in the split files so the project compiles while still flat (use local includes like `#include "HandlerMatcher.h"`).
4. Run the dependency scanner script to detect unresolved includes:

   ```powershell
   .\libraries\HttpServerAdvanced\scripts\generate_deps.ps1 -SrcDir .\libraries\HttpServerAdvanced\src
   ```

   - Fix include directives reported as unresolved or external where they should be project-local.
5. Compile (use the project's compile command). If compile fails, fix the split while files remain in `src/` or revert from `src.bak`.
6. Once the split compiles successfully in-place, update the comprehensive file mapping table in the main reorganization plan to list the new files as still residing in the flat `src/` directory.
7. Mark the file as completed in the split plan and proceed to the next split.

Rationale: Splitting first and confirming correctness while files remain in the flat `src/` layout prevents include path regressions and keeps the scope of each change small and reviewable. Moving files to their target subfolders happens later during the reorganization phase.

---

## Dependency Scanner Usage

- Script: `libraries/HttpServerAdvanced/scripts/generate_deps.ps1`
- Run after each in-place split and again after moving files to confirm no unresolved includes remain.
- The script prints a human-readable list of includes and attempts to resolve them to files under `src/`.

---

## Include Path Policy (for splits)

When splitting files in-place in `src/`, follow these rules:

- Use local, quoted includes for project headers (e.g., `#include "HandlerMatcher.h"`).
- Keep includes simple while files remain in the flat `src/` directory.
- Reserve `<...>` includes for external/system headers only.

Enforcement:

- Run `generate_deps.ps1` to detect unresolved includes before compiling.
- Relative paths for moved files will be applied during the reorganization phase.

---

## Small-Scope Checklist (per file)

1. Add split file(s) in `src/`.
2. Update include directives to local includes.
3. Run `generate_deps.ps1` and fix reported issues.
4. Compile and verify success.
5. Update the file mapping table in the reorganization plan to list new split files.
6. Mark the file as completed in this split plan.

---

## Notes

- Keep commits small and reference the file being split. This makes review and potential reverts simple.
- If a split uncovers larger dependency or API issues, pause and reevaluate split strategy before continuing.
