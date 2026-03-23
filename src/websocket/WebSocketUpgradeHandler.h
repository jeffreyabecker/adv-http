#pragma once

#include "../pipeline/RequestHandlingResult.h"

namespace HttpServerAdvanced
{
    class HttpRequest;
    class IHttpRequestHandlerFactory;

    class WebSocketUpgradeHandler
    {
    public:
        enum class UpgradeFailure
        {
            InvalidMethod,
            MissingUpgradeIntent,
            UnsupportedVersion,
            InvalidKeyLength,
            MalformedKeyText,
            ConflictingHeaders
        };

        static bool isWebSocketUpgradeCandidate(const HttpRequest &request);
        RequestHandlingResult handle(HttpRequest &request, IHttpRequestHandlerFactory &handlerFactory) const;

    private:
        static RequestHandlingResult rejectUpgrade(UpgradeFailure failure, IHttpRequestHandlerFactory &handlerFactory);
    };
}
