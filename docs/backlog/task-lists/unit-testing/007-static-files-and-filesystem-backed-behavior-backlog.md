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

- [ ] Add tests for `DefaultFileLocator` path resolution, root handling, and default-document fallback.
- [ ] Add tests for `AggregateFileLocator` lookup order and first-match behavior across multiple locators.
- [ ] Verify path normalization and traversal-rejection expectations where the current implementation defines them.

### Static File Handler Behavior

- [ ] Add tests for `StaticFileHandlerFactory` matching behavior against request paths and methods.
- [ ] Verify missing-file behavior, directory behavior, and successful file response creation.
- [ ] Verify content-type lookup based on file extension through `HttpContentTypes`.
- [ ] Verify ETag and last-modified header behavior when file metadata is present or absent.

### Filesystem Fixture Strategy

- [ ] Decide which cases should use simple fake `IFile` or `IFileSystem` implementations and which should use host-backed POSIX fixtures.
- [ ] Reuse the existing filesystem compat helpers where possible instead of duplicating fake file models.
- [ ] Keep file-backed response assertions byte-oriented and independent of the pipeline loop.

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