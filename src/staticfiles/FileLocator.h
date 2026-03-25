#pragma once
#include "../compat/IFileSystem.h"
#include "../core/HttpContext.h"

#include <string_view>


namespace HttpServerAdvanced
{
    class FileLocator
    {
    public:
        virtual ~FileLocator() = default;
        virtual FileHandle getFile(HttpContext &context) = 0;
        virtual bool canHandle(std::string_view path) = 0;
    };

} // namespace HttpServerAdvanced


