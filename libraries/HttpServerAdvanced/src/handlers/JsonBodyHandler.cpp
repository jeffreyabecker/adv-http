#include "./JsonBodyHandler.h"
#include "../routing/HandlerMatcher.h"

namespace HttpServerAdvanced
{
    IHttpHandler::HandlerResult JsonBodyHandler::handleBody(HttpRequest &context, std::vector<uint8_t> &&body)
    {
        auto params = extractor_(context);
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);

        return handler_(context, std::move(params), std::move(doc));
    }

    Json::Invocation Json::curryWithoutParams(InvocationWithoutParams handler)
    {
        return [handler](HttpRequest &context, std::vector<String> &&, JsonDocument &&jsonData)
        {
            return handler(context, std::move(jsonData));
        };
    }

    IHttpHandler::Factory Json::makeFactory(Invocation handler, ExtractArgsFromRequest extractor)
    {
        return [handler, extractor](HttpRequest &context) -> std::unique_ptr<IHttpHandler>
        {
            auto params = extractor(context);
            return std::make_unique<JsonBodyHandler>(handler, ExtractArgsFromRequest([params](HttpRequest &c)
                                                                                     { return params; }));
        };
    }

    Json::Invocation Json::curryInterceptor(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](HttpRequest &context, std::vector<String> &&params, JsonDocument &&jsonData)
        {
            return interceptor(context, [handler, params = std::move(params), jsonData = std::move(jsonData)](HttpRequest &context) mutable
                               { return handler(context, std::move(params), std::move(jsonData)); });
        };
    }

    Json::Invocation Json::applyFilter(IHttpHandler::InterceptorCallback interceptor, Invocation handler)
    {
        return [interceptor, handler](HttpRequest &context, std::vector<String> &&params, JsonDocument &&jsonData)
        {
            return interceptor(context, [handler, params = std::move(params), jsonData = std::move(jsonData)](HttpRequest &context) mutable
                               { return handler(context, std::move(params), std::move(jsonData)); });
        };
    }

    Json::Invocation Json::applyResponseFilter(IHttpResponse::ResponseFilter filter, Invocation handler)
    {
        return [filter, handler](HttpRequest &context, std::vector<String> &&params, JsonDocument &&jsonData)
        {
            auto response = handler(context, std::move(params), std::move(jsonData));
            return filter(std::move(response));
        };
    }
} // namespace HttpServerAdvanced

