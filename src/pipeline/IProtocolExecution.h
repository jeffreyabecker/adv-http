#pragma once

#include "ConnectionSession.h"
#include "PipelineError.h"
#include "RequestHandlingResult.h"
#include "TransportInterfaces.h"

namespace HttpServerAdvanced
{
    class IProtocolExecution
    {
    public:
        virtual ~IProtocolExecution() = default;

        virtual ConnectionSessionResult handle(IClient &client, const Compat::Clock &clock) = 0;
        virtual void onError(PipelineError error) = 0;
        virtual void onDisconnect() = 0;

        virtual bool hasPendingResult() const = 0;
        virtual RequestHandlingResult takeResult() = 0;
        virtual bool isFinished() const = 0;
    };
}