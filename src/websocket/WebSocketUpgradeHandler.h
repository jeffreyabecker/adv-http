#pragma once

#include "../handlers/HandlerResult.h"
#include "WebSocketCallbacks.h"

namespace HttpServerAdvanced
{
    class HttpRequest;

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

        static bool isWebSocketUpgradeCandidate(const HttpRequest &request);
        HandlerResult handle(HttpRequest &request, const WebSocketCallbacks &callbacks) const;

    private:
        static HandlerResult rejectUpgrade(HttpRequest &request, UpgradeFailure failure);
    };
}
