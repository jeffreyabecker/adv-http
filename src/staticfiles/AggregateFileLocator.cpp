#include "AggregateFileLocator.h"
#include "../core/HttpRequest.h"

namespace HttpServerAdvanced
{
    void AggregateFileLocator::add(FileLocator *locator, bool takeOwnership)
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

    void AggregateFileLocator::add(FileLocator &locator)
    {
        locators_.push_back(std::shared_ptr<FileLocator>(&locator, [](FileLocator *) {}));
    }

    void AggregateFileLocator::add(std::shared_ptr<FileLocator> locator)
    {
        locators_.push_back(std::move(locator));
    }

    FileHandle AggregateFileLocator::getFile(HttpRequest &context)
    {
        for (auto &locator : locators_)
        {
            if (locator->canHandle(context.urlView()))
            {
                FileHandle file = locator->getFile(context);
                if (file)
                {
                    return file;
                }
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

} // namespace HttpServerAdvanced


