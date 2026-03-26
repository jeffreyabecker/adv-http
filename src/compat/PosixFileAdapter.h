#pragma once

#include "IFileSystem.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>
#include <utility>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#endif

namespace HttpServerAdvanced
{
    namespace Compat
    {
        namespace detail
        {
            struct FileMetadata
            {
                bool exists = false;
                bool directory = false;
                std::optional<std::size_t> size;
                std::optional<uint32_t> lastWrite;
            };

#if defined(_WIN32)
            inline std::optional<uint32_t> ToEpochSeconds(const FILETIME &time)
            {
                ULARGE_INTEGER rawTime;
                rawTime.LowPart = time.dwLowDateTime;
                rawTime.HighPart = time.dwHighDateTime;

                static constexpr unsigned long long WindowsEpochOffset = 116444736000000000ULL;
                static constexpr unsigned long long TicksPerSecond = 10000000ULL;

                if (rawTime.QuadPart <= WindowsEpochOffset)
                {
                    return static_cast<uint32_t>(0);
                }

                const auto seconds = (rawTime.QuadPart - WindowsEpochOffset) / TicksPerSecond;
                if (seconds >= static_cast<unsigned long long>(std::numeric_limits<uint32_t>::max()))
                {
                    return std::numeric_limits<uint32_t>::max();
                }

                return static_cast<uint32_t>(seconds);
            }
#endif

            inline FileMetadata ReadMetadata(std::string_view path)
            {
                FileMetadata metadata;
                const std::string ownedPath(path);

#if defined(_WIN32)
                WIN32_FILE_ATTRIBUTE_DATA fileData;
                if (!GetFileAttributesExA(ownedPath.c_str(), GetFileExInfoStandard, &fileData))
                {
                    return metadata;
                }

                metadata.exists = true;
                metadata.directory = (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

                if (!metadata.directory)
                {
                    ULARGE_INTEGER fileSize;
                    fileSize.LowPart = fileData.nFileSizeLow;
                    fileSize.HighPart = fileData.nFileSizeHigh;
                    metadata.size = static_cast<std::size_t>(fileSize.QuadPart);
                }

                metadata.lastWrite = ToEpochSeconds(fileData.ftLastWriteTime);
#else
                struct stat fileStatus;
                if (stat(ownedPath.c_str(), &fileStatus) != 0)
                {
                    return metadata;
                }

                metadata.exists = true;
                metadata.directory = S_ISDIR(fileStatus.st_mode) != 0;
                if (!metadata.directory)
                {
                    metadata.size = static_cast<std::size_t>(fileStatus.st_size);
                }

                metadata.lastWrite = static_cast<uint32_t>(fileStatus.st_mtime);
#endif

                return metadata;
            }
        }

        class PosixFile : public IFile
        {
        public:
            PosixFile(std::unique_ptr<std::fstream> stream,
                      std::string path,
                      bool directory,
                      std::optional<std::size_t> size,
                      std::optional<uint32_t> lastWrite,
                      FileOpenMode mode)
                : stream_(std::move(stream)),
                  path_(std::move(path)),
                  directory_(directory),
                  size_(size),
                  lastWrite_(lastWrite),
                  mode_(mode)
            {
            }

            ~PosixFile() override
            {
                close();
            }

            AvailableResult available() override
            {
                if (directory_)
                {
                    return ExhaustedResult();
                }

                if (stream_ == nullptr || !isReadable())
                {
                    return ExhaustedResult();
                }

                if (!size_.has_value())
                {
                    return TemporarilyUnavailableResult();
                }

                if (position_ >= *size_)
                {
                    return ExhaustedResult();
                }

                return AvailableBytes(*size_ - position_);
            }

            size_t read(HttpServerAdvanced::span<uint8_t> buffer) override
            {
                if (directory_ || stream_ == nullptr || !isReadable() || buffer.empty())
                {
                    return 0;
                }

                if (size_.has_value() && position_ >= *size_)
                {
                    return 0;
                }

                if (!seekReadPosition())
                {
                    return 0;
                }

                const std::streamsize chunkSize = static_cast<std::streamsize>((std::min)(
                    buffer.size(),
                    static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())));
                stream_->read(reinterpret_cast<char *>(buffer.data()), chunkSize);
                const std::streamsize bytesRead = stream_->gcount();
                if (bytesRead <= 0)
                {
                    return 0;
                }

                position_ += static_cast<std::size_t>(bytesRead);
                return static_cast<std::size_t>(bytesRead);
            }

            size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override
            {
                if (directory_ || stream_ == nullptr || !isReadable() || buffer.empty())
                {
                    return 0;
                }

                if (size_.has_value() && position_ >= *size_)
                {
                    return 0;
                }

                if (!seekReadPosition())
                {
                    return 0;
                }

                const std::streamsize chunkSize = static_cast<std::streamsize>((std::min)(
                    buffer.size(),
                    static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())));
                stream_->read(reinterpret_cast<char *>(buffer.data()), chunkSize);
                const std::streamsize bytesRead = stream_->gcount();
                if (bytesRead <= 0)
                {
                    return 0;
                }

