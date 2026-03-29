#pragma once

#include <dirent.h>
#include <sys/stat.h>

#include "../VirtualPathMapper.h"

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

            inline FileMetadata ReadMetadata(std::string_view path)
            {
                FileMetadata metadata;
                const std::string ownedPath(path);

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
                return metadata;
            }

            inline bool CreateHostDirectory(std::string_view path)
            {
                const std::string ownedPath(path);
                if (::mkdir(ownedPath.c_str(), 0777) == 0)
                {
                    return true;
                }

                if (errno == EEXIST)
                {
                    const FileMetadata metadata = ReadMetadata(ownedPath);
                    return metadata.exists && metadata.directory;
                }

                return false;
            }

            inline bool RemoveHostPath(std::string_view path, bool /*isDirectory*/)
            {
                const std::string ownedPath(path);
                return ::remove(ownedPath.c_str()) == 0;
            }

            inline bool RenameHostPath(std::string_view from, std::string_view to)
            {
                const std::string ownedFrom(from);
                const std::string ownedTo(to);
                return ::rename(ownedFrom.c_str(), ownedTo.c_str()) == 0;
            }

            template <typename Callback>
            inline bool EnumerateHostDirectory(std::string_view directoryPath, Callback&& callback)
            {
                const std::string ownedDirectory(directoryPath);
                DIR* directory = opendir(ownedDirectory.c_str());
                if (directory == nullptr)
                {
                    return false;
                }

                while (dirent* current = readdir(directory))
                {
                    const std::string_view name(current->d_name);
                    if (name == "." || name == "..")
                    {
                        continue;
                    }

                    const std::string entryHostPath = VirtualPathMapper::JoinScopedPath(ownedDirectory, name);
                    bool isDirectory = false;
#if defined(DT_DIR) && defined(DT_UNKNOWN)
                    if (current->d_type == DT_DIR)
                    {
                        isDirectory = true;
                    }
                    else if (current->d_type == DT_UNKNOWN)
                    {
                        const FileMetadata entryMetadata = ReadMetadata(entryHostPath);
                        isDirectory = entryMetadata.exists && entryMetadata.directory;
                    }
#else
                    const FileMetadata entryMetadata = ReadMetadata(entryHostPath);
                    isDirectory = entryMetadata.exists && entryMetadata.directory;
#endif

                    if (!callback(name, isDirectory))
                    {
                        closedir(directory);
                        return true;
                    }
                }

                closedir(directory);
                return true;
            }
        } // namespace platform
    } // namespace Compat
} // namespace HttpServerAdvanced