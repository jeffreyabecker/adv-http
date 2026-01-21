#pragma once
#include <memory>
#include <functional>
#include "../util/Util.h"
#include "./HttpTimeouts.h"

#include <http_parser.h>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <algorithm>

namespace HttpServerAdvanced::Pipeline
{
    // HttpRequestParser types and classes consolidated from HttpParser.h
    class HttpParserError
    {
    public:
        HttpParserError(enum http_errno err) : error_(err) {}
        enum http_errno error() const { return error_; }
        int32_t code() const { return static_cast<int32_t>(error_); }
        const char *message() const { return http_errno_description(error_); }
        const char *name() const { return http_errno_name(error_); }

    private:
        enum http_errno error_;
    };



    // Interface for handling parsed HTTP events
    class IPipelineHandler
    {
    public:
        virtual ~IPipelineHandler() = default;
        virtual int onMessageBegin(const char* method, uint16_t versionMajor, uint16_t versionMinor, String&& url) = 0;
        virtual void setIPAddress(IPAddress remoteIP, uint16_t remotePort, IPAddress localIP, uint16_t localPort) = 0;
        virtual void setResponseStreamCallback(std::function<void(std::unique_ptr<Stream>)> onStreamReady) = 0;
        virtual int onHeader(String&& field, String&& value) = 0;
        virtual int onHeadersComplete() = 0;
        virtual int onBody(const uint8_t *at, std::size_t length) = 0;
        virtual int onMessageComplete() = 0;
        virtual void onError(HttpParserError error) = 0;
        virtual void onResponseStarted() = 0;
        virtual void onResponseCompleted() = 0;
        virtual void onClientDisconnected() = 0;        
    };

    using PipelineHandlerPtr = std::unique_ptr<
        IPipelineHandler,
        std::function<void(IPipelineHandler*)>>;
     

}