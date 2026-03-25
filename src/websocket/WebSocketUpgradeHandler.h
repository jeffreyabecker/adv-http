#pragma once

#include "../handlers/HandlerResult.h"
#include "WebSocketCallbacks.h"

namespace HttpServerAdvanced
{
    class HttpContext;

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

        static bool isWebSocketUpgradeCandidate(const HttpContext &request);
        HandlerResult handle(HttpContext &request, const WebSocketCallbacks &callbacks) const;

    private:
        static HandlerResult rejectUpgrade(HttpContext &request, UpgradeFailure failure);
    };
}
