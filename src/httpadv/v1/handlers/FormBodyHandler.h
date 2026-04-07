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
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, WebUtility::QueryParameters &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequestContext &context, RouteParameters &&, WebUtility::QueryParameters &&postData)
                       { return handler(context, std::move(postData)); }),
              extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(httpadv::v1::core::HttpRequestContext &context, std::vector<uint8_t> &&body) override;
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

} // namespace httpadv::v1::handlers

