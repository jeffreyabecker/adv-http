#pragma once
#include <functional>
#include <string>
#include "BufferingHttpHandlerBase.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"
#include "../util/KeyValuePairView.h"
#include "../util/HttpUtility.h"

namespace lumalink::http::handlers
{
    using lumalink::http::handlers::ExtractArgsFromRequest;
    using lumalink::http::core::HttpRequestContext;
    using lumalink::http::core::MAX_BUFFERED_FORM_BODY_LENGTH;
    using lumalink::http::response::IHttpResponse;
    using lumalink::http::routing::HandlerMatcher;
    using lumalink::http::handlers::RouteParameters;

    class BufferedStringBodyHandler : public BufferingHttpHandlerBase<MAX_BUFFERED_FORM_BODY_LENGTH>
    {
    private:
        std::move_only_function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, std::string &&)> handler_;
        ExtractArgsFromRequest extractor_;

    public:
        BufferedStringBodyHandler(std::move_only_function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, std::string &&)> handler, ExtractArgsFromRequest extractor)
            : handler_(std::move(handler)), extractor_(std::move(extractor)) {}
        BufferedStringBodyHandler(std::move_only_function<IHttpHandler::HandlerResult(HttpRequestContext &, std::string &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler = std::move(handler)](HttpRequestContext &context, RouteParameters &&, std::string &&postData) mutable
                       { return handler(context, std::move(postData)); }),
              extractor_(std::move(extractor)) {}

        virtual IHttpHandler::HandlerResult handleBody(lumalink::http::core::HttpRequestContext &context, std::vector<uint8_t> &&body) override;
    };

    class Buffered
    {
    public:
        using BodyData = std::string;
        using InvocationWithoutParams = std::move_only_function<IHttpHandler::HandlerResult(HttpRequestContext &, BodyData &&)>;
        using Invocation = std::move_only_function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, BodyData &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler);

        static IHttpHandler::Factory makeFactory(Invocation handler, ExtractArgsFromRequest extractor);

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler);

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler);

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler);
        static void restrict(HandlerMatcher &baseUri)
        {
            baseUri.setAllowedContentTypes({"text/plain"});
        }
    };

} // namespace lumalink::http::handlers

