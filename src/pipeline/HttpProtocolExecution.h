#pragma once

#include "IProtocolExecution.h"
#include "IPipelineHandler.h"
#include "RequestParser.h"

namespace httpadv::v1::pipeline
{
    class HttpProtocolExecution : public IProtocolExecution
    {
    public:
        explicit HttpProtocolExecution(PipelineHandlerPtr handler);

        ConnectionSessionResult handle(IClient &client, const Clock &clock) override;
        void onError(PipelineError error) override;
        void onDisconnect() override;
        bool hasPendingResult() const override;
        RequestHandlingResult takeResult() override;
        bool isFinished() const override;

        void setAddresses(std::string_view remoteAddress,
                          std::uint16_t remotePort,
                          std::string_view localAddress,
                          std::uint16_t localPort);
        bool shouldKeepAlive() const;
        bool consumedInput() const;
        void markRequestReadCompleted();
        void onResponseStarted();
        void onResponseCompleted();

    private:
        PipelineHandlerPtr handler_;
        std::unique_ptr<RequestParser> requestParser_;
        bool requestReadCompleted_ = false;
        bool consumedInput_ = false;
    };
}