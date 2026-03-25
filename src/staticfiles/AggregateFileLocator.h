#pragma once

#include "../compat/IFileSystem.h"
#include "FileLocator.h"

#include <memory>
#include <string_view>
#include <vector>

namespace HttpServerAdvanced
{
    class AggregateFileLocator : public FileLocator
    {
    private:
        std::vector<std::shared_ptr<FileLocator>> locators_;

    public:
        void add(FileLocator *locator, bool takeOwnership = true);
        void add(FileLocator &locator);
        void add(std::shared_ptr<FileLocator> locator);
        FileHandle getFile(HttpContext &context) override;
        bool canHandle(std::string_view path) override;
    };

} // namespace HttpServerAdvanced
