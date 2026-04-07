#pragma once

#include "HttpContextRunner.h"
#include "../pipeline/IPipelineHandler.h"

#include <memory>

namespace lumalink::http::core
{
    class HttpContextPipelineAdapter : public lumalink::http::pipeline::IPipelineHandler
    {
    public:
        explicit HttpContextPipelineAdapter(std::unique_ptr<HttpContextRunner> runner);

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
        void onError(lumalink::http::pipeline::PipelineError error) override;
        void onResponseStarted() override;
        void onResponseCompleted() override;
        void onClientDisconnected() override;
        bool hasPendingResult() const override;
        lumalink::http::pipeline::RequestHandlingResult takeResult() override;

        HttpContextRunner &runner();
        const HttpContextRunner &runner() const;

    private:
        std::unique_ptr<HttpContextRunner> runner_;
    };
}
