#pragma once

#include "../handlers/HandlerResult.h"
#include "../core/HttpRequestContext.h"
#include "WebSocketCallbacks.h"

namespace httpadv::v1::websocket
{
    using httpadv::v1::handlers::HandlerResult;
    using httpadv::v1::core::HttpRequestContext;

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

        static bool isWebSocketUpgradeCandidate(const HttpRequestContext &request);
        HandlerResult handle(HttpRequestContext &request, const WebSocketCallbacks &callbacks) const;

    private:
        static HandlerResult rejectUpgrade(HttpRequestContext &request, UpgradeFailure failure);
    };
}
