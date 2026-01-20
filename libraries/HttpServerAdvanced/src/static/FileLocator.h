#pragma once
#include <Arduino.h>
#include "../core/Core.h"
#include <FS.h>

namespace HttpServerAdvanced::StaticFiles
{
    using namespace HttpServerAdvanced::Core;

    class FileLocator
    {
    public:
        virtual ~FileLocator() = default;
        virtual File getFile(HttpContext &context) = 0;
        virtual bool canHandle(const String &path) = 0;
    };

    class AggregateFileLocator : public FileLocator
    {
    private:
        std::vector<std::shared_ptr<FileLocator>> locators_;

    public:
        void add(FileLocator *locator, bool takeOwnership = true)
        {
            if (takeOwnership)
            {
                locators_.push_back(std::shared_ptr<FileLocator>(locator));
            }
            else
            {
                locators_.push_back(std::shared_ptr<FileLocator>(locator, [](FileLocator *) {}));
            }
        }
        void add(FileLocator &locator)
        {
            locators_.push_back(std::shared_ptr<FileLocator>(&locator, [](FileLocator *) {}));
        }
        void add(std::shared_ptr<FileLocator> locator)
        {
            locators_.push_back(locator);
        }
        virtual File getFile(HttpContext &context) override
        {
            for (auto &locator : locators_)
            {
                if (locator->canHandle(context.request().url()))
                {
                    File file = locator->getFile(context);
                    if (file)
                    {
                        return file;
                    }
                }
            }
            return File();
        }
        virtual bool canHandle(const String &path) override
        {
            for (auto &locator : locators_)
            {
                if (locator->canHandle(path))
                {
                    return true;
                }
            }
            return false;
        }
    };

} // namespace HttpServerAdvanced::StaticFiles