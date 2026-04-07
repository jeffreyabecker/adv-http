#pragma once

#include <cstdint>

namespace lumalink::http::websocket
{
    enum class WsErrorCategory
    {
        FrameParseError,
        ProtocolViolation,
        MessageTooLarge,
        WriteFailure,
        IdleTimeout,
        CloseHandshakeTimeout,
        RemoteDisconnect
    };

    struct WsClosePolicy
    {
        bool attemptCloseHandshake = false;
        std::uint16_t closeCode = 1006;
    };

    constexpr WsClosePolicy policyFor(WsErrorCategory category)
    {
        switch (category)
        {
        case WsErrorCategory::FrameParseError:
            return WsClosePolicy{false, 1002};
        case WsErrorCategory::ProtocolViolation:
            return WsClosePolicy{true, 1002};
        case WsErrorCategory::MessageTooLarge:
            return WsClosePolicy{true, 1009};
        case WsErrorCategory::WriteFailure:
            return WsClosePolicy{false, 1006};
        case WsErrorCategory::IdleTimeout:
            return WsClosePolicy{true, 1001};
        case WsErrorCategory::CloseHandshakeTimeout:
            return WsClosePolicy{false, 1006};
        case WsErrorCategory::RemoteDisconnect:
            return WsClosePolicy{false, 1006};
        default:
            return WsClosePolicy{false, 1006};
        }
    }
}
