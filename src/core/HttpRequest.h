#pragma once

#include "../compat/IpAddress.h"
#include "../compat/Stream.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "../server/HttpServerBase.h"
#include "../response/HttpResponse.h"
#include "../core/HttpRequestPhase.h"
#include "../handlers/IHttpHandler.h"
#include "../pipeline/PipelineError.h"
#include "../util/UriView.h"
#include "IHttpRequestHandlerFactory.h"

namespace HttpServerAdvanced
{   

    class HttpRequest : private HttpServerAdvanced::IPipelineHandler
    {
    public:


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
        HttpHeaderCollection headers_;
        IPAddress remoteIP_;
        uint16_t remotePort_;
        IPAddress localIP_;
        uint16_t localPort_;
        IHttpRequestHandlerFactory& handlerFactory_;

        IHttpHandler *tryGetHandler()
        {
            if (!handler_)
            {
                handler_ = handlerFactory_.create(*this);
            }
            return handler_.get();
        }
        void appendBodyContents(const uint8_t *at, std::size_t length);
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

        void handleStep();

        void sendResponse();
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
        virtual void onError(HttpServerAdvanced::PipelineError error) override;
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
        HttpRequest(HttpServerAdvanced::HttpServerBase &server, IHttpRequestHandlerFactory& handlerFactory)
            : server_(server), handlerFactory_(handlerFactory), handler_(nullptr), completedPhases_(0),
              method_(nullptr), version_(), url_(), headers_(),
              remoteIP_(), remotePort_(0), localIP_(), localPort_(0) {}
        mutable std::unique_ptr<UriView> cachedUriView_;

    public:
        ~HttpRequest() = default;

        inline HttpServerAdvanced::HttpServerBase &server() { return server_; }
        inline HttpRequestPhaseFlags completedPhases() const { return completedPhases_; }

        // Merged accessor methods from HttpRequest
        inline const String &version() const { return version_; }
        inline const char *method() const { return method_; }
        inline const String &url() const { return url_; }
        inline const HttpHeaderCollection &headers() const { return headers_; }
        inline IPAddress remoteIP() { return remoteIP_; }
        inline uint16_t remotePort() { return remotePort_; }
        inline IPAddress localIP() { return localIP_; }
        inline uint16_t localPort() { return localPort_; }
        inline std::map<String, std::any> &items() const{
            return items_;
        }

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
        createPipelineHandler(HttpServerAdvanced::HttpServerBase &server, IHttpRequestHandlerFactory& handlerFactory)
        {

            return std::unique_ptr<
                HttpServerAdvanced::IPipelineHandler,
                std::function<void(HttpServerAdvanced::IPipelineHandler *)>>(
                new HttpRequest(server, handlerFactory),
                [](HttpServerAdvanced::IPipelineHandler *p)
                { delete static_cast<HttpRequest *>(p); });
            ;
        }
    };

}




