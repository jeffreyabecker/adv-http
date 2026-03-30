#pragma once

#include "ConnectionSession.h"
#include "PipelineError.h"
#include "RequestHandlingResult.h"
#include "../compat/TransportInterfaces.h"

namespace httpadv::v1::pipeline
{
    using httpadv::v1::server::ConnectionSessionResult;
    using httpadv::v1::transport::IClient;
    using httpadv::v1::util::Clock;

    class IProtocolExecution
    {
    public:
        virtual ~IProtocolExecution() = default;

        virtual ConnectionSessionResult handle(IClient &client, const Clock &clock) = 0;
        virtual void onError(PipelineError error) = 0;
        virtual void onDisconnect() = 0;

        virtual bool hasPendingResult() const = 0;
        virtual RequestHandlingResult takeResult() = 0;
        virtual bool isFinished() const = 0;
    };
}