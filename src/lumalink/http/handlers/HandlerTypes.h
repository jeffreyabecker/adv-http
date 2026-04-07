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
#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
#include "JsonBodyHandler.h"
#endif
#include "BufferedStringBodyHandler.h"
namespace lumalink::http::handlers
{
    using lumalink::http::core::HttpRequestContext;

    // Type definitions
    class GetRequest
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequestContext &)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpRequestContext &context, RouteParameters &&)
            {
                return handler(context);
            };
        }

        static IHttpHandler::Factory makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
        {
            return [handler, extractor](lumalink::http::core::HttpRequestContext &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                return std::make_unique<HttpHandler>(IHttpHandler::InvocationCallback([handler, params = std::move(params)](HttpRequestContext &context) mutable -> IHttpHandler::HandlerResult
                {
                    return handler(context, std::move(params));
                }));
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequestContext &context, RouteParameters &&params)
            {
                return interceptor(context, IHttpHandler::InvocationNext(context, [handler, &context, params = std::move(params)]() mutable
                                   { return handler(context, std::move(params)); }));
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequestContext &context, RouteParameters &&params)
            {
                return interceptor(context, IHttpHandler::InvocationNext(context, [handler, &context, params = std::move(params)]() mutable
                                   { return handler(context, std::move(params)); }));
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpRequestContext &context, RouteParameters &&params)
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
    






} // namespace lumalink::http::handlers


