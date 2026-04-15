#include "RequestParser.h"
#include "IPipelineHandler.h"
#include "PipelineError.h"
#include <cstring> // Required for strncmp

namespace lumalink::http::pipeline
{
    bool RequestParser::appendToBuffer(const char *data, std::size_t length,
                                       std::size_t maxAllowed, std::size_t &startPos,
                                       std::size_t &curLen)
    {
        if (curLen == 0)
        {
            startPos = writePos_;
        }
        if (length == 0)
        {
            return true;
        }
        if (writePos_ + length > buffer_.size())
        {
            return false;
        }
        if (curLen + length > maxAllowed)
        {
            return false;
        }
        std::memcpy(buffer_.data() + writePos_, data, length);
        writePos_ += length;
        curLen += length;
        return true;
    }

    void RequestParser::resetBuffer()
    {
        writePos_ = 0;
        methodLen_ = 0;
        urlLen_ = 0;
        versionLen_ = 0;
        headerFieldLen_ = 0;
        headerValueLen_ = 0;
    }

    PipelineErrorCode RequestParser::mapLlhttpErrorToPipelineError(int llhttpError)
    {
        switch (llhttpError)
        {
        case 0:
            return PipelineErrorCode::None; // HPE_OK
        case 1:
            return PipelineErrorCode::InternalError; // HPE_INTERNAL
        case 2:
            return PipelineErrorCode::StrictError; // HPE_STRICT
        case 25:
            return PipelineErrorCode::CrExpectedError; // HPE_CR_EXPECTED
        case 3:
            return PipelineErrorCode::LfExpectedError; // HPE_LF_EXPECTED
        case 4:
            return PipelineErrorCode::UnexpectedContentLengthError; // HPE_UNEXPECTED_CONTENT_LENGTH
        case 30:
            return PipelineErrorCode::UnexpectedSpaceError; // HPE_UNEXPECTED_SPACE
        case 5:
            return PipelineErrorCode::ClosedConnectionError; // HPE_CLOSED_CONNECTION
        case 6:
            return PipelineErrorCode::InvalidMethodError; // HPE_INVALID_METHOD
        case 7:
            return PipelineErrorCode::InvalidUrlError; // HPE_INVALID_URL
        case 8:
            return PipelineErrorCode::InvalidConstantError; // HPE_INVALID_CONSTANT
        case 9:
            return PipelineErrorCode::InvalidVersionError; // HPE_INVALID_VERSION
        case 10:
            return PipelineErrorCode::InvalidHeaderTokenError; // HPE_INVALID_HEADER_TOKEN
        case 11:
            return PipelineErrorCode::InvalidContentLengthError; // HPE_INVALID_CONTENT_LENGTH
        case 12:
            return PipelineErrorCode::InvalidChunkSizeError; // HPE_INVALID_CHUNK_SIZE
        case 13:
            return PipelineErrorCode::InvalidStatusError; // HPE_INVALID_STATUS
        case 14:
            return PipelineErrorCode::InvalidEofStateError; // HPE_INVALID_EOF_STATE
        case 15:
            return PipelineErrorCode::InvalidTransferEncodingError; // HPE_INVALID_TRANSFER_ENCODING
        default:
            return PipelineErrorCode::ParseError;
        }
    }

    RequestParser::RequestParser(IPipelineHandler &eventHandler) : eventHandler_(eventHandler)
    {
        llhttp_settings_init(&settings_);
        settings_.on_message_begin = on_message_begin_cb;
        settings_.on_protocol = on_protocol_cb;
        settings_.on_url = on_url_cb;
        settings_.on_method = on_method_cb;
        settings_.on_version = on_version_cb;
        settings_.on_header_field = on_header_field_cb;
        settings_.on_header_value = on_header_value_cb;
        settings_.on_headers_complete = on_headers_complete_cb;
        settings_.on_body = on_body_cb;
        settings_.on_message_complete = on_message_complete_cb;
        settings_.on_protocol_complete = on_protocol_complete_cb;
        settings_.on_url_complete = on_url_complete_cb;
        settings_.on_method_complete = on_method_complete_cb;
        settings_.on_version_complete = on_version_complete_cb;
        settings_.on_header_field_complete = on_header_field_complete_cb;
        settings_.on_header_value_complete = on_header_value_complete_cb;

        llhttp_init(&parser_, HTTP_REQUEST, &settings_);
        parser_.data = this;
    }

