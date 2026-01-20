#pragma once
#include "HttpRequest.h"

#include "../pipeline/Pipeline.h"

#include "./HttpResponse.h"
#include "./HttpHandler.h"

namespace HttpServerAdvanced::Core
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
        HttpServerAdvanced::Pipeline::HttpPipeline &pipeline_;
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
        HttpContext(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline) : pipeline_(pipeline), request_(*this), contextHandler_(nullptr), completedPhases_(0) {}
        ~HttpContext() = default;

        HttpServerAdvanced::Pipeline::HttpServerBase &server();
        HttpServerAdvanced::Pipeline::HttpPipeline &pipeline();
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

    class HttpContextPipelineHandler : public HttpServerAdvanced::Pipeline::IHttpPipelineHandler
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
        HttpServerAdvanced::Pipeline::HttpPipeline &pipeline_;

        String currentHeaderField_;
        String currentHeaderValue_;
        PipelineState requestState_ = PipelineState::Begin;
        HttpContext context_;

    public:
        HttpContextPipelineHandler(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline)
            : pipeline_(pipeline), context_(pipeline)
        {
        }
        ~HttpContextPipelineHandler()
        {
        }

        virtual void onInitialized(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline) override;
        virtual void onRequestMessageBegin(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline) override;
        virtual int onRequestUrl(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline, const uint8_t *at, std::size_t length) override;
        virtual int onRequestHeaderField(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline, const uint8_t *at, std::size_t length) override;
        virtual int onRequestHeaderValue(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline, const uint8_t *at, std::size_t length) override;
        virtual int onRequestHeadersComplete(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline) override;
        virtual int onRequestBody(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline, const uint8_t *at, std::size_t length) override;
        virtual int onRequestMessageComplete(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline) override;
        virtual void onRequestParserError(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline, HttpServerAdvanced::Pipeline::HttpParserError error) override;
        virtual void onClientDisconnected(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline) override;
        virtual void onResponseStarted(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline) override;
        virtual void onResponseCompleted(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline) override;
    };
    class ContextHttpPipelineHandlerFactory : public HttpServerAdvanced::Pipeline::IHttpPipelineHandlerFactory
    {
    public:
        virtual std::unique_ptr<HttpServerAdvanced::Pipeline::IHttpPipelineHandler> createHandler(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline);
    };
}