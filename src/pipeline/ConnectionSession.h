#pragma once

#include "TransportInterfaces.h"

#include "../compat/Clock.h"

namespace HttpServerAdvanced
{
    class IProtocolExecution;

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
        virtual IProtocolExecution *protocolExecution()
        {
            return nullptr;
        }
        virtual const IProtocolExecution *protocolExecution() const
        {
            return nullptr;
        }
    };
}