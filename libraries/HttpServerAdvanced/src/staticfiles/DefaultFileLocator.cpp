#include "./DefaultFileLocator.h"

namespace HttpServerAdvanced {

DefaultFileLocator::RequestPathPredicate DefaultFileLocator::createPathPredicate(const String &includePrefix, const String &excludePrefix) {
    return [includePrefix, excludePrefix](const String &path) {
        if (includePrefix.isEmpty() || path.startsWith(includePrefix)) {
            if (!excludePrefix.isEmpty() && path.startsWith(excludePrefix)) {
                return false;
            }
            return true;
        }
        return false;
    };
}

DefaultFileLocator::RequestPathMapper DefaultFileLocator::createPathMapper(const String &root) {
    return [root](const String &path) {
        String mappedPath = path;
        while (mappedPath.endsWith("/")) {
            mappedPath = mappedPath.substring(0, mappedPath.length() - 1);
        }
        while (mappedPath.startsWith("/")) {
            mappedPath = mappedPath.substring(1);
        }
        mappedPath = root + "/" + mappedPath;
        return mappedPath;
    };
}

String DefaultFileLocator::getLocalPath(const HttpRequest &context) {
    String path = context.url();

    int queryIndex = path.indexOf('?');
    if (queryIndex != -1) {
        path = path.substring(0, queryIndex);
    }
    while (path.startsWith("//")) {
        path = path.substring(1);
    }
    while (path.endsWith("/")) {
        path = path.substring(0, path.length() - 1);
    }
    return path;
}

DefaultFileLocator::DefaultFileLocator(FS &fs)
    : filesystem_(fs), pathPredicate_(createPathPredicate(DefaultIncludePrefix, DefaultExcludePrefix)), pathMapper_(createPathMapper(DefaultFSRoot)) {}

DefaultFileLocator::DefaultFileLocator(FS &fs, RequestPathPredicate predicate, RequestPathMapper mapper)
    : filesystem_(fs), pathPredicate_(predicate), pathMapper_(mapper) {}

void DefaultFileLocator::setPathPredicate(RequestPathPredicate predicate) { pathPredicate_ = predicate; }

void DefaultFileLocator::setPathMapper(RequestPathMapper mapper) { pathMapper_ = mapper; }

void DefaultFileLocator::setRequestPathPrefixes(const String &includePrefix, const String &excludePrefix) {
    setPathPredicate(createPathPredicate(includePrefix, excludePrefix));
}

void DefaultFileLocator::setFilesystemContentRoot(const String &root) { setPathMapper(createPathMapper(root)); }

File DefaultFileLocator::getFile(HttpRequest &context) {
    String path = this->getLocalPath(context);
    if (pathMapper_) {
        path = pathMapper_(path);
    }

    File file = filesystem_.open(path, "r");
    if (!file) {
        // Try .gz version if original file not found
        String gzPath = path + ".gz";
        file = filesystem_.open(gzPath, "r");
        if (file) {
            return file;
        }
        return file;
    }
    if (file.isDirectory()) {
        String indexPath = path;
        while (indexPath.endsWith("/")) {
            indexPath = indexPath.substring(0, indexPath.length() - 1);
        }
        indexPath += "/index.html";
        file.close();
        file = filesystem_.open(indexPath, "r");
        if (!file) {
            // Try .gz version of index.html if not found
            String gzIndexPath = indexPath + ".gz";
            file = filesystem_.open(gzIndexPath, "r");
        }
    }
    return file;
}

bool DefaultFileLocator::canHandle(const String& path) {
    return pathPredicate_(path);
}

} // namespace HttpServerAdvanced

