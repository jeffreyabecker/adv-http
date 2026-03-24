#pragma once

#include "WebSocketCallbacks.h"

#include <string>

namespace HttpServerAdvanced
{
    struct WebSocketRoute
    {
        std::string path;
        WebSocketCallbacks callbacks;
    };
}
