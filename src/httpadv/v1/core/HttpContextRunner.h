#pragma once

#include "HttpContextPhase.h"
#include "HttpRequestContext.h"
#include "../pipeline/PipelineError.h"
#include "../pipeline/RequestHandlingResult.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace httpadv::v1::core
{
    class HttpContextRunner
    {
    public:
        virtual ~HttpContextRunner() = default;

        virtual HttpRequestContext &context() = 0;

        virtual int onMessageBegin(const char *method,
                                   std::uint16_t versionMajor,
                                   std::uint16_t versionMinor,
                                   std::string_view url) = 0;
        virtual void setAddresses(std::string_view remoteAddress,
                                  std::uint16_t remotePort,
                                  std::string_view localAddress,
                                  std::uint16_t localPort) = 0;
        virtual int onHeader(std::string_view field, std::string_view value) = 0;
        virtual int onHeadersComplete() = 0;
        virtual int onBody(const std::uint8_t *at, std::size_t length) = 0;

        virtual void advance(HttpContextPhaseFlags trigger) = 0;
        virtual void onError(httpadv::v1::pipeline::PipelineError error) = 0;
        virtual void onClientDisconnected() = 0;

        virtual bool hasPendingResult() const = 0;
        virtual httpadv::v1::pipeline::RequestHandlingResult takeResult() = 0;
    };
}
