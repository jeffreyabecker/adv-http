#pragma once

#include <exception>
#include "LumaLinkPlatform.h"

#include "../util/Clock.h"

namespace lumalink::http::pipeline
{
    class IProtocolExecution;
}

namespace lumalink::http::server
{
    using lumalink::platform::transport::IClient;
    using lumalink::http::util::Clock;
    using lumalink::http::pipeline::IProtocolExecution;

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