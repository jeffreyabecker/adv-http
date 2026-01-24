#pragma once
#include <memory>
#include <functional>

#include "../core/HttpTimeouts.h"
#include "./IPipelineHandler.h"

#include <http_parser.h>
#include "./PipelineError.h"
#include <cstddef>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <algorithm>
#include <array>
#include <cstring>
#include "../core/Defines.h"

namespace HttpServerAdvanced
{
    enum class RequestParserEvent
    {
        None,
        MessageBegin,
        Url,
        HeaderField,
        HeaderValue,
        HeadersComplete,
        Body,
        MessageComplete,
        Error
    };

    // Wrapper class for the http_parser library
    class RequestParser
    {
    private:
        http_parser parser_;
        http_parser_settings settings_;
        RequestParserEvent currentEvent_ = RequestParserEvent::None;
        IPipelineHandler &eventHandler_;
        /**
        Recommendation
        For memory-constrained embedded devices (RP2040, ESP8266):

        Factor	Winner	Why
        Worst-case predictability	Static	No fragmentation, bounded
        Average-case efficiency	Dynamic	Typical requests use less
        Real-time/latency	Static	No malloc during parse
        Long-running stability	Static	Fragmentation accumulates over time
        Verdict: The current static buffer is the safer choice for an HTTP server that must run indefinitely without reboot. The ~800 B savings from dynamic allocation isn't worth the fragmentation risk on systems without virtual memory.

        If you wanted to pursue dynamic allocation, I'd suggest the hybrid approach with a 256-byte inline buffer—but only if profiling shows memory pressure in real workloads.
        */
        std::array<char, HttpServerAdvanced::REQUEST_PARSER_BUFFER_LENGTH> buffer_{};
        std::size_t writePos_ = 0;
        std::size_t urlPos_ = 0;
        std::size_t urlLen_ = 0;
        std::size_t headerFieldPos_ = 0;
        std::size_t headerFieldLen_ = 0;
        std::size_t headerValuePos_ = 0;
        std::size_t headerValueLen_ = 0;
        std::size_t headerCount_ = 0;

        bool appendToBuffer(const char *data, std::size_t length, std::size_t maxAllowed, std::size_t &startPos, std::size_t &curLen)
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

        void resetBuffer()
        {
            writePos_ = 0;
            urlLen_ = 0;
            headerFieldLen_ = 0;
            headerValueLen_ = 0;
        }

        // Helper to get HttpRequestParser* from http_parser
        static RequestParser *get_instance(http_parser *parser);

        // Static trampoline functions for http_parser_settings
        static int on_message_begin_cb(http_parser *parser);
        static int on_url_cb(http_parser *parser, const char *at, std::size_t length);
        static int on_header_field_cb(http_parser *parser, const char *at, std::size_t length);
        static int on_header_value_cb(http_parser *parser, const char *at, std::size_t length);
        static int on_headers_complete_cb(http_parser *parser);
        static int on_body_cb(http_parser *parser, const char *at, std::size_t length);
        static int on_message_complete_cb(http_parser *parser);

    public:
        RequestParser(IPipelineHandler &eventHandler)
            : eventHandler_(eventHandler)
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

        std::size_t execute(const uint8_t *data, std::size_t length)
        {
            int result = http_parser_execute(&parser_, &settings_, reinterpret_cast<const char *>(data), length);
            if (parser_.http_errno != HPE_OK)
            {
                // If we already flagged an error (e.g., buffer overflow) the handler has been notified.
                if (currentEvent_ != RequestParserEvent::Error)
                {
                    currentEvent_ = RequestParserEvent::Error;
                    // Map http_parser internal errno to library-level PipelineErrorCode.
                    // Do not expose http_errno in the public API; report a generic parse error for now.
                    eventHandler_.onError(PipelineError(PipelineErrorCode::ParseError));
                }
            }
            // Reset header counter for the next request on reused parser
            if (currentEvent_ == RequestParserEvent::MessageComplete || currentEvent_ == RequestParserEvent::Error)
            {
                headerCount_ = 0;
                resetBuffer();
            }
            return result;
        }

        const char *method() const { return http_method_str(static_cast<http_method>(parser_.method)); }
        short versionMajor() const { return parser_.http_major; }
        short versionMinor() const { return parser_.http_minor; }

        bool isFinished() const
        {
            return http_body_is_final(&parser_);
        }

        bool shouldKeepAlive() const
        {
            return http_should_keep_alive(&parser_);
        }

        RequestParserEvent currentEvent() const { return currentEvent_; }
    };
}

