#pragma once

#include "../handlers/HandlerResult.h"
#include "../core/HttpContext.h"
#include "WebSocketCallbacks.h"

namespace httpadv::v1::websocket
{
    using httpadv::v1::handlers::HandlerResult;

    class WebSocketUpgradeHandler
    {
    public:
        enum class UpgradeFailure
        {
            InvalidMethod,
            MissingUpgradeIntent,
            UnsupportedVersion,
            InvalidKeyLength,
            MalformedKeyText,
            ConflictingHeaders
        };

        static bool isWebSocketUpgradeCandidate(const httpadv::v1::core::HttpContext &request);
        HandlerResult handle(httpadv::v1::core::HttpContext &request, const WebSocketCallbacks &callbacks) const;

    private:
        static HandlerResult rejectUpgrade(httpadv::v1::core::HttpContext &request, UpgradeFailure failure);
    };
}
