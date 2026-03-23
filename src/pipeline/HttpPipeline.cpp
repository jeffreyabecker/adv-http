#include "HttpPipeline.h"
#include <cassert>

#include "../server/HttpServerBase.h"

namespace HttpServerAdvanced
{
    HttpPipeline::HttpPipeline(std::unique_ptr<HttpServerAdvanced::IClient> client, HttpServerBase &server,
                                                             const HttpTimeouts &timeouts, PipelineHandlerPtr parserEventHandler,
                                                             const Compat::Clock &clock)
                : requestParser_(*parserEventHandler),
                    client_(std::move(client)),
                    server_(server),
                    clock_(clock),
                    responseStream_(nullptr),
                    handler_(std::move(parserEventHandler)),
          timeouts_(timeouts),
          completedRequestRead_(false),
          completedResponseWrite_(false),
          haveStartedWritingResponse_(false),
          aborted_(false),
          erroredUnrecoverably_(false),
          timedOutUnrecoverably_(false),
          lastActivityMillis_(0),
          loopCount_(0),
          startMillis_(0)
    {
        handler_->setAddresses(
            client_->remoteAddress(),
            client_->remotePort(),
            client_->localAddress(),
            client_->localPort());
        handler_->setResponseStreamCallback([this](std::unique_ptr<IByteSource> stream)
                                            { this->setResponseStream(std::move(stream)); });
    }

    void HttpPipeline::readFromClientIntoParser()
    {
        if (completedRequestRead_)
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
            size_t bytesParsed = requestParser_.execute(buffer, bytesRead);
            if (bytesParsed < bytesRead)
            {
                setErroredUnrecoverably();
                return;
            }

            if (requestParser_.currentEvent() == RequestParserEvent::MessageComplete)
            {
                markRequestReadCompleted();
                return;
            }

            if (!checkActivityTimeout())
            {
                return;
            }
        }

