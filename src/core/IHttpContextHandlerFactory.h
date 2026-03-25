#pragma once
#include <string>
#include <string_view>

#include "../handlers/IHttpHandler.h"
#include "../response/HttpResponse.h"
namespace HttpServerAdvanced
{   
    class HttpContext;
    class IHttpContextHandlerFactory{
    public:
        static constexpr const char *ServiceName = "HttpContextHandlerFactory";
        virtual std::unique_ptr<IHttpHandler> create(HttpContext &context) = 0;
        virtual std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, std::string body) = 0;
        virtual ~IHttpContextHandlerFactory() = default;    
        virtual std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, std::string_view body)
        {
            return createResponse(status, std::string(body));
        }
        virtual std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, const char * body)
        {
            return createResponse(status, std::string_view(body != nullptr ? body : ""));
        }
    };
}



