#pragma once
#include <Arduino.h>
#include "./Defines.h"
#include "./IHttpHandler.h"
#include "./HttpHandler.h"
#include "./HttpResponse.h"
#include "./HandlerRestrictions.h"
#include "./HandlerMatcher.h"
#include "./BufferingHttpHandlerBase.h"
#include "./KeyValuePairView.h"
#include "./Buffer.h"
#include "./MultipartFormDataHandler.h"
#include "./RawBodyHandler.h"
#include "./FormBodyHandler.h"
namespace HttpServerAdvanced
{

    // Type definitions
    class Request
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequest &)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpRequest &context, std::vector<String> &&)
            {
                return handler(context);
            };
        }

        static IHttpHandler::Factory makeFactory(Invocation handler, ParameterExtractor extractor)
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
            return [interceptor, handler](HttpRequest &context, std::vector<String> &&params)
            {
                return interceptor(context, [handler, params = std::move(params)](HttpRequest &context) mutable
                                   { return handler(context, std::move(params)); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequest &context, std::vector<String> &&params)
            {
                return interceptor(context, [handler, params = std::move(params)](HttpRequest &context) mutable
                                   { return handler(context, std::move(params)); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpRequest &context, std::vector<String> &&params)
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
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequest &, PostBodyData &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&, PostBodyData &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpRequest &context, std::vector<String> &&, PostBodyData &&postData)
            {
                return handler(context, std::move(postData));
            };
        }

        static IHttpHandler::Factory makeFactory(Invocation handler, ParameterExtractor extractor)
        {
            return [handler, extractor](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                return std::make_unique<FormBodyHandler>(handler, [params](HttpRequest &c)
                                                         { return params; });
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequest &context, std::vector<String> &&params, PostBodyData &&postData)
            {
                return interceptor(context, [handler, params = std::move(params), postData = std::move(postData)](HttpRequest &context) mutable
                                   { return handler(context, std::move(params), std::move(postData)); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequest &context, std::vector<String> &&params, PostBodyData &&postData)
            {
                return interceptor(context, [handler, params = std::move(params), postData = std::move(postData)](HttpRequest &context) mutable
                                   { return handler(context, std::move(params), std::move(postData)); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpRequest &context, std::vector<String> &&params, PostBodyData &&postData)
            {
                auto response = handler(context, std::move(params), std::move(postData));
                return filter(std::move(response));
            };
        }
        static void restrict(HandlerMatcher &baseUri)
        {
            baseUri.setAllowedContentTypes({"application/x-www-form-urlencoded"});
        }
    };
    class RawBody
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequest &, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpRequest &context, std::vector<String> &, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)
            {
                return handler(context, recievedLength, contentLength, chunk, chunkLength);
            };
        }

        static IHttpHandler::Factory makeFactory(Invocation handler, ParameterExtractor extractor)
        {
            return [handler, extractor](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                return std::make_unique<RawBodyHandler>(handler, [params](HttpRequest &c)
                                                        { return params; });
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequest &context, std::vector<String> &params, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)
            {
                return interceptor(context, [handler, &params, recievedLength, contentLength, chunk, chunkLength](HttpRequest &context)
                                   { return handler(context, params, recievedLength, contentLength, chunk, chunkLength); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequest &context, std::vector<String> &params, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)
            {
                return interceptor(context, [handler, &params, &recievedLength, &contentLength, &chunk, &chunkLength](HttpRequest &context)
                                   { return handler(context, params, recievedLength, contentLength, chunk, chunkLength); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpRequest &context, std::vector<String> &params, size_t recievedLength, size_t contentLength, const uint8_t *chunk, size_t chunkLength)
            {
                auto response = handler(context, params, recievedLength, contentLength, chunk, chunkLength);
                return filter(std::move(response));
            };
        }
        static void restrict(HandlerMatcher &baseUri)
        {
        }
    };

    class Multipart
        {
        public:
            using PostBodyData = KeyValuePairView<String, String>;
            using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequest &, PostBodyData &&)>;
            using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&, PostBodyData &&)>;

            static Invocation curryWithoutParams(InvocationWithoutParams handler)
            {
                return [handler](HttpRequest &context, std::vector<String> &&, PostBodyData &&postData)
                {
                    return handler(context, std::move(postData));
                };
            }

            static IHttpHandler::Factory makeFactory(Invocation handler, ParameterExtractor extractor)
            {
                return [handler, extractor](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
                {
                    auto params = extractor(context);
                    return std::make_unique<MultipartFormDataHandler>(handler, [params](HttpRequest &c)
                                                                       { return params; });
                };
            }

            static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
            {
                return [interceptor, handler](HttpRequest &context, std::vector<String> &&params, PostBodyData &&postData)
                {
                    return interceptor(context, [handler, params = std::move(params), postData = std::move(postData)](HttpRequest &context) mutable
                                       { return handler(context, std::move(params), std::move(postData)); });
                };
            }

            static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
            {
                return [interceptor, handler](HttpRequest &context, std::vector<String> &&params, PostBodyData &&postData)
                {
                    return interceptor(context, [handler, params = std::move(params), postData = std::move(postData)](HttpRequest &context) mutable
                                       { return handler(context, std::move(params), std::move(postData)); });
                };
            }

            static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
            {
                return [filter, handler](HttpRequest &context, std::vector<String> &&params, PostBodyData &&postData)
                {
                    auto response = handler(context, std::move(params), std::move(postData));
                    return filter(std::move(response));
                };
            }
            static void restrict(HandlerMatcher &baseUri)
            {
                baseUri.setAllowedContentTypes({"multipart/form-data"});
            }
        };

} // namespace HttpServerAdvanced
