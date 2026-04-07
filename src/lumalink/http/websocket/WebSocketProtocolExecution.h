#pragma once

#include "IWebSocketSessionControl.h"
#include "WebSocketContext.h"
#include "WebSocketFrameCodec.h"

#include "../pipeline/ConnectionSession.h"
#include "../pipeline/IProtocolExecution.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace lumalink::http::websocket
{
    class WebSocketProtocolExecution : public lumalink::http::server::IConnectionSession, public lumalink::http::pipeline::IProtocolExecution, public IWebSocketSessionControl
    {
    public:
        WebSocketProtocolExecution(std::string handshakeResponse, WebSocketContext context);

        lumalink::http::server::ConnectionSessionResult handle(lumalink::platform::transport::IClient &client, const lumalink::http::util::Clock &clock) override;
        lumalink::http::pipeline::IProtocolExecution *protocolExecution() override
        {
            return this;
        }
        const lumalink::http::pipeline::IProtocolExecution *protocolExecution() const override
        {
            return this;
        }
        void onError(lumalink::http::pipeline::PipelineError error) override;
        void onDisconnect() override;
        bool hasPendingResult() const override;
        lumalink::http::pipeline::RequestHandlingResult takeResult() override;
        bool isFinished() const override;

        WebSocketSendResult sendText(std::string_view payload) override;
        WebSocketSendResult sendBinary(span<const std::uint8_t> payload) override;
        WebSocketCloseResult close(WebSocketCloseCode code, std::string_view reason) override;

        WebSocketContext &context();
        const WebSocketContext &context() const;

    private:
        enum class CloseState
        {
            Open,
            CloseQueued,
            CloseSent,
            Closed
        };

        WebSocketContext context_;
        WebSocketFrameParser parser_;
        std::vector<std::uint8_t> pendingWrite_;
        std::size_t pendingWriteOffset_ = 0;
        bool handshakeWritten_ = false;

        bool assemblingMessage_ = false;
        WebSocketOpcode assembledMessageType_ = WebSocketOpcode::Continuation;
        std::vector<std::uint8_t> messageBuffer_;

        CloseState closeState_ = CloseState::Open;
        bool receivedCloseFrame_ = false;
        std::uint16_t closeCode_ = static_cast<std::uint16_t>(WebSocketCloseCode::NormalClosure);
        std::string closeReason_;

        bool flushPendingWrite(lumalink::platform::transport::IClient &client);
        bool queueSerializedFrame(WebSocketOpcode opcode, span<const std::uint8_t> payload, bool fin = true);
        bool queueCloseFrame(WebSocketCloseCode code, span<const std::uint8_t> reason = span<const std::uint8_t>());
        void handleParsedFrame(const WebSocketFrame &frame);
        bool appendMessageFragment(span<const std::uint8_t> payload);
        void beginClosing(std::uint16_t closeCode, std::string_view reason);
        static std::vector<std::uint8_t> buildClosePayload(WebSocketCloseCode code, span<const std::uint8_t> reason);
    };
}