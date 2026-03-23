#include "HttpPipeline.h"
#include <cassert>

#include "../server/HttpServerBase.h"

namespace HttpServerAdvanced
{
    HttpPipeline::HttpPipeline(std::unique_ptr<HttpServerAdvanced::IClient> client, HttpServerBase &server,
                                                             const HttpTimeouts &timeouts, std::function<PipelineHandlerPtr()> handlerFactory,
                                                             const Compat::Clock &clock)
                : client_(std::move(client)),
                    server_(server),
                    clock_(clock),
                    handlerFactory_(std::move(handlerFactory)),
                    responseStream_(nullptr),
                    activeSession_(nullptr),
                    handler_(nullptr),
                    connectionState_(ConnectionState::ReadingHttpRequest),
                    requestReadCompleted_(false),
                    responseStartedNotified_(false),
                    disconnectNotified_(false),
                    timedOutUnrecoverably_(false),
                    lastActivityMillis_(0),
                    startMillis_(0),
                    loopCount_(0),
                    timeouts_(timeouts)
    {
        createRequestHandler();
    }

    void HttpPipeline::createRequestHandler()
    {
        handler_ = handlerFactory_ ? handlerFactory_() : PipelineHandlerPtr(nullptr, [](IPipelineHandler *) {});
        if (!handler_)
        {
            setConnectionState(ConnectionState::Error);
            return;
        }

        handler_->setAddresses(
            client_->remoteAddress(),
            client_->remotePort(),
            client_->localAddress(),
            client_->localPort());
        requestParser_ = std::make_unique<RequestParser>(*handler_);
        responseStream_.reset();
        activeSession_.reset();
        requestReadCompleted_ = false;
        responseStartedNotified_ = false;
        disconnectNotified_ = false;
        setConnectionState(ConnectionState::ReadingHttpRequest);
    }

    void HttpPipeline::readFromClientIntoParser()
    {
        if (connectionState_ != ConnectionState::ReadingHttpRequest || requestReadCompleted_ || !requestParser_)
        {
            return;
        }

        uint8_t buffer[HttpServerAdvanced::PIPELINE_STACK_BUFFER_SIZE];
        AvailableResult available = TemporarilyUnavailableResult();
        while ((available = client_->available()).hasBytes())
        {
            std::size_t bytesRead = client_->read(HttpServerAdvanced::span<uint8_t>(buffer, sizeof(buffer)));
            if (bytesRead == 0)
            {
                break;
            }

            startActivity();
            size_t bytesParsed = requestParser_->execute(buffer, bytesRead);
            if (bytesParsed < bytesRead)
            {
                setErroredUnrecoverably();
                return;
            }

            if (handler_ && handler_->hasPendingResult())
            {
                consumePendingRequestResult();
                if (connectionState_ != ConnectionState::ReadingHttpRequest)
                {
                    return;
                }
            }

            if (requestParser_->currentEvent() == RequestParserEvent::MessageComplete)
            {
                markRequestReadCompleted();
                if (handler_ && handler_->hasPendingResult())
                {
                    consumePendingRequestResult();
                }
                else
                {
                    setErroredUnrecoverably();
                }
                return;
            }

            if (!checkActivityTimeout())
            {
                return;
            }
        }

        if (available.isExhausted() && !client_->connected())
        {
            requestReadCompleted_ = true;
        }
    }

    void HttpPipeline::writeResponseToClientFromStream()
    {
        if (connectionState_ != ConnectionState::WritingHttpResponse || responseStream_ == nullptr)
        {
            return;
        }

        if (!responseStartedNotified_)
        {
            responseStartedNotified_ = true;
            if (handler_)
            {
                handler_->onResponseStarted();
            }
        }

        uint8_t buffer[HttpServerAdvanced::PIPELINE_STACK_BUFFER_SIZE];
        AvailableResult available = TemporarilyUnavailableResult();
        while ((available = responseStream_->available()).hasBytes())
        {
            const size_t bytesToRead = std::min<std::size_t>(sizeof(buffer), available.count);
            const size_t bytesRead = responseStream_->read(HttpServerAdvanced::span<uint8_t>(buffer, bytesToRead));
            if (bytesRead > 0)
            {
                auto written = client_->write(HttpServerAdvanced::span<const uint8_t>(buffer, bytesRead));
                startActivity();
                if (written < bytesRead)
                {
                    setErroredUnrecoverably();
                    return;
                }
            }
            else
            {
                break;
            }
            if (!checkActivityTimeout())
            {
                return;
            }
        }
        if (available.isExhausted())
        {
            responseStream_.reset();
            if (handler_ && responseStartedNotified_)
            {
                handler_->onResponseCompleted();
            }
            responseStartedNotified_ = false;
            if (!beginNextKeepAliveRequest())
            {
                setConnectionState(ConnectionState::Completed);
            }
        }
    }

