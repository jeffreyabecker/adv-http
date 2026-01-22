#pragma once
#include <Arduino.h>
#include "./BufferingHttpHandlerBase.h"
#include "./KeyValuePairView.h"
#include "./HttpUtility.h"

namespace HttpServerAdvanced
{
    class FormBodyHandler : public BufferingHttpHandlerBase<MAX_BUFFERED_FORM_BODY_LENGTH>
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&, KeyValuePairView<String, String> &&)> handler_;
        ExtractArgsFromRequest extractor_;

    public:
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&, KeyValuePairView<String, String> &&)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, KeyValuePairView<String, String> &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequest &context, std::vector<String> &&, KeyValuePairView<String, String> &&postData)
                       { return handler(context, std::move(postData)); }),
              extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(HttpRequest &context, std::vector<uint8_t> &&body) override
        {
            auto params = extractor_(context);

            KeyValuePairView<String, String> postData = HttpUtility::ParseQueryString(reinterpret_cast<const char *>(body.data()), body.size());
            return handler_(context, std::move(params), std::move(postData));
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

        static IHttpHandler::Factory makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
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

} // namespace HttpServerAdvanced
