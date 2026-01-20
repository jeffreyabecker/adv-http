#include "./HttpContext.h"
#include "../util/Util.h"
#include "./HttpHandler.h"
#include "./HttpResponseFilters.h"
namespace HttpServerAdvanced::Core
{
    using namespace HttpServerAdvanced::Pipeline;
    // HttpContext implementations
    IHttpHandler *HttpContext::tryGetHandler()
    {
        if (!contextHandler_)
        {
            HttpHandlerFactory *factory = pipeline_.server().getService<HttpHandlerFactory>(HttpHandlerFactory::ServiceName);
            if (!factory)
            {
                return nullptr;
            }
            contextHandler_ = factory->createContextHandler(*this);
        }
        return contextHandler_.get();
    }

    void HttpContext::appendBodyContents(const uint8_t *at, std::size_t length)
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

    void HttpContext::completedWritingResponse()
    {
        completedPhases_ |= HttpContextPhase::CompletedWritingResponse;
        handleStep();
    }

    void HttpContext::startedWritingResponse()
    {
        completedPhases_ |= HttpContextPhase::WritingResponseStarted;
        handleStep();
    }

    void HttpContext::completedReadingMessage()
    {
        completedPhases_ |= HttpContextPhase::CompletedReadingMessage;
        handleStep();
    }

    void HttpContext::completedReadingHeaders()
    {
        completedPhases_ |= HttpContextPhase::CompletedReadingHeaders;
        handleStep();
    }

    void HttpContext::completedStartingLine()
    {
        request_.uriView_ = UriView(request_.url());
        completedPhases_ |= HttpContextPhase::CompletedStartingLine;
        handleStep();
    }

    HttpServerAdvanced::Pipeline::HttpServerBase &HttpContext::server() { return pipeline_.server(); }
    HttpServerAdvanced::Pipeline::HttpPipeline &HttpContext::pipeline() { return pipeline_; }
    HttpRequest &HttpContext::request() { return request_; }

    void HttpContext::sendResponse()
    {
        haveSentResponse_ = true;
        bool hasResponse = (response_ != nullptr);

        auto filters = pipeline_.server().getService<HttpResponseFilters>(HttpResponseFilters::ServiceName);
        if (filters)
        {
            response_ = filters->apply(std::move(response_));
        }
        if (!response_)
        {
            response_ = HttpResponse::create(HttpStatus::InternalServerError(), String("Internal Server Error: No response generated"));
        }
        completedPhases_ += HttpContextPhase::WritingResponseStarted;
        pipeline_.setResponseStream(CreateResponseStream(std::move(response_)));
    }

    // HttpContextPipelineHandler implementations
    HttpContextPipelineHandler::HttpContextPipelineHandler(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline)
        : pipeline_(pipeline), context_(pipeline)
    {
    }

    HttpContextPipelineHandler::~HttpContextPipelineHandler()
    {
    }

    void HttpContextPipelineHandler::onInitialized(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline)
    {
        // Default implementation: do nothing
    }

    void HttpContextPipelineHandler::onRequestMessageBegin(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline)
    {
        context_.request_.url_ = String();
        currentHeaderField_ = String();
        currentHeaderValue_ = String();
        requestState_ = PipelineState::Begin;
        context_.request_.version_ = String(pipeline.versionMajor()) + "." +
                                     String(pipeline.versionMinor());
        context_.request_.method_ = pipeline.method();
        context_.request_.remoteIP_ = pipeline.client().remoteIP();
        context_.request_.remotePort_ = pipeline.client().remotePort();
        context_.request_.localIP_ = pipeline.client().localIP();
        context_.request_.localPort_ = pipeline.client().localPort();

        // Default implementation: do nothing
    }

    int HttpContextPipelineHandler::onRequestUrl(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline, const uint8_t *at, std::size_t length)
    {
        context_.request_.url_ += String((const char *)at, length);
        // Default implementation: return 0 (success)
        return 0;
    }

    int HttpContextPipelineHandler::onRequestHeaderField(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline, const uint8_t *at, std::size_t length)
    {
        if (requestState_ == PipelineState::Url)
        {

            context_.completedStartingLine();
        }
        if (requestState_ == PipelineState::HeaderValue)
        {
            // Store previous header
            context_.request_.headers_.set(HttpHeader(currentHeaderField_, currentHeaderValue_));
            currentHeaderField_ = String();
            currentHeaderValue_ = String();
        }
        currentHeaderField_ += String((const char *)at, length);
        requestState_ = PipelineState::HeaderField;
        return 0;
    }

    int HttpContextPipelineHandler::onRequestHeaderValue(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline, const uint8_t *at, std::size_t length)
    {
        currentHeaderValue_ += String((const char *)at, length);
        requestState_ = PipelineState::HeaderValue;
        return 0;
    }

    int HttpContextPipelineHandler::onRequestHeadersComplete(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline)
    {
        // Store last header
        if (!currentHeaderField_.isEmpty())
        {
            context_.request_.headers_.set(HttpHeader(currentHeaderField_, currentHeaderValue_));
        }
        currentHeaderField_ = String();
        currentHeaderValue_ = String();
        requestState_ = PipelineState::HeadersComplete;
        context_.completedReadingHeaders();
        // Default implementation: return 0 (success)
        return 0;
    }

    int HttpContextPipelineHandler::onRequestBody(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline, const uint8_t *at, std::size_t length)
    {
        context_.appendBodyContents(at, length);
        requestState_ = PipelineState::Body;
        return 0;
    }

    int HttpContextPipelineHandler::onRequestMessageComplete(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline)
    {
        context_.completedReadingMessage();
        requestState_ = PipelineState::MessageComplete;
        return 0;
    }

    void HttpContextPipelineHandler::onRequestParserError(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline, HttpParserError error)
    {
        if (!context_.haveSentResponse_)
        {
            context_.response_ = HttpResponse::create(HttpStatus::BadRequest(), String("Bad Request: ") + String(error.message()));
            context_.sendResponse();
        }
    }

    void HttpContextPipelineHandler::onClientDisconnected(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline)
    {
        // Default implementation: do nothing
    }

    void HttpContextPipelineHandler::onResponseStarted(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline)
    {
        context_.startedWritingResponse();
    }

    void HttpContextPipelineHandler::onResponseCompleted(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline)
    {
        context_.completedWritingResponse();
    }

    // ContextHttpPipelineHandlerFactory implementations
    std::unique_ptr<IHttpPipelineHandler> ContextHttpPipelineHandlerFactory::createHandler(HttpServerAdvanced::Pipeline::HttpPipeline &pipeline)
    {
        return std::make_unique<HttpContextPipelineHandler>(pipeline);
    }
}