# HttpServerAdvanced Filesystem Interface Plan

## Objective

Define a small, C++17-friendly filesystem seam for static-file serving that removes direct dependencies on Arduino `FS` and `File` from the core while aligning file reads with the existing `IByteSource` response pipeline.

## Summary of findings

The current static-file path only needs a narrow read-only contract:

- filesystem open by path
- readable bytes through the Phase 5 byte-source seam
- directory detection for index fallback
- explicit close for early release
- size metadata for `Content-Length`
- resolved path metadata for gzip detection and content-type lookup
- last-write metadata for `ETag` and `Last-Modified`

The current code does not need write-side byte-channel behavior for file-backed responses. It also does not benefit from moving metadata lookups back onto the filesystem object once a file has been opened.

## Design decisions

- Introduce a dedicated `IFileSystem` and `IFile` seam rather than evolving `compat/FileSystem.h` in place.
- Make `IFile` inherit from `IByteChannel` so filesystem handles can participate in the byte-stream seam uniformly even though static-file responses currently use only the read side.
- Keep metadata on the opened `IFile`, not on `IFileSystem`, so headers are derived from the same opened resource that provides the bytes.
- Use `std::unique_ptr<IFile>` for ownership and use `nullptr` to represent open failure instead of an invalid sentinel file object.
- Use `std::optional` for metadata that may be unavailable instead of `0` or empty-string sentinel values.
- Keep the static-file implementation focused on read-side behavior even though the file seam itself exposes read/write capability.

## Concrete proposed interfaces

Create a header such as `src/compat/IFileSystem.h` with the following contract:

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include "../streams/ByteStream.h"

namespace HttpServerAdvanced
{
    namespace Compat
    {
        enum class FileOpenMode
        {
            Read,
            Write,
            ReadWrite
        };

        class IFile : public IByteChannel
        {
        public:
            ~IFile() override = default;

            virtual bool isDirectory() const = 0;
            virtual void close() = 0;
            virtual std::optional<std::size_t> size() const = 0;
            virtual std::string_view path() const = 0;
            virtual std::optional<uint32_t> lastWriteEpochSeconds() const = 0;
        };

        class IFileSystem
        {
        public:
            virtual ~IFileSystem() = default;

            virtual std::unique_ptr<IFile> open(std::string_view path, FileOpenMode mode) = 0;
        };
    }
}
```

## Contract notes

- `IFileSystem::open()` returns `nullptr` when the path does not exist, cannot be opened in the requested mode, or is otherwise unavailable.
- `IFile` remains valid until it is closed or destroyed. Destruction should release any underlying handle even if `close()` was never called.
- `IFile` inherits `available()`, `read()`, `peek()`, `write()`, and `flush()` from `IByteChannel`.
- Static-file code should continue to use only the read-side operations even though the interface also supports writes.
- `size()` returns `std::nullopt` when the adapter cannot provide a stable byte count. Static-file code can omit `Content-Length` in that case or make an explicit policy decision later.
- `path()` returns the resolved path for the opened resource, not the original request URL. This is the value used for gzip detection and content-type lookup.
- `lastWriteEpochSeconds()` returns `std::nullopt` when the underlying platform cannot provide usable last-write metadata.
- `isDirectory()` remains part of `IFile` because directory fallback is based on the opened resource, not on a second filesystem query.
- `FileOpenMode` should be mapped to the narrowest platform mode string or flags that preserve intended behavior. Adapters may reject unsupported mode combinations by returning `nullptr`.

## Why this shape fits the current code

This contract maps directly onto the current static-file call flow:

1. `DefaultFileLocator` opens a candidate path, checks `isDirectory()`, and retries with `.gz` or `/index.html` variants.
2. `StaticFileHandler` reads metadata from the opened file to build headers.
3. The response pipeline already consumes `std::unique_ptr<IByteSource>`, so `IFile` can still flow into that path through its read-side inheritance while remaining compatible with broader byte-channel usage elsewhere.

This avoids one weaker alternative:

- Putting metadata methods on `IFileSystem`, which would force a second lookup path and risks metadata drifting from the opened handle.

## Mapping from the current compat seam

Current `File` and `FS` behavior in `src/compat/FileSystem.h` maps to the proposed seam as follows:

- `FS::open(path, mode)` becomes `IFileSystem::open(path, FileOpenMode)`
- `File::available()`, `read()`, `peek()`, `write()`, and `flush()` move under `IByteChannel`
- `File::isDirectory()` stays on `IFile`
- `File::close()` stays on `IFile`
- `File::size()` becomes `IFile::size()` returning `std::optional<std::size_t>`
- `File::fullName()` becomes `IFile::path()` returning `std::string_view`
- `File::getLastWrite()` becomes `IFile::lastWriteEpochSeconds()` returning `std::optional<uint32_t>`
- `File` no longer inherits from the legacy compat `Stream`
- invalid default-constructed file handles are replaced by `nullptr`

## Adapter mapping

Arduino adapter:

- Wrap `fs::FS` behind an `IFileSystem` implementation exposing `open()`.
- Wrap `fs::File` behind an `IFile` implementation exposing channel operations, `isDirectory()`, `close()`, and metadata.
- Use the underlying file name API when available; otherwise store the resolved path string in the adapter.
- Return `std::nullopt` for `lastWriteEpochSeconds()` when the board filesystem cannot supply it.
- Return `std::nullopt` for `size()` only if the underlying filesystem genuinely cannot provide a stable size.
- Map `FileOpenMode::Read`, `Write`, and `ReadWrite` to the closest Arduino mode strings accepted by the underlying filesystem implementation.

POSIX adapter:

- Wrap `FILE *` and `stat` behind an `IFile` implementation.
- Implement `available()` as a best-effort `size - current-position` calculation when seeking is supported.
- Implement `write()` and `flush()` in terms of `fwrite()` and `fflush()` when the handle was opened in a writable mode.
- Use `stat` to populate `size()`, `isDirectory()`, and `lastWriteEpochSeconds()`.
- Preserve the resolved path string in adapter storage so `path()` can return a stable `std::string_view`.

## Integration plan

1. Add `src/compat/IFileSystem.h` with the proposed interfaces.
2. Add adapter implementations for Arduino and POSIX.
3. Refactor `src/staticfiles/FileLocator.h` to return `std::unique_ptr<Compat::IFile>`.
4. Refactor `DefaultFileLocator` and `AggregateFileLocator` to use `IFileSystem` and nullable file handles.
5. Refactor `StaticFileHandler` to consume `IFile` directly instead of wrapping `Compat::File` in a local adapter class, while keeping handler logic read-only.
6. Rework static-file header generation so it handles `std::optional` metadata explicitly.
7. Keep any builder-level Arduino ergonomics in adapter helpers rather than leaking raw Arduino filesystem headers back into core-facing types.

## Acceptance criteria

- No core-facing static-file header requires Arduino `FS.h`.
- Static-file bodies flow through the read side of `IByteChannel` without `Stream` inheritance.
- Gzip fallback, directory index fallback, `Content-Length`, `ETag`, and `Last-Modified` behavior remain correct when metadata is available.
- Native tests cover both adapter behavior and static-file fallback behavior through the new seam.

## Recommended next implementation step

Implement the new header and the POSIX adapter first, then refactor `DefaultFileLocator` and `StaticFileHandler` against that seam before adding the Arduino adapter. That sequence gives native compile and behavior coverage while the Arduino-facing mode mapping is still in flux.
