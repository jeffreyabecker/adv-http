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

    class DeferredRegistryHandler : public IHttpHandler
    {
    public:
        explicit DeferredRegistryHandler(HandlerProviderRegistry &providerRegistry)
            : providerRegistry_(providerRegistry)
        {
        }

        HandlerResult handleStep(HttpRequest &context) override
        {
            ensureInnerHandler(context);
            if (!innerHandler_)
            {
                return HandlerResult();
            }

            return innerHandler_->handleStep(context);
        }

        void handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length) override
        {
            ensureInnerHandler(context);
            if (innerHandler_)
            {
                innerHandler_->handleBodyChunk(context, at, length);
            }
        }

    private:
        void ensureInnerHandler(HttpRequest &context)
        {
            if (innerHandler_ || (context.completedPhases() & HttpRequestPhase::CompletedReadingHeaders) == 0)
            {
                return;
            }

            innerHandler_ = providerRegistry_.createContextHandler(context);
        }

        HandlerProviderRegistry &providerRegistry_;
        std::unique_ptr<IHttpHandler> innerHandler_;
    };

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
            static_cast<void>(context);
            return std::make_unique<DeferredRegistryHandler>(providerRegistry_);
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



