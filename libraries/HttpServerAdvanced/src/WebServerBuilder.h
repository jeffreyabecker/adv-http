#pragma once
#include <Arduino.h>
#include <functional>
#include "./core/Defines.h"
#include "./routing/HandlerProviderRegistry.h"
#include "./HttpRequest.h"
#include "./routing/ProviderRegistryBuilder.h"
#include "./core/HttpContentTypes.h"
#include "./HttpRequestHandlerFactory.h"
#include <any>

#include "./HttpServerBase.h"

namespace HttpServerAdvanced
{

    class WebServerBuilder //: ServiceManagerService<HttpServerAdvanced::HttpServerBase, WebServerBuilder>
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
                return HttpRequest::createPipelineHandler(server, handlerFactory_);
            });
            // server_.addService<HandlerProviderRegistry *>(server_, HandlerProviderRegistry::ServiceName, &providerRegistry_);
            // server_.addService<IHttpRequestHandlerFactory *>(server_, IHttpRequestHandlerFactory::ServiceName, &handlerFactory_);
            // server_.addService<HttpContentTypes *>(server_, HttpContentTypes::ServiceName, &contentTypes_);
        }


        WebServerBuilder(HttpServerBase &server) : handlersBuilder_(providerRegistry_), handlerFactory_(providerRegistry_), server_(server) {}
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

