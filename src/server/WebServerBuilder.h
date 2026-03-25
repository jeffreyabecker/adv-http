#pragma once
#include <functional>
#include "../core/Defines.h"
#include "../routing/HandlerProviderRegistry.h"
#include "../core/HttpContext.h"
#include "../routing/ProviderRegistryBuilder.h"
#include "../core/HttpContentTypes.h"
#include "../core/HttpRequestHandlerFactory.h"
#include <any>

#include "HttpServerBase.h"

namespace HttpServerAdvanced
{

    class WebServerBuilder 
    {
    protected:

        HandlerProviderRegistry providerRegistry_;
        ProviderRegistryBuilder handlersBuilder_;
        HttpContentTypes contentTypes_;
        HttpRequestHandlerFactory handlerFactory_;
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

        WebServerBuilder &use(std::function<void(WebServerBuilder &)> component)
        {
            if (component)
            {
                component(*this);
            }
            return *this;
        }

        HandlerProviderRegistry &handlerProviders() { return providerRegistry_; }
        ProviderRegistryBuilder &handlers() { return handlersBuilder_; }
        HttpContentTypes &contentTypes() { return contentTypes_; }
    };

} // namespace HttpServerAdvanced


