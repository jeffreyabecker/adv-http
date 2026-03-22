#pragma once

#ifdef ARDUINO

#include "../compat/IpAddress.h"
#include <WiFi.h>

#include "StandardHttpServer.h"

namespace HttpServerAdvanced
{
    template <uint16_t DEFAULT_PORT = 80>
    class WiFiHttpServer : public StandardHttpServer<WiFiServer, DEFAULT_PORT>
    {
    private:
        using Base = StandardHttpServer<WiFiServer, DEFAULT_PORT>;

    public:
        using Base::Base;

        std::string_view localAddress() const override
        {
            localAddressCache_ = HttpServerAdvanced::IPAddress(WiFi.localIP()).toString();
            return localAddressCache_;
        }

    private:
        mutable std::string localAddressCache_;
    };
}

#endif