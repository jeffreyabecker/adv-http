#pragma once

#include "../util/Span.h"

#include <cstdint>
#include <functional>
#include <string_view>

namespace httpadv::v1::websocket
{
    using lumalink::Span;

    class WebSocketContext;

    struct WebSocketCallbacks
    {
        std::function<void(WebSocketContext &)> onOpen;
        std::function<void(WebSocketContext &, std::string_view)> onText;
        std::function<void(WebSocketContext &, span<const std::uint8_t>)> onBinary;
        std::function<void(WebSocketContext &, std::uint16_t, std::string_view)> onClose;
        std::function<void(WebSocketContext &, std::string_view)> onError;
    };
}
