#pragma once

#include "TransportInterfaces.h"

#include "../compat/Clock.h"

namespace HttpServerAdvanced
{
    enum class ConnectionSessionResult
    {
        Continue,
        Completed,
        AbortConnection,
        ProtocolError
    };

    class IConnectionSession
    {
    public:
        virtual ~IConnectionSession() = default;
        virtual ConnectionSessionResult handle(IClient &client, const Compat::Clock &clock) = 0;
    };
}