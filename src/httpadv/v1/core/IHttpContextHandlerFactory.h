#pragma once

#include "../handlers/IHttpHandler.h"
namespace httpadv::v1::core
{   
    class IHttpContextHandlerFactory{
    public:
        static constexpr const char *ServiceName = "HttpContextHandlerFactory";
        virtual std::unique_ptr<httpadv::v1::handlers::IHttpHandler> create(HttpRequestContext &context) = 0;
        virtual ~IHttpContextHandlerFactory() = default;
    };
}



