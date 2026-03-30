#pragma once
#include <memory>
#include <functional>

#include "../compat/Clock.h"
#include "../core/HttpTimeouts.h"
#include "../compat/ByteStream.h"
#include "ConnectionSession.h"
#include "HttpProtocolExecution.h"
#include "IPipelineHandler.h"
#include "IProtocolExecution.h"
#include "RequestParser.h"
#include "PipelineHandleClientResult.h"
#include "../compat/TransportInterfaces.h"

#include "../llhttp/include/llhttp.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <algorithm>

namespace httpadv::v1::server
{
    class HttpServerBase;
}

namespace httpadv::v1::pipeline
{
    using httpadv::v1::core::HttpTimeouts;
    using httpadv::v1::server::HttpServerBase;
    using httpadv::v1::util::Clock;
    using httpadv::v1::util::ClockMillis;

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
        std::unique_ptr<httpadv::v1::transport::IClient> client_;
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
        HttpPipeline(std::unique_ptr<httpadv::v1::transport::IClient> client, HttpServerBase &server,
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
        httpadv::v1::transport::IClient &client();
        ConnectionState connectionState() const;
    };

} // namespace httpadv::v1::pipeline

