#pragma once
#include "../core/Defines.h"
#if HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON == 1
#include "BufferingHttpHandlerBase.h"
#include "HandlerRestrictions.h"
#include "../routing/HandlerMatcher.h"
#include "../util/KeyValuePairView.h"
#include "../util/HttpUtility.h"


#include <ArduinoJson.h>

namespace httpadv::v1::handlers
{
    using httpadv::v1::handlers::ExtractArgsFromRequest;
    using httpadv::v1::core::HttpRequestContext;
    using httpadv::v1::core::MAX_BUFFERED_JSON_BODY_LENGTH;
    using httpadv::v1::response::IHttpResponse;
    using httpadv::v1::routing::HandlerMatcher;
    using httpadv::v1::handlers::RouteParameters;

    class JsonBodyHandler : public BufferingHttpHandlerBase<MAX_BUFFERED_JSON_BODY_LENGTH>
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, JsonDocument &&)> handler_;
        ExtractArgsFromRequest extractor_;

    public:
        JsonBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, JsonDocument &&)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        JsonBodyHandler(std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, RouteParameters &&, JsonDocument &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequestContext &context, RouteParameters &&params, JsonDocument &&postData)
                       { return handler(static_cast<httpadv::v1::core::HttpContext &>(context), std::move(params), std::move(postData)); }),
              extractor_(extractor) {}
        JsonBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequestContext &, JsonDocument &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequestContext &context, RouteParameters &&, JsonDocument &&postData)
                       { return handler(context, std::move(postData)); }),
              extractor_(extractor) {}
        JsonBodyHandler(std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, JsonDocument &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequestContext &context, RouteParameters &&, JsonDocument &&postData)
                       { return handler(static_cast<httpadv::v1::core::HttpContext &>(context), std::move(postData)); }),
              extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(httpadv::v1::core::HttpContext &context, std::vector<uint8_t> &&body) override;
    };
    class Json
    {
    public:
        using LegacyInvocationWithoutParams = std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, JsonDocument &&)>;
        using LegacyInvocation = std::function<IHttpHandler::HandlerResult(httpadv::v1::core::HttpContext &, RouteParameters &&, JsonDocument &&)>;
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, JsonDocument &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequestContext &, RouteParameters &&, JsonDocument &&)>;

        static constexpr const char *DeserializationErrorItemKey = "Json::DeserializationError";

        static InvocationWithoutParams adaptLegacyInvocationWithoutParams(LegacyInvocationWithoutParams handler)
        {
            return [handler](HttpRequestContext &context, JsonDocument &&body)
            {
                return handler(static_cast<httpadv::v1::core::HttpContext &>(context), std::move(body));
            };
        }

        static Invocation adaptLegacyInvocation(LegacyInvocation handler)
        {
            return [handler](HttpRequestContext &context, RouteParameters &&params, JsonDocument &&body)
            {
                return handler(static_cast<httpadv::v1::core::HttpContext &>(context), std::move(params), std::move(body));
            };
        }

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

} // namespace httpadv::v1::handlers
#endif