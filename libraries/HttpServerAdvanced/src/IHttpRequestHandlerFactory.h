#pragma once
#include "./IHttpHandler.h"
#include "./HttpResponse.h"
namespace HttpServerAdvanced
{   
    class HttpRequest;
    class IHttpRequestHandlerFactory{
    public:
        static constexpr const char *ServiceName = "HttpRequestHandlerFactory";
        virtual std::unique_ptr<IHttpHandler> create(HttpRequest &context) = 0;
        virtual std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, String &&body, HttpHeaderCollection headers = HttpHeaderCollection()) = 0;
        virtual ~IHttpRequestHandlerFactory() = default;    
        virtual std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, const char * body, HttpHeaderCollection headers = HttpHeaderCollection())
        {
            return HttpResponse::create(status, std::move(String(body)), std::move(headers));
        }
    };
}