#include "WebSocketSessionRuntime.h"

#include "../core/Defines.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace HttpServerAdvanced
{
    WebSocketSessionRuntime::WebSocketSessionRuntime(std::string handshakeResponse)
        : parser_(true),
          pendingWrite_(handshakeResponse.begin(), handshakeResponse.end())
    {
    }

    ConnectionSessionResult WebSocketSessionRuntime::handle(IClient &client, const Compat::Clock &)
    {
        if (!client.connected())
        {
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
            return ConnectionSessionResult::Continue;
        }

        if (hadPendingBeforeFlush && closeState_ == CloseState::CloseQueued)
        {
            closeState_ = CloseState::CloseSent;
            if (receivedCloseFrame_)
            {
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
                    queueCloseFrame(WebSocketCloseCode::ProtocolError);
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
                    queueCloseFrame(WebSocketCloseCode::MessageTooBig);
                    closeState_ = CloseState::CloseQueued;
                    return;
                }

                assemblingMessage_ = false;
                messageBuffer_.clear();
                return;
            }

            assemblingMessage_ = true;
            messageBuffer_.clear();
            if (!appendMessageFragment(span<const std::uint8_t>(frame.payload.data(), frame.payload.size())))
            {
                queueCloseFrame(WebSocketCloseCode::MessageTooBig);
                closeState_ = CloseState::CloseQueued;
            }
            return;
        }
        case WebSocketOpcode::Continuation:
        {
            if (!assemblingMessage_)
            {
                queueCloseFrame(WebSocketCloseCode::ProtocolError);
                closeState_ = CloseState::CloseQueued;
                return;
            }

            if (!appendMessageFragment(span<const std::uint8_t>(frame.payload.data(), frame.payload.size())))
            {
                queueCloseFrame(WebSocketCloseCode::MessageTooBig);
                closeState_ = CloseState::CloseQueued;
                return;
            }

            if (frame.header.fin)
            {
                assemblingMessage_ = false;
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
