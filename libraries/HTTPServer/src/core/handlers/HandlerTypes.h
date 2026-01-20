#pragma once
#include <Arduino.h>
#include "../HttpHandler.h"
#include "../HttpResponse.h"
#include "./HandlerUtilities.h"
#include "./HandlerMatcher.h"
#include "./HandlerImplementations.h"

namespace HTTPServer::Core
{
    class Request
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpContext &)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpContext &, std::vector<String> &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpContext &context, std::vector<String> &&)
            {
                return handler(context);
            };
        }

        static HttpHandlerFactory::Factory makeFactory(Invocation handler, ParameterExtractor extractor)
        {
            return [handler, extractor](HttpContext &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                return std::make_unique<NoBodyHandler>(handler, [params](HttpContext &c)
                                                       { return params; });
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpContext &context, std::vector<String> &&params)
            {
                return interceptor(context, [handler, &params](HttpContext &context)
                                   { return handler(context, std::move(params)); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpContext &context, std::vector<String> &&params)
            {
                return interceptor(context, [handler, &params](HttpContext &context)
                                   { return handler(context, std::move(params)); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpContext &context, std::vector<String> &&params)
            {
                auto response = handler(context, std::move(params));
                return filter(std::move(response));
            };
        }
        static void restrict(HandlerMatcher &baseUri)
        {
        }
    };

    class Form
    {
    public:
        using PostBodyData = KeyValuePairView<String, String>;
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpContext &, PostBodyData &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpContext &, std::vector<String> &&, PostBodyData &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpContext &context, std::vector<String> &&, PostBodyData &&postData)
            {
                return handler(context, std::move(postData));
            };
        }

        static HttpHandlerFactory::Factory makeFactory(Invocation handler, ParameterExtractor extractor)
        {
            return [handler, extractor](HttpContext &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                return std::make_unique<FormBodyHandler>(handler, [params](HttpContext &c)
                                                         { return params; });
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpContext &context, std::vector<String> &&params, PostBodyData &&postData)
            {
                return interceptor(context, [handler, &params, &postData](HttpContext &context)
                                   { return handler(context, std::move(params), std::move(postData)); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpContext &context, std::vector<String> &&params, PostBodyData &&postData)
            {
                return interceptor(context, [handler, &params, &postData](HttpContext &context)
                                   { return handler(context, std::move(params), std::move(postData)); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpContext &context, std::vector<String> &&params, PostBodyData &&postData)
            {
                auto response = handler(context, std::move(params), std::move(postData));
                return filter(std::move(response));
            };
        }
        static void restrict(HandlerMatcher &baseUri)
        {
        }
    };

    class RawBody
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpContext &, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpContext &, std::vector<String> &, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpContext &context, std::vector<String> &, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)
            {
                return handler(context, recievedLength, contentLength, chunk, chunkLength);
            };
        }

        static HttpHandlerFactory::Factory makeFactory(Invocation handler, ParameterExtractor extractor)
        {
            return [handler, extractor](HttpContext &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                return std::make_unique<RawBodyHandler>(handler, [params](HttpContext &c)
                                                        { return params; });
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpContext &context, std::vector<String> &params, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)
            {
                return interceptor(context, [handler, &params, &recievedLength, &contentLength, &chunk, &chunkLength](HttpContext &context)
                                   { return handler(context, params, recievedLength, contentLength, chunk, chunkLength); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpContext &context, std::vector<String> &params, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)
            {
                return interceptor(context, [handler, &params, &recievedLength, &contentLength, &chunk, &chunkLength](HttpContext &context)
                                   { return handler(context, params, recievedLength, contentLength, chunk, chunkLength); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpContext &context, std::vector<String> &params, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)
            {
                auto response = handler(context, params, recievedLength, contentLength, chunk, chunkLength);
                return filter(std::move(response));
            };
        }
        static void restrict(HandlerMatcher &baseUri)
        {
        }
    };

} // namespace HTTPServer::Core
