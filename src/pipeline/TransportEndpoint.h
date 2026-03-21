#pragma once

#include "../compat/IpAddress.h"

#include <cstdint>

namespace HttpServerAdvanced
{
    struct TransportEndpoint
    {
        IPAddress address;
        std::uint16_t port = 0;

        TransportEndpoint() = default;

        TransportEndpoint(const IPAddress &addressValue, std::uint16_t portValue)
            : address(addressValue), port(portValue)
        {
        }

        bool hasPort() const
        {
            return port != 0;
        }
    };
}