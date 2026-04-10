#include "WebSocketProtocolExecution.h"

#include "WebSocketErrorPolicy.h"

#include "../core/Defines.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace lumalink::http::websocket
{
    using lumalink::platform::buffers::HasAvailableBytes;

    namespace
    {
        WsErrorCategory mapCodecErrorCategory(WebSocketCodecError error)
        {
            switch (error)
            {
            case WebSocketCodecError::ControlFrameFragmented:
            case WebSocketCodecError::ControlFrameTooLarge:
            case WebSocketCodecError::UnexpectedContinuationFrame:
            case WebSocketCodecError::InterruptedContinuationSequence:
            case WebSocketCodecError::UnmaskedClientFrame:
                return WsErrorCategory::ProtocolViolation;
            case WebSocketCodecError::InvalidReservedBits:
            case WebSocketCodecError::InvalidOpcode:
            case WebSocketCodecError::MalformedExtendedPayloadLength:
            case WebSocketCodecError::MalformedClosePayload:
            case WebSocketCodecError::BufferTooSmall:
            default:
                return WsErrorCategory::FrameParseError;
            }
        }

        std::span<const std::uint8_t> asBytes(std::string_view text)
        {
            return std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t *>(text.data()), text.size());
        }
    }

    WebSocketProtocolExecution::WebSocketProtocolExecution(std::string handshakeResponse, WebSocketContext context)
        : context_(std::move(context)),
          parser_(true),
          pendingWrite_(handshakeResponse.begin(), handshakeResponse.end())
    {
        context_.bindControl(this);
    }

    lumalink::http::server::ConnectionSessionResult WebSocketProtocolExecution::handle(lumalink::platform::transport::IClient &client, const lumalink::platform::time::IMonotonicClock &)
    {
        if (!client.connected())
        {
            const WsClosePolicy policy = policyFor(WsErrorCategory::RemoteDisconnect);
            context_.notifyError("WebSocket disconnected");
            context_.notifyClose(policy.closeCode, "");
            closeState_ = CloseState::Closed;
            return lumalink::http::server::ConnectionSessionResult::AbortConnection;
        }

        const bool hadPendingBeforeFlush = !pendingWrite_.empty();
        if (!flushPendingWrite(client))
        {
            return lumalink::http::server::ConnectionSessionResult::Continue;
        }

        if (!handshakeWritten_)
        {
            handshakeWritten_ = true;
            context_.notifyOpen();
            return lumalink::http::server::ConnectionSessionResult::Continue;
        }

        if (hadPendingBeforeFlush && closeState_ == CloseState::CloseQueued)
        {
            closeState_ = CloseState::CloseSent;
            if (receivedCloseFrame_)
            {
                context_.notifyClose(closeCode_, closeReason_);
                closeState_ = CloseState::Closed;
                return lumalink::http::server::ConnectionSessionResult::Completed;
            }
        }

        std::array<std::uint8_t, lumalink::http::core::PIPELINE_STACK_BUFFER_SIZE> readBuffer = {};
        while (HasAvailableBytes(client.available()))
        {
            const std::size_t bytesRead = client.read(std::span<std::uint8_t>(readBuffer.data(), readBuffer.size()));
            if (bytesRead == 0)
            {
                break;
            }

            std::size_t offset = 0;
            while (offset < bytesRead)
            {
                const auto input = std::span<const std::uint8_t>(readBuffer.data() + offset, bytesRead - offset);
                const WebSocketParseResult parseResult = parser_.parse(input);
                offset += parseResult ? parseResult->bytesConsumed : 0;

                if (parseResult && !parseResult->frame.has_value())
                {
                    break;
                }

                if (!parseResult)
                {
                    const WsErrorCategory category = mapCodecErrorCategory(parseResult.error());
                    const WsClosePolicy policy = policyFor(category);
                    context_.notifyError("WebSocket protocol error");

                    if (!policy.attemptCloseHandshake)
                    {
                        context_.notifyClose(policy.closeCode, "");
                        closeState_ = CloseState::Closed;
                        return lumalink::http::server::ConnectionSessionResult::AbortConnection;
                    }

                    queueCloseFrame(static_cast<WebSocketCloseCode>(policy.closeCode));
                    beginClosing(policy.closeCode, "");
                    break;
                }

                if (parseResult->frame.has_value())
                {
                    handleParsedFrame(*parseResult->frame);
                }

                if (!pendingWrite_.empty() || closeState_ != CloseState::Open)
                {
                    break;
                }
            }

            if (!pendingWrite_.empty() || closeState_ != CloseState::Open)
            {
                break;
            }
        }

        if (!pendingWrite_.empty() && closeState_ == CloseState::Open)
        {
            if (!flushPendingWrite(client))
            {
                return lumalink::http::server::ConnectionSessionResult::Continue;
            }
        }

        if (closeState_ == CloseState::CloseQueued)
        {
            if (!flushPendingWrite(client))
            {
                return lumalink::http::server::ConnectionSessionResult::Continue;
            }

            closeState_ = CloseState::CloseSent;
            if (receivedCloseFrame_)
            {
                context_.notifyClose(closeCode_, closeReason_);
                closeState_ = CloseState::Closed;
                return lumalink::http::server::ConnectionSessionResult::Completed;
            }
        }

        return lumalink::http::server::ConnectionSessionResult::Continue;
    }

    WebSocketSendResult WebSocketProtocolExecution::sendText(std::string_view payload)
    {
        if (closeState_ == CloseState::Closed)
        {
            return WebSocketSendResult::Closed;
        }

        if (closeState_ != CloseState::Open)
        {
            return WebSocketSendResult::Closing;
        }

        if (!handshakeWritten_ || !context_.isOpen())
        {
            return WebSocketSendResult::InvalidState;
        }

        if (payload.size() > lumalink::http::core::WsMaxMessageSize)
        {
            return WebSocketSendResult::TooLarge;
        }

        if (!pendingWrite_.empty())
        {
            return WebSocketSendResult::Backpressured;
        }

        if (!queueSerializedFrame(WebSocketOpcode::Text, asBytes(payload), true))
        {
            return WebSocketSendResult::InternalError;
        }

        return WebSocketSendResult::Queued;
    }

    WebSocketSendResult WebSocketProtocolExecution::sendBinary(std::span<const std::uint8_t> payload)
    {
        if (closeState_ == CloseState::Closed)
        {
            return WebSocketSendResult::Closed;
        }

        if (closeState_ != CloseState::Open)
        {
            return WebSocketSendResult::Closing;
        }

        if (!handshakeWritten_ || !context_.isOpen())
        {
            return WebSocketSendResult::InvalidState;
        }

        if (payload.size() > lumalink::http::core::WsMaxMessageSize)
        {
            return WebSocketSendResult::TooLarge;
        }

        if (!pendingWrite_.empty())
        {
            return WebSocketSendResult::Backpressured;
        }

        if (!queueSerializedFrame(WebSocketOpcode::Binary, payload, true))
        {
            return WebSocketSendResult::InternalError;
        }

        return WebSocketSendResult::Queued;
    }

    WebSocketCloseResult WebSocketProtocolExecution::close(WebSocketCloseCode code, std::string_view reason)
    {
        if (closeState_ == CloseState::Closed)
        {
            return WebSocketCloseResult::AlreadyClosed;
        }

        if (closeState_ != CloseState::Open)
        {
            return WebSocketCloseResult::AlreadyClosing;
        }

        if (!handshakeWritten_ || !context_.isOpen())
        {
            return WebSocketCloseResult::InvalidState;
        }

        if (reason.size() > 123U)
        {
            return WebSocketCloseResult::ReasonTooLarge;
        }

        if (!pendingWrite_.empty())
        {
            return WebSocketCloseResult::InvalidState;
        }

        if (!queueCloseFrame(code, asBytes(reason)))
        {
            return WebSocketCloseResult::InternalError;
        }

        beginClosing(static_cast<std::uint16_t>(code), reason);
        return WebSocketCloseResult::CloseQueued;
    }

    WebSocketContext &WebSocketProtocolExecution::context()
    {
        return context_;
    }

    const WebSocketContext &WebSocketProtocolExecution::context() const
    {
        return context_;
    }

    void WebSocketProtocolExecution::onError(lumalink::http::pipeline::PipelineError error)
    {
        if (closeState_ == CloseState::Closed)
        {
            return;
        }

        const WsClosePolicy policy = policyFor(error.code() == lumalink::http::pipeline::PipelineErrorCode::Timeout ? WsErrorCategory::IdleTimeout : WsErrorCategory::WriteFailure);
        context_.notifyError(error.message());
        context_.notifyClose(policy.closeCode, "");
        closeState_ = CloseState::Closed;
    }

    void WebSocketProtocolExecution::onDisconnect()
    {
        if (closeState_ == CloseState::Closed)
        {
            return;
        }

        const WsClosePolicy policy = policyFor(WsErrorCategory::RemoteDisconnect);
        context_.notifyError("WebSocket disconnected");
        context_.notifyClose(policy.closeCode, "");
        closeState_ = CloseState::Closed;
    }

    bool WebSocketProtocolExecution::hasPendingResult() const
    {
        return false;
    }

    lumalink::http::pipeline::RequestHandlingResult WebSocketProtocolExecution::takeResult()
    {
        return lumalink::http::pipeline::EmptyRequestHandlingResult();
    }

    bool WebSocketProtocolExecution::isFinished() const
    {
        return closeState_ == CloseState::Closed;
    }

    bool WebSocketProtocolExecution::flushPendingWrite(lumalink::platform::transport::IClient &client)
    {
        while (pendingWriteOffset_ < pendingWrite_.size())
        {
            const std::size_t remaining = pendingWrite_.size() - pendingWriteOffset_;
            const std::size_t written = client.write(std::span<const std::uint8_t>(pendingWrite_.data() + pendingWriteOffset_, remaining));
            if (written == 0)
            {
                return false;
            }

            pendingWriteOffset_ += written;
        }

        pendingWrite_.clear();
        pendingWriteOffset_ = 0;
        return true;
    }

    bool WebSocketProtocolExecution::queueSerializedFrame(WebSocketOpcode opcode, std::span<const std::uint8_t> payload, bool fin)
    {
        if (!pendingWrite_.empty())
        {
            return false;
        }

        pendingWrite_.resize(WebSocketFrameSerializer::maxSerializedSize(payload.size()));
        const WebSocketSerializeResult result = WebSocketFrameSerializer::serialize(
            std::span<std::uint8_t>(pendingWrite_.data(), pendingWrite_.size()),
            payload,
            opcode,
            fin);

        if (!result)
        {
            pendingWrite_.clear();
            pendingWriteOffset_ = 0;
            return false;
        }

        pendingWrite_.resize(*result);
        pendingWriteOffset_ = 0;
        return true;
    }

    bool WebSocketProtocolExecution::queueCloseFrame(WebSocketCloseCode code, std::span<const std::uint8_t> reason)
    {
        const std::vector<std::uint8_t> payload = buildClosePayload(code, reason);
        return queueSerializedFrame(WebSocketOpcode::Close, std::span<const std::uint8_t>(payload.data(), payload.size()), true);
    }

    void WebSocketProtocolExecution::handleParsedFrame(const WebSocketFrame &frame)
    {
        if (frame.header.payloadLength > lumalink::http::core::WsMaxFramePayloadSize)
        {
            const WsClosePolicy policy = policyFor(WsErrorCategory::MessageTooLarge);
            context_.notifyError("WebSocket frame payload too large");
            if (policy.attemptCloseHandshake)
            {
                queueCloseFrame(static_cast<WebSocketCloseCode>(policy.closeCode));
                beginClosing(policy.closeCode, "");
            }
            else
            {
                context_.notifyClose(policy.closeCode, "");
                closeState_ = CloseState::Closed;
            }
            return;
        }

        switch (frame.header.opcode)
        {
        case WebSocketOpcode::Ping:
            queueSerializedFrame(WebSocketOpcode::Pong, std::span<const std::uint8_t>(frame.payload.data(), frame.payload.size()), true);
            return;
        case WebSocketOpcode::Pong:
            return;
        case WebSocketOpcode::Close:
        {
            receivedCloseFrame_ = true;
            closeCode_ = static_cast<std::uint16_t>(WebSocketCloseCode::NormalClosure);
            closeReason_.clear();
            if (frame.payload.size() >= 2)
            {
                closeCode_ = static_cast<std::uint16_t>((static_cast<std::uint16_t>(frame.payload[0]) << 8U) | frame.payload[1]);
                if (frame.payload.size() > 2)
                {
                    closeReason_.assign(reinterpret_cast<const char *>(frame.payload.data() + 2), frame.payload.size() - 2);
                }
            }

            if (closeState_ == CloseState::Open)
            {
                if (!frame.payload.empty())
                {
                    queueSerializedFrame(WebSocketOpcode::Close, std::span<const std::uint8_t>(frame.payload.data(), frame.payload.size()), true);
                }
                else
                {
                    queueCloseFrame(WebSocketCloseCode::NormalClosure);
                }
                beginClosing(closeCode_, closeReason_);
            }
            else
            {
                context_.markClosing(closeCode_, closeReason_);
            }
            return;
        }
        case WebSocketOpcode::Text:
        case WebSocketOpcode::Binary:
        {
            if (frame.header.fin)
            {
                if (frame.payload.size() > lumalink::http::core::WsMaxMessageSize)
                {
                    const WsClosePolicy policy = policyFor(WsErrorCategory::MessageTooLarge);
                    context_.notifyError("WebSocket message too big");
                    if (policy.attemptCloseHandshake)
                    {
                        queueCloseFrame(static_cast<WebSocketCloseCode>(policy.closeCode));
                        beginClosing(policy.closeCode, "");
                    }
                    return;
                }

                if (frame.header.opcode == WebSocketOpcode::Text)
                {
                    context_.notifyText(std::string_view(reinterpret_cast<const char *>(frame.payload.data()), frame.payload.size()));
                }
                else
                {
                    context_.notifyBinary(std::span<const std::uint8_t>(frame.payload.data(), frame.payload.size()));
                }

                assemblingMessage_ = false;
                messageBuffer_.clear();
                return;
            }

            assemblingMessage_ = true;
            assembledMessageType_ = frame.header.opcode;
            messageBuffer_.clear();
                    if (!appendMessageFragment(std::span<const std::uint8_t>(frame.payload.data(), frame.payload.size())))
            {
                const WsClosePolicy policy = policyFor(WsErrorCategory::MessageTooLarge);
                context_.notifyError("WebSocket message too big");
                if (policy.attemptCloseHandshake)
                {
                    queueCloseFrame(static_cast<WebSocketCloseCode>(policy.closeCode));
                    beginClosing(policy.closeCode, "");
                }
            }
            return;
        }
        case WebSocketOpcode::Continuation:
        {
            if (!assemblingMessage_)
            {
                context_.notifyError("WebSocket continuation without active message");
                queueCloseFrame(WebSocketCloseCode::ProtocolError);
                beginClosing(static_cast<std::uint16_t>(WebSocketCloseCode::ProtocolError), "");
                return;
            }

            if (!appendMessageFragment(std::span<const std::uint8_t>(frame.payload.data(), frame.payload.size())))
            {
                const WsClosePolicy policy = policyFor(WsErrorCategory::MessageTooLarge);
                context_.notifyError("WebSocket message too big");
                if (policy.attemptCloseHandshake)
                {
                    queueCloseFrame(static_cast<WebSocketCloseCode>(policy.closeCode));
                    beginClosing(policy.closeCode, "");
                }
                return;
            }

            if (frame.header.fin)
            {
                if (assembledMessageType_ == WebSocketOpcode::Text)
                {
                    context_.notifyText(std::string_view(reinterpret_cast<const char *>(messageBuffer_.data()), messageBuffer_.size()));
                }
                else if (assembledMessageType_ == WebSocketOpcode::Binary)
                {
                    context_.notifyBinary(std::span<const std::uint8_t>(messageBuffer_.data(), messageBuffer_.size()));
                }

                assemblingMessage_ = false;
                assembledMessageType_ = WebSocketOpcode::Continuation;
                messageBuffer_.clear();
            }
            return;
        }
        }
    }

    bool WebSocketProtocolExecution::appendMessageFragment(std::span<const std::uint8_t> payload)
    {
        if (payload.empty())
        {
            return messageBuffer_.size() <= lumalink::http::core::WsMaxMessageSize;
        }

        const std::size_t remaining = lumalink::http::core::WsMaxMessageSize - (std::min)(messageBuffer_.size(), lumalink::http::core::WsMaxMessageSize);
        if (payload.size() > remaining)
        {
            return false;
        }

        messageBuffer_.insert(messageBuffer_.end(), payload.begin(), payload.end());
        return true;
    }

    void WebSocketProtocolExecution::beginClosing(std::uint16_t closeCode, std::string_view reason)
    {
        closeCode_ = closeCode;
        closeReason_.assign(reason.data(), reason.size());
        closeState_ = CloseState::CloseQueued;
        context_.markClosing(closeCode_, closeReason_);
    }

    std::vector<std::uint8_t> WebSocketProtocolExecution::buildClosePayload(WebSocketCloseCode code, std::span<const std::uint8_t> reason)
    {
        std::vector<std::uint8_t> payload;
        payload.reserve(2 + reason.size());
        const std::uint16_t rawCode = static_cast<std::uint16_t>(code);
        payload.push_back(static_cast<std::uint8_t>((rawCode >> 8U) & 0xFFU));
        payload.push_back(static_cast<std::uint8_t>(rawCode & 0xFFU));
        payload.insert(payload.end(), reason.begin(), reason.end());
        return payload;
    }
}