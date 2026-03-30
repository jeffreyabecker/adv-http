#pragma once

#include "../compat/TransportInterfaces.h"

#include "../compat/Clock.h"

namespace httpadv::v1::pipeline
{
    class IProtocolExecution;
}

namespace httpadv::v1::server
{
    using httpadv::v1::transport::IClient;
    using httpadv::v1::util::Clock;
    using httpadv::v1::pipeline::IProtocolExecution;

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
        virtual ConnectionSessionResult handle(IClient &client, const Clock &clock) = 0;
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