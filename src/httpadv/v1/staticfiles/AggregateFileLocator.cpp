#include "AggregateFileLocator.h"

namespace httpadv::v1::staticfiles
{


    void AggregateFileLocator::add(std::unique_ptr<FileLocator> locator)
    {
        locators_.push_back(std::move(locator));
    }

    FileHandle AggregateFileLocator::getFile(HttpRequestContext &context, std::string_view requestPath)
    {
        for (auto &locator : locators_)
        {
            if (!locator->canHandle(requestPath))
            {
                continue;
            }

            FileHandle file = locator->getFile(context, requestPath);
            if (file)
            {
                return file;
            }
        }
        return nullptr;
    }

    bool AggregateFileLocator::canHandle(std::string_view path)
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

} // namespace httpadv::v1::staticfiles


