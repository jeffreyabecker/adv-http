#include "StaticFilesBuilder.h"
#include <FS.h>
namespace HttpServerAdvanced
{
    StaticFilesBuilder::StaticFilesBuilder(FS &fs, HttpServerAdvanced::HttpContentTypes &contentTypes, std::function<void(StaticFilesBuilder &)> setupFunc)
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
        handlerFactory.add(fileHandlerFactory_, HandlerProviderRegistry::AddAt::Beginning);
    }

    StaticFilesBuilder &StaticFilesBuilder::setPathPredicate(std::function<bool(const String &)> predicate)
    {
        fileLocator_.setPathPredicate(predicate);
        return *this;
    }

    StaticFilesBuilder &StaticFilesBuilder::setPathMapper(std::function<String(const String &)> mapper)
    {
        fileLocator_.setPathMapper(mapper);
        return *this;
    }

    StaticFilesBuilder &StaticFilesBuilder::setRequestPathPrefixes(const String &prefix, const String &excludePrefix)
    {
        fileLocator_.setRequestPathPrefixes(prefix, excludePrefix);
        return *this;
    }

    StaticFilesBuilder &StaticFilesBuilder::setFilesystemContentRoot(const String &root)
    {
        fileLocator_.setFilesystemContentRoot(root);
        return *this;
    }

    std::function<void(WebServerBuilder &)> &StaticFiles(FS &fs, std::function<void(StaticFilesBuilder &)> setupFunc)
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
