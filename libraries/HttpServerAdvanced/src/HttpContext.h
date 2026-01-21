#pragma once
#include "./HttpRequest.h"

#include "./HttpServerBase.h"
#include "./HttpHandlerFactory.h"
#include "./HttpResponse.h"
#include "./HttpContextPhase.h"
#include "./IHttpHandler.h"

namespace HttpServerAdvanced
{
    class HttpContext : private HttpServerAdvanced::IPipelineHandler
    {

    private:
        std::shared_ptr<IHttpHandler> handler_;
        HttpRequest request_;
        std::unique_ptr<IHttpResponse> response_;
        bool haveSentResponse_ = false;
        HttpServerAdvanced::HttpServerBase &server_;
        size_t bodyBytesReceived_ = 0;
        HttpContextPhaseFlags completedPhases_ = 0;
        std::function<void(std::unique_ptr<Stream>)> onStreamReady_;

        IHttpHandler *tryGetHandler()
        {
            if (!handler_)
            {
                HttpHandlerFactory *factory = server_.getService<HttpHandlerFactory>(HttpHandlerFactory::ServiceName);
                if (!factory)
                {
                    return nullptr;
                }
                handler_ = factory->createContextHandler(*this);
            }
            return handler_.get();
        }
        void appendBodyContents(const uint8_t *at, std::size_t length)
        {

            auto handler = tryGetHandler();
            if (bodyBytesReceived_ == 0)
            {
                completedPhases_ |= HttpContextPhase::BeginReadingBody;
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
            completedPhases_ |= HttpContextPhase::CompletedWritingResponse;
            handleStep();
        }

        void startedWritingResponse()
        {
            completedPhases_ |= HttpContextPhase::WritingResponseStarted;
            handleStep();
        }

        void completedReadingMessage()
        {
            completedPhases_ |= HttpContextPhase::CompletedReadingMessage;
            handleStep();
        }
        void completedReadingHeaders()
        {
            completedPhases_ |= HttpContextPhase::CompletedReadingHeaders;
            handleStep();
        }
        void completedStartingLine()
        {
            
            completedPhases_ |= HttpContextPhase::CompletedStartingLine;
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
            completedPhases_ += HttpContextPhase::WritingResponseStarted;
            onStreamReady_(CreateResponseStream(std::move(response_)));
        }
        virtual int onMessageBegin(const char *method, uint16_t versionMajor, uint16_t versionMinor, String &&url) override
        {
            request_.method_ = method;
            request_.version_ = String(versionMajor) + "." + String(versionMinor);
            request_.url_ = std::move(url);
            completedStartingLine();
            return 0;
        }
        virtual void setIPAddress(IPAddress remoteIP, uint16_t remotePort, IPAddress localIP, uint16_t localPort) override
        {
            request_.remoteIP_ = remoteIP;
            request_.remotePort_ = remotePort;
            request_.localIP_ = localIP;
            request_.localPort_ = localPort;
        }

        // virtual int onRequestHeaderField(const uint8_t *at, std::size_t length) override;
        // virtual int onRequestHeaderValue(const uint8_t *at, std::size_t length) override;
        virtual int onHeader(String &&field, String &&value) override
        {
            request_.headers_.set(HttpHeader(std::move(field), std::move(value)));

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
        virtual void onError(HttpServerAdvanced::PipelineParserError error) override
        {
            if (!haveSentResponse_)
            {
                response_ = HttpResponse::create(HttpStatus::BadRequest(), String("Bad Request: ") + String(error.message()));
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
        HttpContext(HttpServerAdvanced::HttpServerBase &server) : server_(server), request_(*this), handler_(nullptr), completedPhases_(0) {}

    public:
        ~HttpContext() = default;

        HttpServerAdvanced::HttpServerBase &server() { return server_; }
        HttpRequest &request() { return request_; }
        HttpContextPhaseFlags completedPhases() const { return completedPhases_; }

        // void sendResponse(std::unique_ptr<IHttpResponse> response);

    public:
        // Factory that can create an IPipelineHandler owning a HttpContext instance.
        // Uses a custom deleter so it's safe even when IPipelineHandler doesn't have a virtual dtor.
        static std::unique_ptr<
            HttpServerAdvanced::IPipelineHandler,
            std::function<void(HttpServerAdvanced::IPipelineHandler *)>>
        createPipelineHandler(HttpServerAdvanced::HttpServerBase &server)
        {
            return std::unique_ptr<
                HttpServerAdvanced::IPipelineHandler,
                std::function<void(HttpServerAdvanced::IPipelineHandler *)>>(
                new HttpContext(server),
                [](HttpServerAdvanced::IPipelineHandler *p)
                { delete static_cast<HttpContext *>(p); });
        }
    };

}