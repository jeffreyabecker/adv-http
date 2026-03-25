#pragma once
#include "BufferingHttpHandlerBase.h"
#include "../util/KeyValuePairView.h"
#include "../util/HttpUtility.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"

namespace HttpServerAdvanced
{
    class FormBodyHandler : public BufferingHttpHandlerBase<MAX_BUFFERED_FORM_BODY_LENGTH>
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpContext &, RouteParameters &&, WebUtility::QueryParameters &&)> handler_;
        ExtractArgsFromRequest extractor_;

    public:
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(HttpContext &, RouteParameters &&, WebUtility::QueryParameters &&)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(HttpContext &, WebUtility::QueryParameters &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpContext &context, RouteParameters &&, WebUtility::QueryParameters &&postData)
                       { return handler(context, std::move(postData)); }),
              extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(HttpContext &context, std::vector<uint8_t> &&body) override;
    };


class Form
    {
    public:
        using PostBodyData = WebUtility::QueryParameters;
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpContext &, PostBodyData &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpContext &, RouteParameters &&, PostBodyData &&)>;

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

