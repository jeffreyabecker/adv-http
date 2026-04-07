#pragma once
#include <memory>
#include <functional>

#include <lumalink/platform/time/Clock.h>
#include <lumalink/platform/time/ClockTypes.h>
#include "../core/HttpTimeouts.h"
#include "LumaLinkPlatform.h"
#include "ConnectionSession.h"
#include "HttpProtocolExecution.h"
#include "IPipelineHandler.h"
#include "IProtocolExecution.h"
#include "RequestParser.h"
#include "PipelineHandleClientResult.h"
#include "LumaLinkPlatform.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <algorithm>

namespace lumalink::http::server
{
    class HttpServerBase;
}

namespace lumalink::http::pipeline
{
    using lumalink::http::core::HttpTimeouts;
    using lumalink::http::server::HttpServerBase;
    using lumalink::platform::time::IMonotonicClock;
    using lumalink::platform::time::MonotonicMillis;

    class HttpPipeline
    {
    public:
        enum class ConnectionState
        {
            ReadingHttpRequest,
            WritingHttpResponse,
            UpgradedSessionActive,
            Closing,
            Completed,
            Aborted,
            Error
        };

    private:
        std::unique_ptr<lumalink::platform::transport::IClient> client_;
        HttpServerBase &server_;
        const IMonotonicClock &clock_;
        std::function<PipelineHandlerPtr()> handlerFactory_;
        std::unique_ptr<IByteSource> responseStream_;
        std::unique_ptr<IConnectionSession> activeSession_;
        std::unique_ptr<HttpProtocolExecution> requestExecution_;
        ConnectionState connectionState_ = ConnectionState::ReadingHttpRequest;
        bool responseStartedNotified_ = false;
        bool disconnectNotified_ = false;
        bool timedOutUnrecoverably_ = false;

        MonotonicMillis lastActivityMillis_;
        MonotonicMillis startMillis_;
        std::uint32_t loopCount_;
        HttpTimeouts timeouts_;

        void createRequestHandler();
        void readFromClientIntoParser();
        void writeResponseToClientFromStream();
        void handleActiveSession();
        void consumePendingRequestResult();
        bool beginNextKeepAliveRequest();
        void setupPipeline();
        PipelineHandleClientResult checkStateInHandleClient();
        bool isFirstLoop() const;
        bool isTerminalState() const;
        void startActivity();
        bool checkActivityTimeout();
        bool checkIdleTimeout();
        void setErroredUnrecoverably();
        void setConnectionState(ConnectionState state);
        MonotonicMillis currentMillis() const;
        IProtocolExecution *currentProtocolExecution();
        const IProtocolExecution *currentProtocolExecution() const;
    public:
        HttpPipeline(std::unique_ptr<lumalink::platform::transport::IClient> client, HttpServerBase &server,
                     const HttpTimeouts &timeouts, std::function<PipelineHandlerPtr()> handlerFactory,
                     const IMonotonicClock &clock);

        ~HttpPipeline() = default;

        bool isFinished() const;
        bool shouldKeepAlive() const;
        PipelineHandleClientResult handleClient();
        void abort();
        void abortReadingRequest();
        void abortWritingResponse();
        MonotonicMillis startedAt() const;
        lumalink::platform::transport::IClient &client();
        ConnectionState connectionState() const;
    };

} // namespace lumalink::http::pipeline