    void HttpPipeline::handleActiveSession()
    {
        if (connectionState_ != ConnectionState::UpgradedSessionActive)
        {
            return;
        }

        if (!activeSession_)
        {
            setErroredUnrecoverably();
            return;
        }

        const ConnectionSessionResult result = activeSession_->handle(*client_, clock_);
        startActivity();

        switch (result)
        {
        case ConnectionSessionResult::Continue:
            return;
        case ConnectionSessionResult::Completed:
            activeSession_.reset();
            setConnectionState(ConnectionState::Completed);
            return;
        case ConnectionSessionResult::AbortConnection:
            client_->stop();
            setConnectionState(ConnectionState::Aborted);
            return;
        case ConnectionSessionResult::ProtocolError:
        default:
            setErroredUnrecoverably();
            return;
        }
    }

    void HttpPipeline::consumePendingRequestResult()
    {
        if (!handler_ || !handler_->hasPendingResult())
        {
            return;
        }

        RequestHandlingResult result = handler_->takeResult();
        switch (result.kind)
        {
        case RequestHandlingResult::Kind::Response:
            requestReadCompleted_ = true;
            responseStream_ = std::move(result.responseStream);
            if (!responseStream_)
            {
                setErroredUnrecoverably();
                return;
            }
            setConnectionState(ConnectionState::WritingHttpResponse);
            return;
        case RequestHandlingResult::Kind::Upgrade:
            requestReadCompleted_ = true;
            activeSession_ = std::move(result.upgradedSession);
            if (!activeSession_)
            {
                setErroredUnrecoverably();
                return;
            }
            setConnectionState(ConnectionState::UpgradedSessionActive);
            return;
        case RequestHandlingResult::Kind::NoResponse:
            requestReadCompleted_ = true;
            if (!beginNextKeepAliveRequest())
            {
                setConnectionState(ConnectionState::Completed);
            }
            return;
        case RequestHandlingResult::Kind::Error:
            setErroredUnrecoverably();
            return;
        case RequestHandlingResult::Kind::None:
        default:
            return;
        }
    }

    bool HttpPipeline::beginNextKeepAliveRequest()
    {
        if (!requestParser_ || !requestParser_->shouldKeepAlive() || !client_->connected())
        {
            return false;
        }

        createRequestHandler();
        return connectionState_ == ConnectionState::ReadingHttpRequest;
    }

    void HttpPipeline::markRequestReadCompleted()
    {
        if (requestReadCompleted_ || !requestParser_)
        {
            return;
        }

        requestParser_->execute(nullptr, 0);
        requestReadCompleted_ = true;
    }

    void HttpPipeline::setupPipeline()
    {
        startMillis_ = currentMillis();
        lastActivityMillis_ = startMillis_;
        client_->setTimeout(timeouts_.getReadTimeout());
    }

    PipelineHandleClientResult HttpPipeline::checkStateInHandleClient()
    {
        const Compat::ClockMillis currentMillis = this->currentMillis();
        if (currentMillis - startMillis_ > timeouts_.getTotalRequestLengthMs())
        {
            timedOutUnrecoverably_ = true;
            if (handler_)
            {
                handler_->onError(PipelineError(PipelineErrorCode::Timeout));
            }

            if (connectionState_ == ConnectionState::WritingHttpResponse || connectionState_ == ConnectionState::UpgradedSessionActive)
            {
                client_->stop();
            }

            setConnectionState(ConnectionState::Error);
            return PipelineHandleClientResult::TimedOutUnrecoverably;
        }
        if (timedOutUnrecoverably_)
        {
            return PipelineHandleClientResult::TimedOutUnrecoverably;
        }
        if (connectionState_ == ConnectionState::Aborted)
        {
            return PipelineHandleClientResult::Aborted;
        }
        if (connectionState_ == ConnectionState::Error)
        {
            return PipelineHandleClientResult::ErroredUnrecoverably;
        }
        if (connectionState_ == ConnectionState::Completed)
        {
            return PipelineHandleClientResult::Completed;
        }
        if (!client_->connected())
        {
            if (!disconnectNotified_ && handler_)
            {
                handler_->onClientDisconnected();
                disconnectNotified_ = true;
            }
            setConnectionState(ConnectionState::Closing);
            return PipelineHandleClientResult::ClientDisconnected;
        }

        if (connectionState_ == ConnectionState::Closing)
        {
            return PipelineHandleClientResult::ClientDisconnected;
        }

        return PipelineHandleClientResult::Processing;
    }

    bool HttpPipeline::isFirstLoop() const
    {
        return loopCount_ == 0;
    }

