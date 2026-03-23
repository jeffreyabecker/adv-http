#pragma once
#include "../compat/IFileSystem.h"
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
        FileLocator *fileLocator_;

        static std::optional<String> getEtag(const IFile &file);
        static std::optional<String> getLastWriteValue(const IFile &file);
        String getUrlPath(const String &url);

    public:
        StaticFileHandlerFactory(FileLocator &fileLocator, HttpServerAdvanced::HttpContentTypes &contentTypes);
        bool canHandle(HttpRequest &context);
        std::unique_ptr<IHttpHandler> create(HttpRequest &context) override;
        void setFileLocator(FileLocator &fileLocator);
    };

} // namespace HttpServerAdvanced





