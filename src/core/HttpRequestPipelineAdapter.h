#pragma once

#include "HttpRequestRunner.h"
#include "../pipeline/IPipelineHandler.h"

#include <memory>

namespace HttpServerAdvanced
{
    class HttpRequestPipelineAdapter : public IPipelineHandler
    {
    public:
        explicit HttpRequestPipelineAdapter(std::unique_ptr<HttpRequestRunner> runner);

        int onMessageBegin(const char *method,
                           std::uint16_t versionMajor,
                           std::uint16_t versionMinor,
                           std::string_view url) override;
        void setAddresses(std::string_view remoteAddress,
                          std::uint16_t remotePort,
                          std::string_view localAddress,
                          std::uint16_t localPort) override;
        int onHeader(std::string_view field, std::string_view value) override;
        int onHeadersComplete() override;
        int onBody(const std::uint8_t *at, std::size_t length) override;
        int onMessageComplete() override;
        void onError(PipelineError error) override;
        void onResponseStarted() override;
        void onResponseCompleted() override;
        void onClientDisconnected() override;
        bool hasPendingResult() const override;
        RequestHandlingResult takeResult() override;

        HttpRequestRunner &runner();
        const HttpRequestRunner &runner() const;

    private:
        std::unique_ptr<HttpRequestRunner> runner_;
    };
}
