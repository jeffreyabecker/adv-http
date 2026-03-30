#pragma once
#include "../compat/IFileSystem.h"
#include "../core/HttpContext.h"

#include <string_view>


namespace httpadv::v1::staticfiles
{
    using httpadv::v1::core::HttpContext;
    using httpadv::v1::transport::FileHandle;

    class FileLocator
    {
    public:
        virtual ~FileLocator() = default;
        virtual FileHandle getFile(HttpContext &context) = 0;
        virtual bool canHandle(std::string_view path) = 0;
    };

} // namespace httpadv::v1::staticfiles


