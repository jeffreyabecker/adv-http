#pragma once
#include "../compat/FileSystem.h"
#include <Arduino.h>

#include "FileLocator.h"
#include "../core/HttpContentTypes.h"
#include "../response/HttpResponse.h"
#include "../core/HttpRequest.h"
#include "../handlers/IHandlerProvider.h"
namespace HttpServerAdvanced
{
    using namespace HttpServerAdvanced;

    class StaticFileHandlerFactory : public IHandlerProvider
    {
    private:
        HttpServerAdvanced::HttpContentTypes &contentTypes_;
        FileLocator &fileLocator_;

        class FileStreamWrapper : public Stream
        {
        private:
            File file_;

        public:
            using Stream::write;

            FileStreamWrapper(File file);
            virtual int available() override;
            virtual int read() override;
            virtual int peek() override;
            virtual size_t write(uint8_t b) override;
            File &getFile();
        };

        static String getEtag(File &file_);
        static String getLastWriteValue(File &file_);
        String getUrlPath(const String &url);

    public:
        StaticFileHandlerFactory(FileLocator &fileLocator, HttpServerAdvanced::HttpContentTypes &contentTypes);
        bool canHandle(HttpRequest &context);
        std::unique_ptr<IHttpHandler> create(HttpRequest &context) override;
        void setFileLocator(FileLocator &fileLocator);
    };

} // namespace HttpServerAdvanced





