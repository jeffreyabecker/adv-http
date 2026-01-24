#include "StaticFileHandler.h"
#include "./handlers/HttpHandler.h"
#include "./response/StringResponse.h"
#include <ctime>

namespace HttpServerAdvanced
{
    // FileStreamWrapper implementation
    StaticFileHandlerFactory::FileStreamWrapper::FileStreamWrapper(File file)
        : file_(std::move(file))
    {
    }

    int StaticFileHandlerFactory::FileStreamWrapper::available()
    {
        return file_.available();
    }

    int StaticFileHandlerFactory::FileStreamWrapper::read()
    {
        return file_.read();
    }

    int StaticFileHandlerFactory::FileStreamWrapper::peek()
    {
        return file_.peek();
    }

    size_t StaticFileHandlerFactory::FileStreamWrapper::write(uint8_t b)
    {
        return 0;
    }

    File &StaticFileHandlerFactory::FileStreamWrapper::getFile()
    {
        return file_;
    }

    // Static helper methods
    String StaticFileHandlerFactory::getEtag(File &file_)
    {
        size_t nameLen = strlen(file_.fullName());
        size_t size = file_.size();
        uint32_t combined = (static_cast<uint32_t>(size) << 16) | (nameLen & 0x0000FFFF);
        uint32_t lastWrite = file_.getLastWrite();
        uint64_t etagValue = (static_cast<uint64_t>(combined) << 32) | (static_cast<uint64_t>(lastWrite));
        char hex[17];
        snprintf(hex, sizeof(hex), "%016llx", etagValue);
        return String(hex);
    }

    String StaticFileHandlerFactory::getLastWriteValue(File &file_)
    {
        time_t lastWrite = file_.getLastWrite();
        if (lastWrite == 0)
        {
            return "Thu, 01 Jan 1970 00:00:00 GMT"; // Default value for epoch
        }
        char buffer[30];
        struct tm *tm_info = gmtime(&lastWrite);
        strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
        return String(buffer);
    }

    String StaticFileHandlerFactory::getUrlPath(const String &url)
    {
        String path = url;
        int queryIndex = path.indexOf('?');
        if (queryIndex != -1)
        {
            path = path.substring(0, queryIndex);
        }
        while (path.startsWith("//"))
        {
            path = path.substring(1);
        }
        return path;
    }

    // Public methods
    StaticFileHandlerFactory::StaticFileHandlerFactory(FileLocator &fileLocator, HttpServerAdvanced::HttpContentTypes &contentTypes)
        : fileLocator_(fileLocator), contentTypes_(contentTypes)
    {
    }

    bool StaticFileHandlerFactory::canHandle(HttpRequest &context)
    {
        if (!fileLocator_.canHandle(context.url()))
        {
            return false;
        }
        File file = fileLocator_.getFile(context);
        if (!file)
        {
            return false;
        }
        file.close();
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

        File file = fileLocator_.getFile(context);

        // Check if the file is gzipped (ends with .gz)
        String fullName = file.fullName();
        bool isGzipped = fullName.endsWith(".gz");

        // Get content type from filename without .gz extension if gzipped
        String contentType;
        if (isGzipped)
        {
            String nameWithoutGz = fullName.substring(0, fullName.length() - 3);
            contentType = contentTypes_.getContentTypeFromPath(nameWithoutGz.c_str());
        }
        else
        {
            contentType = contentTypes_.getContentTypeFromPath(fullName.c_str());
        }

        // Build headers
        HttpHeaderCollection headers;
        headers.push_back(HttpHeader::ContentType(contentType));
        headers.push_back(HttpHeader::ContentLength(String(file.size())));
        headers.push_back(HttpHeader::ETag(getEtag(file)));
        headers.push_back(HttpHeader::LastModified(getLastWriteValue(file)));

        // Add Content-Encoding header if gzipped
        if (isGzipped)
        {
            headers.push_back(HttpHeader::ContentEncoding("gzip"));
        }

        String path = getUrlPath(context.url());
        bool isHeadRequest = (HttpMethod::Head == context.method());

        return HttpHandler::create(
            std::make_unique<HttpResponse>(
                HttpStatus::Ok(),
                std::make_unique<FileStreamWrapper>(file),
                std::move(headers)));
    }

    void StaticFileHandlerFactory::setFileLocator(FileLocator &fileLocator)
    {
        fileLocator_ = fileLocator;
    }

} // namespace HttpServerAdvanced
