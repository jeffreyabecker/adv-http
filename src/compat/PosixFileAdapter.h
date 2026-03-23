#pragma once

#include "IFileSystem.h"

#include <cstdio>
#include <string>
#include <sys/stat.h>

namespace HttpServerAdvanced
{
    namespace Compat
    {
        class PosixFile : public IFile
        {
        public:
            PosixFile(FILE *file,
                      std::string path,
                      bool directory,
                      std::optional<std::size_t> size,
                      std::optional<uint32_t> lastWrite,
                      FileOpenMode mode)
                : file_(file),
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

                if (file_ == nullptr)
                {
                    return ExhaustedResult();
                }

                if (!size_.has_value())
                {
                    return TemporarilyUnavailableResult();
                }

                const long currentOffset = ftell(file_);
                if (currentOffset < 0)
                {
                    return TemporarilyUnavailableResult();
                }

                const std::size_t currentPosition = static_cast<std::size_t>(currentOffset);
                if (currentPosition >= *size_)
                {
                    return ExhaustedResult();
                }

                return AvailableBytes(*size_ - currentPosition);
            }

            size_t read(HttpServerAdvanced::span<uint8_t> buffer) override
            {
                if (directory_ || file_ == nullptr || buffer.empty())
                {
                    return 0;
                }

                return fread(buffer.data(), 1, buffer.size(), file_);
            }

            size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override
            {
                if (directory_ || file_ == nullptr || buffer.empty())
                {
                    return 0;
                }

                const long currentOffset = ftell(file_);
                if (currentOffset < 0)
                {
                    return 0;
                }

                const size_t bytesRead = fread(buffer.data(), 1, buffer.size(), file_);
                if (fseek(file_, currentOffset, SEEK_SET) != 0)
                {
                    return 0;
                }

                return bytesRead;
            }

            std::size_t write(HttpServerAdvanced::span<const uint8_t> buffer) override
            {
                if (file_ == nullptr || directory_ || !isWritable() || buffer.empty())
                {
                    return 0;
                }

                return fwrite(buffer.data(), 1, buffer.size(), file_);
            }

            void flush() override
            {
                if (file_ != nullptr && !directory_)
                {
                    fflush(file_);
                }
            }

            bool isDirectory() const override
            {
                return directory_;
            }

            void close() override
            {
                if (file_ != nullptr)
                {
                    fclose(file_);
                    file_ = nullptr;
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
            bool isWritable() const
            {
                return mode_ == FileOpenMode::Write || mode_ == FileOpenMode::ReadWrite;
            }

            FILE *file_ = nullptr;
            std::string path_;
            bool directory_ = false;
            std::optional<std::size_t> size_;
            std::optional<uint32_t> lastWrite_;
            FileOpenMode mode_ = FileOpenMode::Read;
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
                struct stat fileStatus;
                const bool pathExists = stat(ownedPath.c_str(), &fileStatus) == 0;
                if (!pathExists && mode == FileOpenMode::Read)
                {
                    return nullptr;
                }

                const bool isDirectory = pathExists && S_ISDIR(fileStatus.st_mode) != 0;
                std::optional<std::size_t> fileSize;
                if (pathExists && !isDirectory)
                {
                    fileSize = static_cast<std::size_t>(fileStatus.st_size);
                }

                std::optional<uint32_t> lastWrite;
                if (pathExists)
                {
                    lastWrite = static_cast<uint32_t>(fileStatus.st_mtime);
                }

                if (isDirectory)
                {
                    return std::make_unique<PosixFile>(nullptr, std::move(ownedPath), true, std::nullopt, lastWrite, mode);
                }

                FILE *handle = nullptr;
                handle = fopen(ownedPath.c_str(), toModeString(mode));
                if (handle == nullptr && mode == FileOpenMode::ReadWrite)
                {
                    handle = fopen(ownedPath.c_str(), "w+b");
                }

                if (handle == nullptr)
                {
                    return nullptr;
                }

                if (stat(ownedPath.c_str(), &fileStatus) == 0)
                {
                    fileSize = static_cast<std::size_t>(fileStatus.st_size);
                    lastWrite = static_cast<uint32_t>(fileStatus.st_mtime);
                }

                return std::make_unique<PosixFile>(handle, std::move(ownedPath), isDirectory, fileSize, lastWrite, mode);
            }

        private:
            static const char *toModeString(FileOpenMode mode)
            {
                switch (mode)
                {
                case FileOpenMode::Read:
                    return "rb";

                case FileOpenMode::Write:
                    return "wb";

                case FileOpenMode::ReadWrite:
                default:
                    return "r+b";
                }
            }
        };
    }
}
