#include "WebSocketSessionRuntime.h"

#include "../core/Defines.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace HttpServerAdvanced
{
    WebSocketSessionRuntime::WebSocketSessionRuntime(std::string handshakeResponse, WebSocketCallbacks callbacks)
        : parser_(true),
          pendingWrite_(handshakeResponse.begin(), handshakeResponse.end()),
          callbacks_(std::move(callbacks))
    {
    }

    ConnectionSessionResult WebSocketSessionRuntime::handle(IClient &client, const Compat::Clock &)
    {
        if (!client.connected())
        {
            notifyError("WebSocket disconnected");
            notifyClose(1006, "");
            closeState_ = CloseState::Closed;
            return ConnectionSessionResult::Completed;
        }

        const bool hadPendingBeforeFlush = !pendingWrite_.empty();
        if (!flushPendingWrite(client))
        {
            return ConnectionSessionResult::Continue;
        }

        if (!handshakeWritten_)
        {
            handshakeWritten_ = true;
            notifyOpen();
            return ConnectionSessionResult::Continue;
        }

        if (hadPendingBeforeFlush && closeState_ == CloseState::CloseQueued)
        {
            closeState_ = CloseState::CloseSent;
            if (receivedCloseFrame_)
            {
                notifyClose(closeCode_, closeReason_);
                closeState_ = CloseState::Closed;
                return ConnectionSessionResult::Completed;
            }
        }

        std::array<std::uint8_t, PIPELINE_STACK_BUFFER_SIZE> readBuffer = {};
        while (client.available().hasBytes())
        {
            const std::size_t bytesRead = client.read(span<std::uint8_t>(readBuffer.data(), readBuffer.size()));
            if (bytesRead == 0)
            {
                break;
            }

            std::size_t offset = 0;
            while (offset < bytesRead)
            {
                const auto input = span<const std::uint8_t>(readBuffer.data() + offset, bytesRead - offset);
                const WebSocketParseResult parseResult = parser_.parse(input);
                offset += parseResult.bytesConsumed;

                if (parseResult.status == WebSocketCodecStatus::NeedMoreData)
                {
                    break;
                }

                if (parseResult.status == WebSocketCodecStatus::ProtocolError)
                {
                    notifyError("WebSocket protocol error");
                    queueCloseFrame(WebSocketCloseCode::ProtocolError);
                    closeCode_ = static_cast<std::uint16_t>(WebSocketCloseCode::ProtocolError);
                    closeReason_.clear();
                    closeState_ = CloseState::CloseQueued;
                    break;
                }

                if (parseResult.status == WebSocketCodecStatus::Ok && parseResult.frameReady)
                {
                    handleParsedFrame(parseResult.frame);
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
                return ConnectionSessionResult::Continue;
            }
        }

        if (closeState_ == CloseState::CloseQueued)
        {
            if (!flushPendingWrite(client))
            {
                return ConnectionSessionResult::Continue;
            }

            closeState_ = CloseState::CloseSent;
            if (receivedCloseFrame_)
            {
                notifyClose(closeCode_, closeReason_);
                closeState_ = CloseState::Closed;
                return ConnectionSessionResult::Completed;
            }
        }

        return ConnectionSessionResult::Continue;
    }

    bool WebSocketSessionRuntime::flushPendingWrite(IClient &client)
    {
        while (pendingWriteOffset_ < pendingWrite_.size())
        {
            const std::size_t remaining = pendingWrite_.size() - pendingWriteOffset_;
            const std::size_t written = client.write(span<const std::uint8_t>(pendingWrite_.data() + pendingWriteOffset_, remaining));
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

    bool WebSocketSessionRuntime::queueSerializedFrame(WebSocketOpcode opcode, span<const std::uint8_t> payload, bool fin)
    {
        if (!pendingWrite_.empty())
        {
            return false;
        }

        pendingWrite_.resize(WebSocketFrameSerializer::maxSerializedSize(payload.size()));
        const WebSocketSerializeResult result = WebSocketFrameSerializer::serialize(
            span<std::uint8_t>(pendingWrite_.data(), pendingWrite_.size()),
            payload,
            opcode,
            fin);

        if (result.status != WebSocketCodecStatus::Ok)
        {
            pendingWrite_.clear();
            pendingWriteOffset_ = 0;
            return false;
        }

        pendingWrite_.resize(result.bytesWritten);
        pendingWriteOffset_ = 0;
        return true;
    }

    bool WebSocketSessionRuntime::queueCloseFrame(WebSocketCloseCode code, span<const std::uint8_t> reason)
    {
        const std::vector<std::uint8_t> payload = buildClosePayload(code, reason);
        return queueSerializedFrame(WebSocketOpcode::Close, span<const std::uint8_t>(payload.data(), payload.size()), true);
    }

    void WebSocketSessionRuntime::handleParsedFrame(const WebSocketFrame &frame)
    {
        switch (frame.header.opcode)
        {
        case WebSocketOpcode::Ping:
            queueSerializedFrame(
                WebSocketOpcode::Pong,
                span<const std::uint8_t>(frame.payload.data(), frame.payload.size()),
                true);
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
                    queueSerializedFrame(
                        WebSocketOpcode::Close,
                        span<const std::uint8_t>(frame.payload.data(), frame.payload.size()),
                        true);
                }
                else
                {
                    queueCloseFrame(WebSocketCloseCode::NormalClosure);
                }
                closeState_ = CloseState::CloseQueued;
            }
            return;
        }
        case WebSocketOpcode::Text:
        case WebSocketOpcode::Binary:
        {
            if (frame.header.fin)
            {
                if (frame.payload.size() > WsMaxMessageSize)
                {
                    notifyError("WebSocket message too big");
                    queueCloseFrame(WebSocketCloseCode::MessageTooBig);
                    closeCode_ = static_cast<std::uint16_t>(WebSocketCloseCode::MessageTooBig);
                    closeReason_.clear();
                    closeState_ = CloseState::CloseQueued;
                    return;
                }

                if (frame.header.opcode == WebSocketOpcode::Text)
                {
                    if (callbacks_.onText)
                    {
                        callbacks_.onText(std::string_view(reinterpret_cast<const char *>(frame.payload.data()), frame.payload.size()));
                    }
                }
                else if (callbacks_.onBinary)
                {
                    callbacks_.onBinary(span<const std::uint8_t>(frame.payload.data(), frame.payload.size()));
                }

                assemblingMessage_ = false;
                messageBuffer_.clear();
                return;
            }

            assemblingMessage_ = true;
            assembledMessageType_ = frame.header.opcode;
            messageBuffer_.clear();
            if (!appendMessageFragment(span<const std::uint8_t>(frame.payload.data(), frame.payload.size())))
            {
                notifyError("WebSocket message too big");
                queueCloseFrame(WebSocketCloseCode::MessageTooBig);
                closeCode_ = static_cast<std::uint16_t>(WebSocketCloseCode::MessageTooBig);
                closeReason_.clear();
                closeState_ = CloseState::CloseQueued;
            }
            return;
        }
        case WebSocketOpcode::Continuation:
        {
            if (!assemblingMessage_)
            {
                notifyError("WebSocket continuation without active message");
                queueCloseFrame(WebSocketCloseCode::ProtocolError);
                closeCode_ = static_cast<std::uint16_t>(WebSocketCloseCode::ProtocolError);
                closeReason_.clear();
                closeState_ = CloseState::CloseQueued;
                return;
            }

            if (!appendMessageFragment(span<const std::uint8_t>(frame.payload.data(), frame.payload.size())))
            {
                notifyError("WebSocket message too big");
                queueCloseFrame(WebSocketCloseCode::MessageTooBig);
                closeCode_ = static_cast<std::uint16_t>(WebSocketCloseCode::MessageTooBig);
                closeReason_.clear();
                closeState_ = CloseState::CloseQueued;
                return;
            }

            if (frame.header.fin)
            {
                if (assembledMessageType_ == WebSocketOpcode::Text)
                {
                    if (callbacks_.onText)
                    {
                        callbacks_.onText(std::string_view(reinterpret_cast<const char *>(messageBuffer_.data()), messageBuffer_.size()));
                    }
                }
                else if (assembledMessageType_ == WebSocketOpcode::Binary)
                {
                    if (callbacks_.onBinary)
                    {
                        callbacks_.onBinary(span<const std::uint8_t>(messageBuffer_.data(), messageBuffer_.size()));
                    }
                }

                assemblingMessage_ = false;
                assembledMessageType_ = WebSocketOpcode::Continuation;
                messageBuffer_.clear();
            }
            return;
        }
        }
    }

    bool WebSocketSessionRuntime::appendMessageFragment(span<const std::uint8_t> payload)
    {
        if (payload.empty())
        {
            return messageBuffer_.size() <= WsMaxMessageSize;
        }

        const std::size_t remaining = WsMaxMessageSize - (std::min)(messageBuffer_.size(), WsMaxMessageSize);
        if (payload.size() > remaining)
        {
            return false;
        }

        messageBuffer_.insert(messageBuffer_.end(), payload.begin(), payload.end());
        return true;
    }

    void WebSocketSessionRuntime::notifyOpen()
    {
        if (openNotified_)
        {
            return;
        }

        openNotified_ = true;
        if (callbacks_.onOpen)
        {
            callbacks_.onOpen();
        }
    }

    void WebSocketSessionRuntime::notifyClose(std::uint16_t closeCode, std::string_view reason)
    {
        if (closeNotified_)
        {
            return;
        }

        closeNotified_ = true;
        if (callbacks_.onClose)
        {
            callbacks_.onClose(closeCode, reason);
        }
    }

    void WebSocketSessionRuntime::notifyError(std::string_view message)
    {
        if (callbacks_.onError)
        {
            callbacks_.onError(message);
        }
    }

    std::vector<std::uint8_t> WebSocketSessionRuntime::buildClosePayload(WebSocketCloseCode code, span<const std::uint8_t> reason)
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
