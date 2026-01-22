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
        std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&,JsonDocument &&)> handler_;
        ParameterExtractor extractor_;

        std::vector<uint8_t> bodyBuffer_;
    public:
        JsonBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&,JsonDocument &&)> handler, ParameterExtractor extractor)
            : handler_(handler), extractor_(extractor) {}
        JsonBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &,JsonDocument &&)> handler, ParameterExtractor extractor)
            : handler_([handler](HttpRequest &context, std::vector<String> &&,JsonDocument &&postData)
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



} // namespace HttpServerAdvanced