        if (available.isExhausted() && !client_->connected())
        {
            markRequestReadCompleted();
        }
    }

    void HttpPipeline::writeResponseToClientFromStream()
    {
        if (responseStream_ == nullptr || completedResponseWrite_)
        {
            return;
        }
        // Mark that response writing has started before emitting bytes
        if (!haveStartedWritingResponse_)
        {
            haveStartedWritingResponse_ = true;
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
            markResponseWriteCompleted(true);
        }
    }

    void HttpPipeline::setupPipeline()
    {
        startMillis_ = currentMillis();
        lastActivityMillis_ = startMillis_;
        client_->setTimeout(timeouts_.getReadTimeout());
    }

    PipelineHandleClientResult HttpPipeline::_checkStateInHandleClient()
    {
        const Compat::ClockMillis currentMillis = this->currentMillis();
        if (currentMillis - startMillis_ > timeouts_.getTotalRequestLengthMs())
        {
            timedOutUnrecoverably_ = true;
            // If we already started sending the response, close connection and abort immediately.
            if (haveStartedWritingResponse_)
            {
                if (handler_)
                {
                    handler_->onError(PipelineError(PipelineErrorCode::Timeout));
                }
                // Force close the connection and abort pipeline
                client_->stop();
                abort();
                return PipelineHandleClientResult::TimedOutUnrecoverably;
            }
            // Otherwise, notify handler and allow the pipeline to transition to timed-out state
            if (handler_)
            {
                handler_->onError(PipelineError(PipelineErrorCode::Timeout));
            }
            return PipelineHandleClientResult::TimedOutUnrecoverably;
        }
        if (timedOutUnrecoverably_)
        {
            return PipelineHandleClientResult::TimedOutUnrecoverably;
        }
        if (aborted_)
        {
            return PipelineHandleClientResult::Aborted;
        }
        if (erroredUnrecoverably_)
        {
            return PipelineHandleClientResult::ErroredUnrecoverably;
        }
        if (completedRequestRead_ && completedResponseWrite_)
        {
            return PipelineHandleClientResult::Completed;
        }
        if (!client_->connected())
        {
            if (handler_)
            {
                handler_->onClientDisconnected();
            }
            return PipelineHandleClientResult::ClientDisconnected;
        }
        return PipelineHandleClientResult::Processing; // Default case
    }

    bool HttpPipeline::isFirstLoop() const
    {
        return loopCount_ == 0;
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
            // Activity timeout: notify handler and mark pipeline as timed out
            if (handler_)
            {
                handler_->onError(PipelineError(PipelineErrorCode::Timeout));
            }
            timedOutUnrecoverably_ = true;
            // If response already started, close the connection
            if (haveStartedWritingResponse_)
            {
                client_->stop();
                abort();
            }
            return false;
        }
        return true;
    }

    bool HttpPipeline::checkIdleTimeout()
    {
        if (completedRequestRead_ && (completedResponseWrite_ || responseStream_ == nullptr))
        {
            return true;
        }

        const Compat::ClockMillis currentMillis = this->currentMillis();
        const bool hasPendingResponse = responseStream_ != nullptr || haveStartedWritingResponse_;
        const Compat::ClockMillis timeout = hasPendingResponse ? timeouts_.getActivityTimeout() : timeouts_.getReadTimeout();
        if (currentMillis - lastActivityMillis_ <= timeout)
        {
            return true;
        }

        if (handler_)
        {
            handler_->onError(PipelineError(PipelineErrorCode::Timeout));
        }

        timedOutUnrecoverably_ = true;
        if (hasPendingResponse)
        {
            client_->stop();
            abort();
        }

        return false;
    }

    void HttpPipeline::setErroredUnrecoverably()
    {
        erroredUnrecoverably_ = true;
        markRequestReadCompleted();
        markResponseWriteCompleted();
    }

    void HttpPipeline::markRequestReadCompleted()
    {
        if (completedRequestRead_)
        {
            return;
        }

        requestParser_.execute(nullptr, 0); // Signal end of input to parser
        completedRequestRead_ = true;
    }

    void HttpPipeline::markResponseWriteCompleted(bool notifyHandler)
    {
        if (completedResponseWrite_)
        {
            return;
        }

        completedResponseWrite_ = true;
        responseStream_ = nullptr;
        if (notifyHandler && haveStartedWritingResponse_ && handler_)
        {
            handler_->onResponseCompleted();
        }
    }

    Compat::ClockMillis HttpPipeline::currentMillis() const
    {
        return clock_.nowMillis();
    }

    bool HttpPipeline::isFinished() const
    {
        return requestParser_.isFinished();
    }

    bool HttpPipeline::shouldKeepAlive() const
    {
        return requestParser_.shouldKeepAlive();
    }

    void HttpPipeline::setResponseStream(std::unique_ptr<IByteSource> responseStream)
    {
        if (haveStartedWritingResponse_)
        {
            assert(false && "Response writing already started");
        }
        responseStream_ = std::move(responseStream);
    }

    PipelineHandleClientResult HttpPipeline::handleClient()
    {
        if (isFirstLoop())
        {
            setupPipeline();
        }
        loopCount_++;

        auto validationResult = _checkStateInHandleClient();
        if (isPipelineHandleClientResultFinal(validationResult))
        {
            return validationResult;
        }

        readFromClientIntoParser();
        validationResult = _checkStateInHandleClient();
        if (isPipelineHandleClientResultFinal(validationResult))
        {
            return validationResult;
        }

        writeResponseToClientFromStream();
        validationResult = _checkStateInHandleClient();
        if (isPipelineHandleClientResultFinal(validationResult))
        {
            return validationResult;
        }

        checkIdleTimeout();
        return _checkStateInHandleClient();
    }

    void HttpPipeline::abort()
    {
        aborted_ = true;
        abortReadingRequest();
        abortWritingResponse();
    }

    void HttpPipeline::abortReadingRequest()
    {
        markRequestReadCompleted();
    }

    void HttpPipeline::abortWritingResponse()
    {
        markResponseWriteCompleted();
    }

    Compat::ClockMillis HttpPipeline::startedAt() const
    {
        return startMillis_;
    }

    HttpServerAdvanced::IClient &HttpPipeline::client()
    {
        return *client_;
    }

} // namespace HttpServerAdvanced
