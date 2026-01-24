# HttpServerAdvanced Split Plan

This document defines the controlled, one-at-a-time file splitting workflow that must run before the main reorganization (moving files into folders). The goal is to extract large or inline-implemented files into smaller declaration/implementation units while keeping the repository in a compilable state at every step.

## Goals

- Split large headers with inline implementations into declaration + implementation files.
- Verify compilation after each split while files remain in the flat `src/` layout.
- Use dependency scanning to detect and correct include paths before moving files.
- Only move files into the new folder structure after the split compiles in-place.

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
3. Update include directives in the `src/` copies so the project compiles while still flat (use local includes like `#include "HandlerMatcher.h"`).
4. Run the dependency scanner script to detect unresolved includes:

   ```powershell
   .\libraries\HttpServerAdvanced\scripts\generate_deps.ps1 -SrcDir .\libraries\HttpServerAdvanced\src
   ```

   - Fix include directives reported as unresolved or external where they should be project-local.
5. Compile (use the project's compile command). If compile fails, fix the split while files remain in `src/` or revert from `src.bak`.
6. When the split compiles successfully in-place, move the split files and any directly dependent files to the appropriate destination subfolder and update include paths to the new relative paths (e.g., `#include "../core/HttpHeader.h"`).
7. Immediately delete the original flat-file versions from `src/` to avoid duplicate-symbol issues with Arduino's recursive build.
8. Run the dependency scanner again and then compile. If it fails, restore from `src.bak`, revert the move, and fix issues before retrying.
9. Once compilation succeeds, update the comprehensive file mapping table in the main reorganization plan to list the new files and destinations, then proceed to the next split.

Rationale: Splitting first and confirming correctness while files remain in the flat `src/` layout prevents include path regressions and keeps the scope of each change small and reviewable.

---

## Dependency Scanner Usage

- Script: `libraries/HttpServerAdvanced/scripts/generate_deps.ps1`
- Run after each in-place split and again after moving files to confirm no unresolved includes remain.
- The script prints a human-readable list of includes and attempts to resolve them to files under `src/`.

---

## Include Path Policy (for splits and subsequent moves)

When splitting files and later moving them, follow these rules:

- Use relative, quoted includes for project headers (e.g., `#include "../core/HttpHeader.h"`).
- When files remain in `src/` during splitting, prefer local includes (e.g., `#include "HandlerMatcher.h"`).
- Use `"./"` for same-folder includes when moved into a subfolder.
- Reserve `<...>` includes for external/system headers only.

Enforcement:

- Run `generate_deps.ps1` to detect unresolved includes before compiling.
- Use grep/regex to ensure no old absolute or root-based includes remain before deleting originals.

---

## Small-Scope Checklist (per file)

1. Add split file(s) in `src/`.
2. Update include directives locally.
3. Run `generate_deps.ps1` and fix reported issues.
4. Compile.
5. Move validated split file(s) to target subfolder.
6. Update includes to relative paths and run `generate_deps.ps1` again.
7. Compile; if successful, delete originals and update mapping in reorg plan.

---

## Notes

- Keep commits small and reference the file being split. This makes review and potential reverts simple.
- If a split uncovers larger dependency or API issues, pause and reevaluate split strategy before continuing.
