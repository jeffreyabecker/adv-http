#pragma once
#include "./IHttpRequestHandlerFactory.h"
#include "./HandlerProviderRegistry.h"
#include "./HttpResponse.h"

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

        virtual std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, String &&body, HttpHeadersCollection headers = HttpHeadersCollection()) override
        {
            return HttpResponse::create(status, std::move(body), headers);
        }
    };
} // namespace HttpServerAdvanced