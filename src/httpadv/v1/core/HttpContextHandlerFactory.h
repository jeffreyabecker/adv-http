#pragma once
#include "IHttpContextHandlerFactory.h"
#include "../routing/HandlerProviderRegistry.h"

namespace lumalink::http::core
{
    using lumalink::http::handlers::HandlerResult;
    using lumalink::http::handlers::IHttpHandler;
    using lumalink::http::routing::HandlerProviderRegistry;

    class DeferredRegistryHandler : public IHttpHandler
    {
    public:
        explicit DeferredRegistryHandler(HandlerProviderRegistry &providerRegistry)
            : providerRegistry_(providerRegistry)
        {
        }

        HandlerResult handleStep(HttpRequestContext &context) override
        {
            ensureInnerHandler(context);
            if (!innerHandler_)
            {
                return HandlerResult();
            }

            return innerHandler_->handleStep(context);
        }

        void handleBodyChunk(HttpRequestContext &context, const uint8_t *at, std::size_t length) override
        {
            ensureInnerHandler(context);
            if (innerHandler_)
            {
                innerHandler_->handleBodyChunk(context, at, length);
            }
        }

    private:
        void ensureInnerHandler(HttpRequestContext &context)
        {
            if (innerHandler_ || (context.completedPhases() & HttpContextPhase::CompletedReadingHeaders) == 0)
            {
                return;
            }

            innerHandler_ = providerRegistry_.createContextHandler(context);
        }

        HandlerProviderRegistry &providerRegistry_;
        std::unique_ptr<IHttpHandler> innerHandler_;
    };

    class HttpContextHandlerFactory : public IHttpContextHandlerFactory
    {
    private:
        HandlerProviderRegistry &providerRegistry_;

    public:
        HttpContextHandlerFactory(HandlerProviderRegistry &providerRegistry)
            : providerRegistry_(providerRegistry) {}
        
        

        virtual std::unique_ptr<IHttpHandler> create(HttpRequestContext &context) override
        {
            static_cast<void>(context);
            return std::make_unique<DeferredRegistryHandler>(providerRegistry_);
        }
    };
} // namespace lumalink::http::core



