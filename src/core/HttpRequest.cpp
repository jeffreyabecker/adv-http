#include "../core/HttpRequest.h"
#include "../handlers/IHttpHandler.h"
#include "IHttpRequestHandlerFactory.h"
#include "../websocket/WebSocketUpgradeHandler.h"

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
        if (pendingResult_.hasValue())
        {
            return;
        }

        if ((completedPhases_ & HttpRequestPhase::CompletedReadingHeaders) != 0)
        {
            if (WebSocketUpgradeHandler::isWebSocketUpgradeCandidate(*this))
            {
                WebSocketUpgradeHandler upgradeHandler;
                RequestHandlingResult upgradeResult = upgradeHandler.handle(*this, handlerFactory_);
                if (upgradeResult.hasValue())
                {
                    if (upgradeResult.kind == RequestHandlingResult::Kind::Response)
                    {
                        setPendingResult(std::move(upgradeResult));
                        sendResponse();
                    }
                    else
                    {
                        setPendingResult(std::move(upgradeResult));
                    }
                    return;
                }
            }
        }

        auto handler = tryGetHandler();
        if (handler)
        {
            auto newResponse = handler->handleStep(*this);
            if (newResponse)
            {
                setPendingResult(RequestHandlingResult::response(CreateResponseStream(std::move(newResponse))));
                sendResponse();
                return;
            }
        }

        if ((completedPhases_ & HttpRequestPhase::CompletedReadingMessage) != 0 && !pendingResult_.hasValue())
        {
            setPendingResult(RequestHandlingResult::noResponse());
        }
    }

    void HttpRequest::sendResponse()
    {
        completedPhases_ |= HttpRequestPhase::WritingResponseStarted;
    }

    void HttpRequest::onError(HttpServerAdvanced::PipelineError error)
    {
        if (!pendingResult_.hasValue())
        {
            HttpStatus status;
            std::string message;

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

            std::unique_ptr<IHttpResponse> response = handlerFactory_.createResponse(status, message + std::string(error.message()));
            setPendingResult(RequestHandlingResult::response(CreateResponseStream(std::move(response))));
            sendResponse();
        }
    }
} // namespace HttpServerAdvanced



