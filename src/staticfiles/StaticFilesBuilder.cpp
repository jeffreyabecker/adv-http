#include "StaticFilesBuilder.h"

namespace HttpServerAdvanced
{
    StaticFilesBuilder::StaticFilesBuilder(IFileSystem &fs, HttpServerAdvanced::HttpContentTypes &contentTypes, std::function<void(StaticFilesBuilder &)> setupFunc)
                : setupFunc_(setupFunc),
                    fileLocator_(fs),
                    fileHandlerFactory_(fileLocator_, contentTypes)
    {
    }

    StaticFilesBuilder::~StaticFilesBuilder()
    {
    }

    void StaticFilesBuilder::init(HttpServerAdvanced::WebServerBuilder &coreBuilder)
    {
        if (setupFunc_)
        {
            setupFunc_(*this);
        }
        auto &handlerFactory = coreBuilder.handlerProviders();
        handlerFactory.add(fileHandlerFactory_, 0);
    }

        StaticFilesBuilder &StaticFilesBuilder::setPathPredicate(std::function<bool(std::string_view)> predicate)
    {
        fileLocator_.setPathPredicate(predicate);
        return *this;
    }

        StaticFilesBuilder &StaticFilesBuilder::setPathMapper(std::function<std::string(std::string_view)> mapper)
    {
        fileLocator_.setPathMapper(mapper);
        return *this;
    }

        StaticFilesBuilder &StaticFilesBuilder::setRequestPathPrefixes(std::string_view prefix, std::string_view excludePrefix)
    {
        fileLocator_.setRequestPathPrefixes(prefix, excludePrefix);
        return *this;
    }

        StaticFilesBuilder &StaticFilesBuilder::setFilesystemContentRoot(std::string_view root)
    {
        fileLocator_.setFilesystemContentRoot(root);
        return *this;
    }

    std::function<void(WebServerBuilder &)> &StaticFiles(IFileSystem &fs, std::function<void(StaticFilesBuilder &)> setupFunc)
    {
        static std::function<void(WebServerBuilder &)> *instance = nullptr;
        if (instance == nullptr)
        {
            instance = new std::function<void(WebServerBuilder &)>([setupFunc, &fs](WebServerBuilder &coreBuilder)
                                                                      {
                auto &contentTypes = coreBuilder.contentTypes();
                StaticFilesBuilder staticFilesBuilder(fs, contentTypes, setupFunc);
                staticFilesBuilder.init(coreBuilder); });
        }
        return *instance;
    }

} // namespace HttpServerAdvanced
