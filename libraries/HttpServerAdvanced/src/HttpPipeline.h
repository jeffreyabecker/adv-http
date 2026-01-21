#pragma once
#include <memory>
#include <functional>

#include "./HttpTimeouts.h"
#include "./IPipelineHandler.h"
#include "./RequestParser.h"
#include "./PipelineHandleClientResult.h"
#include "./NetClient.h"

#include <http_parser.h>
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
        std::unique_ptr<Stream> responseStream_;
        PipelineHandlerPtr handler_;
        bool completedRequestRead_ = false;
        bool completedResponseWrite_ = false;
        bool haveStartedWritingResponse_ = false;
        bool aborted_ = false;
        bool erroredUnrecoverably_ = false;
        bool timedOutUnrecoverably_ = false;

        uint32_t lastActivityMillis_;
        uint32_t startMillis_;
        uint32_t loopCount_;

        uint8_t _requestBuffer[HttpServerAdvanced::REQUEST_BUFFER_SIZE];
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


    public:
        HttpPipeline(std::unique_ptr<HttpServerAdvanced::IClient> client, HttpServerBase &server,
                     const HttpTimeouts &timeouts, PipelineHandlerPtr parserEventHandler);

        ~HttpPipeline() = default;

        bool isFinished() const;
        bool shouldKeepAlive() const;
        void setResponseStream(std::unique_ptr<Stream> responseStream);
        PipelineHandleClientResult handleClient();
        void abort();
        void abortReadingRequest();
        void abortWritingResponse();
        uint32_t startedAt() const;
        HttpServerAdvanced::IClient &client();
    };

} // namespace HttpServerAdvanced
