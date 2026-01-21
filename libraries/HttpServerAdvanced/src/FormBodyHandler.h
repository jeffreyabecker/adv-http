#pragma once
#include <Arduino.h>
#include "./BufferingHttpHandlerBase.h"
#include "./KeyValuePairView.h"
#include "./HttpUtility.h"

namespace HttpServerAdvanced
{
    class FormBodyHandler : public BufferingHttpHandlerBase
    {
    private:
        std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&, KeyValuePairView<String, String> &&)> handler_;
        ParameterExtractor extractor_;

    public:
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, std::vector<String> &&, KeyValuePairView<String, String> &&)> handler, ParameterExtractor extractor)
            : handler_(handler), extractor_(extractor) {}
        FormBodyHandler(std::function<IHttpHandler::HandlerResult(HttpRequest &, KeyValuePairView<String, String> &&)> handler, ParameterExtractor extractor)
            : handler_([handler](HttpRequest &context, std::vector<String> &&, KeyValuePairView<String, String> &&postData)
                       { return handler(context, std::move(postData)); }),
              extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(HttpRequest &context, std::vector<uint8_t> &&body) override
        {
            auto params = extractor_(context);

            KeyValuePairView<String, String> postData = HttpUtility::ParseQueryString(reinterpret_cast<const char *>(body.data()), body.size());
            return handler_(context, std::move(params), std::move(postData));
        }
    };

} // namespace HttpServerAdvanced
