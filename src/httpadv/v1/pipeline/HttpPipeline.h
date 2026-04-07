#pragma once
#include <memory>
#include <functional>

#include "../util/Clock.h"
#include "../core/HttpTimeouts.h"
#include "../transport/ByteStream.h"
#include "ConnectionSession.h"
#include "HttpProtocolExecution.h"
#include "IPipelineHandler.h"
#include "IProtocolExecution.h"
#include "RequestParser.h"
#include "PipelineHandleClientResult.h"
#include "../transport/TransportInterfaces.h"
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
    using lumalink::http::util::Clock;
    using lumalink::http::util::ClockMillis;

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
        std::unique_ptr<lumalink::http::transport::IClient> client_;
        HttpServerBase &server_;
        const Clock &clock_;
        std::function<PipelineHandlerPtr()> handlerFactory_;
        std::unique_ptr<IByteSource> responseStream_;
        std::unique_ptr<IConnectionSession> activeSession_;
        std::unique_ptr<HttpProtocolExecution> requestExecution_;
        ConnectionState connectionState_ = ConnectionState::ReadingHttpRequest;
        bool responseStartedNotified_ = false;
        bool disconnectNotified_ = false;
        bool timedOutUnrecoverably_ = false;

        ClockMillis lastActivityMillis_;
        ClockMillis startMillis_;
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
        ClockMillis currentMillis() const;
        IProtocolExecution *currentProtocolExecution();
        const IProtocolExecution *currentProtocolExecution() const;
    public:
        HttpPipeline(std::unique_ptr<lumalink::http::transport::IClient> client, HttpServerBase &server,
                     const HttpTimeouts &timeouts, std::function<PipelineHandlerPtr()> handlerFactory,
                     const Clock &clock);

        ~HttpPipeline() = default;

        bool isFinished() const;
        bool shouldKeepAlive() const;
        PipelineHandleClientResult handleClient();
        void abort();
        void abortReadingRequest();
        void abortWritingResponse();
        ClockMillis startedAt() const;
        lumalink::http::transport::IClient &client();
        ConnectionState connectionState() const;
    };

} // namespace lumalink::http::pipeline

