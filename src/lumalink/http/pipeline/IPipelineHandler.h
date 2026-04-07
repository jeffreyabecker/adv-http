#pragma once
#include <memory>
#include <functional>

#include "../core/HttpTimeouts.h"
#include "RequestHandlingResult.h"
#include "LumaLinkPlatform.h"
#include "PipelineError.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <algorithm>
#include <string_view>

namespace lumalink::http::pipeline
{




    // Interface for handling parsed HTTP events
    class IPipelineHandler
    {
    public:
        virtual ~IPipelineHandler() = default;
        virtual int onMessageBegin(const char* method, uint16_t versionMajor, uint16_t versionMinor, std::string_view url) = 0;
        virtual void setAddresses(std::string_view remoteAddress, uint16_t remotePort, std::string_view localAddress, uint16_t localPort) = 0;
        virtual int onHeader(std::string_view field, std::string_view value) = 0;
        virtual int onHeadersComplete() = 0;
        virtual int onBody(const uint8_t *at, std::size_t length) = 0;
        virtual int onMessageComplete() = 0;
        virtual void onError(PipelineError error) = 0;
        virtual void onResponseStarted() = 0;
        virtual void onResponseCompleted() = 0;
        virtual void onClientDisconnected() = 0;
        virtual bool hasPendingResult() const = 0;
        virtual RequestHandlingResult takeResult() = 0;
    };

    using PipelineHandlerPtr = std::unique_ptr<
        IPipelineHandler,
        std::function<void(IPipelineHandler*)>>;
     

}

