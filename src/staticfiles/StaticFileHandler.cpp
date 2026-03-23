#include "StaticFileHandler.h"
#include "../handlers/HttpHandler.h"
#include "../response/StringResponse.h"
#include <ctime>

#include <string_view>

namespace
{
    String ToArduinoString(std::string_view value)
    {
        return String(value.data(), value.size());
    }

    bool EndsWith(std::string_view value, std::string_view suffix)
    {
        return value.size() >= suffix.size() &&
               value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    String NormalizeUrlPath(const String &url)
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

        return String(path.data(), path.size());
    }
}

namespace HttpServerAdvanced
{
    std::optional<String> StaticFileHandlerFactory::getEtag(const IFile &file)
    {
        const std::optional<std::size_t> size = file.size();
        const std::optional<uint32_t> lastWrite = file.lastWriteEpochSeconds();
        if (!size.has_value() || !lastWrite.has_value())
        {
            return std::nullopt;
        }

        const std::size_t nameLen = file.path().size();
        const uint32_t combined = (static_cast<uint32_t>(*size) << 16) | (static_cast<uint32_t>(nameLen) & 0x0000FFFF);
        uint64_t etagValue = (static_cast<uint64_t>(combined) << 32) | static_cast<uint64_t>(*lastWrite);
        char hex[17];
        snprintf(hex, sizeof(hex), "%016llx", etagValue);
        return String(hex);
    }

    std::optional<String> StaticFileHandlerFactory::getLastWriteValue(const IFile &file)
    {
        const std::optional<uint32_t> lastWrite = file.lastWriteEpochSeconds();
        if (!lastWrite.has_value())
        {
            return std::nullopt;
        }

        char buffer[30];
        const time_t lastWriteTime = static_cast<time_t>(*lastWrite);
        struct tm *tm_info = gmtime(&lastWriteTime);
        if (tm_info == nullptr)
        {
            return std::nullopt;
        }

        strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
        return String(buffer);
    }

    String StaticFileHandlerFactory::getUrlPath(const String &url)
    {
        return NormalizeUrlPath(url);
    }

    // Public methods
    StaticFileHandlerFactory::StaticFileHandlerFactory(FileLocator &fileLocator, HttpServerAdvanced::HttpContentTypes &contentTypes)
        : fileLocator_(&fileLocator), contentTypes_(contentTypes)
    {
    }

    bool StaticFileHandlerFactory::canHandle(HttpRequest &context)
    {
        if (fileLocator_ == nullptr || !fileLocator_->canHandle(context.url()))
        {
            return false;
        }
        FileHandle file = fileLocator_->getFile(context);
        if (!file)
        {
            return false;
        }
        file->close();
        return true;
    }

    std::unique_ptr<IHttpHandler> StaticFileHandlerFactory::create(HttpRequest &context)
    {
        bool isGet = (HttpMethod::Get == context.method());
        bool isHead = (HttpMethod::Head == context.method());

        if (!isGet && !isHead)
        {
            return HttpHandler::create(
                StringResponse::create(HttpStatus::MethodNotAllowed(),
                                       "Method Not Allowed",
                                       {std::move(HttpHeader::Allow("GET, HEAD"))}));
        }

        if (fileLocator_ == nullptr)
        {
            return nullptr;
        }

        FileHandle file = fileLocator_->getFile(context);
        if (!file)
        {
            return HttpHandler::create(
                StringResponse::create(HttpStatus::NotFound(),
                                       "Not Found",
                                       {}));
        }

        const std::string_view filePath = file->path();
        const bool isGzipped = EndsWith(filePath, ".gz");
        const String fullPath = ToArduinoString(filePath);

        String contentType;
        if (isGzipped)
        {
            String nameWithoutGz = ToArduinoString(filePath.substr(0, filePath.size() - 3));
            contentType = contentTypes_.getContentTypeFromPath(nameWithoutGz.c_str());
        }
        else
        {
            contentType = contentTypes_.getContentTypeFromPath(fullPath.c_str());
        }

        HttpHeaderCollection headers;
        headers.push_back(HttpHeader::ContentType(contentType));
        if (const std::optional<std::size_t> fileSize = file->size(); fileSize.has_value())
        {
            const std::string contentLength = std::to_string(*fileSize);
            headers.push_back(HttpHeader::ContentLength(contentLength.c_str()));
        }

        if (const std::optional<String> etag = getEtag(*file); etag.has_value())
        {
            headers.push_back(HttpHeader::ETag(*etag));
        }

        if (const std::optional<String> lastModified = getLastWriteValue(*file); lastModified.has_value())
        {
            headers.push_back(HttpHeader::LastModified(*lastModified));
        }

        if (isGzipped)
        {
            headers.push_back(HttpHeader::ContentEncoding("gzip"));
        }

        std::unique_ptr<IByteSource> body = std::move(file);
        return HttpHandler::create(
            std::make_unique<HttpResponse>(
                HttpStatus::Ok(),
                std::move(body),
                std::move(headers)));
    }

    void StaticFileHandlerFactory::setFileLocator(FileLocator &fileLocator)
    {
        fileLocator_ = &fileLocator;
    }

} // namespace HttpServerAdvanced

