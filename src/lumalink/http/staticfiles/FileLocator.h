#pragma once
#include "LumaLinkPlatform.h"

#include <string_view>

namespace lumalink::http::core
{
    class HttpRequestContext;
}

namespace lumalink::http::staticfiles
{
    using lumalink::http::core::HttpRequestContext;
    using lumalink::platform::filesystem::FileHandle;

    class FileLocator
    {
    public:
        virtual ~FileLocator() = default;
        virtual FileHandle getFile(HttpRequestContext &context, std::string_view requestPath) = 0;
        virtual bool canHandle(std::string_view path) = 0;
    };

} // namespace lumalink::http::staticfiles


