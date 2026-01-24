#include "./RequestParser.h"
#include "./IPipelineHandler.h"
#include "./PipelineError.h"
#include <cstring>

namespace HttpServerAdvanced
{
    // Static method implementations
    RequestParser *RequestParser::get_instance(http_parser *parser)
    {
        return static_cast<RequestParser *>(parser->data);
    }

    int RequestParser::on_message_begin_cb(http_parser *parser)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = RequestParserEvent::MessageBegin;
        }
        return 0;
    }

    int RequestParser::on_url_cb(http_parser *parser, const char *at, std::size_t length)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = RequestParserEvent::Url;
            // Buffer the URL chunk
            if (!self->appendToBuffer(at, length, HttpServerAdvanced::MAX_REQUEST_URI_LENGTH, self->urlPos_, self->urlLen_))
            {
                // Notify handler about the overflow and abort parsing
                self->currentEvent_ = RequestParserEvent::Error;
                self->eventHandler_.onError(PipelineError(PipelineErrorCode::UriTooLong));
                return -1;
            }
        }
        return 0;
    }

    int RequestParser::on_header_field_cb(http_parser *parser, const char *at, std::size_t length)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            // If we haven't invoked onMessageBegin yet and we're starting headers, do it now
            if (self->currentEvent_ == RequestParserEvent::Url && self->urlLen_ > 0)
            {
                int result = self->eventHandler_.onMessageBegin(
                    http_method_str(static_cast<http_method>(self->parser_.method)),
                    self->parser_.http_major,
                    self->parser_.http_minor,
                    String(self->buffer_.data() + self->urlPos_, self->urlLen_));
                self->resetBuffer();

                if (result != 0)
                {
                    return result;
                }
            }

            // If we were processing a header value and now we're starting a new field,
            // it means the previous header is complete
            if (self->currentEvent_ == RequestParserEvent::HeaderValue && self->headerFieldLen_ > 0)
            {
                if (self->headerCount_ >= HttpServerAdvanced::MAX_REQUEST_HEADER_COUNT)
                {
                    // Too many headers: inform handler and abort parsing
                    self->currentEvent_ = RequestParserEvent::Error;
                    self->eventHandler_.onError(PipelineError(PipelineErrorCode::HeaderTooLarge));
                    return -1;
                }
                ++self->headerCount_;
                int result = self->eventHandler_.onHeader(
                    String(self->buffer_.data() + self->headerFieldPos_, self->headerFieldLen_),
                    String(self->buffer_.data() + self->headerValuePos_, self->headerValueLen_));
                self->resetBuffer();
                if (result != 0)
                {
                    return result;
                }
            }

            self->currentEvent_ = RequestParserEvent::HeaderField;
            // Buffer the header field chunk
            if (!self->appendToBuffer(at, length, HttpServerAdvanced::MAX_REQUEST_HEADER_NAME_LENGTH, self->headerFieldPos_, self->headerFieldLen_))
            {
                // Header name too long: notify handler and stop parsing
                self->currentEvent_ = RequestParserEvent::Error;
                self->eventHandler_.onError(PipelineError(PipelineErrorCode::HeaderTooLarge));
                return -1;
            }
        }
        return 0;
    }

    int RequestParser::on_header_value_cb(http_parser *parser, const char *at, std::size_t length)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = RequestParserEvent::HeaderValue;
            // Buffer the header value chunk
            if (!self->appendToBuffer(at, length, HttpServerAdvanced::MAX_REQUEST_HEADER_VALUE_LENGTH, self->headerValuePos_, self->headerValueLen_))
            {
                // Header value too long: notify handler and abort parsing
                self->currentEvent_ = RequestParserEvent::Error;
                self->eventHandler_.onError(PipelineError(PipelineErrorCode::HeaderTooLarge));
                return -1;
            }
        }
        return 0;
    }

    int RequestParser::on_headers_complete_cb(http_parser *parser)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            // Complete the last header if we have one
            if (self->headerFieldLen_ > 0)
            {
                if (self->headerCount_ >= HttpServerAdvanced::MAX_REQUEST_HEADER_COUNT)
                {
                    return -1;
                }
                ++self->headerCount_;
                int result = self->eventHandler_.onHeader(
                    String(self->buffer_.data() + self->headerFieldPos_, self->headerFieldLen_),
                    String(self->buffer_.data() + self->headerValuePos_, self->headerValueLen_));
                self->resetBuffer();
                if (result != 0)
                {
                    return result;
                }
            }

            self->currentEvent_ = RequestParserEvent::HeadersComplete;
            return self->eventHandler_.onHeadersComplete();
        }
        return 0;
    }

    int RequestParser::on_body_cb(http_parser *parser, const char *at, std::size_t length)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = RequestParserEvent::Body;
            return self->eventHandler_.onBody(reinterpret_cast<const uint8_t *>(at), length);
        }
        return 0;
    }

    int RequestParser::on_message_complete_cb(http_parser *parser)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = RequestParserEvent::MessageComplete;
            return self->eventHandler_.onMessageComplete();
        }
        return 0;
    }
}
