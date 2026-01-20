#pragma once
#include <memory>
#include <functional>
#include "../util/Util.h"
#include "./HttpPipelineTimeouts.h"
#include <http_parser.h>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <algorithm>

namespace HTTPServer::Pipeline
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

    enum class HttpRequestParserEvent
    {
        None,
        MessageBegin,
        Url,
        HeaderField,
        HeaderValue,
        HeadersComplete,
        Body,
        MessageComplete,
        Error
    };

    class HttpPipeline; // Forward declaration

    class IHttpPipelineHandler
    {
    public:
        virtual ~IHttpPipelineHandler() = default;
        virtual void onInitialized(HttpPipeline &pipeline) = 0;
        virtual void onRequestMessageBegin(HttpPipeline &pipeline) = 0;
        virtual int onRequestUrl(HttpPipeline &pipeline, const uint8_t *at, std::size_t length) = 0;
        virtual int onRequestHeaderField(HttpPipeline &pipeline, const uint8_t *at, std::size_t length) = 0;
        virtual int onRequestHeaderValue(HttpPipeline &pipeline, const uint8_t *at, std::size_t length) = 0;
        virtual int onRequestHeadersComplete(HttpPipeline &pipeline) = 0;
        virtual int onRequestBody(HttpPipeline &pipeline, const uint8_t *at, std::size_t length) = 0;
        virtual int onRequestMessageComplete(HttpPipeline &pipeline) = 0;
        virtual void onRequestParserError(HttpPipeline &pipeline, HttpParserError error) = 0;
        virtual void onResponseStarted(HttpPipeline &pipeline) = 0;
        virtual void onResponseCompleted(HttpPipeline &pipeline) = 0;
        virtual void onClientDisconnected(HttpPipeline &pipeline) = 0;
    };

    enum class HttpPipelineHandleClientResult
    {
        Idle,
        Processing,
        Completed,
        ClientDisconnected,
        NoPipelineHandlerPresent,
        ErroredUnrecoverably,
        TimedOutUnrecoverably,
        Aborted
    };

    bool isHttpPipelineHandleClientResultFinal(HttpPipelineHandleClientResult result);

    class HttpPipeline
    {
    private:
        // HTTP parsing members
        http_parser parser_;
        http_parser_settings settings_;
        HttpRequestParserEvent currentEvent_ = HttpRequestParserEvent::None;

        // Pipeline members
        std::unique_ptr<HTTPServer::IClient> client_;
        HttpServerBase &server_;
        std::unique_ptr<Stream> responseStream_;
        bool completedRequestRead_ = false;
        bool completedResponseWrite_ = false;
        bool haveStartedWritingResponse_ = false;
        bool aborted_ = false;
        bool erroredUnrecoverably_ = false;
        bool timedOutUnrecoverably_ = false;
        std::unique_ptr<IHttpPipelineHandler> handler_;
        uint32_t lastActivityMillis_;
        uint32_t startMillis_;
        uint32_t loopCount_;

        uint8_t _requestBuffer[HTTPServer::REQUEST_BUFFER_SIZE];
        HttpPipelineTimeouts timeouts_;

        void readFromClientIntoParser();
        void writeResponseToClientFromStream();
        void setupPipeline();
        HttpPipelineHandleClientResult _checkStateInHandleClient();
        bool isFirstLoop() const;
        void startActivity();
        bool checkActivityTimeout();
        void setErroredUnrecoverably();
        void markRequestReadCompleted();
        void markResponseWriteCompleted();

        // HTTP parsing event handlers
        int onMessageBegin();
        int onUrl(const uint8_t *at, std::size_t length);
        int onHeaderField(const uint8_t *at, std::size_t length);
        int onHeaderValue(const uint8_t *at, std::size_t length);
        int onHeadersComplete();
        int onBody(const uint8_t *at, std::size_t length);
        int onMessageComplete();
        void onError(HttpParserError error);

        // Static trampoline functions for http_parser_settings
        static int on_message_begin_cb(http_parser *parser);
        static int on_url_cb(http_parser *parser, const char *at, std::size_t length);
        static int on_header_field_cb(http_parser *parser, const char *at, std::size_t length);
        static int on_header_value_cb(http_parser *parser, const char *at, std::size_t length);
        static int on_headers_complete_cb(http_parser *parser);
        static int on_body_cb(http_parser *parser, const char *at, std::size_t length);
        static int on_message_complete_cb(http_parser *parser);

    public:
        HttpPipeline(std::unique_ptr<HTTPServer::IClient> client, HttpServerBase &server);
        ~HttpPipeline();

        // HTTP parsing methods
        std::size_t execute(const uint8_t *data, std::size_t length);
        const char *method() const { return http_method_str(static_cast<http_method>(parser_.method)); }
        const short versionMajor() const { return parser_.http_major; }
        const short versionMinor() const { return parser_.http_minor; }
        bool isFinished() const;
        bool shouldKeepAlive() const;
        HttpRequestParserEvent currentEvent() const { return currentEvent_; }

        // Pipeline methods
        void setHandler(std::unique_ptr<IHttpPipelineHandler> handler);
        void setResponseStream(std::unique_ptr<ReadStream> responseStream);
        HttpPipelineHandleClientResult handleClient();
        void abort();
        void abortReadingRequest();
        void abortWritingResponse();
        uint32_t startedAt() const;
        HTTPServer::IClient &client();
        HttpPipelineTimeouts &timeouts();
        void setTimeouts(const HttpPipelineTimeouts &timeouts);
        HttpServerBase &server();
    };

} // namespace HTTPServer
