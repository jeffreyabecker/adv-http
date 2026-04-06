#pragma once
#include "../transport/IFileSystem.h"

#include <string_view>

namespace httpadv::v1::core
{
    class HttpRequestContext;
}

namespace httpadv::v1::staticfiles
{
    using httpadv::v1::core::HttpRequestContext;
    using httpadv::v1::transport::FileHandle;

    class FileLocator
    {
    public:
        virtual ~FileLocator() = default;
        virtual FileHandle getFile(HttpRequestContext &context, std::string_view requestPath) = 0;
        virtual bool canHandle(std::string_view path) = 0;
    };

} // namespace httpadv::v1::staticfiles


