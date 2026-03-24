#pragma once

#include "WebSocketFrameCodec.h"

#include "../pipeline/ConnectionSession.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace HttpServerAdvanced
{
    class WebSocketSessionRuntime : public IConnectionSession
    {
    public:
        explicit WebSocketSessionRuntime(std::string handshakeResponse);

        ConnectionSessionResult handle(IClient &client, const Compat::Clock &clock) override;

    private:
        enum class CloseState
        {
            Open,
            CloseQueued,
            CloseSent,
            Closed
        };

        WebSocketFrameParser parser_;
        std::vector<std::uint8_t> pendingWrite_;
        std::size_t pendingWriteOffset_ = 0;
        bool handshakeWritten_ = false;

        bool assemblingMessage_ = false;
        std::vector<std::uint8_t> messageBuffer_;

        CloseState closeState_ = CloseState::Open;
        bool receivedCloseFrame_ = false;

        bool flushPendingWrite(IClient &client);
        bool queueSerializedFrame(WebSocketOpcode opcode, span<const std::uint8_t> payload, bool fin = true);
        bool queueCloseFrame(WebSocketCloseCode code, span<const std::uint8_t> reason = span<const std::uint8_t>());
        void handleParsedFrame(const WebSocketFrame &frame);
        bool appendMessageFragment(span<const std::uint8_t> payload);
        static std::vector<std::uint8_t> buildClosePayload(WebSocketCloseCode code, span<const std::uint8_t> reason);
    };
}
