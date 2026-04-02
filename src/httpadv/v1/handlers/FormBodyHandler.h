#pragma once
#include "BufferingHttpHandlerBase.h"
#include "../util/KeyValuePairView.h"
#include "../util/HttpUtility.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"

namespace httpadv::v1::handlers
{
    using httpadv::v1::core::MAX_BUFFERED_FORM_BODY_LENGTH;
    using httpadv::v1::core::HttpRequestContext;
    using httpadv::v1::handlers::ExtractArgsFromRequest;
    using httpadv::v1::handlers::RouteParameters;
    using httpadv::v1::response::IHttpResponse;
    using httpadv::v1::routing::HandlerMatcher;
    using httpadv::v1::util::WebUtility;

    class FormBodyHandler : public BufferingHttpHandlerBase<MAX_BUFFERED_FORM_BODY_LENGTH>
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, WebUtility::QueryParameters &&)> handler_;
        ExtractArgsFromRequest extractor_;

    public:
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, WebUtility::QueryParameters &&)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, RouteParameters &&, WebUtility::QueryParameters &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequestContext &context, RouteParameters &&params, WebUtility::QueryParameters &&postData)
                       { return handler(static_cast<httpadv::v1::core::HttpContext &>(context), std::move(params), std::move(postData)); }),
              extractor_(extractor) {}
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, WebUtility::QueryParameters &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequestContext &context, RouteParameters &&, WebUtility::QueryParameters &&postData)
                       { return handler(context, std::move(postData)); }),
              extractor_(extractor) {}
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, WebUtility::QueryParameters &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequestContext &context, RouteParameters &&, WebUtility::QueryParameters &&postData)
                       { return handler(static_cast<httpadv::v1::core::HttpContext &>(context), std::move(postData)); }),
              extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(httpadv::v1::core::HttpContext &context, std::vector<uint8_t> &&body) override;
    };


class Form
    {
    public:
        using PostBodyData = WebUtility::QueryParameters;
        using LegacyInvocationWithoutParams = std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, PostBodyData &&)>;
        using LegacyInvocation = std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, RouteParameters &&, PostBodyData &&)>;
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, PostBodyData &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, PostBodyData &&)>;

        static InvocationWithoutParams adaptLegacyInvocationWithoutParams(LegacyInvocationWithoutParams handler)
        {
            return [handler](HttpRequestContext &context, PostBodyData &&postData)
            {
                return handler(static_cast<httpadv::v1::core::HttpContext &>(context), std::move(postData));
            };
        }

        static Invocation adaptLegacyInvocation(LegacyInvocation handler)
        {
            return [handler](HttpRequestContext &context, RouteParameters &&params, PostBodyData &&postData)
            {
                return handler(static_cast<httpadv::v1::core::HttpContext &>(context), std::move(params), std::move(postData));
            };
        }

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

} // namespace httpadv::v1::handlers

