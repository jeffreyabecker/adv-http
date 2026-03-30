#include "DefaultFileLocator.h"

#include <string>
#include <string_view>

namespace
{
    httpadv::v1::transport::FileHandle OpenFile(httpadv::v1::transport::IFileSystem &filesystem, std::string_view path)
    {
        return filesystem.open(path, httpadv::v1::transport::FileOpenMode::Read);
    }

    bool StartsWith(std::string_view value, std::string_view prefix)
    {
        return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
    }

    std::string TrimMappedPath(std::string_view root, std::string_view path)
    {
        std::string_view pathView(path);
        while (!pathView.empty() && pathView.back() == '/')
        {
            pathView.remove_suffix(1);
        }

        while (!pathView.empty() && pathView.front() == '/')
        {
            pathView.remove_prefix(1);
        }

        std::string mappedPath(root);
        mappedPath += "/";
        if (!pathView.empty())
        {
            mappedPath.append(pathView.data(), pathView.size());
        }

        return mappedPath;
    }

    std::string NormalizeRequestPath(std::string_view url)
    {
        std::string_view path(url);
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

        return std::string(path);
    }

    std::string TrimTrailingSlashes(std::string_view path)
    {
        std::string_view trimmed(path);
        while (!trimmed.empty() && trimmed.back() == '/')
        {
            trimmed.remove_suffix(1);
        }

        return std::string(trimmed);
    }
}

namespace httpadv::v1::staticfiles {

DefaultFileLocator::RequestPathPredicate DefaultFileLocator::createPathPredicate(std::string_view includePrefix, std::string_view excludePrefix) {
    return [includePrefix = std::string(includePrefix), excludePrefix = std::string(excludePrefix)](std::string_view path) {
        if (includePrefix.empty() || StartsWith(path, includePrefix)) {
            if (!excludePrefix.empty() && StartsWith(path, excludePrefix)) {
                return false;
            }
            return true;
        }
        return false;
    };
}

DefaultFileLocator::RequestPathMapper DefaultFileLocator::createPathMapper(std::string_view root) {
    return [root = std::string(root)](std::string_view path) {
        return TrimMappedPath(root, path);
    };
}

std::string DefaultFileLocator::getLocalPath(const HttpContext &context) {
    return NormalizeRequestPath(context.urlView());
}

DefaultFileLocator::DefaultFileLocator(IFileSystem &fs)
    : pathPredicate_(createPathPredicate(DefaultIncludePrefix, DefaultExcludePrefix)), pathMapper_(createPathMapper(DefaultFSRoot)), filesystem_(fs) {}

DefaultFileLocator::DefaultFileLocator(IFileSystem &fs, RequestPathPredicate predicate, RequestPathMapper mapper)
    : pathPredicate_(predicate), pathMapper_(mapper), filesystem_(fs) {}

void DefaultFileLocator::setPathPredicate(RequestPathPredicate predicate) { pathPredicate_ = predicate; }

void DefaultFileLocator::setPathMapper(RequestPathMapper mapper) { pathMapper_ = mapper; }

void DefaultFileLocator::setRequestPathPrefixes(std::string_view includePrefix, std::string_view excludePrefix) {
    setPathPredicate(createPathPredicate(includePrefix, excludePrefix));
}

void DefaultFileLocator::setFilesystemContentRoot(std::string_view root) { setPathMapper(createPathMapper(root)); }

FileHandle DefaultFileLocator::getFile(HttpContext &context) {
    std::string path = this->getLocalPath(context);
    if (pathMapper_) {
        path = pathMapper_(path);
    }

    FileHandle file = OpenFile(filesystem_, path);
    if (!file) {
        std::string gzPath = path + ".gz";
        file = OpenFile(filesystem_, gzPath);
        if (file) {
            return file;
        }
        return file;
    }
    if (file->isDirectory()) {
        std::string indexPath = TrimTrailingSlashes(path);
        indexPath += "/index.html";
        file->close();
        file = OpenFile(filesystem_, indexPath);
        if (!file) {
            std::string gzIndexPath = indexPath + ".gz";
            file = OpenFile(filesystem_, gzIndexPath);
        }
    }
    return file;
}

bool DefaultFileLocator::canHandle(std::string_view path) {
    return pathPredicate_(path);
}

} // namespace httpadv::v1::staticfiles

