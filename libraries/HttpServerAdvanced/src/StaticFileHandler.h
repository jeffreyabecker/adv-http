#pragma once
#include <Arduino.h>

#include <FS.h>
#include "./FileLocator.h"
#include "./HttpContentTypes.h"
#include "./HttpResponse.h"
#include "./HttpContext.h"

namespace HttpServerAdvanced
{
    using namespace HttpServerAdvanced;

    class StaticFileHandlerFactory : public HttpHandlerFactory::IHttpHandlerFactoryItem
    {
    private:
        HttpServerAdvanced::HttpContentTypes &contentTypes_;
        FileLocator &fileLocator_;

        class FileStreamWrapper : public Stream
        {
        private:
            File file_;

        public:
            FileStreamWrapper(File file) : file_(std::move(file)) {}
            virtual int available() override { return file_.available(); }
            virtual int read() override { return file_.read(); }
            virtual int peek() override { return file_.peek(); }
            virtual size_t write(uint8_t b) override { return 0; }
            File &getFile() { return file_; }
        };

        static String getEtag(File &file_)
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

        static String getLastWriteValue(File &file_)
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

        String getUrlPath(const String &url)
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

    public:
        StaticFileHandlerFactory(FileLocator &fileLocator, HttpServerAdvanced::HttpContentTypes &contentTypes) : fileLocator_(fileLocator), contentTypes_(contentTypes)
        {
        }

        bool canHandle(HttpContext &context)
        {
            if (!fileLocator_.canHandle(context.request().url()))
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

        std::unique_ptr<IHttpHandler> create(HttpContext &context) override
        {
            bool isGet = (HttpMethod::Get == context.request().method());
            bool isHead = (HttpMethod::Head == context.request().method());

            if (!isGet && !isHead)
            {
                return HttpHandler::create(
                    HttpResponse::create(HttpStatus::MethodNotAllowed(),
                                         "Method Not Allowed",
                                         HttpHeadersCollection{HttpHeader(HttpHeader::Allow, "GET, HEAD")}),
                    HttpContextPhase::CompletedStartingLine);
            }

            File file = fileLocator_.getFile(context);

            String contentType = contentTypes_.getContentTypeFromPath(file.fullName());
            String path = getUrlPath(context.request().url());
            bool isHeadRequest = (HttpMethod::Head == context.request().method());
            return HttpHandler::create(
                HttpResponse::create(HttpStatus::Ok(),
                                     std::make_unique<FileStreamWrapper>(std::move(file)),
                                     HttpHeadersCollection{HttpHeader(HttpHeader::ContentType, contentType),
                                                           HttpHeader(HttpHeader::ContentLength, String(file.size())),
                                                           HttpHeader(HttpHeader::ETag, getEtag(file)),
                                                           HttpHeader(HttpHeader::LastModified, getLastWriteValue(file))}),
                HttpContextPhase::CompletedStartingLine);
        }
        void setFileLocator(FileLocator &fileLocator)
        {
            fileLocator_ = fileLocator;
        }
    };

} // namespace HttpServerAdvanced