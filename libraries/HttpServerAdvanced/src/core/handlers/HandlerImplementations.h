#pragma once
#include <Arduino.h>
#include "../HttpHandler.h"
#include "./HandlerTypes.h"
#include "./HandlerRestrictions.h"
namespace HttpServerAdvanced::Core
{

    class NoBodyHandler : public HttpHandler
    {
    private:
        // Request::Invocation handler_;
        // ParameterExtractor extractor_;

    public:
        NoBodyHandler(Request::Invocation handler, ParameterExtractor extractor)
            : HttpHandler([handler, extractor](HttpContext &context) -> IHttpHandler::HandlerResult
                                {
                                    auto params = extractor(context);
                                    return handler(context, std::move(params)); }) {}
        NoBodyHandler(Request::InvocationWithoutParams handler, ParameterExtractor extractor)
            : HttpHandler([handler](HttpContext &context) -> IHttpHandler::HandlerResult
                                { return handler(context); }) {}
    };

    class FormBodyHandler : public BufferingHttpHandlerBase
    {
    private:
        Form::Invocation handler_;
        ParameterExtractor extractor_;
    public:
        FormBodyHandler(Form::Invocation handler, ParameterExtractor extractor)
            : handler_(handler), extractor_(extractor) {}
        FormBodyHandler(Form::InvocationWithoutParams handler, ParameterExtractor extractor)
            : handler_(Form::curryWithoutParams(handler)), extractor_(extractor) {}

        virtual IHttpHandler::HandlerResult handleBody(HttpContext &context, std::vector<uint8_t> &&body) override
        {
            auto params = extractor_(context);
            StringView bodyView(reinterpret_cast<const char *>(body.data()), body.size());
            KeyValuePairView<String, String> postData = HttpUtility::parseQueryString(bodyView);
            return handler_(context, std::move(params), std::move(postData));
        }
    };

    class RawBodyHandler : public IHttpHandler
    {
    private:
        RawBody::Invocation handler_;
        ParameterExtractor extractor_;
        HandlerResult response_;
        InvocationParams params_;
        size_t receivedLength_{0};
        size_t contentLength_{0};
    public:
        RawBodyHandler(RawBody::Invocation handler, ParameterExtractor extractor)
            : handler_(handler), extractor_(extractor) {}
        RawBodyHandler(RawBody::InvocationWithoutParams handler, ParameterExtractor extractor)
            : handler_(RawBody::curryWithoutParams(handler)), extractor_(extractor) {}

        virtual HandlerResult handleStep(HttpContext &context){
            if(!response_ && context.completedPhases() >= HttpContextPhase::CompletedReadingMessage){
                handleBodyChunk(context, nullptr, 0);
            }
            if(response_){
                return std::move(response_);
            }
            return nullptr;
        }
        virtual void handleBodyChunk(HttpContext &context, const uint8_t *at, std::size_t length){
            if(response_){
                return;
            }
            if(receivedLength_ == 0){
                params_ = extractor_(context);
                std::optional<HttpHeader> contentLengthHeader = context.request().headers().find(HttpHeaders::CONTENT_LENGTH);
                contentLength_ = contentLengthHeader.has_value() ? contentLengthHeader->value().toInt() : 0;
            }
            response_ = handler_(context, params_, receivedLength_, contentLength_, at, length);
            receivedLength_ += length;
        }

    };

} // namespace HttpServerAdvanced::Core
