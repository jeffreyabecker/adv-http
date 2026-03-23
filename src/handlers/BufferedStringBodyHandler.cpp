#include "BufferedStringBodyHandler.h"
#include "../routing/HandlerMatcher.h"

namespace HttpServerAdvanced
{
    IHttpHandler::HandlerResult BufferedStringBodyHandler::handleBody(HttpRequest &context, std::vector<uint8_t> &&body)
    {
        auto params = extractor_(context);
        std::string postData(reinterpret_cast<const char *>(body.data()), body.size());
        return handler_(context, std::move(params), std::move(postData));
    }

    Buffered::Invocation Buffered::curryWithoutParams(InvocationWithoutParams handler)
    {
        return [handler](HttpRequest &context, RouteParameters &&, BodyData &&postData)
        {
            return handler(context, std::move(postData));
        };
    }

    IHttpHandler::Factory Buffered::makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
    {
        return [handler, extractor](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
        {
            auto params = extractor(context);
            return std::make_unique<BufferedStringBodyHandler>(handler, [params](HttpRequest &c)
                                                       { return params; });
        };
    }

    Buffered::Invocation Buffered::curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](HttpRequest &context, RouteParameters &&params, BodyData &&postData)
        {
            return interceptor(context, [handler, params = std::move(params), postData = std::move(postData)](HttpRequest &context) mutable
                               { return handler(context, std::move(params), std::move(postData)); });
        };
    }

    Buffered::Invocation Buffered::applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](HttpRequest &context, RouteParameters &&params, BodyData &&postData)
        {
            return interceptor(context, [handler, params = std::move(params), postData = std::move(postData)](HttpRequest &context) mutable
                               { return handler(context, std::move(params), std::move(postData)); });
        };
    }

    Buffered::Invocation Buffered::applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
    {
        return [filter, handler](HttpRequest &context, RouteParameters &&params, BodyData &&postData)
        {
            auto response = handler(context, std::move(params), std::move(postData));
            return filter(std::move(response));
        };
    }
} // namespace HttpServerAdvanced
