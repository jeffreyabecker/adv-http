#pragma once
#include "IHttpRequestHandlerFactory.h"
#include "../routing/HandlerProviderRegistry.h"
#include "../response/StringResponse.h"

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

    public:
        HttpRequestHandlerFactory(HandlerProviderRegistry &providerRegistry)
            : providerRegistry_(providerRegistry) {}
        
        

        virtual std::unique_ptr<IHttpHandler> create(HttpRequest &context) override
        {
            static_cast<void>(context);
            return std::make_unique<DeferredRegistryHandler>(providerRegistry_);
        }

        virtual std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, std::string body) override
        {
            return StringResponse::create(status, std::move(body), {});
        }
    };
} // namespace HttpServerAdvanced



