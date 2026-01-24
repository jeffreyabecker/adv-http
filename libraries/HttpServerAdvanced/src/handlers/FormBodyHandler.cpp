#include "./FormBodyHandler.h"
#include "../util/HttpUtility.h"
#include "../routing/HandlerMatcher.h"

namespace HttpServerAdvanced
{
    IHttpHandler::HandlerResult FormBodyHandler::handleBody(HttpRequest &context, std::vector<uint8_t> &&body)
    {
        auto params = extractor_(context);
        KeyValuePairView<String, String> postData = WebUtility::ParseQueryString(reinterpret_cast<const char *>(body.data()), body.size());
        return handler_(context, std::move(params), std::move(postData));
    }

    Form::Invocation Form::curryWithoutParams(InvocationWithoutParams handler)
    {
        return [handler](HttpRequest &context, std::vector<String> &&, PostBodyData &&postData)
        {
            return handler(context, std::move(postData));
        };
    }

    IHttpHandler::Factory Form::makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
    {
        return [handler, extractor](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
        {
            auto params = extractor(context);
            return std::make_unique<FormBodyHandler>(handler, [params](HttpRequest &c)
                                                     { return params; });
        };
    }

    Form::Invocation Form::curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](HttpRequest &context, std::vector<String> &&params, PostBodyData &&postData)
        {
            return interceptor(context, [handler, params = std::move(params), postData = std::move(postData)](HttpRequest &context) mutable
                               { return handler(context, std::move(params), std::move(postData)); });
        };
    }

    Form::Invocation Form::applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](HttpRequest &context, std::vector<String> &&params, PostBodyData &&postData)
        {
            return interceptor(context, [handler, params = std::move(params), postData = std::move(postData)](HttpRequest &context) mutable
                               { return handler(context, std::move(params), std::move(postData)); });
        };
    }

    Form::Invocation Form::applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
    {
        return [filter, handler](HttpRequest &context, std::vector<String> &&params, PostBodyData &&postData)
        {
            auto response = handler(context, std::move(params), std::move(postData));
            return filter(std::move(response));
        };
    }
} // namespace HttpServerAdvanced

