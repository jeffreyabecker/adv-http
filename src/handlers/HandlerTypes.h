#pragma once
#include <Arduino.h>
#include <vector>
#include "IHttpHandler.h"
#include "../response/IHttpResponse.h"
#include "HttpHandler.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"
#include "MultipartFormDataHandler.h"
#include "RawBodyHandler.h"
#include "FormBodyHandler.h"
#include "JsonBodyHandler.h" // JsonBodyHandler is excluded until I can figure out how to efficently serialize a response without needing to preallocate a buffer
#include "BufferedStringBodyHandler.h"
namespace HttpServerAdvanced
{

    // Type definitions
    class GetRequest
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequest &)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequest &, RouteParameters &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpRequest &context, RouteParameters &&)
            {
                return handler(context);
            };
        }

        static IHttpHandler::Factory makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
        {
            return [handler, extractor](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                return std::make_unique<HttpHandler>([handler, params = std::move(params)](HttpRequest &context) mutable -> IHttpHandler::HandlerResult
                {
                    return handler(context, std::move(params));
                });
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequest &context, RouteParameters &&params)
            {
                return interceptor(context, [handler, params = std::move(params)](HttpRequest &context) mutable
                                   { return handler(context, std::move(params)); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequest &context, RouteParameters &&params)
            {
                return interceptor(context, [handler, params = std::move(params)](HttpRequest &context) mutable
                                   { return handler(context, std::move(params)); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpRequest &context, RouteParameters &&params)
            {
                auto response = handler(context, std::move(params));
                return filter(std::move(response));
            };
        }
        static void restrict(HandlerMatcher &baseUri)
        {
        }
    };
    






} // namespace HttpServerAdvanced


