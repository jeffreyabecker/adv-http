#pragma once

#include "../transport/IFileSystem.h"
#include "FileLocator.h"

#include <memory>
#include <string_view>
#include <vector>

namespace lumalink::http::staticfiles
{
    class AggregateFileLocator : public FileLocator
    {
    private:
        std::vector<std::unique_ptr<FileLocator>> locators_;
    public:

        void add(std::unique_ptr<FileLocator> locator);
        FileHandle getFile(HttpRequestContext &context, std::string_view requestPath) override;
        bool canHandle(std::string_view path) override;
    };

} // namespace lumalink::http::staticfiles
