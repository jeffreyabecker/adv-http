#pragma once
#include <memory>
#include <functional>
#include <utility>

#include "../core/HttpTimeouts.h"
#include "IPipelineHandler.h"

#include "../llhttp/include/llhttp.h"
#include "PipelineError.h"
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
        llhttp_t parser_;
        llhttp_settings_t settings_;
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
        std::size_t methodPos_ = 0;
        std::size_t methodLen_ = 0;
        std::size_t versionPos_ = 0;
        std::size_t versionLen_ = 0;
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
            methodLen_ = 0;
            urlLen_ = 0;
            versionLen_ = 0;
            headerFieldLen_ = 0;
            headerValueLen_ = 0;
        }

        // Helper to get RequestParser* from llhttp_t
        static RequestParser *get_instance(llhttp_t *parser);

        // Static trampoline functions for llhttp_settings_t
        static int on_message_begin_cb(llhttp_t *parser);
        static int on_protocol_cb(llhttp_t *parser, const char *at, std::size_t length);
        static int on_url_cb(llhttp_t *parser, const char *at, std::size_t length);
        static int on_method_cb(llhttp_t *parser, const char *at, std::size_t length);
        static int on_version_cb(llhttp_t *parser, const char *at, std::size_t length);
        static int on_header_field_cb(llhttp_t *parser, const char *at, std::size_t length);
        static int on_header_value_cb(llhttp_t *parser, const char *at, std::size_t length);
        static int on_headers_complete_cb(llhttp_t *parser);
        static int on_body_cb(llhttp_t *parser, const char *at, std::size_t length);
        static int on_message_complete_cb(llhttp_t *parser);
        static int on_protocol_complete_cb(llhttp_t *parser);
        static int on_url_complete_cb(llhttp_t *parser);
        static int on_method_complete_cb(llhttp_t *parser);
        static int on_version_complete_cb(llhttp_t *parser);
        static int on_header_field_complete_cb(llhttp_t *parser);
        static int on_header_value_complete_cb(llhttp_t *parser);

        String method_;
        short versionMajor_ = 1;
        short versionMinor_ = 1;

    public:
        RequestParser(IPipelineHandler &eventHandler)
            : eventHandler_(eventHandler)
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

        std::size_t execute(const uint8_t *data, std::size_t length)
        {
            llhttp_errno_t result = llhttp_execute(&parser_, reinterpret_cast<const char *>(data), length);
            if (result != HPE_OK)
            {
                // If we already flagged an error (e.g., buffer overflow) the handler has been notified.
                if (currentEvent_ != RequestParserEvent::Error)
                {
                    currentEvent_ = RequestParserEvent::Error;
                    // Map llhttp internal errno to library-level PipelineErrorCode.
                    // Do not expose llhttp errno in the public API; report a generic parse error for now.
                    eventHandler_.onError(PipelineError(PipelineErrorCode::ParseError));
                }
            }
            // Reset header counter for the next request on reused parser
            if (currentEvent_ == RequestParserEvent::MessageComplete || currentEvent_ == RequestParserEvent::Error)
            {
                headerCount_ = 0;
                resetBuffer();
            }
            return (result == HPE_OK) ? length : 0;
        }

        const char *method() const { return method_.c_str(); }
        short versionMajor() const { return versionMajor_; }
        short versionMinor() const { return versionMinor_; }

        bool isFinished() const
        {
            return llhttp_message_needs_eof(&parser_) == 0;
        }

        bool shouldKeepAlive() const
        {
            return llhttp_should_keep_alive(&parser_) != 0;
        }

        RequestParserEvent currentEvent() const { return currentEvent_; }
    };
}

