#pragma once
#include "IHttpRequestHandlerFactory.h"
#include "../routing/HandlerProviderRegistry.h"
#include "../response/HttpResponse.h"
#include "../response/StringResponse.h"
#include "../routing/HandlerMatcher.h"
#include "../websocket/WebSocketRoute.h"
#include "../websocket/WebSocketUpgradeHandler.h"

#include <vector>

namespace HttpServerAdvanced
{
    class HttpRequest;
    class HttpRequestHandlerFactory : public IHttpRequestHandlerFactory
    {
    private:
        HandlerProviderRegistry &providerRegistry_;
        const std::vector<WebSocketRoute> *webSocketRoutes_ = nullptr;

    public:
        HttpRequestHandlerFactory(HandlerProviderRegistry &providerRegistry, const std::vector<WebSocketRoute> *webSocketRoutes = nullptr)
            : providerRegistry_(providerRegistry),
              webSocketRoutes_(webSocketRoutes) {}
        
        

        virtual std::unique_ptr<IHttpHandler> create(HttpRequest &context) override
        {
            return providerRegistry_.createContextHandler(context);
        }

        RequestHandlingResult tryCreateRequestResult(HttpRequest &context) override
        {
            if (!WebSocketUpgradeHandler::isWebSocketUpgradeCandidate(context) || webSocketRoutes_ == nullptr)
            {
                return RequestHandlingResult();
            }

            const WebSocketCallbacks *matchedCallbacks = nullptr;
            for (const auto &route : *webSocketRoutes_)
            {
                if (defaultCheckUriPattern(context.uriView().path(), route.path))
                {
                    matchedCallbacks = &route.callbacks;
                    break;
                }
            }

            if (matchedCallbacks == nullptr)
            {
                return RequestHandlingResult();
            }

            WebSocketUpgradeHandler upgradeHandler;
            return upgradeHandler.handle(context, *this, *matchedCallbacks);
        }

        virtual std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, std::string body) override
        {
            return StringResponse::create(status, std::move(body), {});
        }
    };
} // namespace HttpServerAdvanced



