#include "./HttpPipeline.h"
#include "./HttpPipelineTimeouts.h"
#include "./HttpServerBase.h"
#include <cstring>

namespace HttpServerAdvanced::Pipeline
{
    // Helper to get HttpPipeline* from http_parser
    static HttpPipeline *get_instance(http_parser *parser)
    {
        return static_cast<HttpPipeline *>(parser->data);
    }

    // Static trampoline functions for http_parser_settings
    int HttpPipeline::on_message_begin_cb(http_parser *parser)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = HttpRequestParserEvent::MessageBegin;
            return self->onMessageBegin();
        }
        return 0;
    }

    int HttpPipeline::on_url_cb(http_parser *parser, const char *at, std::size_t length)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = HttpRequestParserEvent::Url;
            return self->onUrl(reinterpret_cast<const uint8_t *>(at), length);
        }
        return 0;
    }

    int HttpPipeline::on_header_field_cb(http_parser *parser, const char *at, std::size_t length)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = HttpRequestParserEvent::HeaderField;
            return self->onHeaderField(reinterpret_cast<const uint8_t *>(at), length);
        }
        return 0;
    }

    int HttpPipeline::on_header_value_cb(http_parser *parser, const char *at, std::size_t length)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = HttpRequestParserEvent::HeaderValue;
            return self->onHeaderValue(reinterpret_cast<const uint8_t *>(at), length);
        }
        return 0;
    }

    int HttpPipeline::on_headers_complete_cb(http_parser *parser)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = HttpRequestParserEvent::HeadersComplete;
            return self->onHeadersComplete();
        }
        return 0;
    }

    int HttpPipeline::on_body_cb(http_parser *parser, const char *at, std::size_t length)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = HttpRequestParserEvent::Body;
            return self->onBody(reinterpret_cast<const uint8_t *>(at), length);
        }
        return 0;
    }

    int HttpPipeline::on_message_complete_cb(http_parser *parser)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = HttpRequestParserEvent::MessageComplete;
            return self->onMessageComplete();
        }
        return 0;
    }

    // HttpPipeline HTTP parsing methods
    std::size_t HttpPipeline::execute(const uint8_t *data, std::size_t length)
    {
        int result = http_parser_execute(&parser_, &settings_, reinterpret_cast<const char *>(data), length);
        if (parser_.http_errno != HPE_OK)
        {
            currentEvent_ = HttpRequestParserEvent::Error;
            onError(HttpParserError(static_cast<enum http_errno>(parser_.http_errno)));
        }
        return result;
    }

    bool HttpPipeline::isFinished() const
    {
        return http_body_is_final(&parser_);
    }

    bool HttpPipeline::shouldKeepAlive() const
    {
        return http_should_keep_alive(&parser_);
    }

    bool isHttpPipelineHandleClientResultFinal(HttpPipelineHandleClientResult result)
    {
        switch (result)
        {
        case HttpPipelineHandleClientResult::Completed:
        case HttpPipelineHandleClientResult::ClientDisconnected:
        case HttpPipelineHandleClientResult::ErroredUnrecoverably:
        case HttpPipelineHandleClientResult::TimedOutUnrecoverably:
        case HttpPipelineHandleClientResult::Aborted:
        case HttpPipelineHandleClientResult::NoPipelineHandlerPresent:
            return true;
        case HttpPipelineHandleClientResult::Idle:
        case HttpPipelineHandleClientResult::Processing:
        default:
            return false;
        }
    }

    // HttpPipeline implementation
    HttpPipeline::HttpPipeline(std::unique_ptr<IClient> client, HttpServerBase &server)
        : client_(std::move(client)),
          server_(server),
          timeouts_(server_.timeouts()),
          responseStream_(nullptr),
          completedRequestRead_(false),
          completedResponseWrite_(false),
          lastActivityMillis_(0),
          loopCount_(0)
    {
        http_parser_settings_init(&settings_);
        settings_.on_message_begin = on_message_begin_cb;
        settings_.on_url = on_url_cb;
        settings_.on_header_field = on_header_field_cb;
        settings_.on_header_value = on_header_value_cb;
        settings_.on_headers_complete = on_headers_complete_cb;
        settings_.on_body = on_body_cb;
        settings_.on_message_complete = on_message_complete_cb;

        http_parser_init(&parser_, HTTP_REQUEST);
        parser_.data = this;
    }

    HttpPipeline::~HttpPipeline() = default;

    int HttpPipeline::onMessageBegin()
    {
        if (handler_)
        {
            handler_->onRequestMessageBegin(*this);
        }
        return 0;
    }

    int HttpPipeline::onUrl(const uint8_t *at, std::size_t length)
    {
        if (handler_)
        {
            return handler_->onRequestUrl(*this, at, length);
        }
        return 0;
    }

    int HttpPipeline::onHeaderField(const uint8_t *at, std::size_t length)
    {
        if (handler_)
        {
            return handler_->onRequestHeaderField(*this, at, length);
        }
        return 0;
    }

    int HttpPipeline::onHeaderValue(const uint8_t *at, std::size_t length)
    {
        if (handler_)
        {
            return handler_->onRequestHeaderValue(*this, at, length);
        }
        return 0;
    }

    int HttpPipeline::onHeadersComplete()
    {
        if (handler_)
        {
            return handler_->onRequestHeadersComplete(*this);
        }
        return 0;
    }

    int HttpPipeline::onBody(const uint8_t *at, std::size_t length)
    {
        if (handler_)
        {
            return handler_->onRequestBody(*this, at, length);
        }
        return 0;
    }

    int HttpPipeline::onMessageComplete()
    {
        completedRequestRead_ = true;
        if (handler_)
        {
            return handler_->onRequestMessageComplete(*this);
        }
        return 0;
    }

    void HttpPipeline::onError(HttpParserError error)
    {
        if (handler_)
        {
            handler_->onRequestParserError(*this, error);
        }
    }

    void HttpPipeline::setHandler(std::unique_ptr<IHttpPipelineHandler> handler)
    {
        if (handler_)
        {
            assert(false && "Handler already set");
        }
        handler_ = std::move(handler);
    }

    void HttpPipeline::setResponseStream(std::unique_ptr<ReadStream> responseStream)
    {
        if (haveStartedWritingResponse_)
        {
            assert(false && "Response writing already started");
        }
        responseStream_ = std::move(responseStream);
    }

    HttpPipelineHandleClientResult HttpPipeline::handleClient()
    {
        loopCount_++;

        if (isFirstLoop())
        {
            setupPipeline();
        }
        auto validationResult = _checkStateInHandleClient();
        if (isHttpPipelineHandleClientResultFinal(validationResult))
        {
            return validationResult;
        }
        readFromClientIntoParser();
        writeResponseToClientFromStream();
        return HttpPipelineHandleClientResult::Processing;
    }

    IClient &HttpPipeline::client()
    {
        return *client_;
    }

    void HttpPipeline::readFromClientIntoParser()
    {
        if (completedRequestRead_)
        {
            return;
        }
        startActivity();
        int available = 0;
        while ((available = client_->available()) >= 0)
        {

            std::size_t bytesRead = client_->read(_requestBuffer, sizeof(_requestBuffer));

            size_t bytesParsed = execute(_requestBuffer, bytesRead);
            if (bytesParsed < bytesRead)
            {
                setErroredUnrecoverably();
                return;
            }
            if (!checkActivityTimeout())
            {
                return;
            }
        }
        if (available == 0)
        {
            markRequestReadCompleted();
        }
    }

    void HttpPipeline::writeResponseToClientFromStream()
    {
        if (responseStream_ == nullptr || completedResponseWrite_)
        {
            return;
        }
        startActivity();
        // ClientContext.h uses a 256 byte buffer for copying streams so we will do the same here
        // see: Wifi/src/include/ClientContext.h:379
        uint8_t buff[256];
        int available = 0;
        while ((available = responseStream_->available()) > 0)
        {
            size_t bytesRead = 0;
            for (bytesRead = 0; bytesRead < sizeof(buff) && responseStream_->available(); bytesRead++)
            {
                int byte = responseStream_->read();
                if (byte == -1)
                {
                    break;
                }
                buff[bytesRead] = static_cast<uint8_t>(byte);
            }
            if (bytesRead > 0)
            {
                auto written = client_->write(buff, bytesRead);
                if (written < bytesRead)
                {

                    setErroredUnrecoverably();
                    return;
                }
            }
            else
            {
                break;
            }
            if (!checkActivityTimeout())
            {
                return;
            }
        }
        if (available <= 0)
        {
            markResponseWriteCompleted();
        }
    }

    void HttpPipeline::setupPipeline()
    {
        startMillis_ = millis();
        client_->setTimeout(timeouts_.getActivityTimeout());
    }
    void HttpPipeline::abort()
    {
        aborted_ = true;
        abortReadingRequest();
        abortWritingResponse();
    }
    void HttpPipeline::abortReadingRequest()
    {
        markRequestReadCompleted();
    }
    void HttpPipeline::abortWritingResponse()
    {
        markResponseWriteCompleted();
    }

    HttpPipelineHandleClientResult HttpPipeline::_checkStateInHandleClient()
    {
        uint32_t currentMillis = millis();
        if (!client_->connected())
        {
            if (handler_)
            {
                handler_->onClientDisconnected(*this);
            }
            return HttpPipelineHandleClientResult::ClientDisconnected;
        }
        if (!handler_)
        {
            return HttpPipelineHandleClientResult::NoPipelineHandlerPresent;
        }
        if (currentMillis - startMillis_ > timeouts_.getTotalRequestLengthMs())
        {
            timedOutUnrecoverably_ = true;
            return HttpPipelineHandleClientResult::TimedOutUnrecoverably;
        }
        if (aborted_)
        {
            return HttpPipelineHandleClientResult::Aborted;
        }
        if (erroredUnrecoverably_)
        {
            return HttpPipelineHandleClientResult::ErroredUnrecoverably;
        }
        if (completedRequestRead_ && completedResponseWrite_)
        {
            return HttpPipelineHandleClientResult::Completed;
        }
        return HttpPipelineHandleClientResult::Processing; // Default case
    }

    bool HttpPipeline::isFirstLoop() const
    {
        return loopCount_ == 0;
    }

    void HttpPipeline::startActivity()
    {
        lastActivityMillis_ = millis();
    }

    bool HttpPipeline::checkActivityTimeout()
    {
        uint32_t currentMillis = millis();
        if (currentMillis - lastActivityMillis_ > timeouts_.getActivityTimeout())
        {
            return false;
        }
        return true;
    }

    void HttpPipeline::setErroredUnrecoverably()
    {
        erroredUnrecoverably_ = true;
        markRequestReadCompleted();
        markResponseWriteCompleted();
    }

    void HttpPipeline::markRequestReadCompleted()
    {
        execute(nullptr, 0); // Signal end of input to parser
        completedRequestRead_ = true;
    }

    void HttpPipeline::markResponseWriteCompleted()
    {
        completedResponseWrite_ = true;
        responseStream_ = nullptr;
    }

    uint32_t HttpPipeline::startedAt() const
    {
        return startMillis_;
    }

} // namespace HttpServerAdvanced
