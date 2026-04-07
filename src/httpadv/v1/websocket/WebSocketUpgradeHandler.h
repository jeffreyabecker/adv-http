#pragma once

#include "../handlers/HandlerResult.h"
#include "../core/HttpRequestContext.h"
#include "WebSocketCallbacks.h"

namespace lumalink::http::websocket
{
    using lumalink::http::handlers::HandlerResult;
    using lumalink::http::core::HttpRequestContext;

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
