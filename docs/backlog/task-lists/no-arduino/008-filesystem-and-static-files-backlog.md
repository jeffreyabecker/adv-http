2026-03-23 - Copilot: removed the non-Arduino legacy `Compat::File : Stream` inheritance, kept the compatibility wrapper as a plain value handle, and marked the file-inherits-stream cleanup task complete.
2026-03-22 - Copilot: moved `StaticFilesBuilder` onto `IFileSystem` and added Arduino adapter helpers so sketch-facing `StaticFiles(fs::FS&, ...)` usage remains available without re-coupling the builder headers.
2026-03-22 - Copilot: migrated static-file locators and handler metadata usage onto `IFileSystem` and `IFile`, and added native POSIX coverage for the new seam.
2026-03-22 - Copilot: recorded the proposed `IFileSystem` and `IFile` seam, including `IByteChannel` inheritance and explicit open-mode selection.
2026-03-22 - Copilot: refreshed the phase summary after Phase 5 response work, marked the file-backed byte-source wrapper task complete, and aligned the backlog wording with the current `FileByteSource` implementation.
2026-03-21 - Copilot: created detailed Phase 6 filesystem and static files backlog.

# No-Arduino Phase 6 Filesystem And Static Files Backlog

## Summary

This phase narrows the filesystem dependency to the actual operations static-file serving needs and detaches file-backed responses from Arduino filesystem types. The repository already has `compat/FileSystem.h`, a POSIX adapter, and a `FileByteSource` bridge in the static-file handler, so file-backed responses already flow through the Phase 5 byte-source response path. The remaining work is to tighten adapter and metadata semantics, preserve board behavior through adapters, and broaden filesystem regression coverage.

## Goal / Acceptance Criteria

- Static-file serving depends on a minimal filesystem/file contract rather than raw Arduino `fs::FS` and `fs::File` behavior.
- File-backed responses work through the library-owned read abstraction without reintroducing Arduino inheritance into the core.
- Arduino filesystem behavior remains available through a thin adapter.
- Host-side tests can validate static-file behavior through POSIX-backed implementations.

## Tasks

### Interface Definition

- [x] Decide whether `src/compat/FileSystem.h` evolves in place or is replaced by a narrower `IFile` and `IFileSystem` contract.
- [x] Define the minimum file operations required for static-file serving: open, validity, directory check, close, size, name/path, last-write metadata, and readable-byte access.
- [x] Decide how file readability participates in the new stream or byte-source contract so filesystem work aligns with Phase 5.
- [x] Eliminate the assumption that `File` must be a subclass of the legacy compat `Stream` type.

Selected contract direction:
Replace `src/compat/FileSystem.h` in core-facing code with `IFileSystem::open(path, FileOpenMode)` returning `std::unique_ptr<IFile>`, where `IFile` extends `IByteChannel` and carries directory, size, path, and last-write metadata. Keep static-file handler logic read-only even though the file seam exposes broader channel behavior.

### Adapter Strategy

- [ ] Preserve direct Arduino aliasing under `ARDUINO` only where it does not leak raw framework types into the core-facing headers.
- [ ] Define the Arduino filesystem adapter layer and decide whether it wraps `fs::FS` and `fs::File` directly or continues to alias them through compatibility types.
- [ ] Review `src/compat/PosixFileAdapter.h` and align it with the final file-interface contract.
- [ ] Ensure the POSIX adapter exposes metadata with the same best-effort semantics expected by the static-file layer.

### Static File Locator Migration

- [x] Refactor `src/staticfiles/FileLocator.h` to return the new file abstraction.
- [x] Refactor `src/staticfiles/DefaultFileLocator.h` and `src/staticfiles/DefaultFileLocator.cpp` to use the narrowed filesystem interface.
- [x] Refactor `src/staticfiles/AggregateFileLocator.h` and `src/staticfiles/AggregateFileLocator.cpp` to aggregate the new file abstraction cleanly.
- [x] Preserve current index-file fallback and `.gz` fallback behavior during the migration.

### Static File Handler Migration

- [x] Refactor `src/staticfiles/StaticFileHandler.h` and `src/staticfiles/StaticFileHandler.cpp` so file-backed responses wrap the new file or byte-source abstraction instead of legacy `Stream` inheritance.
- [x] Rework ETag and last-modified helpers so they use the narrowed metadata contract.
- [x] Decide how file name or full path should be exposed for gzip detection without overspecifying the file interface.
- [ ] Preserve content-length, ETag, last-modified, and content-type behavior.

### Builder And Public Wiring

- [x] Refactor `src/staticfiles/StaticFilesBuilder.h` and `src/staticfiles/StaticFilesBuilder.cpp` so builder code consumes the narrowed filesystem contract.
- [x] Review any `WebServerBuilder` coupling introduced by static-file helper entrypoints and keep the filesystem seam from leaking raw Arduino headers back into those builders.
- [x] Ensure existing Arduino example ergonomics remain feasible through adapter helpers or transitional overloads.

### Host-Side Validation

- [ ] Expand native filesystem tests to cover the final file-interface contract, not just the current compatibility wrapper.
- [ ] Add tests for missing files, gzip fallback, directory index fallback, ETag generation, last-modified behavior, and content-length population.
- [ ] Decide whether fixture files should live under `test/support/fixtures/static_files/` or another dedicated directory.

### Documentation

- [ ] Document the final filesystem seam and adapter expectations so downstream contributors do not reintroduce raw Arduino FS usage into core headers.
- [ ] Update any docs that still imply static files require Arduino-only types in the core layer.

## Owner

TBD

## Priority

High

## References

- `src/compat/FileSystem.h`
- `src/compat/PosixFileAdapter.h`
- `src/staticfiles/FileLocator.h`
- `src/staticfiles/DefaultFileLocator.h`
- `src/staticfiles/DefaultFileLocator.cpp`
- `src/staticfiles/AggregateFileLocator.h`
- `src/staticfiles/AggregateFileLocator.cpp`
- `src/staticfiles/StaticFileHandler.h`
- `src/staticfiles/StaticFileHandler.cpp`
- `src/staticfiles/StaticFilesBuilder.h`
- `src/staticfiles/StaticFilesBuilder.cpp`
- `test/test_native/test_filesystem_compat.cpp`
- `test/test_native/test_filesystem_posix.cpp`
- `docs/plans/no-arduino/filesystem-interface-plan.md`
