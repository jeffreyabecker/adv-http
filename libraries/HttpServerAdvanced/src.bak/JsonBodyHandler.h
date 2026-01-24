#pragma once
#include <Arduino.h>
#include "./BufferingHttpHandlerBase.h"
#include "./HandlerRestrictions.h"
#include "./KeyValuePairView.h"
#include "./HttpUtility.h"


#include <ArduinoJson.h>

namespace HttpServerAdvanced
{
    class JsonBodyHandler : public BufferingHttpHandlerBase<MAX_BUFFERED_JSON_BODY_LENGTH>
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&, JsonDocument &&)> handler_;
        ExtractArgsFromRequest extractor_;

        std::vector<uint8_t> bodyBuffer_;

    public:
        JsonBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&, JsonDocument &&)> handler, ExtractArgsFromRequest extractor)
            : handler_(handler), extractor_(extractor) {}
        JsonBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, JsonDocument &&)> handler, ExtractArgsFromRequest extractor)
            : handler_([handler](HttpRequest &context, std::vector<String> &&, JsonDocument &&postData)
                       { return handler(context, std::move(postData)); }),
              extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(HttpRequest &context, std::vector<uint8_t> &&body) override
        {
            auto params = extractor_(context);
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, body);

            return handler_(context, std::move(params), std::move(doc));
        }
    };
    class Json
    {
    public:
        using InvocationWithoutParams = std::function<IHttpHandler::HandlerResult(HttpRequest &, JsonDocument &&)>;
        using Invocation = std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&, JsonDocument &&)>;

        static Invocation curryWithoutParams(InvocationWithoutParams handler)
        {
            return [handler](HttpRequest &context, std::vector<String> &&, JsonDocument &&jsonData)
            {
                return handler(context, std::move(jsonData));
            };
        }

        static IHttpHandler::Factory makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
        {
            return [handler, extractor](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
            {
                auto params = extractor(context);
                return std::make_unique<JsonBodyHandler>(handler, ExtractArgsFromRequest([params](HttpRequest &c)
                                                                                         { return params; }));
            };
        }

        static Invocation curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequest &context, std::vector<String> &&params, JsonDocument &&jsonData)
            {
                return interceptor(context, [handler, params = std::move(params), jsonData = std::move(jsonData)](HttpRequest &context) mutable
                                   { return handler(context, std::move(params), std::move(jsonData)); });
            };
        }

        static Invocation applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
        {
            return [interceptor, handler](HttpRequest &context, std::vector<String> &&params, JsonDocument &&jsonData)
            {
                return interceptor(context, [handler, params = std::move(params), jsonData = std::move(jsonData)](HttpRequest &context) mutable
                                   { return handler(context, std::move(params), std::move(jsonData)); });
            };
        }

        static Invocation applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
        {
            return [filter, handler](HttpRequest &context, std::vector<String> &&params, JsonDocument &&jsonData)
            {
                auto response = handler(context, std::move(params), std::move(jsonData));
                return filter(std::move(response));
            };
        }

        static void restrict(HandlerMatcher &baseUri)
        {
            baseUri.setAllowedContentTypes({"application/json"});
        }
    };

} // namespace HttpServerAdvanced
