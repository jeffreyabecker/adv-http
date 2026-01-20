#pragma once
#include "HttpRequest.h"

#include "../pipeline/Pipeline.h"

#include "./HttpResponse.h"
#include "./HttpHandler.h"

namespace HTTPServer::Core
{
    using HttpContextPhaseFlags = uint8_t;
    namespace HttpContextPhase
    {
        constexpr uint8_t CompletedStartingLine = 1 << 0;
        constexpr uint8_t CompletedReadingHeaders = 1 << 1;
        constexpr uint8_t BeginReadingBody = 1 << 2;
        constexpr uint8_t CompletedReadingMessage = 1 << 3;
        constexpr uint8_t WritingResponseStarted = 1 << 4;
        constexpr uint8_t CompletedWritingResponse = 1 << 5;
    }

    class HttpContext
    {
        friend class HttpContextPipelineHandler;

    private:
        std::shared_ptr<IHttpHandler> contextHandler_;
        HttpRequest request_;
        std::unique_ptr<IHttpResponse> response_;
        bool haveSentResponse_ = false;
        HTTPServer::Pipeline::HttpPipeline &pipeline_;
        size_t bodyBytesReceived_ = 0;

        HttpContextPhaseFlags completedPhases_ = 0;

        IHttpHandler *tryGetHandler();
        void appendBodyContents(const uint8_t *at, std::size_t length);
        void completedWritingResponse();
        void startedWritingResponse();
        void completedReadingMessage();
        void completedReadingHeaders();
        void completedStartingLine();


        void handleStep(){
            auto handler = tryGetHandler();
            if (handler)
            {
                auto newResponse = handler->handleStep(*this);
                if(newResponse && !haveSentResponse_){
                    response_ = std::move(newResponse);
                    sendResponse();
                }
            }
        }

        void sendResponse();

    public:
        HttpContext(HTTPServer::Pipeline::HttpPipeline &pipeline) : pipeline_(pipeline), request_(*this), contextHandler_(nullptr), completedPhases_(0) {}
        ~HttpContext() = default;

        HTTPServer::Pipeline::HttpServerBase &server();
        HTTPServer::Pipeline::HttpPipeline &pipeline();
        HttpRequest &request();
        HttpContextPhaseFlags completedPhases() const { return completedPhases_; }

        // void sendResponse(std::unique_ptr<IHttpResponse> response);

        // template <typename T = HttpResponse, typename... Args>
        // void sendResponse(Args &&...args)
        // {
        //     static_assert(std::is_convertible<T, IHttpResponse>::value, "T must be derived from IHttpResponse");
        //     sendResponse(std::make_unique<T>(std::forward<Args>(args)...));
        // }
    };

    class HttpContextPipelineHandler : public HTTPServer::Pipeline::IHttpPipelineHandler
    {
    private:
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
        HTTPServer::Pipeline::HttpPipeline &pipeline_;

        String currentHeaderField_;
        String currentHeaderValue_;
        PipelineState requestState_ = PipelineState::Begin;
        HttpContext context_;

    public:
        HttpContextPipelineHandler(HTTPServer::Pipeline::HttpPipeline &pipeline)
            : pipeline_(pipeline), context_(pipeline)
        {
        }
        ~HttpContextPipelineHandler()
        {
        }

        virtual void onInitialized(HTTPServer::Pipeline::HttpPipeline &pipeline) override;
        virtual void onRequestMessageBegin(HTTPServer::Pipeline::HttpPipeline &pipeline) override;
        virtual int onRequestUrl(HTTPServer::Pipeline::HttpPipeline &pipeline, const uint8_t *at, std::size_t length) override;
        virtual int onRequestHeaderField(HTTPServer::Pipeline::HttpPipeline &pipeline, const uint8_t *at, std::size_t length) override;
        virtual int onRequestHeaderValue(HTTPServer::Pipeline::HttpPipeline &pipeline, const uint8_t *at, std::size_t length) override;
        virtual int onRequestHeadersComplete(HTTPServer::Pipeline::HttpPipeline &pipeline) override;
        virtual int onRequestBody(HTTPServer::Pipeline::HttpPipeline &pipeline, const uint8_t *at, std::size_t length) override;
        virtual int onRequestMessageComplete(HTTPServer::Pipeline::HttpPipeline &pipeline) override;
        virtual void onRequestParserError(HTTPServer::Pipeline::HttpPipeline &pipeline, HTTPServer::Pipeline::HttpParserError error) override;
        virtual void onClientDisconnected(HTTPServer::Pipeline::HttpPipeline &pipeline) override;
        virtual void onResponseStarted(HTTPServer::Pipeline::HttpPipeline &pipeline) override;
        virtual void onResponseCompleted(HTTPServer::Pipeline::HttpPipeline &pipeline) override;
    };
    class ContextHttpPipelineHandlerFactory : public HTTPServer::Pipeline::IHttpPipelineHandlerFactory
    {
    public:
        virtual std::unique_ptr<HTTPServer::Pipeline::IHttpPipelineHandler> createHandler(HTTPServer::Pipeline::HttpPipeline &pipeline);
    };
}