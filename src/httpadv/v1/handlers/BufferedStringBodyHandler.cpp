#include "BufferedStringBodyHandler.h"
#include "../routing/HandlerMatcher.h"

namespace httpadv::v1::handlers
{
    IHttpHandler::HandlerResult BufferedStringBodyHandler::handleBody(httpadv::v1::core::HttpRequestContext &context, std::vector<uint8_t> &&body)
    {
        auto params = extractor_(context);
        std::string postData(reinterpret_cast<const char *>(body.data()), body.size());
        return handler_(context, std::move(params), std::move(postData));
    }

    Buffered::Invocation Buffered::curryWithoutParams(InvocationWithoutParams handler)
    {
        return [handler](httpadv::v1::core::HttpRequestContext &context, RouteParameters &&, BodyData &&postData)
        {
            return handler(context, std::move(postData));
        };
    }

    IHttpHandler::Factory Buffered::makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
    {
        return [handler, extractor](httpadv::v1::core::HttpRequestContext &context) -> std::unique_ptr<IHttpHandler>
        {
            auto params = extractor(context);
            return std::make_unique<BufferedStringBodyHandler>(handler,
                                                               [params](httpadv::v1::core::HttpRequestContext &c)
                                                               {
                                                                   (void)c;
                                                                   return params;
                                                               });
        };
    }

    Buffered::Invocation Buffered::curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](httpadv::v1::core::HttpRequestContext &context, RouteParameters &&params, BodyData &&postData)
        {
            return interceptor(context, IHttpHandler::InvocationNext(context, [handler, &context, params = std::move(params), postData = std::move(postData)]() mutable
                               { return handler(context, std::move(params), std::move(postData)); }));
        };
    }

    Buffered::Invocation Buffered::applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](httpadv::v1::core::HttpRequestContext &context, RouteParameters &&params, BodyData &&postData)
        {
            return interceptor(context, IHttpHandler::InvocationNext(context, [handler, &context, params = std::move(params), postData = std::move(postData)]() mutable
                               { return handler(context, std::move(params), std::move(postData)); }));
        };
    }

    Buffered::Invocation Buffered::applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
    {
        return [filter, handler](httpadv::v1::core::HttpRequestContext &context, RouteParameters &&params, BodyData &&postData)
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
} // namespace HttpServerAdvanced
