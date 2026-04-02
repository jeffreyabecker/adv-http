#include "FormBodyHandler.h"
#include "../util/HttpUtility.h"
#include "../routing/HandlerMatcher.h"

namespace httpadv::v1::handlers
{
    IHttpHandler::HandlerResult FormBodyHandler::handleBody(httpadv::v1::core::HttpContext &context, std::vector<uint8_t> &&body)
    {
        auto params = extractor_(context);
        WebUtility::QueryParameters postData = WebUtility::ParseQueryParameters(reinterpret_cast<const char *>(body.data()), body.size());
        return handler_(context, std::move(params), std::move(postData));
    }

    Form::Invocation Form::curryWithoutParams(InvocationWithoutParams handler)
    {
        return [handler](httpadv::v1::core::HttpRequestContext &context, RouteParameters &&, PostBodyData &&postData)
        {
            return handler(context, std::move(postData));
        };
    }

    IHttpHandler::Factory Form::makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
    {
        return [handler, extractor](httpadv::v1::core::HttpContext &context) -> std::unique_ptr<IHttpHandler>
        {
            auto params = extractor(context);
            return std::make_unique<FormBodyHandler>(handler, [params](httpadv::v1::core::HttpContext &c)
                                                     { return params; });
        };
    }

    Form::Invocation Form::curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](httpadv::v1::core::HttpRequestContext &context, RouteParameters &&params, PostBodyData &&postData)
        {
            return interceptor(context, IHttpHandler::InvocationNext(context, [handler, &context, params = std::move(params), postData = std::move(postData)]() mutable
                               { return handler(context, std::move(params), std::move(postData)); }));
        };
    }

    Form::Invocation Form::applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](httpadv::v1::core::HttpRequestContext &context, RouteParameters &&params, PostBodyData &&postData)
        {
            return interceptor(context, IHttpHandler::InvocationNext(context, [handler, &context, params = std::move(params), postData = std::move(postData)]() mutable
                               { return handler(context, std::move(params), std::move(postData)); }));
        };
    }

    Form::Invocation Form::applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
    {
        return [filter, handler](httpadv::v1::core::HttpRequestContext &context, RouteParameters &&params, PostBodyData &&postData)
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