    std::size_t RequestParser::execute(const uint8_t *data, std::size_t length)
    {
        if (finished_)
        {
            return 0;
        }
        llhttp_errno_t result = HPE_OK;
        if (data == nullptr || length == 0)
        {
            finished_ = true;
            result = llhttp_finish(&parser_);
            if (result != HPE_OK)
            {
                if (currentEvent_ != RequestParserEvent::Error)
                {
                    currentEvent_ = RequestParserEvent::Error;
                    eventHandler_.onError(PipelineError(mapLlhttpErrorToPipelineError(result)));
                }
            }
        }
        else
        {
            result = llhttp_execute(&parser_, reinterpret_cast<const char *>(data), length);
            if (result != HPE_OK)
            {
                if (result == HPE_PAUSED_UPGRADE)
                {
                    finished_ = true;
                    return length;
                }

                if (currentEvent_ != RequestParserEvent::Error)
                {
                    currentEvent_ = RequestParserEvent::Error;
                    eventHandler_.onError(PipelineError(mapLlhttpErrorToPipelineError(result)));
                }
            }
            if (currentEvent_ == RequestParserEvent::MessageComplete ||
                currentEvent_ == RequestParserEvent::Error)
            {
                headerCount_ = 0;
                resetBuffer();
            }
        }
        return (result == HPE_OK) ? length : 0;
    }

    void RequestParser::reset()
    {
        currentEvent_ = RequestParserEvent::None;
        finished_ = false;
        headerCount_ = 0;
        method_.clear();
        versionMajor_ = 1;
        versionMinor_ = 1;
        resetBuffer();
        llhttp_init(&parser_, HTTP_REQUEST, &settings_);
        parser_.data = this;
    }

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
            if (!self->appendToBuffer(at, length, lumalink::http::core::MAX_REQUEST_URI_LENGTH, self->urlPos_, self->urlLen_))
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
            if (!self->appendToBuffer(at, length, lumalink::http::core::MAX_REQUEST_HEADER_NAME_LENGTH, self->headerFieldPos_, self->headerFieldLen_))
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
            if (!self->appendToBuffer(at, length, lumalink::http::core::MAX_REQUEST_HEADER_VALUE_LENGTH, self->headerValuePos_, self->headerValueLen_))
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
            if (!self->appendToBuffer(at, length, lumalink::http::core::MAX_REQUEST_METHOD_LENGTH, self->methodPos_, self->methodLen_))
            {
                self->currentEvent_ = RequestParserEvent::Error;
                self->eventHandler_.onError(PipelineError(PipelineErrorCode::InvalidMethodError));
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
                std::string_view(self->buffer_.data() + self->urlPos_, self->urlLen_));
            
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
            self->method_.assign(self->buffer_.data() + self->methodPos_, self->methodLen_);
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
            if (self->headerCount_ >= lumalink::http::core::MAX_REQUEST_HEADER_COUNT)
            {
                self->currentEvent_ = RequestParserEvent::Error;
                self->eventHandler_.onError(PipelineError(PipelineErrorCode::HeaderTooLarge));
                return -1;
            }
            ++self->headerCount_;
            int result = self->eventHandler_.onHeader(
                std::string_view(self->buffer_.data() + self->headerFieldPos_, self->headerFieldLen_),
                std::string_view(self->buffer_.data() + self->headerValuePos_, self->headerValueLen_));
            
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
