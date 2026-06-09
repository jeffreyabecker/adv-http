#include "BufferedStringBodyHandler.h"
#include "../routing/HandlerMatcher.h"

namespace lumalink::http::handlers
{
    IHttpHandler::HandlerResult BufferedStringBodyHandler::handleBody(lumalink::http::core::HttpRequestContext &context, std::vector<uint8_t> &&body)
    {
        auto params = extractor_(context);
        std::string postData(reinterpret_cast<const char *>(body.data()), body.size());
        return handler_(context, std::move(params), std::move(postData));
    }

    Buffered::Invocation Buffered::curryWithoutParams(InvocationWithoutParams handler)
    {
        return [handler = std::move(handler)](lumalink::http::core::HttpRequestContext &context, RouteParameters &&, BodyData &&postData) mutable
        {
            return handler(context, std::move(postData));
        };
    }

    IHttpHandler::Factory Buffered::makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
    {
        auto handlerRef = std::make_shared<Invocation>(std::move(handler));
        return [handlerRef, extractor = std::move(extractor)](lumalink::http::core::HttpRequestContext &context) mutable -> std::unique_ptr<IHttpHandler>
        {
            auto params = extractor(context);
            return std::make_unique<BufferedStringBodyHandler>(Invocation([handlerRef](lumalink::http::core::HttpRequestContext &innerContext, RouteParameters &&innerParams, BodyData &&innerBody) mutable -> IHttpHandler::HandlerResult
                                                                          {
                                                                              return (*handlerRef)(innerContext, std::move(innerParams), std::move(innerBody));
                                                                          }),
                                                               [params](lumalink::http::core::HttpRequestContext &c)
                                                               {
                                                                   (void)c;
                                                                   return params;
                                                               });
        };
    }

    Buffered::Invocation Buffered::curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        auto interceptorRef = std::make_shared<IHttpHandler::InterceptorCallback>(std::move(interceptor));
        auto handlerRef = std::make_shared<Invocation>(std::move(handler));
        return [interceptorRef, handlerRef](lumalink::http::core::HttpRequestContext &context, RouteParameters &&params, BodyData &&postData) mutable
        {
            return (*interceptorRef)(context, IHttpHandler::InvocationNext(context, [handlerRef, &context, params = std::move(params), postData = std::move(postData)]() mutable
                               { return (*handlerRef)(context, std::move(params), std::move(postData)); }));
        };
    }

    Buffered::Invocation Buffered::applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        auto interceptorRef = std::make_shared<IHttpHandler::InterceptorCallback>(std::move(interceptor));
        auto handlerRef = std::make_shared<Invocation>(std::move(handler));
        return [interceptorRef, handlerRef](lumalink::http::core::HttpRequestContext &context, RouteParameters &&params, BodyData &&postData) mutable
        {
            return (*interceptorRef)(context, IHttpHandler::InvocationNext(context, [handlerRef, &context, params = std::move(params), postData = std::move(postData)]() mutable
                               { return (*handlerRef)(context, std::move(params), std::move(postData)); }));
        };
    }

    Buffered::Invocation Buffered::applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
    {
        return [filter, handler = std::move(handler)](lumalink::http::core::HttpRequestContext &context, RouteParameters &&params, BodyData &&postData) mutable
        {
            auto response = handler(context, std::move(params), std::move(postData));
            if (!response.isResponse())
            {
                return response;
            }

            response.response = filter(std::move(response.response));
            return response;
        };
    }
} // namespace lumalink::http::handlers
