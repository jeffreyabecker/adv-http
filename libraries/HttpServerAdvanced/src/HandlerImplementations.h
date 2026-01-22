#pragma once
#include <Arduino.h>
#include "./Defines.h"
#include "./HandlerTypes.h"
#include "./HttpHandler.h"
#include "./KeyValuePairView.h"

namespace HttpServerAdvanced
{

    class NoBodyHandler : public HttpHandler
    {
    private:
        // Request::Invocation handler_;
        // ParameterExtractor extractor_;

    public:
        NoBodyHandler(Request::Invocation handler, ParameterExtractor extractor)
            : HttpHandler([handler, extractor](HttpRequest &context) -> IHttpHandler::HandlerResult
                          {
                                    auto params = extractor(context);
                                    return handler(context, std::move(params)); }) {}
        NoBodyHandler(Request::InvocationWithoutParams handler, ParameterExtractor extractor)
            : HttpHandler([handler](HttpRequest &context) -> IHttpHandler::HandlerResult
                          { return handler(context); }) {}
    };

    // FormBodyHandler is defined in `FormBodyHandler.h` (extracted to separate header)

    class RawBodyHandler : public IHttpHandler
    {
    private:
        RawBody::Invocation handler_;
        ParameterExtractor extractor_;
        HandlerResult response_;
        std::vector<String> params_;
        size_t receivedLength_{0};
        size_t contentLength_{0};

    public:
        RawBodyHandler(RawBody::Invocation handler, ParameterExtractor extractor)
            : handler_(handler), extractor_(extractor) {}
        RawBodyHandler(RawBody::InvocationWithoutParams handler, ParameterExtractor extractor)
            : handler_(RawBody::curryWithoutParams(handler)), extractor_(extractor) {}

        virtual HandlerResult handleStep(HttpRequest &context)
        {
            if (!response_ && context.completedPhases() >= HttpRequestPhase::CompletedReadingMessage)
            {
                handleBodyChunk(context, nullptr, 0);
            }
            if (response_)
            {
                return std::move(response_);
            }
            return nullptr;
        }
        virtual void handleBodyChunk(HttpRequest &context, const uint8_t *at, std::size_t length)
        {
            if (response_)
            {
                return;
            }
            if (receivedLength_ == 0)
            {
                params_ = extractor_(context);
                std::optional<HttpHeader> contentLengthHeader = context.request().headers().find(HttpHeader::ContentLength);
                contentLength_ = contentLengthHeader.has_value() ? contentLengthHeader->value().toInt() : 0;
            }
            response_ = handler_(context, params_, receivedLength_, contentLength_, at, length);
            receivedLength_ += length;
        }
    };

} // namespace HttpServerAdvanced
