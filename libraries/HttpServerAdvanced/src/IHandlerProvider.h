#pragma once
#include <memory>
#include <functional>
#include <vector>

#include "./IHttpHandler.h"
#include "./HttpResponse.h"
#include "./HttpStatus.h"
#include "./HttpHeader.h"
#include "./HttpHeaderCollection.h"
#include "./HttpHandler.h"
#include "./IHttpResponse.h"
namespace HttpServerAdvanced
{
    // Forward declarations
    class HttpRequest;

    class IHandlerProvider
    {
    public:
        virtual ~IHandlerProvider() = default;
        virtual bool canHandle(HttpRequest &context) = 0;
        virtual std::unique_ptr<IHttpHandler> create(HttpRequest &context) = 0;
    };
    class HandlerProvider : public IHandlerProvider
    {
    private:
        IHttpHandler::Factory factory_;
        IHttpHandler::Predicate request_;

    public:
        HandlerProvider(IHttpHandler::Factory factory, IHttpHandler::Predicate request)
            : factory_(factory), request_(request) {}
        virtual bool canHandle(HttpRequest &context) override
        {
            return request_(context);
        }
        virtual std::unique_ptr<IHttpHandler> create(HttpRequest &context) override
        {
            return factory_(context);
        }
    };

}
