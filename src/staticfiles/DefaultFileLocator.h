#pragma once
#include "../compat/IFileSystem.h"

#include "../core/HttpContext.h"
#include "FileLocator.h"

#include <functional>
#include <string>
#include <string_view>

namespace HttpServerAdvanced
{
    class DefaultFileLocator : public FileLocator
    {
    public:
        using RequestPathPredicate = std::function<bool(std::string_view)>;
        using RequestPathMapper = std::function<std::string(std::string_view)>;
        static constexpr std::string_view DefaultFSRoot = "/www";
        static constexpr std::string_view DefaultIncludePrefix = "/";
        static constexpr std::string_view DefaultExcludePrefix = "/api/";

    private:
        RequestPathPredicate pathPredicate_;
        RequestPathMapper pathMapper_;
        IFileSystem &filesystem_;

        static RequestPathPredicate createPathPredicate(std::string_view includePrefix, std::string_view excludePrefix);

        static RequestPathMapper createPathMapper(std::string_view root);

        virtual std::string getLocalPath(const HttpContext &context);

    public:
        DefaultFileLocator(IFileSystem &fs);
        DefaultFileLocator(IFileSystem &fs, RequestPathPredicate predicate, RequestPathMapper mapper);

        virtual void setPathPredicate(RequestPathPredicate predicate);

        virtual void setPathMapper(RequestPathMapper mapper);

        virtual void setRequestPathPrefixes(std::string_view includePrefix, std::string_view excludePrefix = DefaultFileLocator::DefaultExcludePrefix);

        virtual void setFilesystemContentRoot(std::string_view root);

        virtual FileHandle getFile(HttpContext &context) override;

        virtual bool canHandle(std::string_view path) override;
    };

} // namespace HttpServerAdvanced



