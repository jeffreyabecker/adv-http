#pragma once
#include <Arduino.h>
#include "../pipeline/Pipeline.h"
#include "../core/Core.h"
#include <FS.h>

#include "./StaticFileHandler.h"
#include "./DefaultFileLocator.h"
#include "./FileLocator.h"

namespace HttpServerAdvanced::StaticFiles
{

    class StaticFilesBuilder
    {
    private:
        StaticFileHandlerFactory fileHandlerFactory_;
        std::function<void(StaticFilesBuilder &)> setupFunc_;
        DefaultFileLocator fileLocator_;

        friend std::function<void(CoreServicesBuilder &)> &StaticFiles(FS &fs, std::function<void(StaticFilesBuilder &)> setupFunc);

    protected:
        static constexpr const char *NAME = "StaticFiles";
        void init(HttpServerAdvanced::Core::CoreServicesBuilder &coreBuilder)
        {
            if (setupFunc_)
            {
                setupFunc_(*this);
            }
            auto &handlerFactory = coreBuilder.handlerFactory();
            handlerFactory.add(fileHandlerFactory_, HttpHandlerFactory::AddAt::Beginning);
        }

    public:
        StaticFilesBuilder(FS &fs, HttpServerAdvanced::Core::HttpContentTypes &contentTypes, std::function<void(StaticFilesBuilder &)> setupFunc) : setupFunc_(setupFunc),
                                                                                                                                            fileLocator_(fs),
                                                                                                                                            fileHandlerFactory_(fileLocator_, contentTypes)
        {
        }
        ~StaticFilesBuilder();
        StaticFilesBuilder &setPathPredicate(std::function<bool(const String &)> predicate)
        {
            fileLocator_.setPathPredicate(predicate);
            return *this;
        }
        StaticFilesBuilder &setPathMapper(std::function<String(const String &)> mapper)
        {
            fileLocator_.setPathMapper(mapper);
            return *this;
        }
        StaticFilesBuilder &setRequestPathPrefixes(const String &prefix = "/", const String &excludePrefix = "/api")
        {
            fileLocator_.setRequestPathPrefixes(prefix, excludePrefix);
            return *this;
        }
        StaticFilesBuilder &setFilesystemContentRoot(const String &root)
        {
            fileLocator_.setFilesystemContentRoot(root);
            return *this;
        }
    };
    std::function<void(CoreServicesBuilder &)> &StaticFiles(FS &fs, std::function<void(StaticFilesBuilder &)> setupFunc = nullptr)
    {
        static std::function<void(CoreServicesBuilder &)> *instance = nullptr;
        if (instance == nullptr)
        {
            instance = new std::function<void(CoreServicesBuilder &)>([&fs, setupFunc](CoreServicesBuilder &coreBuilder)
                                                                      {
                auto &contentTypes = coreBuilder.contentTypes();
                StaticFilesBuilder staticFilesBuilder(fs, contentTypes, setupFunc);
                staticFilesBuilder.init(coreBuilder); });
        }
        return *instance;
    }
};