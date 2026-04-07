#pragma once

#include "IWebSocketSessionControl.h"
#include "WebSocketActivationSnapshot.h"
#include "WebSocketCallbacks.h"

#include "../core/HttpHeaderCollection.h"

#include <any>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace httpadv::v1::websocket
{
    using httpadv::v1::core::HttpHeaderCollection;
    using lumalink::Span;

    class WebSocketProtocolExecution;
    class WebSocketContextTestProbe;

    class WebSocketContext
    {
    public:
        WebSocketContext(WebSocketActivationSnapshot activation, WebSocketCallbacks callbacks);

        std::map<std::string, std::any> &items();
        const std::map<std::string, std::any> &items() const;

        std::string_view methodView() const;
        std::string_view versionView() const;
        std::string_view urlView() const;
        const HttpHeaderCollection &headers() const;

        std::string_view remoteAddress() const;
        std::uint16_t remotePort() const;
        std::string_view localAddress() const;
        std::uint16_t localPort() const;

        WebSocketSendResult sendText(std::string_view payload);
        WebSocketSendResult sendBinary(span<const std::uint8_t> payload);
        WebSocketCloseResult close(WebSocketCloseCode code = WebSocketCloseCode::NormalClosure, std::string_view reason = {});

        bool isOpen() const;
        bool isClosing() const;
        std::optional<std::uint16_t> closeCode() const;
        std::string_view closeReason() const;

    private:
        enum class State
        {
            Handshaking,
            Open,
            Closing,
            Closed
        };

        WebSocketActivationSnapshot activation_;
        WebSocketCallbacks callbacks_;
        IWebSocketSessionControl *control_ = nullptr;
        State state_ = State::Handshaking;
        bool openNotified_ = false;
        bool closeNotified_ = false;
        std::optional<std::uint16_t> closeCode_;
        std::string closeReason_;

        void bindControl(IWebSocketSessionControl *control);
        void markOpen();
        void markClosing(std::uint16_t closeCode, std::string_view reason);
        void markClosed(std::uint16_t closeCode, std::string_view reason);
        void notifyOpen();
        void notifyText(std::string_view message);
        void notifyBinary(span<const std::uint8_t> payload);
        void notifyClose(std::uint16_t closeCode, std::string_view reason);
        void notifyError(std::string_view message);

        friend class WebSocketProtocolExecution;
        friend class WebSocketContextTestProbe;
    };
}