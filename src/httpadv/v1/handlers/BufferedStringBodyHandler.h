#pragma once
#include <string>
#include "BufferingHttpHandlerBase.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"
#include "../util/KeyValuePairView.h"
#include "../util/HttpUtility.h"

namespace httpadv::v1::handlers
{
    using httpadv::v1::handlers::ExtractArgsFromRequest;
    using httpadv::v1::core::HttpRequestContext;
    using httpadv::v1::core::MAX_BUFFERED_FORM_BODY_LENGTH;
    using httpadv::v1::response::IHttpResponse;
    using httpadv::v1::routing::HandlerMatcher;
    using httpadv::v1::handlers::RouteParameters;

    class BufferedStringBodyHandler : public BufferingHttpHandlerBase<MAX_BUFFERED_FORM_BODY_LENGTH>
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, std::string &&)> handler_;
        ExtractArgsFromRequest extractor_;

    public:
        BufferedStringBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, std::string &&)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        BufferedStringBodyHandler(std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, RouteParameters &&, std::string &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequestContext &context, RouteParameters &&params, std::string &&postData)
                       { return handler(static_cast<httpadv::v1::core::HttpContext &>(context), std::move(params), std::move(postData)); }),
              extractor_(extractor) {}
        BufferedStringBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, std::string &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequestContext &context, RouteParameters &&, std::string &&postData)
                       { return handler(context, std::move(postData)); }),
              extractor_(extractor) {}
        BufferedStringBodyHandler(std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, std::string &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequestContext &context, RouteParameters &&, std::string &&postData)
                       { return handler(static_cast<httpadv::v1::core::HttpContext &>(context), std::move(postData)); }),
              extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(httpadv::v1::core::HttpContext &context, std::vector<uint8_t> &&body) override;
    };

    class Buffered
    {
    public:
        using BodyData = std::string;
        using LegacyInvocationWithoutParams = std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, BodyData &&)>;
        using LegacyInvocation = std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, RouteParameters &&, BodyData &&)>;
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, BodyData &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, BodyData &&)>;

        static InvocationWithoutParams adaptLegacyInvocationWithoutParams(LegacyInvocationWithoutParams handler)
        {
            return [handler](HttpRequestContext &context, BodyData &&postData)
            {
                return handler(static_cast<httpadv::v1::core::HttpContext &>(context), std::move(postData));
            };
        }

        static Invocation adaptLegacyInvocation(LegacyInvocation handler)
        {
            return [handler](HttpRequestContext &context, RouteParameters &&params, BodyData &&postData)
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
            baseUri.setAllowedContentTypes({"text/plain"});
        }
    };

} // namespace httpadv::v1::handlers

