#pragma once
#include "../core/Defines.h"
#if HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON == 1
#include "BufferingHttpHandlerBase.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"
#include "../util/KeyValuePairView.h"
#include "../util/HttpUtility.h"


#include <ArduinoJson.h>

namespace HttpServerAdvanced
{
    class JsonBodyHandler : public BufferingHttpHandlerBase<MAX_BUFFERED_JSON_BODY_LENGTH>
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpRequest &, RouteParameters &&, JsonDocument &&)> handler_;
        ExtractArgsFromRequest extractor_;

    public:
        JsonBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, RouteParameters &&, JsonDocument &&)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        JsonBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, JsonDocument &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequest &context, RouteParameters &&, JsonDocument &&postData)
                       { return handler(context, std::move(postData)); }),
              extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(HttpRequest &context, std::vector<uint8_t> &&body) override;
    };
    class Json
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequest &, JsonDocument &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequest &, RouteParameters &&, JsonDocument &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler);

        static IHttpHandler::Factory makeFactory(Invocation handler, ExtractArgsFromRequest extractor);

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler);

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler);

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler);

        static void restrict(HandlerMatcher &baseUri)
        {
            baseUri.setAllowedContentTypes({"application/json"});
        }
    };

} // namespace HttpServerAdvanced
#endif