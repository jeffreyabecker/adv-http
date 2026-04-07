#pragma once
#include <memory>
#include <functional>
#include <vector>

#include "IHttpHandler.h"
#include "../response/HttpResponse.h"
#include "../core/HttpStatus.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "HttpHandler.h"
#include "../response/IHttpResponse.h"
namespace httpadv::v1::handlers
{
    using httpadv::v1::core::HttpRequestContext;

    // Forward declarations
    class IHandlerProvider
    {
    public:
        virtual ~IHandlerProvider() = default;
        virtual bool canHandle(httpadv::v1::core::HttpRequestContext &context) = 0;
        virtual std::unique_ptr<IHttpHandler> create(httpadv::v1::core::HttpRequestContext &context) = 0;
    };
    class HandlerProvider : public IHandlerProvider
    {
    private:
        IHttpHandler::Factory factory_;
        IHttpHandler::Predicate request_;

    public:
        HandlerProvider(IHttpHandler::Factory factory, IHttpHandler::Predicate request)
            : factory_(factory), request_(request) {}
        virtual bool canHandle(httpadv::v1::core::HttpRequestContext &context) override
        {
            return request_(context);
        }
        virtual std::unique_ptr<IHttpHandler> create(httpadv::v1::core::HttpRequestContext &context) override
        {
            return factory_(context);
        }
    };

}


