#include "DefaultFileLocator.h"

#include <string_view>

namespace
{
    HttpServerAdvanced::FileHandle OpenFile(HttpServerAdvanced::IFileSystem &filesystem, const String &path)
    {
        return filesystem.open(std::string_view(path.c_str(), path.length()), HttpServerAdvanced::FileOpenMode::Read);
    }

    bool StartsWith(const String &value, std::string_view prefix)
    {
        const std::string_view view(value.c_str(), value.length());
        return view.size() >= prefix.size() && view.compare(0, prefix.size(), prefix) == 0;
    }

    String TrimMappedPath(const String &root, const String &path)
    {
        std::string_view pathView(path.c_str(), path.length());
        while (!pathView.empty() && pathView.back() == '/')
        {
            pathView.remove_suffix(1);
        }

        while (!pathView.empty() && pathView.front() == '/')
        {
            pathView.remove_prefix(1);
        }

        String mappedPath = root + "/";
        if (!pathView.empty())
        {
            mappedPath += String(pathView.data(), pathView.size());
        }

        return mappedPath;
    }

    String NormalizeRequestPath(const String &url)
    {
        std::string_view path(url.c_str(), url.length());
        const std::size_t queryIndex = path.find('?');
        if (queryIndex != std::string_view::npos)
        {
            path = path.substr(0, queryIndex);
        }

        while (path.size() >= 2 && path[0] == '/' && path[1] == '/')
        {
            path.remove_prefix(1);
        }

        while (!path.empty() && path.back() == '/')
        {
            path.remove_suffix(1);
        }

        return String(path.data(), path.size());
    }

    String TrimTrailingSlashes(const String &path)
    {
        std::string_view trimmed(path.c_str(), path.length());
        while (!trimmed.empty() && trimmed.back() == '/')
        {
            trimmed.remove_suffix(1);
        }

        return String(trimmed.data(), trimmed.size());
    }
}

namespace HttpServerAdvanced {

DefaultFileLocator::RequestPathPredicate DefaultFileLocator::createPathPredicate(const String &includePrefix, const String &excludePrefix) {
    return [includePrefix, excludePrefix](const String &path) {
        if (includePrefix.isEmpty() || StartsWith(path, std::string_view(includePrefix.c_str(), includePrefix.length()))) {
            if (!excludePrefix.isEmpty() && StartsWith(path, std::string_view(excludePrefix.c_str(), excludePrefix.length()))) {
                return false;
            }
            return true;
        }
        return false;
    };
}

DefaultFileLocator::RequestPathMapper DefaultFileLocator::createPathMapper(const String &root) {
    return [root](const String &path) {
        return TrimMappedPath(root, path);
    };
}

String DefaultFileLocator::getLocalPath(const HttpRequest &context) {
    return NormalizeRequestPath(context.url());
}

DefaultFileLocator::DefaultFileLocator(IFileSystem &fs)
    : filesystem_(fs), pathPredicate_(createPathPredicate(DefaultIncludePrefix, DefaultExcludePrefix)), pathMapper_(createPathMapper(DefaultFSRoot)) {}

DefaultFileLocator::DefaultFileLocator(IFileSystem &fs, RequestPathPredicate predicate, RequestPathMapper mapper)
    : filesystem_(fs), pathPredicate_(predicate), pathMapper_(mapper) {}

void DefaultFileLocator::setPathPredicate(RequestPathPredicate predicate) { pathPredicate_ = predicate; }

void DefaultFileLocator::setPathMapper(RequestPathMapper mapper) { pathMapper_ = mapper; }

void DefaultFileLocator::setRequestPathPrefixes(const String &includePrefix, const String &excludePrefix) {
    setPathPredicate(createPathPredicate(includePrefix, excludePrefix));
}

void DefaultFileLocator::setFilesystemContentRoot(const String &root) { setPathMapper(createPathMapper(root)); }

FileHandle DefaultFileLocator::getFile(HttpRequest &context) {
    String path = this->getLocalPath(context);
    if (pathMapper_) {
        path = pathMapper_(path);
    }

    FileHandle file = OpenFile(filesystem_, path);
    if (!file) {
        String gzPath = path + ".gz";
        file = OpenFile(filesystem_, gzPath);
        if (file) {
            return file;
        }
        return file;
    }
    if (file->isDirectory()) {
        String indexPath = TrimTrailingSlashes(path);
        indexPath += "/index.html";
        file->close();
        file = OpenFile(filesystem_, indexPath);
        if (!file) {
            String gzIndexPath = indexPath + ".gz";
            file = OpenFile(filesystem_, gzIndexPath);
        }
    }
    return file;
}

bool DefaultFileLocator::canHandle(const String& path) {
    return pathPredicate_(path);
}

} // namespace HttpServerAdvanced

