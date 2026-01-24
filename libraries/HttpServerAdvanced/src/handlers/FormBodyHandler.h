#pragma once
#include <Arduino.h>
#include "BufferingHttpHandlerBase.h"
#include "../util/KeyValuePairView.h"
#include "../util/HttpUtility.h"
#include "HandlerRestrictions.h"
#include "./HandlerMatcher.h"

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

        virtual IHttpHandler::HandlerResult handleBody(HttpRequest &context, std::vector<uint8_t> &&body) override;
    };


class Form
    {
    public:
        using PostBodyData = KeyValuePairView<String, String>;
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequest &, PostBodyData &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&, PostBodyData &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler);

        static IHttpHandler::Factory makeFactory(Invocation handler, ExtractArgsFromRequest extractor);

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler);

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler);

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler);
        static void restrict(HandlerMatcher &baseUri)
        {
            baseUri.setAllowedContentTypes({"application/x-www-form-urlencoded"});
        }
    };

} // namespace HttpServerAdvanced

