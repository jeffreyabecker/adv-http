#pragma once

#include "../compat/Span.h"

#include <cstdint>
#include <functional>
#include <string_view>

namespace HttpServerAdvanced
{
    struct WebSocketCallbacks
    {
        std::function<void()> onOpen;
        std::function<void(std::string_view)> onText;
        std::function<void(span<const std::uint8_t>)> onBinary;
        std::function<void(std::uint16_t, std::string_view)> onClose;
        std::function<void(std::string_view)> onError;
    };
}
