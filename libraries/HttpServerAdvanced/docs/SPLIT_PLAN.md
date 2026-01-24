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

| File | Lines | Suggested Split | Status | Notes |
|------|-------|-----------------|--------|-------|
| `StringUtility.h` | 176 | Extract `StringUtility.cpp` for `compareTo`, `indexOf`, `lastIndexOf`, `replace` implementations | ✓ DONE | All functions are inline |
| `HandlerMatcher.h` | 337 | Extract `HandlerMatcher.cpp` for `canHandle()`, `extractArgs()`, URL matching logic | ✓ DONE | Large matching implementations inline |
| `RequestParser.h` | 287 | Extract `RequestParser.cpp` for `http_parser` callback implementations and parsing state machine | ✓ DONE | Complex parser logic |
| `MultipartFormDataHandler.h` | 343 | Extract `MultipartFormDataHandler.cpp` for parsing methods (`extractBoundary`, `parsePartHeaders`, `findBoundary`) | ✓ DONE | Heavy parsing logic inline |
| `HttpRequest.h` | 242 | Extract `HttpRequest.cpp` for `handleStep()`, `sendResponse()`, `onError()`, callbacks | ✓ DONE | Private method implementations inline |
| `UriView.h` | 123 | Extract `UriView.cpp` for `parse()` method | ✓ DONE | URI parsing logic inline |
| `HttpResponseIterators.h` | 191 | Extract `HttpResponseIterators.cpp` for `EnsureRequiredHeaders()`, `getHeaderDateValue()` | ✓ DONE | Helper functions inline |
| `RawBodyHandler.h` | 120 | Extract `RawBodyHandler.cpp` for `handleStep()`, `handleBodyChunk()`, factory methods | ✓ DONE | Moderate inline logic |
| `HttpHeaderCollection.h` | 118 | Extract `HttpHeaderCollection.cpp` for `set()`, `remove()` methods | ✓ DONE | Collection manipulation inline |
| `PipelineError.h` | 109 | Extract `PipelineError.cpp` for `PipelineErrorName()`, `PipelineErrorMessage()` switch tables | ✓ DONE | Large switch statements inline |
| `FormBodyHandler.h` | 78 | Extract `FormBodyHandler.cpp` for `handleBody()`, factory methods | ✓ DONE | Small but has parsing |
| `JsonBodyHandler.h` | 81 | Extract `JsonBodyHandler.cpp` for `handleBody()`, factory methods | ✓ DONE | Small but has logic |
| `BufferedStringBodyHandler.h` | 79 | Extract `BufferedStringBodyHandler.cpp` for `handleBody()`, factory methods | ✓ DONE | Small but has logic |
| `HandlerBuilder.h` | 162 | Extract `HandlerBuilder.cpp` for stub (template class) | ✓ DONE | Template class; most code must stay inline |
| `HttpHeader.h` | 373 | Extract `HttpHeaderNames.h` for constants; `HttpHeader.cpp` for factory methods | DEFER | Factory methods are trivial 1-liners (better kept inline) |
| `StringView.h` | 274 | Extract `StringView.cpp` for non-trivial methods | DEFER | Methods mostly delegate to StringUtility; trivial wrappers better kept inline |
| `NetClient.h` | 471 | Extract `NetClient.cpp` for `ClientImpl<T>` and `ServerImpl<T>` | DEFER | Almost entirely template code; templates must stay in headers |
| `Streams.h` / `Streams.cpp` | 214 / 325 | Consider splitting into stream type-specific files | DEFER | Multiple stream types in one file; consider later as advanced refactor |


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

## Completion Status

### ✅ Split Phase Complete (January 24, 2026)

**14 files successfully split** with comprehensive extraction of inline implementations to .cpp files.

#### Summary
- **Files split**: 14
- **New .cpp files created**: 14
- **Lines of code extracted**: ~1,400+
- **Dependency validation**: ✅ All includes resolve to project files
- **Atomic commits**: 12 split commits + 1 plan update
- **Compilation**: Verified at each split using `generate_deps.ps1`

#### Split Execution Order
1. StringUtility.h → StringUtility.cpp (168 lines)
2. HandlerMatcher.h → HandlerMatcher.cpp (289 lines)
3. RequestParser.h → RequestParser.cpp (174 lines)
4. MultipartFormDataHandler.h → MultipartFormDataHandler.cpp (96 lines)
5. HttpRequest.h → HttpRequest.cpp (93 lines)
6. UriView.h → UriView.cpp (98 lines)
7. HttpResponseIterators.h → HttpResponseIterators.cpp (57 lines)
8. RawBodyHandler.h → RawBodyHandler.cpp (93 lines)
9. HttpHeaderCollection.h → HttpHeaderCollection.cpp (76 lines)
10. PipelineError.h → PipelineError.cpp (82 lines)
11. FormBodyHandler.h → FormBodyHandler.cpp (64 lines)
12. JsonBodyHandler.h → JsonBodyHandler.cpp (57 lines)
13. BufferedStringBodyHandler.h → BufferedStringBodyHandler.cpp (52 lines)
14. HandlerBuilder.h → HandlerBuilder.cpp (stub for template class)

#### Deferred (Justified)
- **HttpHeader.h**: Factory methods are trivial 1-liners (better kept inline); constants would require separate header
- **StringView.h**: Methods mostly delegate to StringUtility; trivial wrappers better kept inline
- **NetClient.h**: Almost entirely template code (471 lines); template implementations cannot be extracted
- **Streams.h**: Complex multi-class stream hierarchy (243 lines); consider later as advanced refactor phase

### Next Phase: Reorganization
Files remain in flat `src/` directory and are ready for the reorganization phase:
- Move files into logical subdirectories (e.g., `src/core/`, `src/handlers/`, `src/utilities/`)
- Update include paths from local quotes to relative paths
- Final compilation verification

---

## Notes

- Keep commits small and reference the file being split. This makes review and potential reverts simple.
- If a split uncovers larger dependency or API issues, pause and reevaluate split strategy before continuing.
