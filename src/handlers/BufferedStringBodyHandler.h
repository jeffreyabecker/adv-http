#pragma once
#include <string>
#include "BufferingHttpHandlerBase.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"
#include "../util/KeyValuePairView.h"
#include "../util/HttpUtility.h"

namespace HttpServerAdvanced
{
    class BufferedStringBodyHandler : public BufferingHttpHandlerBase<MAX_BUFFERED_FORM_BODY_LENGTH>
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpContext &, RouteParameters &&, std::string &&)> handler_;
        ExtractArgsFromRequest extractor_;

    public:
        BufferedStringBodyHandler(std::function<IHttpHandler::HandlerResult(HttpContext &, RouteParameters &&, std::string &&)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        BufferedStringBodyHandler(std::function<IHttpHandler::HandlerResult(HttpContext &, std::string &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpContext &context, RouteParameters &&, std::string &&postData)
                       { return handler(context, std::move(postData)); }),
              extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(HttpContext &context, std::vector<uint8_t> &&body) override;
    };

    class Buffered
    {
    public:
        using BodyData = std::string;
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpContext &, BodyData &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpContext &, RouteParameters &&, BodyData &&)>;

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

} // namespace HttpServerAdvanced

