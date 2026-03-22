#include "HttpPipeline.h"
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
        handler_->setResponseStreamCallback([this](std::unique_ptr<Stream> stream)
                                            { this->setResponseStream(std::move(stream)); });
    }

    void HttpPipeline::readFromClientIntoParser()
    {
        if (completedRequestRead_)
        {
            return;
        }
        startActivity();
        uint8_t buffer[HttpServerAdvanced::PIPELINE_STACK_BUFFER_SIZE];
        int available = 0;
        while ((available = client_->available()) >= 0)
        {
            std::size_t bytesRead = client_->read(buffer, sizeof(buffer));

            size_t bytesParsed = requestParser_.execute(buffer, bytesRead);
            if (bytesParsed < bytesRead)
            {
                setErroredUnrecoverably();
                return;
            }
            if (!checkActivityTimeout())
            {
                return;
            }
        }
        if (available == 0)
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

        startActivity();
        // ClientContext.h uses a 256 byte buffer for copying streams so we will do the same here
        // see: Wifi/src/include/ClientContext.h:379
        uint8_t buffer[HttpServerAdvanced::PIPELINE_STACK_BUFFER_SIZE];
        int available = 0;
        while ((available = responseStream_->available()) > 0)
        {
            size_t bytesRead = 0;
            for (bytesRead = 0; bytesRead < sizeof(buffer) && responseStream_->available(); bytesRead++)
            {
                int byte = responseStream_->read();
                if (byte == -1)
                {
                    break;
                }
                buffer[bytesRead] = static_cast<uint8_t>(byte);
            }
            if (bytesRead > 0)
            {
                auto written = client_->write(buffer, bytesRead);
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
        if (available <= 0)
        {
            markResponseWriteCompleted();
        }
    }

    void HttpPipeline::setupPipeline()
    {
        startMillis_ = currentMillis();
        client_->setTimeout(timeouts_.getActivityTimeout());
    }

    PipelineHandleClientResult HttpPipeline::_checkStateInHandleClient()
    {
        const Compat::ClockMillis currentMillis = this->currentMillis();
        if (!client_->connected())
        {
            if (handler_)
            {
                handler_->onClientDisconnected();
            }
            return PipelineHandleClientResult::ClientDisconnected;
        }

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

    void HttpPipeline::setErroredUnrecoverably()
    {
        erroredUnrecoverably_ = true;
        markRequestReadCompleted();
        markResponseWriteCompleted();
    }

    void HttpPipeline::markRequestReadCompleted()
    {
        requestParser_.execute(nullptr, 0); // Signal end of input to parser
        completedRequestRead_ = true;
    }

    void HttpPipeline::markResponseWriteCompleted()
    {
        completedResponseWrite_ = true;
        responseStream_ = nullptr;
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

    void HttpPipeline::setResponseStream(std::unique_ptr<Stream> responseStream)
    {
        if (haveStartedWritingResponse_)
        {
            assert(false && "Response writing already started");
        }
        responseStream_ = std::move(responseStream);
    }

    PipelineHandleClientResult HttpPipeline::handleClient()
    {
        loopCount_++;

        if (isFirstLoop())
        {
            setupPipeline();
        }
        auto validationResult = _checkStateInHandleClient();
        if (isPipelineHandleClientResultFinal(validationResult))
        {
            return validationResult;
        }
        readFromClientIntoParser();
        writeResponseToClientFromStream();
        return PipelineHandleClientResult::Processing;
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
