#pragma once

#include <Arduino.h>
#include <FS.h>
#include <memory>
#include <vector>

#include "./FileLocator.h"

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
        File getFile(HttpRequest &context) override;
        bool canHandle(const String &path) override;
    };

} // namespace HttpServerAdvanced
