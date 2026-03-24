#pragma once

#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "../server/HttpServerBase.h"
#include "../response/HttpResponse.h"
#include "../core/HttpRequestPhase.h"
#include "../handlers/IHttpHandler.h"
#include "../pipeline/PipelineError.h"
#include "../pipeline/RequestHandlingResult.h"
#include "../streams/ByteStream.h"
#include "../util/UriView.h"
#include "IHttpRequestHandlerFactory.h"

#include <any>
#include <map>
#include <string>
#include <string_view>

namespace HttpServerAdvanced
{   

    class HttpRequest : private HttpServerAdvanced::IPipelineHandler
    {
    public:


    private:
        std::unique_ptr<IHttpHandler> handler_;
        RequestHandlingResult pendingResult_;
        HttpServerAdvanced::HttpServerBase &server_;
        size_t bodyBytesReceived_ = 0;
        HttpRequestPhaseFlags completedPhases_ = 0;
        mutable std::map<std::string, std::any> items_;

        // Merged from HttpRequest
        std::string method_;
        std::string version_;
        std::string url_;
        HttpHeaderCollection headers_;
        std::string remoteAddress_;
        uint16_t remotePort_;
        std::string localAddress_;
        uint16_t localPort_;
        IHttpRequestHandlerFactory& handlerFactory_;

        void invalidateTextCaches()
        {
            cachedUriView_.reset();
        }

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
        void setPendingResult(RequestHandlingResult result)
        {
            pendingResult_ = std::move(result);
        }
        virtual int onMessageBegin(const char *method, uint16_t versionMajor, uint16_t versionMinor, std::string_view url) override
        {
            method_ = method != nullptr ? method : "";
            version_ = std::to_string(versionMajor) + "." + std::to_string(versionMinor);
            url_.assign(url.data(), url.size());
            invalidateTextCaches();
            completedStartingLine();
            return 0;
        }
        virtual void setAddresses(std::string_view remoteAddress, uint16_t remotePort, std::string_view localAddress, uint16_t localPort) override
        {
            remoteAddress_.assign(remoteAddress.data(), remoteAddress.size());
            remotePort_ = remotePort;
            localAddress_.assign(localAddress.data(), localAddress.size());
            localPort_ = localPort;
        }

        // virtual int onRequestHeaderField(const uint8_t *at, std::size_t length) override;
        // virtual int onRequestHeaderValue(const uint8_t *at, std::size_t length) override;
        virtual int onHeader(std::string_view field, std::string_view value) override
        {
            headers_.set(HttpHeader(field, value));
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
        HttpRequest(HttpServerAdvanced::HttpServerBase &server, IHttpRequestHandlerFactory& handlerFactory)
            : server_(server), handlerFactory_(handlerFactory), handler_(nullptr), completedPhases_(0),
              method_(), version_(), url_(), headers_(),
              remoteAddress_(), remotePort_(0), localAddress_(), localPort_(0) {}
        mutable std::unique_ptr<UriView> cachedUriView_;

    public:
        ~HttpRequest() = default;

        inline HttpServerAdvanced::HttpServerBase &server() { return server_; }
        inline HttpRequestPhaseFlags completedPhases() const { return completedPhases_; }

        // Merged accessor methods from HttpRequest
        inline std::string_view version() const { return versionView(); }
        inline std::string_view versionView() const { return std::string_view(version_.data(), version_.size()); }
        inline const char *method() const { return method_.c_str(); }
        inline std::string_view methodView() const { return std::string_view(method_.data(), method_.size()); }
        inline std::string_view url() const { return urlView(); }
        inline std::string_view urlView() const { return std::string_view(url_.data(), url_.size()); }
        inline const HttpHeaderCollection &headers() const { return headers_; }
        inline std::string_view remoteAddress() const { return std::string_view(remoteAddress_.data(), remoteAddress_.size()); }
        inline uint16_t remotePort() { return remotePort_; }
        inline std::string_view localAddress() const { return std::string_view(localAddress_.data(), localAddress_.size()); }
        inline uint16_t localPort() { return localPort_; }
        inline std::map<std::string, std::any> &items() const{
            return items_;
        }
        inline std::unique_ptr<IHttpResponse> createResponse(HttpStatus status, std::string body)
        {
            return handlerFactory_.createResponse(status, std::move(body));
        }

        UriView &uriView() const
        {
            if (!cachedUriView_)
            {
                cachedUriView_ = std::make_unique<UriView>(url_);
            }
            return *cachedUriView_;
        }
        bool hasPendingResult() const override
        {
            return pendingResult_.hasValue();
        }
        RequestHandlingResult takeResult() override
        {
            RequestHandlingResult result = std::move(pendingResult_);
            pendingResult_ = RequestHandlingResult();
            return result;
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




