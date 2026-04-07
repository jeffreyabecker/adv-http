#pragma once

#include "../handlers/IHttpHandler.h"
namespace lumalink::http::core
{   
    class IHttpContextHandlerFactory{
    public:
        static constexpr const char *ServiceName = "HttpContextHandlerFactory";
        virtual std::unique_ptr<lumalink::http::handlers::IHttpHandler> create(HttpRequestContext &context) = 0;
        virtual ~IHttpContextHandlerFactory() = default;
    };
}



