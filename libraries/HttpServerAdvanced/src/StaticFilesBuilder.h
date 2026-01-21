#pragma once
#include <Arduino.h>

#include "./CoreServices.h"
#include <FS.h>
#include "./HttpContentTypes.h"
#include "./HttpHandlerFactory.h"
#include "./StaticFileHandler.h"
#include "./DefaultFileLocator.h"
#include "./FileLocator.h"

namespace HttpServerAdvanced
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
        void init(HttpServerAdvanced::CoreServicesBuilder &coreBuilder);

    public:
        StaticFilesBuilder(FS &fs, HttpServerAdvanced::HttpContentTypes &contentTypes, std::function<void(StaticFilesBuilder &)> setupFunc);
        ~StaticFilesBuilder();
        StaticFilesBuilder &setPathPredicate(std::function<bool(const String &)> predicate);
        StaticFilesBuilder &setPathMapper(std::function<String(const String &)> mapper);
        StaticFilesBuilder &setRequestPathPrefixes(const String &prefix = "/", const String &excludePrefix = "/api");
        StaticFilesBuilder &setFilesystemContentRoot(const String &root);
    };

    std::function<void(CoreServicesBuilder &)> &StaticFiles(FS &fs, std::function<void(StaticFilesBuilder &)> setupFunc = nullptr);
};