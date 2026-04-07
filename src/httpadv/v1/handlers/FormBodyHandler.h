#pragma once
#include "BufferingHttpHandlerBase.h"
#include "../util/KeyValuePairView.h"
#include "../util/HttpUtility.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"

namespace lumalink::http::handlers
{
    using lumalink::http::core::MAX_BUFFERED_FORM_BODY_LENGTH;
    using lumalink::http::core::HttpRequestContext;
    using lumalink::http::handlers::ExtractArgsFromRequest;
    using lumalink::http::handlers::RouteParameters;
    using lumalink::http::response::IHttpResponse;
    using lumalink::http::routing::HandlerMatcher;
    using lumalink::http::util::WebUtility;

    class FormBodyHandler : public BufferingHttpHandlerBase<MAX_BUFFERED_FORM_BODY_LENGTH>
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, WebUtility::QueryParameters &&)> handler_;
        ExtractArgsFromRequest extractor_;

    public:
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, WebUtility::QueryParameters &&)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, WebUtility::QueryParameters &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequestContext &context, RouteParameters &&, WebUtility::QueryParameters &&postData)
                       { return handler(context, std::move(postData)); }),
              extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(lumalink::http::core::HttpRequestContext &context, std::vector<uint8_t> &&body) override;
    };


class Form
    {
    public:
        using PostBodyData = WebUtility::QueryParameters;
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, PostBodyData &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, PostBodyData &&)>;

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

} // namespace lumalink::http::handlers

