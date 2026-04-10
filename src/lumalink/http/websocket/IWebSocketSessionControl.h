#pragma once

#include <exception>
#include "WebSocketFrameCodec.h"

#include "LumaLinkPlatform.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace lumalink::http::websocket
{
    

    enum class WebSocketSendResult
    {
        Queued,
        Backpressured,
        Closing,
        Closed,
        TooLarge,
        InvalidState,
        InternalError
    };

    enum class WebSocketCloseResult
    {
        CloseQueued,
        AlreadyClosing,
        AlreadyClosed,
        ReasonTooLarge,
        InvalidState,
        InternalError
    };

    class IWebSocketSessionControl
    {
    public:
        virtual ~IWebSocketSessionControl() = default;

        virtual WebSocketSendResult sendText(std::string_view payload) = 0;
        virtual WebSocketSendResult sendBinary(std::span<const std::uint8_t> payload) = 0;
        virtual WebSocketCloseResult close(WebSocketCloseCode code, std::string_view reason) = 0;
    };
}