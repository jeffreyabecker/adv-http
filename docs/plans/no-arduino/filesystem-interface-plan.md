# HttpServerAdvanced Filesystem Interface Plan

## Objective

Define a small, C++17-friendly filesystem access interface to replace direct dependencies on Arduino `FS`/`File` in the library's static-file serving code. The interface should be minimal (only the operations the static-file handlers need), easily adapted to Arduino `FS`/`File` and to POSIX `FILE*`/`stat`, and support clear ownership and testability.

## Summary of findings

From scanning the codebase (notably `src/staticfiles/*` and `src/compat/FileSystem.h`) the static-file path expects the following filesystem operations:

- open(path, mode) -> file handle (movable, checkable for validity)
- file validity test (boolean)
- stream reads: `available()`, `read()`, `peek()`
- metadata: `size()`, `fullName()` or `name`, `getLastWrite()`
- `isDirectory()`
- `close()`

Most code uses the file handle as a stream (wrapping it as a `Stream`), and also reads `size()` and `getLastWrite()` for headers (Content-Length, ETag). Directory checks and index-file fallback are used by the `DefaultFileLocator`.

Operations not currently used in static-file paths but possibly useful later: `exists()`, `listDir()`, `remove()`, `rename()`, `seek()`.

## Goals for the interface

- Satisfy the static-file handler needs without exposing Arduino types in public headers.
- Keep the interface small and testable.
- Return ownership as `std::unique_ptr<IFile>` so callers manage lifetimes explicitly.
- Provide best-effort metadata (empty / zero when unavailable).
- Provide adapters for Arduino `FS`/`File` and POSIX `FILE*` + `stat`.

## Proposed interfaces

Create a header `src/compat/IFileSystem.h` (or similar) exposing `IFile` and `IFileSystem` in a `HttpServerAdvanced::Compat` namespace.

Suggested content:

```cpp
// src/compat/IFileSystem.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace HttpServerAdvanced::Compat {

class IFile {
public:
    virtual ~IFile() = default;
    virtual explicit operator bool() const = 0; // truthy when open/readable
    // Returns a small readiness result instead of an overloaded int. See stream replacement plan
    // for `AvailableResult` semantics: `HasBytes`, `Exhausted`, `TemporarilyUnavailable`, `Error`.
    // The `count` field is valid only when the state is `HasBytes`.
    virtual AvailableResult available() = 0;
    virtual int read() = 0;                     // read single byte, -1 on EOF/error
    virtual int read(unsigned char* buffer, std::size_t len) = 0; // read up to len bytes
    virtual int peek() = 0;
    virtual void close() = 0;
    virtual std::size_t size() const = 0;      // total size in bytes (0 if unknown)
    virtual std::string fullName() const = 0;  // file path or empty
    virtual uint32_t getLastWrite() const = 0; // epoch seconds, 0 if unknown
    virtual bool isDirectory() const = 0;
};

class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    // open for read-only use; mode kept as const char* for future extension
    virtual std::unique_ptr<IFile> open(const std::string &path, const char *mode) = 0;
};

} // namespace HttpServerAdvanced::Compat
```

Design notes:

- `IFileSystem::open` returns `nullptr` when the file is not present or cannot be opened.
- `IFile` is used through `std::unique_ptr<IFile>` to express single ownership. This maps naturally to the static-file code which passes `File` objects by value but treats them as unique handles.
- `read()` returns `-1` on EOF/error to mirror existing `Stream` semantics used in the codebase; `read(buffer,len)` returns number of bytes read or `-1` on unrecoverable error.
- `available()` previously returned an `int` with overloaded meanings. The new `AvailableResult` separates readiness from quantity and reduces ambiguity.
- If an underlying platform cannot provide an accurate available count, adapters should map best-effort to `TemporarilyUnavailable` (count 0) or to `HasBytes` with an approximation when possible. See `docs/plans/no-arduino/stream-replacement-plan.md` for mapping rules.
- `size()` is used for `Content-Length`. If size is unknown, returning `0` is acceptable but should be documented.

## Adapter mapping

Arduino `FS` / `File` adapter:

- `open(path, "r")` -> call `fs.open(path, "r")` and wrap into an `IFile` implementation that forwards `available()`, `read()`, `peek()`, `close()`, `size()`, `isDirectory()`.
- `fullName()` can map to `file.name()` when available; otherwise return the requested `path` string stored by the adapter.
- `getLastWrite()` is not available in all Arduino FS implementations; return `0` when unsupported or use the underlying API if provided (LittleFS/VFS may supply metadata).

POSIX `FILE*` + `stat` adapter:

- `open(path, "r")` -> `fopen(path, "rb")`; wrap `FILE*` in IFile.
- `read()` -> `fgetc()` or `fread()` wrapper; `read(buffer,len)` uses `fread()`.
- `available()` can be implemented as `size() - ftell(file)` when `fseek/ftell` work; otherwise return 0.
- `size()` -> use `stat(path, &st)` and return `st.st_size`.
- `getLastWrite()` -> map from `st.st_mtime`.
- `isDirectory()` -> `S_ISDIR(st.st_mode)` from `stat`.

## Integration plan

1. Add `src/compat/IFileSystem.h` with the `IFile`/`IFileSystem` definitions.
2. Update `src/staticfiles` to accept an `IFileSystem &` (or `std::shared_ptr<IFileSystem>`) instead of an Arduino `FS &`.
   - Keep a transitional adapter layer so existing example code can pass `LittleFS` via a small wrapper that implements `IFileSystem`.
3. Provide two adapters in `src/compat/adapters`:
   - `ArduinoFsAdapter` wrapping `fs::FS`/`fs::File`.
   - `PosixFileAdapter` for native builds/tests.
4. Update `DefaultFileLocator` and `StaticFileHandler` to use `std::unique_ptr<IFile>` returned by `IFileSystem::open`.
5. Add unit tests for content-length, ETag extraction, index-file fallback, and directory handling using the POSIX adapter in native builds.

## Backwards compatibility and deployment

- Provide a small transitional header `src/compat/AdapterHelpers.h` with convenience functions to build an `ArduinoFsAdapter` from an `fs::FS &` so examples keep working with minimal changes (e.g., `StaticFiles(LittleFS)` can be adapted by the example harness to `StaticFiles(makeArduinoFsAdapter(LittleFS))`).
- Document adapter creation in `docs/EXAMPLES.md` and update examples incrementally.

## Acceptance criteria

- All static-file examples compile using the Arduino adapter (no Arduino types in public headers of the library core).
- `Content-Length` and `ETag` headers are correct when `size()` and `getLastWrite()` are available.
- Directory index fallback behavior matches current behavior.
- Unit tests exercise the public `IFile`/`IFileSystem` behavior via the POSIX adapter.

## Next steps

- I can implement `src/compat/IFileSystem.h` and the `ArduinoFsAdapter` next, then update `DefaultFileLocator` to accept the new interface. Which adapter do you want first (Arduino or POSIX)?


*Document generated by Copilot assistant.*
