#pragma once
#include <Arduino.h>

#include <FS.h>
#include "FileLocator.h"
#include <functional>
#include "../core/HttpRequest.h"

namespace HttpServerAdvanced
{
    using namespace HttpServerAdvanced;

    class DefaultFileLocator : public FileLocator
    {
    public:
        using RequestPathPredicate = std::function<bool(const String &)>;
        using RequestPathMapper = std::function<String(const String &)>;
        static constexpr const char *DefaultFSRoot = "/www";
        static constexpr const char *DefaultIncludePrefix = "/";
        static constexpr const char *DefaultExcludePrefix = "/api/";

    private:
        RequestPathPredicate pathPredicate_;
        RequestPathMapper pathMapper_;
        FS &filesystem_;

        static RequestPathPredicate createPathPredicate(const String &includePrefix, const String &excludePrefix);

        static RequestPathMapper createPathMapper(const String &root);

        virtual String getLocalPath(const HttpRequest &context);

    public:
        DefaultFileLocator(FS &fs);
        DefaultFileLocator(FS &fs, RequestPathPredicate predicate, RequestPathMapper mapper);

        virtual void setPathPredicate(RequestPathPredicate predicate);

        virtual void setPathMapper(RequestPathMapper mapper);

        virtual void setRequestPathPrefixes(const String &includePrefix, const String &excludePrefix = DefaultFileLocator::DefaultExcludePrefix);

        virtual void setFilesystemContentRoot(const String &root);

        virtual File getFile(HttpRequest &context) override;

        virtual bool canHandle(const String& path) override;
    };

} // namespace HttpServerAdvanced



