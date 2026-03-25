#pragma once

#include "../core/HttpHeaderCollection.h"

#include <any>
#include <cstdint>
#include <map>
#include <string>

namespace HttpServerAdvanced
{
    struct WebSocketActivationSnapshot
    {
        std::map<std::string, std::any> items;
        std::string method;
        std::string version;
        std::string url;
        HttpHeaderCollection headers;
        std::string remoteAddress;
        std::uint16_t remotePort = 0;
        std::string localAddress;
        std::uint16_t localPort = 0;
    };
}