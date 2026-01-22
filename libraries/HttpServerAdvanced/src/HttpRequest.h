#pragma once

#include "./HttpHeader.h"
#include "./HttpServerBase.h"
#include "./HttpResponse.h"
#include "./HttpRequestPhase.h"
#include "./IHttpHandler.h"
#include "./PipelineError.h"
#include "./UriView.h"

namespace HttpServerAdvanced
{
    class HttpRequest : private HttpServerAdvanced::IPipelineHandler
    {
    public:
        static constexpr const char *HandlerFactoryServiceName = "HttpRequestHandlerFactory";
        using HandlerFactoryFunction = std::function<std::unique_ptr<IHttpHandler>(HttpRequest &)>;

    private:
        std::unique_ptr<IHttpHandler> handler_;
        std::unique_ptr<IHttpResponse> response_;
        bool haveSentResponse_ = false;
        HttpServerAdvanced::HttpServerBase &server_;
        size_t bodyBytesReceived_ = 0;
        HttpRequestPhaseFlags completedPhases_ = 0;
        std::function<void(std::unique_ptr<Stream>)> onStreamReady_;
        mutable std::map<String, std::any> items_;

        // Merged from HttpRequest
        const char *method_;
        String version_;
        String url_;
        HttpHeadersCollection headers_;
        IPAddress remoteIP_;
        uint16_t remotePort_;
        IPAddress localIP_;
        uint16_t localPort_;

        IHttpHandler *tryGetHandler()
        {
            if (!handler_)
            {
                HandlerFactoryFunction *factoryPtr = server_.getService<HttpRequest::HandlerFactoryFunction>(HttpRequest::HandlerFactoryServiceName);
                if (!factoryPtr)
                {
                    return nullptr;
                }
                handler_ = (*factoryPtr)(*this);
            }
            return handler_.get();
        }
        void appendBodyContents(const uint8_t *at, std::size_t length)
        {

            auto handler = tryGetHandler();
            if (bodyBytesReceived_ == 0)
            {
                completedPhases_ |= HttpRequestPhase::BeginReadingBody;
                handleStep();
            }
            if (handler)
            {
                handler->handleBodyChunk(*this, at, length);
            }

            bodyBytesReceived_ += length;
            handleStep();
        }
        void completedWritingResponse()
        {
            completedPhases_ |= HttpRequestPhase::CompletedWritingResponse;
            handleStep();
        }

        void startedWritingResponse()
        {
            completedPhases_ |= HttpRequestPhase::WritingResponseStarted;
            handleStep();
        }

        void completedReadingMessage()
        {
            completedPhases_ |= HttpRequestPhase::CompletedReadingMessage;
            handleStep();
        }
        void completedReadingHeaders()
        {
            completedPhases_ |= HttpRequestPhase::CompletedReadingHeaders;
            handleStep();
        }
        void completedStartingLine()
        {

            completedPhases_ |= HttpRequestPhase::CompletedStartingLine;
            handleStep();
        }

        void handleStep()
        {
            auto handler = tryGetHandler();
            if (handler)
            {
                auto newResponse = handler->handleStep(*this);
                if (newResponse && !haveSentResponse_)
                {
                    response_ = std::move(newResponse);
                    sendResponse();
                }
            }
        }

        void sendResponse()
        {
            haveSentResponse_ = true;
            bool hasResponse = (response_ != nullptr);

            if (!response_)
            {
                response_ = HttpResponse::create(HttpStatus::InternalServerError(), String("Internal Server Error: No response generated"));
            }
            completedPhases_ |= HttpRequestPhase::WritingResponseStarted;
            onStreamReady_(CreateResponseStream(std::move(response_)));
        }
        virtual int onMessageBegin(const char *method, uint16_t versionMajor, uint16_t versionMinor, String &&url) override
        {
            method_ = method;
            version_ = String(versionMajor) + "." + String(versionMinor);
            url_ = std::move(url);
            completedStartingLine();
            return 0;
        }
        virtual void setIPAddress(IPAddress remoteIP, uint16_t remotePort, IPAddress localIP, uint16_t localPort) override
        {
            remoteIP_ = remoteIP;
            remotePort_ = remotePort;
            localIP_ = localIP;
            localPort_ = localPort;
        }

