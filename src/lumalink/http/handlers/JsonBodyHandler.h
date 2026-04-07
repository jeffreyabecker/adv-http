#pragma once
#include "../core/Defines.h"
#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
#include "BufferingHttpHandlerBase.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"
#include "../util/KeyValuePairView.h"
#include "../util/HttpUtility.h"


#include <ArduinoJson.h>

namespace lumalink::http::handlers
{
    using lumalink::http::handlers::ExtractArgsFromRequest;
    using lumalink::http::core::HttpRequestContext;
    using lumalink::http::core::MAX_BUFFERED_JSON_BODY_LENGTH;
    using lumalink::http::response::IHttpResponse;
    using lumalink::http::routing::HandlerMatcher;
    using lumalink::http::handlers::RouteParameters;

    class JsonBodyHandler : public BufferingHttpHandlerBase<MAX_BUFFERED_JSON_BODY_LENGTH>
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, JsonDocument &&)> handler_;
        ExtractArgsFromRequest extractor_;

    public:
        JsonBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, JsonDocument &&)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        JsonBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, JsonDocument &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequestContext &context, RouteParameters &&, JsonDocument &&postData)
                       { return handler(context, std::move(postData)); }),
              extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(lumalink::http::core::HttpRequestContext &context, std::vector<uint8_t> &&body) override;
    };
    class Json
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, JsonDocument &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, JsonDocument &&)>;

        static constexpr const char *DeserializationErrorItemKey = "Json::DeserializationError";

        static Invocation curryWithoutParams(InvocationWithoutParams handler);

        static IHttpHandler::Factory makeFactory(Invocation handler, ExtractArgsFromRequest extractor);

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler);

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler);

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler);

        static const DeserializationError *deserializationError(const HttpRequestContext &context)
        {
            const auto &items = context.items();
            auto iterator = items.find(DeserializationErrorItemKey);
            if (iterator == items.end())
            {
                return nullptr;
            }

            return std::any_cast<DeserializationError>(&iterator->second);
        }

        static void restrict(HandlerMatcher &baseUri)
        {
            baseUri.setAllowedContentTypes({"application/json"});
        }
    };

} // namespace lumalink::http::handlers
#endif