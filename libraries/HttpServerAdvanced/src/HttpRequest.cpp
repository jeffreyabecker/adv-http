#include "HttpRequest.h"
#include "IHttpHandler.h"
#include "IHttpRequestHandlerFactory.h"

namespace HttpServerAdvanced
{
    void HttpRequest::appendBodyContents(const uint8_t *at, std::size_t length)
    {
        auto handler = tryGetHandler();
        if (bodyBytesReceived_ == 0)
        {
            completedPhases_ |= HttpRequestPhase::BeginReadingBody;
            handleStep();
        }
        if (handler)
        {
            handler->handleBodyChunk(*this, at, length);
        }

        bodyBytesReceived_ += length;
        handleStep();
    }

    void HttpRequest::handleStep()
    {
        auto handler = tryGetHandler();
        if (handler)
        {
            auto newResponse = handler->handleStep(*this);
            if (newResponse && !haveSentResponse_)
            {
                response_ = std::move(newResponse);
                sendResponse();
            }
        }
    }

    void HttpRequest::sendResponse()
    {
        haveSentResponse_ = true;
        bool hasResponse = (response_ != nullptr);

        if (!response_)
        {
            response_ = handlerFactory_.createResponse(HttpStatus::InternalServerError(), String("Internal Server Error: No response generated"));
        }
        completedPhases_ |= HttpRequestPhase::WritingResponseStarted;
        onStreamReady_(CreateResponseStream(std::move(response_)));
    }

    void HttpRequest::onError(HttpServerAdvanced::PipelineError error)
    {
        if (!haveSentResponse_)
        {
            HttpStatus status;
            String message;

            switch (error.code())
            {
            case HttpServerAdvanced::PipelineErrorCode::InvalidVersion:
            case HttpServerAdvanced::PipelineErrorCode::InvalidMethod:
                status = HttpStatus::BadRequest();
                message = "Bad Request: ";
                break;
            case HttpServerAdvanced::PipelineErrorCode::UriTooLong:
            case HttpServerAdvanced::PipelineErrorCode::HeaderTooLarge:
            case HttpServerAdvanced::PipelineErrorCode::BodyTooLarge:
                status = HttpStatus::PayloadTooLarge();
                message = "Payload Too Large: ";
                break;
            case HttpServerAdvanced::PipelineErrorCode::Timeout:
                status = HttpStatus::RequestTimeout();
                message = "Request Timeout: ";
                break;
            case HttpServerAdvanced::PipelineErrorCode::UnsupportedMediaType:
                status = HttpStatus::UnsupportedMediaType();
                message = "Unsupported Media Type: ";
                break;
            default:
                status = HttpStatus::BadRequest();
                message = "Bad Request: ";
                break;
            }

            response_ = handlerFactory_.createResponse(status, std::move(message + String(error.message())));
            sendResponse();
        }
    }
} // namespace HttpServerAdvanced
