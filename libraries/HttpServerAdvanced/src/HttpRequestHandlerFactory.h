#pragma once
#include "./server/IHttpRequestHandlerFactory.h"
#include "./routing/HandlerProviderRegistry.h"
#include "./response/HttpResponse.h"
#include "./response/StringResponse.h"

namespace HttpServerAdvanced
{
    class HttpRequest;
    class HttpRequestHandlerFactory : public IHttpRequestHandlerFactory
    {
    private:
        HandlerProviderRegistry &providerRegistry_;

    public:
        HttpRequestHandlerFactory(HandlerProviderRegistry &providerRegistry)
            : providerRegistry_(providerRegistry) {}
        
        

        virtual std::unique_ptr<IHttpHandler> create(HttpRequest &context) override
        {
            return providerRegistry_.createContextHandler(context);
        }

        virtual std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, String &&body) override
        {
            return StringResponse::create(status, std::move(body), {});
        }
    };
} // namespace HttpServerAdvanced



