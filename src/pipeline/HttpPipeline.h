#pragma once
#include <memory>
#include <functional>

#include "../compat/Clock.h"
#include "../core/HttpTimeouts.h"
#include "../streams/ByteStream.h"
#include "IPipelineHandler.h"
#include "RequestParser.h"
#include "PipelineHandleClientResult.h"
#include "TransportInterfaces.h"

#include "../llhttp/include/llhttp.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <algorithm>

namespace HttpServerAdvanced
{
    class HttpServerBase;
    class HttpPipeline
    {
    private:
        // HTTP parsing members
        RequestParser requestParser_;

        // Pipeline members
        std::unique_ptr<HttpServerAdvanced::IClient> client_;
        HttpServerBase &server_;
        const Compat::Clock &clock_;
        std::unique_ptr<IByteSource> responseStream_;
        PipelineHandlerPtr handler_;
        bool completedRequestRead_ = false;
        bool completedResponseWrite_ = false;
        bool haveStartedWritingResponse_ = false;
        bool aborted_ = false;
        bool erroredUnrecoverably_ = false;
        bool timedOutUnrecoverably_ = false;

        Compat::ClockMillis lastActivityMillis_;
        Compat::ClockMillis startMillis_;
        std::uint32_t loopCount_;
        HttpTimeouts timeouts_;

        enum class PipelineState
        {
            Begin,
            Url,
            HeaderField,
            HeaderValue,
            HeadersComplete,
            Body,
            MessageComplete
        };

        // Internal event handler for HttpRequestParser

        void readFromClientIntoParser();
        void writeResponseToClientFromStream();
        void setupPipeline();
        PipelineHandleClientResult _checkStateInHandleClient();
        bool isFirstLoop() const;
        void startActivity();
        bool checkActivityTimeout();
        void setErroredUnrecoverably();
        void markRequestReadCompleted();
        void markResponseWriteCompleted();
        Compat::ClockMillis currentMillis() const;


    public:
        HttpPipeline(std::unique_ptr<HttpServerAdvanced::IClient> client, HttpServerBase &server,
                     const HttpTimeouts &timeouts, PipelineHandlerPtr parserEventHandler,
                     const Compat::Clock &clock);

        ~HttpPipeline() = default;

        bool isFinished() const;
        bool shouldKeepAlive() const;
        void setResponseStream(std::unique_ptr<IByteSource> responseStream);
        PipelineHandleClientResult handleClient();
        void abort();
        void abortReadingRequest();
        void abortWritingResponse();
        Compat::ClockMillis startedAt() const;
        HttpServerAdvanced::IClient &client();
    };

} // namespace HttpServerAdvanced

