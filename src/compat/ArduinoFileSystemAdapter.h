#pragma once

#ifdef ARDUINO

#include <FS.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "IFileSystem.h"

namespace HttpServerAdvanced
{
    namespace Compat
    {
        namespace ArduinoFileSystemAdapterDetail
        {
            template <typename T, typename = void>
            struct HasNameMethod : std::false_type
            {
            };

            template <typename T>
            struct HasNameMethod<T, std::void_t<decltype(std::declval<T &>().name())>> : std::true_type
            {
            };

            template <typename T, typename = void>
            struct HasLastWriteMethod : std::false_type
            {
            };

            template <typename T>
            struct HasLastWriteMethod<T, std::void_t<decltype(std::declval<T &>().getLastWrite())>> : std::true_type
            {
            };

            template <typename TFile>
            std::string ResolvePath(TFile &file, std::string_view requestedPath)
            {
                if constexpr (HasNameMethod<TFile>::value)
                {
                    const char *name = file.name();
                    if (name != nullptr && name[0] != '\0')
                    {
                        return std::string(name);
                    }
                }

                return std::string(requestedPath.data(), requestedPath.size());
            }

            template <typename TFile>
            std::optional<uint32_t> ResolveLastWrite(TFile &file)
            {
                if constexpr (HasLastWriteMethod<TFile>::value)
                {
                    return static_cast<uint32_t>(file.getLastWrite());
                }

                static_cast<void>(file);
                return std::nullopt;
            }

            inline const char *ToModeString(FileOpenMode mode)
            {
                switch (mode)
                {
                case FileOpenMode::Read:
                    return "r";

                case FileOpenMode::Write:
                    return "w";

                case FileOpenMode::ReadWrite:
                default:
                    return "r+";
                }
            }
        }

        class ArduinoFile : public IFile
        {
        public:
            explicit ArduinoFile(fs::File file, std::string requestedPath)
                : file_(std::move(file)),
                  path_(ArduinoFileSystemAdapterDetail::ResolvePath(file_, requestedPath)),
                  lastWrite_(ArduinoFileSystemAdapterDetail::ResolveLastWrite(file_))
            {
            }

            AvailableResult available() override
            {
                if (!file_)
                {
                    return ExhaustedResult();
                }

                const int count = file_.available();
                if (count > 0)
                {
                    return AvailableBytes(static_cast<size_t>(count));
                }

                return count == 0 ? ExhaustedResult() : TemporarilyUnavailableResult();
            }

            size_t read(HttpServerAdvanced::span<uint8_t> buffer) override
            {
                if (!file_ || buffer.empty())
                {
                    return 0;
                }

                size_t totalRead = 0;
                while (totalRead < buffer.size())
                {
                    const int value = file_.read();
                    if (value < 0)
                    {
                        break;
                    }

                    buffer[totalRead++] = static_cast<uint8_t>(value);
                }

                return totalRead;
            }

            size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override
            {
                if (!file_ || buffer.empty())
                {
                    return 0;
                }

                const int value = file_.peek();
                if (value < 0)
                {
                    return 0;
                }

                buffer[0] = static_cast<uint8_t>(value);
                return 1;
            }

            std::size_t write(HttpServerAdvanced::span<const uint8_t> buffer) override
            {
                if (!file_ || buffer.empty())
                {
                    return 0;
                }

                return file_.write(buffer.data(), buffer.size());
            }

            void flush() override
            {
                if (file_)
                {
                    file_.flush();
                }
            }

            bool isDirectory() const override
            {
                return file_ ? file_.isDirectory() : false;
            }

            void close() override
            {
                if (file_)
                {
                    file_.close();
                }
            }

            std::optional<std::size_t> size() const override
            {
                if (!file_ || isDirectory())
                {
                    return std::nullopt;
                }

                return static_cast<std::size_t>(file_.size());
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
            fs::File file_;
            std::string path_;
            std::optional<uint32_t> lastWrite_;
        };

        class ArduinoFileSystemAdapter : public IFileSystem
        {
        public:
            explicit ArduinoFileSystemAdapter(fs::FS &filesystem)
                : filesystem_(&filesystem)
            {
            }

            FileHandle open(std::string_view path, FileOpenMode mode) override
            {
                if (filesystem_ == nullptr || path.empty())
                {
                    return nullptr;
                }

                std::string ownedPath(path.data(), path.size());
                fs::File file = filesystem_->open(ownedPath.c_str(), ArduinoFileSystemAdapterDetail::ToModeString(mode));
                if (!file && mode == FileOpenMode::ReadWrite)
                {
                    file = filesystem_->open(ownedPath.c_str(), "w+");
                }

                if (!file)
                {
                    return nullptr;
                }

                return std::make_unique<ArduinoFile>(std::move(file), std::move(ownedPath));
            }

            bool wraps(fs::FS &filesystem) const
            {
                return filesystem_ == &filesystem;
            }

        private:
            fs::FS *filesystem_ = nullptr;
        };

        inline IFileSystem &WrapArduinoFileSystem(fs::FS &filesystem)
        {
            static std::vector<std::unique_ptr<ArduinoFileSystemAdapter>> adapters;

            for (auto &adapter : adapters)
            {
                if (adapter->wraps(filesystem))
                {
                    return *adapter;
                }
            }

            adapters.push_back(std::make_unique<ArduinoFileSystemAdapter>(filesystem));
            return *adapters.back();
        }
    }
}

#endif