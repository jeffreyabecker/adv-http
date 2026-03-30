#pragma once
#include <vector>
#include "IHttpHandler.h"
#include "../response/IHttpResponse.h"
#include "HttpHandler.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"
#include "MultipartFormDataHandler.h"
#include "RawBodyHandler.h"
#include "FormBodyHandler.h"
#include "../core/Defines.h"
#if HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON == 1
#include "JsonBodyHandler.h"
#endif
#include "BufferedStringBodyHandler.h"
namespace httpadv::v1::handlers
{

    // Type definitions
    class GetRequest
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, RouteParameters &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](httpadv::v1::core::HttpContext &context, RouteParameters &&)
            {
                return handler(context);
            };
        }

        static IHttpHandler::Factory makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
        {
            return [handler, extractor](httpadv::v1::core::HttpContext &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                return std::make_unique<HttpHandler>([handler, params = std::move(params)](httpadv::v1::core::HttpContext &context) mutable -> IHttpHandler::HandlerResult
                {
                    return handler(context, std::move(params));
                });
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](httpadv::v1::core::HttpContext &context, RouteParameters &&params)
            {
                return interceptor(context, [handler, params = std::move(params)](httpadv::v1::core::HttpContext &context) mutable
                                   { return handler(context, std::move(params)); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](httpadv::v1::core::HttpContext &context, RouteParameters &&params)
            {
                return interceptor(context, [handler, params = std::move(params)](httpadv::v1::core::HttpContext &context) mutable
                                   { return handler(context, std::move(params)); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](httpadv::v1::core::HttpContext &context, RouteParameters &&params)
            {
                auto response = handler(context, std::move(params));
                if (!response.isResponse())
                {
                    return response;
                }

                response.response = filter(std::move(response.response));
                return response;
            };
        }
        static void restrict(HandlerMatcher &baseUri)
        {
        }
    };
    






} // namespace HttpServerAdvanced


