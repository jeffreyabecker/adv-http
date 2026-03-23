2026-03-23 - Copilot: completed native static-file and filesystem-backed behavior coverage.
2026-03-23 - Copilot: created detailed Phase 7 static files and filesystem-backed behavior backlog.

# Unit Testing Phase 7 Static Files And Filesystem-Backed Behavior Backlog

## Summary

This phase extends the existing filesystem compatibility work into static-file serving and locator behavior. The codebase already has portable filesystem seams and POSIX-oriented native coverage for parts of that layer, but static-file lookup, fallback, and metadata-based response behavior still need stronger direct tests. The goal is to validate static asset behavior from fake or host-backed filesystems without coupling tests to sockets or device-specific FS implementations.

## Goal / Acceptance Criteria

- Static-file lookup and response logic are covered through the library-owned filesystem seam.
- Directory fallback, content-type selection, metadata-derived headers, and missing-file behavior are validated in native tests.
- Filesystem-backed response behavior can be exercised entirely in-process.

## Tasks

### File Locator Behavior

- [x] Add tests for `DefaultFileLocator` path resolution, root handling, and default-document fallback.
- [x] Add tests for `AggregateFileLocator` lookup order and first-match behavior across multiple locators.
- [x] Verify path normalization and traversal-rejection expectations where the current implementation defines them.

### Static File Handler Behavior

- [x] Add tests for `StaticFileHandlerFactory` matching behavior against request paths and methods.
- [x] Verify missing-file behavior, directory behavior, and successful file response creation.
- [x] Verify content-type lookup based on file extension through `HttpContentTypes`.
- [x] Verify ETag and last-modified header behavior when file metadata is present or absent.

### Filesystem Fixture Strategy

- [x] Decide which cases should use simple fake `IFile` or `IFileSystem` implementations and which should use host-backed POSIX fixtures.
- [x] Reuse the existing filesystem compat helpers where possible instead of duplicating fake file models.
- [x] Keep file-backed response assertions byte-oriented and independent of the pipeline loop.

## Notes

- Native coverage now lives in `test/test_native/test_static_files.cpp` and exercises `DefaultFileLocator`, `AggregateFileLocator`, and `StaticFileHandlerFactory` entirely in-process with fake filesystem handles.
- The current implementation defines query trimming, trailing-slash normalization, gzip fallback, directory index fallback, and prefix filtering. It does not currently define explicit traversal rejection semantics, so this phase freezes the implemented normalization behavior rather than inventing new policy.
- Existing host-backed POSIX tests remain the primary coverage for real filesystem adapter behavior; this phase keeps static-file assertions fake-backed and byte-oriented by using `TestSupport::CaptureResponse(...)`.

## Owner

TBD

## Priority

Medium

## References

- `src/staticfiles/FileLocator.h`
- `src/staticfiles/DefaultFileLocator.h`
- `src/staticfiles/DefaultFileLocator.cpp`
- `src/staticfiles/AggregateFileLocator.h`
- `src/staticfiles/AggregateFileLocator.cpp`
- `src/staticfiles/StaticFileHandler.h`
- `src/staticfiles/StaticFileHandler.cpp`
- `src/staticfiles/StaticFilesBuilder.h`
- `src/staticfiles/StaticFilesBuilder.cpp`
- `src/compat/IFileSystem.h`
- `src/core/HttpContentTypes.h`
- `test/test_native/test_filesystem_compat.cpp`
- `test/test_native/test_filesystem_posix.cpp`
- `test/test_native/test_static_files.cpp`