#include "HttpProtocolExecution.h"

#include "../core/Defines.h"

namespace httpadv::v1::pipeline
{
    HttpProtocolExecution::HttpProtocolExecution(PipelineHandlerPtr handler)
        : handler_(std::move(handler))
    {
        if (handler_)
        {
            requestParser_ = std::make_unique<RequestParser>(*handler_);
        }
    }

    ConnectionSessionResult HttpProtocolExecution::handle(IClient &client, const Clock &)
    {
        consumedInput_ = false;

        if (requestReadCompleted_ || !requestParser_)
        {
            return ConnectionSessionResult::Continue;
        }

        std::uint8_t buffer[httpadv::v1::core::PIPELINE_STACK_BUFFER_SIZE] = {};
        httpadv::v1::transport::AvailableResult available = httpadv::v1::transport::TemporarilyUnavailableResult();
        while ((available = client.available()).hasBytes())
        {
            const std::size_t bytesRead = client.read(httpadv::v1::util::span<std::uint8_t>(buffer, sizeof(buffer)));
            if (bytesRead == 0)
            {
                break;
            }

            consumedInput_ = true;

            const std::size_t bytesParsed = requestParser_->execute(buffer, bytesRead);
            if (bytesParsed < bytesRead)
            {
                return ConnectionSessionResult::ProtocolError;
            }

            if (handler_ && handler_->hasPendingResult())
            {
                return ConnectionSessionResult::Continue;
            }

            if (requestParser_->currentEvent() == RequestParserEvent::MessageComplete)
            {
                markRequestReadCompleted();
                if (handler_ && handler_->hasPendingResult())
                {
                    return ConnectionSessionResult::Continue;
                }

                return ConnectionSessionResult::ProtocolError;
            }
        }

        if (available.isExhausted() && !client.connected())
        {
            requestReadCompleted_ = true;
        }

        return ConnectionSessionResult::Continue;
    }

    void HttpProtocolExecution::onError(PipelineError error)
    {
        if (handler_)
        {
            handler_->onError(error);
        }
    }

    void HttpProtocolExecution::onDisconnect()
    {
        if (handler_)
        {
            handler_->onClientDisconnected();
        }
    }

    bool HttpProtocolExecution::hasPendingResult() const
    {
        return handler_ && handler_->hasPendingResult();
    }

    RequestHandlingResult HttpProtocolExecution::takeResult()
    {
        if (!handler_)
        {
            return RequestHandlingResult();
        }

        return handler_->takeResult();
    }

    bool HttpProtocolExecution::isFinished() const
    {
        return requestReadCompleted_;
    }

    void HttpProtocolExecution::setAddresses(std::string_view remoteAddress,
                                             std::uint16_t remotePort,
                                             std::string_view localAddress,
                                             std::uint16_t localPort)
    {
        if (handler_)
        {
            handler_->setAddresses(remoteAddress, remotePort, localAddress, localPort);
        }
    }

    bool HttpProtocolExecution::shouldKeepAlive() const
    {
        return requestParser_ != nullptr && requestParser_->shouldKeepAlive();
    }

    bool HttpProtocolExecution::consumedInput() const
    {
        return consumedInput_;
    }

    void HttpProtocolExecution::markRequestReadCompleted()
    {
        if (requestReadCompleted_ || !requestParser_)
        {
            return;
        }

        requestParser_->execute(nullptr, 0);
        requestReadCompleted_ = true;
    }

    void HttpProtocolExecution::onResponseStarted()
    {
        if (handler_)
        {
            handler_->onResponseStarted();
        }
    }

    void HttpProtocolExecution::onResponseCompleted()
    {
        if (handler_)
        {
            handler_->onResponseCompleted();
        }
    }
}