                return static_cast<std::size_t>(bytesRead);
            }

            std::size_t write(HttpServerAdvanced::span<const uint8_t> buffer) override
            {
                if (stream_ == nullptr || directory_ || !isWritable() || buffer.empty())
                {
                    return 0;
                }

                if (!seekWritePosition())
                {
                    return 0;
                }

                const std::streamsize chunkSize = static_cast<std::streamsize>((std::min)(
                    buffer.size(),
                    static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())));
                stream_->write(reinterpret_cast<const char *>(buffer.data()), chunkSize);
                if (!stream_->good())
                {
                    return 0;
                }

                position_ += static_cast<std::size_t>(chunkSize);
                size_ = (std::max)(size_.value_or(0), position_);
                return static_cast<std::size_t>(chunkSize);
            }

            void flush() override
            {
                if (stream_ != nullptr && !directory_ && isWritable())
                {
                    stream_->flush();
                    refreshMetadata();
                }
            }

            bool isDirectory() const override
            {
                return directory_;
            }

            void close() override
            {
                if (stream_ != nullptr)
                {
                    if (isWritable())
                    {
                        stream_->flush();
                    }

                    stream_->close();
                    stream_.reset();
                    refreshMetadata();
                }
            }

            std::optional<std::size_t> size() const override
            {
                return size_;
            }

            std::string_view path() const override
            {
                return path_;
            }

            std::optional<uint32_t> lastWriteEpochSeconds() const override
            {
                return lastWrite_;
            }

        private:
            bool isReadable() const
            {
                return mode_ == FileOpenMode::Read || mode_ == FileOpenMode::ReadWrite;
            }

            bool isWritable() const
            {
                return mode_ == FileOpenMode::Write || mode_ == FileOpenMode::ReadWrite;
            }

            bool seekReadPosition()
            {
                if (stream_ == nullptr)
                {
                    return false;
                }

                stream_->clear();
                stream_->seekg(static_cast<std::streamoff>(position_), std::ios::beg);
                return static_cast<bool>(*stream_);
            }

            bool seekWritePosition()
            {
                if (stream_ == nullptr)
                {
                    return false;
                }

                stream_->clear();
                stream_->seekp(static_cast<std::streamoff>(position_), std::ios::beg);
                return static_cast<bool>(*stream_);
            }

            void refreshMetadata()
            {
                if (directory_)
                {
                    return;
                }

                const detail::FileMetadata metadata = detail::ReadMetadata(path_);
                if (metadata.exists)
                {
                    size_ = metadata.size;
                    lastWrite_ = metadata.lastWrite;
                }
            }

            std::unique_ptr<std::fstream> stream_;
            std::string path_;
            bool directory_ = false;
            std::optional<std::size_t> size_;
            std::optional<uint32_t> lastWrite_;
            FileOpenMode mode_ = FileOpenMode::Read;
            std::size_t position_ = 0;
        };

        class PosixFS : public IFileSystem
        {
        public:
            FileHandle open(std::string_view path, FileOpenMode mode) override
            {
                if (path.empty())
                {
                    return nullptr;
                }

                std::string ownedPath(path.data(), path.size());
                const detail::FileMetadata metadata = detail::ReadMetadata(ownedPath);
                if (!metadata.exists && mode == FileOpenMode::Read)
                {
                    return nullptr;
                }

                if (metadata.directory)
                {
                    return std::make_unique<PosixFile>(nullptr, std::move(ownedPath), true, std::nullopt, metadata.lastWrite, mode);
                }

                auto stream = std::make_unique<std::fstream>();
                stream->open(ownedPath, toOpenMode(mode, metadata.exists));
                if (!stream->is_open())
                {
                    return nullptr;
                }

                const detail::FileMetadata refreshedMetadata = detail::ReadMetadata(ownedPath);

                return std::make_unique<PosixFile>(
                    std::move(stream),
                    std::move(ownedPath),
                    false,
                    refreshedMetadata.size,
                    refreshedMetadata.lastWrite,
                    mode);
            }

        private:
            static std::ios::openmode toOpenMode(FileOpenMode mode, bool pathExists)
            {
                switch (mode)
                {
                case FileOpenMode::Read:
                    return std::ios::binary | std::ios::in;

                case FileOpenMode::Write:
                    return std::ios::binary | std::ios::out | std::ios::trunc;

                case FileOpenMode::ReadWrite:
                default:
                    return pathExists
                               ? (std::ios::binary | std::ios::in | std::ios::out)
                               : (std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
                }
            }
        };
    }
}