    bool HttpPipeline::isTerminalState() const
    {
        switch (connectionState_)
        {
        case ConnectionState::Completed:
        case ConnectionState::Aborted:
        case ConnectionState::Error:
        case ConnectionState::Closing:
            return true;
        case ConnectionState::ReadingHttpRequest:
        case ConnectionState::WritingHttpResponse:
        case ConnectionState::UpgradedSessionActive:
        default:
            return false;
        }
    }

    void HttpPipeline::startActivity()
    {
        lastActivityMillis_ = currentMillis();
    }

    bool HttpPipeline::checkActivityTimeout()
    {
        const Compat::ClockMillis currentMillis = this->currentMillis();
        if (currentMillis - lastActivityMillis_ > timeouts_.getActivityTimeout())
        {
            if (handler_)
            {
                handler_->onError(PipelineError(PipelineErrorCode::Timeout));
            }
            timedOutUnrecoverably_ = true;
            if (connectionState_ == ConnectionState::WritingHttpResponse || connectionState_ == ConnectionState::UpgradedSessionActive)
            {
                client_->stop();
            }
            setConnectionState(ConnectionState::Error);
            return false;
        }
        return true;
    }

    bool HttpPipeline::checkIdleTimeout()
    {
        if (isTerminalState())
        {
            return true;
        }

        const Compat::ClockMillis currentMillis = this->currentMillis();
        Compat::ClockMillis timeout = timeouts_.getReadTimeout();
        if (connectionState_ == ConnectionState::WritingHttpResponse || connectionState_ == ConnectionState::UpgradedSessionActive)
        {
            timeout = timeouts_.getActivityTimeout();
        }
        if (currentMillis - lastActivityMillis_ <= timeout)
        {
            return true;
        }

        if (handler_)
        {
            handler_->onError(PipelineError(PipelineErrorCode::Timeout));
        }

        timedOutUnrecoverably_ = true;
        if (connectionState_ == ConnectionState::WritingHttpResponse || connectionState_ == ConnectionState::UpgradedSessionActive)
        {
            client_->stop();
        }
        setConnectionState(ConnectionState::Error);

        return false;
    }

    void HttpPipeline::setErroredUnrecoverably()
    {
        responseStream_ = nullptr;
        activeSession_.reset();
        setConnectionState(ConnectionState::Error);
    }

    void HttpPipeline::setConnectionState(ConnectionState state)
    {
        connectionState_ = state;
    }

    Compat::ClockMillis HttpPipeline::currentMillis() const
    {
        return clock_.nowMillis();
    }

    bool HttpPipeline::isFinished() const
    {
        return isTerminalState();
    }

    bool HttpPipeline::shouldKeepAlive() const
    {
        return requestParser_ != nullptr && requestParser_->shouldKeepAlive();
    }

    PipelineHandleClientResult HttpPipeline::handleClient()
    {
        if (isFirstLoop())
        {
            setupPipeline();
        }
        loopCount_++;

        auto validationResult = checkStateInHandleClient();
        if (isPipelineHandleClientResultFinal(validationResult))
        {
            return validationResult;
        }

        if (connectionState_ == ConnectionState::ReadingHttpRequest)
        {
            readFromClientIntoParser();
            validationResult = checkStateInHandleClient();
            if (isPipelineHandleClientResultFinal(validationResult))
            {
                return validationResult;
            }
        }

        if (connectionState_ == ConnectionState::WritingHttpResponse)
        {
            writeResponseToClientFromStream();
            validationResult = checkStateInHandleClient();
            if (isPipelineHandleClientResultFinal(validationResult))
            {
                return validationResult;
            }
        }

        if (connectionState_ == ConnectionState::UpgradedSessionActive)
        {
            handleActiveSession();
            validationResult = checkStateInHandleClient();
            if (isPipelineHandleClientResultFinal(validationResult))
            {
                return validationResult;
            }
        }

        checkIdleTimeout();
        return checkStateInHandleClient();
    }

    void HttpPipeline::abort()
    {
        setConnectionState(ConnectionState::Aborted);
        abortReadingRequest();
        abortWritingResponse();
    }

    void HttpPipeline::abortReadingRequest()
    {
        markRequestReadCompleted();
    }

    void HttpPipeline::abortWritingResponse()
    {
        responseStream_.reset();
        activeSession_.reset();

        if (connectionState_ == ConnectionState::WritingHttpResponse || connectionState_ == ConnectionState::UpgradedSessionActive)
        {
            if (!beginNextKeepAliveRequest())
            {
                setConnectionState(ConnectionState::Completed);
            }
        }
    }

    Compat::ClockMillis HttpPipeline::startedAt() const
    {
        return startMillis_;
    }

    HttpServerAdvanced::IClient &HttpPipeline::client()
    {
        return *client_;
    }

    HttpPipeline::ConnectionState HttpPipeline::connectionState() const
    {
        return connectionState_;
    }

} // namespace HttpServerAdvanced