        // virtual int onRequestHeaderField(const uint8_t *at, std::size_t length) override;
        // virtual int onRequestHeaderValue(const uint8_t *at, std::size_t length) override;
        virtual int onHeader(String &&field, String &&value) override
        {
            headers_.set(HttpHeader(std::move(field), std::move(value)));
            return 0;
        }
        virtual int onHeadersComplete() override
        {
            completedReadingHeaders();
            return 0;
        }
        virtual int onBody(const uint8_t *at, std::size_t length) override
        {
            appendBodyContents(at, length);
            return 0;
        }
        virtual int onMessageComplete() override
        {
            completedReadingMessage();
            return 0;
        }
        virtual void onError(HttpServerAdvanced::PipelineError error) override
        {
            if (!haveSentResponse_)
            {
                HttpStatus status;
                String message;

                switch (error.code())
                {
                case HttpServerAdvanced::PipelineErrorCode::InvalidVersion:
                case HttpServerAdvanced::PipelineErrorCode::InvalidMethod:
                    status = HttpStatus::BadRequest();
                    message = "Bad Request: ";
                    break;
                case HttpServerAdvanced::PipelineErrorCode::UriTooLong:
                case HttpServerAdvanced::PipelineErrorCode::HeaderTooLarge:
                case HttpServerAdvanced::PipelineErrorCode::BodyTooLarge:
                    status = HttpStatus::PayloadTooLarge();
                    message = "Payload Too Large: ";
                    break;
                case HttpServerAdvanced::PipelineErrorCode::Timeout:
                    status = HttpStatus::RequestTimeout();
                    message = "Request Timeout: ";
                    break;
                case HttpServerAdvanced::PipelineErrorCode::UnsupportedMediaType:
                    status = HttpStatus::UnsupportedMediaType();
                    message = "Unsupported Media Type: ";
                    break;
                default:
                    status = HttpStatus::BadRequest();
                    message = "Bad Request: ";
                    break;
                }

                response_ = HttpResponse::create(status, message + String(error.message()));
                sendResponse();
            }
        }
        virtual void onClientDisconnected() override
        {
        }
        virtual void onResponseStarted() override
        {
            startedWritingResponse();
        }
        virtual void onResponseCompleted() override
        {
            completedWritingResponse();
        }
        void setResponseStreamCallback(std::function<void(std::unique_ptr<Stream>)> onStreamReady) override
        {
            onStreamReady_ = onStreamReady;
        }
        HttpRequest(HttpServerAdvanced::HttpServerBase &server)
            : server_(server), handler_(nullptr), completedPhases_(0),
              method_(nullptr), version_(), url_(), headers_(),
              remoteIP_(), remotePort_(0), localIP_(), localPort_(0) {}
        mutable std::unique_ptr<UriView> cachedUriView_;

    public:
        ~HttpRequest() = default;

        HttpServerAdvanced::HttpServerBase &server() { return server_; }
        HttpRequestPhaseFlags completedPhases() const { return completedPhases_; }

        // Merged accessor methods from HttpRequest
        const String &version() const { return version_; }
        const char *method() const { return method_; }
        const String &url() const { return url_; }
        const HttpHeadersCollection &headers() const { return headers_; }
        IPAddress remoteIP() { return remoteIP_; }
        uint16_t remotePort() { return remotePort_; }
        IPAddress localIP() { return localIP_; }
        uint16_t localPort() { return localPort_; }
        std::map<String, std::any> &items() const;

        UriView &uriView() const
        {
            if (!cachedUriView_)
            {
                cachedUriView_ = std::make_unique<UriView>(url_);
            }
            return *cachedUriView_;
        }
        // void sendResponse(std::unique_ptr<IHttpResponse> response);

    public:
        // Factory that can create an IPipelineHandler owning a HttpRequest instance.
        // Uses a custom deleter so it's safe even when IPipelineHandler doesn't have a virtual dtor.
        static std::unique_ptr<
            HttpServerAdvanced::IPipelineHandler,
            std::function<void(HttpServerAdvanced::IPipelineHandler *)>>
        createPipelineHandler(HttpServerAdvanced::HttpServerBase &server)
        {
            return std::unique_ptr<
                HttpServerAdvanced::IPipelineHandler,
                std::function<void(HttpServerAdvanced::IPipelineHandler *)>>(
                new HttpRequest(server),
                [](HttpServerAdvanced::IPipelineHandler *p)
                { delete static_cast<HttpRequest *>(p); });
            ;
        }
    };

}