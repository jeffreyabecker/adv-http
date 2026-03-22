#pragma once
#include "../compat/FileSystem.h"
#include "../streams/ByteStream.h"
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

        class FileByteSource : public IByteSource
        {
        private:
            File file_;

        public:
            explicit FileByteSource(File file);
            AvailableResult available() override;
            size_t read(HttpServerAdvanced::span<uint8_t> buffer) override;
            size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override;
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





