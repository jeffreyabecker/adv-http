#include "HttpPipeline.h"
#include <cassert>

#include "../server/HttpServerBase.h"

namespace lumalink::http::pipeline
{
    HttpPipeline::HttpPipeline(std::unique_ptr<lumalink::platform::transport::IClient> client, HttpServerBase &server,
                                                             const HttpTimeouts &timeouts, std::function<PipelineHandlerPtr()> handlerFactory,
                                                             const Clock &clock)
                : client_(std::move(client)),
                    server_(server),
                    clock_(clock),
                    handlerFactory_(std::move(handlerFactory)),
                    responseStream_(nullptr),
                    activeSession_(nullptr),
                    requestExecution_(nullptr),
                    connectionState_(ConnectionState::ReadingHttpRequest),
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
        PipelineHandlerPtr handler = handlerFactory_ ? handlerFactory_() : PipelineHandlerPtr(nullptr, [](IPipelineHandler *) {});
        if (!handler)
        {
            setConnectionState(ConnectionState::Error);
            return;
        }

        requestExecution_ = std::make_unique<HttpProtocolExecution>(std::move(handler));
        requestExecution_->setAddresses(
            client_->remoteAddress(),
            client_->remotePort(),
            client_->localAddress(),
            client_->localPort());
        responseStream_.reset();
        activeSession_.reset();
        responseStartedNotified_ = false;
        disconnectNotified_ = false;
        setConnectionState(ConnectionState::ReadingHttpRequest);
    }

    void HttpPipeline::readFromClientIntoParser()
    {
        if (connectionState_ != ConnectionState::ReadingHttpRequest || !requestExecution_)
        {
            return;
        }

        const ConnectionSessionResult result = requestExecution_->handle(*client_, clock_);
        if (result == ConnectionSessionResult::ProtocolError)
        {
            setErroredUnrecoverably();
            return;
        }

        if (requestExecution_->consumedInput())
        {
            startActivity();
        }

        if (requestExecution_->hasPendingResult())
        {
            consumePendingRequestResult();
            return;
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
            if (requestExecution_)
            {
                requestExecution_->onResponseStarted();
            }
        }

        uint8_t buffer[lumalink::http::core::PIPELINE_STACK_BUFFER_SIZE];
        lumalink::platform::buffers::AvailableResult available = lumalink::platform::buffers::TemporarilyUnavailableResult();
        while ((available = responseStream_->available()).hasBytes())
        {
            const size_t bytesToRead = std::min<std::size_t>(sizeof(buffer), available.count);
            const size_t bytesRead = responseStream_->read(lumalink::span<uint8_t>(buffer, bytesToRead));
            if (bytesRead > 0)
            {
                auto written = client_->write(lumalink::span<const uint8_t>(buffer, bytesRead));
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
            if (requestExecution_ && responseStartedNotified_)
            {
                requestExecution_->onResponseCompleted();
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

        IProtocolExecution *execution = currentProtocolExecution();
        const ConnectionSessionResult result = execution ? execution->handle(*client_, clock_) : activeSession_->handle(*client_, clock_);
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
        if (!requestExecution_ || !requestExecution_->hasPendingResult())
        {
            return;
        }

        RequestHandlingResult result = requestExecution_->takeResult();
        switch (result.kind)
        {
        case RequestHandlingResult::Kind::Response:
            requestExecution_->markRequestReadCompleted();
            responseStream_ = std::move(result.responseStream);
            if (!responseStream_)
            {
                setErroredUnrecoverably();
                return;
            }
            setConnectionState(ConnectionState::WritingHttpResponse);
            return;
        case RequestHandlingResult::Kind::Upgrade:
            requestExecution_->markRequestReadCompleted();
            activeSession_ = std::move(result.upgradedSession);
            if (!activeSession_)
            {
                setErroredUnrecoverably();
                return;
            }
            setConnectionState(ConnectionState::UpgradedSessionActive);
            return;
        case RequestHandlingResult::Kind::NoResponse:
            requestExecution_->markRequestReadCompleted();
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
        if (!requestExecution_ || !requestExecution_->shouldKeepAlive() || !client_->connected())
        {
            return false;
        }

        createRequestHandler();
        return connectionState_ == ConnectionState::ReadingHttpRequest;
    }

    void HttpPipeline::setupPipeline()
    {
        startMillis_ = currentMillis();
        lastActivityMillis_ = startMillis_;
        client_->setTimeout(timeouts_.getReadTimeout());
    }

    PipelineHandleClientResult HttpPipeline::checkStateInHandleClient()
    {
        const ClockMillis currentMillis = this->currentMillis();
        if (currentMillis - startMillis_ > timeouts_.getTotalRequestLengthMs())
        {
            timedOutUnrecoverably_ = true;
            if (IProtocolExecution *execution = currentProtocolExecution())
            {
                execution->onError(PipelineError(PipelineErrorCode::Timeout));
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
            if (!disconnectNotified_)
            {
                if (IProtocolExecution *execution = currentProtocolExecution())
                {
                    execution->onDisconnect();
                }
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
        const ClockMillis currentMillis = this->currentMillis();
        if (currentMillis - lastActivityMillis_ > timeouts_.getActivityTimeout())
        {
            if (IProtocolExecution *execution = currentProtocolExecution())
            {
                execution->onError(PipelineError(PipelineErrorCode::Timeout));
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

        const ClockMillis currentMillis = this->currentMillis();
        ClockMillis timeout = timeouts_.getReadTimeout();
        if (connectionState_ == ConnectionState::WritingHttpResponse || connectionState_ == ConnectionState::UpgradedSessionActive)
        {
            timeout = timeouts_.getActivityTimeout();
        }
        if (currentMillis - lastActivityMillis_ <= timeout)
        {
            return true;
        }

        if (IProtocolExecution *execution = currentProtocolExecution())
        {
            execution->onError(PipelineError(PipelineErrorCode::Timeout));
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
        requestExecution_.reset();
        setConnectionState(ConnectionState::Error);
    }

    void HttpPipeline::setConnectionState(ConnectionState state)
    {
        connectionState_ = state;
    }

    ClockMillis HttpPipeline::currentMillis() const
    {
        return clock_.nowMillis();
    }

    bool HttpPipeline::isFinished() const
    {
        return isTerminalState();
    }

    bool HttpPipeline::shouldKeepAlive() const
    {
        return requestExecution_ != nullptr && requestExecution_->shouldKeepAlive();
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
        if (requestExecution_)
        {
            requestExecution_->markRequestReadCompleted();
        }
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

    ClockMillis HttpPipeline::startedAt() const
    {
        return startMillis_;
    }

    lumalink::platform::transport::IClient &HttpPipeline::client()
    {
        return *client_;
    }

    HttpPipeline::ConnectionState HttpPipeline::connectionState() const
    {
        return connectionState_;
    }

    IProtocolExecution *HttpPipeline::currentProtocolExecution()
    {
        if (connectionState_ == ConnectionState::ReadingHttpRequest || connectionState_ == ConnectionState::WritingHttpResponse)
        {
            return requestExecution_.get();
        }

        if (connectionState_ == ConnectionState::UpgradedSessionActive)
        {
            return activeSession_ ? activeSession_->protocolExecution() : nullptr;
        }

        return nullptr;
    }

    const IProtocolExecution *HttpPipeline::currentProtocolExecution() const
    {
        if (connectionState_ == ConnectionState::ReadingHttpRequest || connectionState_ == ConnectionState::WritingHttpResponse)
        {
            return requestExecution_.get();
        }

        if (connectionState_ == ConnectionState::UpgradedSessionActive)
        {
            return activeSession_ ? activeSession_->protocolExecution() : nullptr;
        }

        return nullptr;
    }

} // namespace lumalink::http::pipeline
