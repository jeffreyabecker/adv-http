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

        using FileHandle = std::unique_ptr<IFile>;

        class IFileSystem
        {
        public:
            virtual ~IFileSystem() = default;

            virtual FileHandle open(std::string_view path, FileOpenMode mode) = 0;
        };
    }

    using FileHandle = Compat::FileHandle;
    using FileOpenMode = Compat::FileOpenMode;
    using IFile = Compat::IFile;
    using IFileSystem = Compat::IFileSystem;
}