#include "RequestParser.h"
#include "IPipelineHandler.h"
#include "PipelineError.h"
#include <cstring> // Required for strncmp

namespace HttpServerAdvanced
{
    // Static method implementations
    RequestParser *RequestParser::get_instance(llhttp_t *parser)
    {
        return static_cast<RequestParser *>(parser->data);
    }

    int RequestParser::on_message_begin_cb(llhttp_t *parser)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = RequestParserEvent::MessageBegin;
        }
        return 0;
    }

    int RequestParser::on_url_cb(llhttp_t *parser, const char *at, std::size_t length)
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

    int RequestParser::on_header_field_cb(llhttp_t *parser, const char *at, std::size_t length)
    {
        auto *self = get_instance(parser);
        if (self)
        {
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

    int RequestParser::on_header_value_cb(llhttp_t *parser, const char *at, std::size_t length)
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

    int RequestParser::on_headers_complete_cb(llhttp_t *parser)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = RequestParserEvent::HeadersComplete;
            return self->eventHandler_.onHeadersComplete();
        }
        return 0;
    }

    int RequestParser::on_body_cb(llhttp_t *parser, const char *at, std::size_t length)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = RequestParserEvent::Body;
            return self->eventHandler_.onBody(reinterpret_cast<const uint8_t *>(at), length);
        }
        return 0;
    }

    int RequestParser::on_message_complete_cb(llhttp_t *parser)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            self->currentEvent_ = RequestParserEvent::MessageComplete;
            return self->eventHandler_.onMessageComplete();
        }
        return 0;
    }

    // New llhttp-specific callbacks
    int RequestParser::on_protocol_cb(llhttp_t *parser, const char *at, std::size_t length)
    {
        // Protocol callback - not typically used for HTTP/1.1, but could be useful for HTTP/2
        return 0;
    }

 

    int RequestParser::on_method_cb(llhttp_t *parser, const char *at, std::size_t length)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            // Just buffer the method data
            if (!self->appendToBuffer(at, length, 16, self->methodPos_, self->methodLen_))
            {
                self->currentEvent_ = RequestParserEvent::Error;
                self->eventHandler_.onError(PipelineError(PipelineErrorCode::ParseError));
                return -1;
            }
        }
        return 0;
    }

    int RequestParser::on_version_cb(llhttp_t *parser, const char *at, std::size_t length)
    {
        auto *self = get_instance(parser);
        if (self)
        {
            // Just buffer the version data
            if (!self->appendToBuffer(at, length, 16, self->versionPos_, self->versionLen_))
            {
                self->currentEvent_ = RequestParserEvent::Error;
                self->eventHandler_.onError(PipelineError(PipelineErrorCode::ParseError));
                return -1;
            }
        }
        return 0;
    }

    // Completion callbacks - these are called when parsing of each component is complete
    int RequestParser::on_protocol_complete_cb(llhttp_t *parser)
    {
        return 0;
    }

    int RequestParser::on_url_complete_cb(llhttp_t *parser)
    {
        auto *self = get_instance(parser);
        if (self && self->urlLen_ > 0)
        {
            // URL is complete, call onMessageBegin with method, version, and URL
            int result = self->eventHandler_.onMessageBegin(
                self->method_.c_str(),
                self->versionMajor_,
                self->versionMinor_,
                String(self->buffer_.data() + self->urlPos_, self->urlLen_));
            
            if (result != 0)
            {
                return result;
            }
        }
        return 0;
    }

    int RequestParser::on_method_complete_cb(llhttp_t *parser)
    {
        auto *self = get_instance(parser);
        if (self && self->methodLen_ > 0)
        {
            // Copy method from buffer to method_ string
            self->method_ = String(self->buffer_.data() + self->methodPos_, self->methodLen_);
        }
        return 0;
    }

    int RequestParser::on_version_complete_cb(llhttp_t *parser)
    {
        auto *self = get_instance(parser);
        if (self && self->versionLen_ > 0)
        {
            // Parse version string like "HTTP/1.1" or "1.1"
            const char* versionStr = self->buffer_.data() + self->versionPos_;
            size_t len = self->versionLen_;
            
            // Find the version numbers after "HTTP/" or at start
            const char* numberStart = versionStr;
            if (len >= 5 && strncmp(versionStr, "HTTP/", 5) == 0)
            {
                numberStart = versionStr + 5;
                len -= 5;
            }
            
            // Parse major.minor format
            if (len >= 3)
            {
                self->versionMajor_ = numberStart[0] - '0';
                if (len >= 3 && numberStart[1] == '.')
                {
                    self->versionMinor_ = numberStart[2] - '0';
                }
            }
        }
        return 0;
    }

    int RequestParser::on_header_field_complete_cb(llhttp_t *parser)
    {
        // Header field is complete - just return success
        // The actual header processing happens in on_header_value_complete_cb
        // when both field and value are available
        return 0;
    }

    int RequestParser::on_header_value_complete_cb(llhttp_t *parser)
    {
        auto *self = get_instance(parser);
        if (self && self->headerFieldLen_ > 0 && self->headerValueLen_ > 0)
        {
            // Header field and value are complete, call onHeader
            if (self->headerCount_ >= HttpServerAdvanced::MAX_REQUEST_HEADER_COUNT)
            {
                self->currentEvent_ = RequestParserEvent::Error;
                self->eventHandler_.onError(PipelineError(PipelineErrorCode::HeaderTooLarge));
                return -1;
            }
            ++self->headerCount_;
            int result = self->eventHandler_.onHeader(
                String(self->buffer_.data() + self->headerFieldPos_, self->headerFieldLen_),
                String(self->buffer_.data() + self->headerValuePos_, self->headerValueLen_));
            
            // Reset header field and value lengths for next header
            self->headerFieldLen_ = 0;
            self->headerValueLen_ = 0;
            
            if (result != 0)
            {
                return result;
            }
        }
        return 0;
    }
}
