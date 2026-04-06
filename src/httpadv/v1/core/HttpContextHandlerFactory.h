#pragma once
#include "IHttpContextHandlerFactory.h"
#include "../routing/HandlerProviderRegistry.h"

namespace httpadv::v1::core
{
    using httpadv::v1::handlers::HandlerResult;
    using httpadv::v1::handlers::IHttpHandler;
    using httpadv::v1::routing::HandlerProviderRegistry;

    class HttpContext;

    class DeferredRegistryHandler : public IHttpHandler
    {
    public:
        explicit DeferredRegistryHandler(HandlerProviderRegistry &providerRegistry)
            : providerRegistry_(providerRegistry)
        {
        }

        HandlerResult handleStep(HttpContext &context) override
        {
            ensureInnerHandler(context);
            if (!innerHandler_)
            {
                return HandlerResult();
            }

            return innerHandler_->handleStep(context);
        }

        void handleBodyChunk(HttpContext &context, const uint8_t *at, std::size_t length) override
        {
            ensureInnerHandler(context);
            if (innerHandler_)
            {
                innerHandler_->handleBodyChunk(context, at, length);
            }
        }

    private:
        void ensureInnerHandler(HttpContext &context)
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
        
        

        virtual std::unique_ptr<IHttpHandler> create(HttpContext &context) override
        {
            static_cast<void>(context);
            return std::make_unique<DeferredRegistryHandler>(providerRegistry_);
        }
    };
} // namespace httpadv::v1::core



