#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "../VirtualPathMapper.h"

#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace HttpServerAdvanced
{
    namespace Compat
    {
        namespace platform
        {
            struct FileMetadata
            {
                bool exists = false;
                bool directory = false;
                std::optional<std::size_t> size;
                std::optional<uint32_t> lastWrite;
            };

            inline std::optional<uint32_t> ToEpochSeconds(const FILETIME& time)
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

            inline FileMetadata ReadMetadata(std::string_view path)
            {
                FileMetadata metadata;
                const std::string ownedPath(path);

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
                return metadata;
            }

            inline bool CreateHostDirectory(std::string_view path)
            {
                const std::string ownedPath(path);
                if (CreateDirectoryA(ownedPath.c_str(), nullptr) != 0)
                {
                    return true;
                }

                if (GetLastError() == ERROR_ALREADY_EXISTS)
                {
                    const FileMetadata metadata = ReadMetadata(ownedPath);
                    return metadata.exists && metadata.directory;
                }

                return false;
            }

            inline bool RemoveHostPath(std::string_view path, bool isDirectory)
            {
                const std::string ownedPath(path);
                return isDirectory ? (RemoveDirectoryA(ownedPath.c_str()) != 0) : (DeleteFileA(ownedPath.c_str()) != 0);
            }

            inline bool RenameHostPath(std::string_view from, std::string_view to)
            {
                const std::string ownedFrom(from);
                const std::string ownedTo(to);
                return MoveFileA(ownedFrom.c_str(), ownedTo.c_str()) != 0;
            }

            template <typename Callback>
            inline bool EnumerateHostDirectory(std::string_view directoryPath, Callback&& callback)
            {
                const std::string ownedDirectory(directoryPath);
                const std::string searchPattern = VirtualPathMapper::JoinScopedPath(ownedDirectory, "*");

                WIN32_FIND_DATAA findData;
                HANDLE handle = FindFirstFileA(searchPattern.c_str(), &findData);
                if (handle == INVALID_HANDLE_VALUE)
                {
                    return GetLastError() == ERROR_FILE_NOT_FOUND;
                }

                DWORD lastError = ERROR_SUCCESS;
                do
                {
                    const std::string_view name(findData.cFileName);
                    if (name == "." || name == "..")
                    {
                        continue;
                    }

                    const bool isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    if (!callback(name, isDirectory))
                    {
                        FindClose(handle);
                        return true;
                    }
                } while (FindNextFileA(handle, &findData) != 0);

                lastError = GetLastError();
                FindClose(handle);
                return lastError == ERROR_NO_MORE_FILES;
            }
        } // namespace platform
    } // namespace Compat
} // namespace HttpServerAdvanced