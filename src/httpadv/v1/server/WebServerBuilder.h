#pragma once
#include <functional>
#include <type_traits>
#include <utility>
#include "../core/Defines.h"
#include "../routing/HandlerProviderRegistry.h"
#include "../core/HttpContext.h"
#include "../routing/ProviderRegistryBuilder.h"
#include "../core/HttpContentTypes.h"
#include "../core/HttpContextHandlerFactory.h"
#include <any>

#include "HttpServerBase.h"

namespace httpadv::v1::server
{
    using httpadv::v1::core::HttpContentTypes;
    using httpadv::v1::core::HttpContext;
    using httpadv::v1::core::HttpContextHandlerFactory;
    using httpadv::v1::routing::HandlerProviderRegistry;
    using httpadv::v1::routing::ProviderRegistryBuilder;

    class WebServerBuilder 
    {
    protected:

        HandlerProviderRegistry providerRegistry_;
        ProviderRegistryBuilder handlersBuilder_;
        HttpContentTypes contentTypes_;
        HttpContextHandlerFactory handlerFactory_;
        HttpServerBase & server_;


    public:
        void init()
        {
            server_.setPipelineHandlerFactory([this](HttpServerBase &server) {
                return HttpContext::createPipelineHandler(server, handlerFactory_);
            });
        }


        WebServerBuilder(HttpServerBase &server)
        : handlersBuilder_(providerRegistry_),
          handlerFactory_(providerRegistry_),
              server_(server)
        {
        }
        ~WebServerBuilder() = default;

        template <typename TComponent>
        WebServerBuilder &use(TComponent &&component)
        {
            using ComponentType = std::remove_reference_t<TComponent>;
            static_assert(std::is_invocable_v<TComponent, WebServerBuilder &>, "component must be invocable with WebServerBuilder&");

            if constexpr (std::is_convertible_v<const ComponentType &, bool>)
            {
                if (!component)
                {
                    return *this;
                }
            }

            std::forward<TComponent>(component)(*this);
            return *this;
        }

        HandlerProviderRegistry &handlerProviders() { return providerRegistry_; }
        ProviderRegistryBuilder &handlers() { return handlersBuilder_; }
        HttpContentTypes &contentTypes() { return contentTypes_; }
    };

} // namespace httpadv::v1::server


