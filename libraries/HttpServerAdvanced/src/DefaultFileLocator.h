#pragma once
#include <Arduino.h>

#include <FS.h>
#include "./FileLocator.h"
#include <functional>
#include "./HttpRequest.h"
#include "./HttpContext.h"

namespace HttpServerAdvanced
{
    using namespace HttpServerAdvanced;

    class DefaultFileLocator : public FileLocator
    {
    public:
        using RequestPathPredicate = std::function<bool(const String &)>;
        using RequestPathMapper = std::function<String(const String &)>;
        static constexpr char *DefaultFSRoot = "/www";
        static constexpr char *DefaultIncludePrefix = "/";
        static constexpr char *DefaultExcludePrefix = "/api/";

    private:
        RequestPathPredicate pathPredicate_;
        RequestPathMapper pathMapper_;
        FS &filesystem_;

        static RequestPathPredicate createPathPredicate(const String &includePrefix, const String &excludePrefix)
        {
            return [includePrefix, excludePrefix](const String &path)
            {
                if (includePrefix.isEmpty() || path.startsWith(includePrefix))
                {
                    if (!excludePrefix.isEmpty() && path.startsWith(excludePrefix))
                    {
                        return false;
                    }
                    return true;
                }
                return false;
            };
        }

        static RequestPathMapper createPathMapper(const String &root)
        {
            return [root](const String &path)
            {
                String mappedPath = path;
                while (mappedPath.endsWith("/"))
                {
                    mappedPath = mappedPath.substring(0, mappedPath.length() - 1);
                }
                while (mappedPath.startsWith("/"))
                {
                    mappedPath = mappedPath.substring(1);
                }
                mappedPath = root + "/" + mappedPath;
                return mappedPath;
            };
        }

        virtual String getLocalPath(const HttpRequest &request)
        {
            String path = request.url();

            int queryIndex = path.indexOf('?');
            if (queryIndex != -1)
            {
                path = path.substring(0, queryIndex);
            }
            while (path.startsWith("//"))
            {
                path = path.substring(1);
            }
            while (path.endsWith("/"))
            {
                path = path.substring(0, path.length() - 1);
            }
            return path;
        }

    public:
        DefaultFileLocator(FS &fs) : filesystem_(fs), pathPredicate_(createPathPredicate(DefaultIncludePrefix, DefaultExcludePrefix)), pathMapper_(createPathMapper(DefaultFSRoot)) {}
        DefaultFileLocator(FS &fs, RequestPathPredicate predicate, RequestPathMapper mapper)
            : filesystem_(fs), pathPredicate_(predicate), pathMapper_(mapper)
        {
        }

        virtual void setPathPredicate(RequestPathPredicate predicate)
        {
            pathPredicate_ = predicate;
        }

        virtual void setPathMapper(RequestPathMapper mapper)
        {
            pathMapper_ = mapper;
        }

        virtual void setRequestPathPrefixes(const String &includePrefix, const String &excludePrefix = DefaultFileLocator::DefaultExcludePrefix)
        {
            setPathPredicate(createPathPredicate(includePrefix, excludePrefix));
        }

        virtual void setFilesystemContentRoot(const String &root)
        {
            setPathMapper(createPathMapper(root));
        }

        virtual File getFile(HttpContext &context) override
        {
            String path = this->getLocalPath(context.request());
            if (pathMapper_)
            {
                path = pathMapper_(path);
            }

            File file = filesystem_.open(path, "r");
            if (!file)
            {
                return file;
            }
            if (file.isDirectory())
            {
                String indexPath = path;
                while (indexPath.endsWith("/"))
                {
                    indexPath = indexPath.substring(0, indexPath.length() - 1);
                }
                indexPath += "/index.html";
                file.close();
                file = filesystem_.open(indexPath, "r");
            }
            return file;
        }

        virtual bool canHandle(const String& path) override
        {
            return pathPredicate_(path);
        }
    };

} // namespace HttpServerAdvanced