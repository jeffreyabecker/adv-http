#pragma once
#include <memory>
#include <functional>

#include "./HttpTimeouts.h"
#include "./IPipelineHandler.h"

#include <http_parser.h>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <algorithm>

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
        String urlBuffer_;
        String headerFieldBuffer_;
        String headerValueBuffer_;

        // Helper to get HttpRequestParser* from http_parser
        static RequestParser *get_instance(http_parser *parser)
        {
            return static_cast<RequestParser *>(parser->data);
        }

        // Static trampoline functions for http_parser_settings
        static int on_message_begin_cb(http_parser *parser)
        {
            auto *self = get_instance(parser);
            if (self)
            {
                self->currentEvent_ = RequestParserEvent::MessageBegin;
                // return self->eventHandler_.onMessageBegin();
            }
            return 0;
        }

        static int on_url_cb(http_parser *parser, const char *at, std::size_t length)
        {
            auto *self = get_instance(parser);
            if (self)
            {
                self->currentEvent_ = RequestParserEvent::Url;
                // Buffer the URL chunk
                self->urlBuffer_.concat(at, length);
            }
            return 0;
        }

        static int on_header_field_cb(http_parser *parser, const char *at, std::size_t length)
        {
            auto *self = get_instance(parser);
            if (self)
            {
                // If we haven't invoked onMessageBegin yet and we're starting headers, do it now
                if (self->currentEvent_ == RequestParserEvent::Url && !self->urlBuffer_.isEmpty())
                {
                    int result = self->eventHandler_.onMessageBegin(
                        http_method_str(static_cast<http_method>(self->parser_.method)),
                        self->parser_.http_major,
                        self->parser_.http_minor,
                        std::move(self->urlBuffer_));
                    self->urlBuffer_ = String(); // Clear buffer after moving

                    if (result != 0)
                    {
                        return result;
                    }
                }
                
                // If we were processing a header value and now we're starting a new field,
                // it means the previous header is complete
                if (self->currentEvent_ == RequestParserEvent::HeaderValue && !self->headerFieldBuffer_.isEmpty())
                {
                    int result = self->eventHandler_.onHeader(
                        std::move(self->headerFieldBuffer_),
                        std::move(self->headerValueBuffer_)
                    );
                    self->headerFieldBuffer_ = String();
                    self->headerValueBuffer_ = String();
                    if (result != 0)
                    {
                        return result;
                    }
                }
                
                self->currentEvent_ = RequestParserEvent::HeaderField;
                // Buffer the header field chunk
                self->headerFieldBuffer_.concat(at, length);
            }
            return 0;
        }

        static int on_header_value_cb(http_parser *parser, const char *at, std::size_t length)
        {
            auto *self = get_instance(parser);
            if (self)
            {
                self->currentEvent_ = RequestParserEvent::HeaderValue;
                // Buffer the header value chunk
                self->headerValueBuffer_.concat(at, length);
            }
            return 0;
        }

        static int on_headers_complete_cb(http_parser *parser)
        {
            auto *self = get_instance(parser);
            if (self)
            {
                // Complete the last header if we have one
                if (!self->headerFieldBuffer_.isEmpty())
                {
                    int result = self->eventHandler_.onHeader(
                        std::move(self->headerFieldBuffer_),
                        std::move(self->headerValueBuffer_)
                    );
                    self->headerFieldBuffer_ = String();
                    self->headerValueBuffer_ = String();
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

        static int on_body_cb(http_parser *parser, const char *at, std::size_t length)
        {
            auto *self = get_instance(parser);
            if (self)
            {
                self->currentEvent_ = RequestParserEvent::Body;
                return self->eventHandler_.onBody(reinterpret_cast<const uint8_t *>(at), length);
            }
            return 0;
        }

        static int on_message_complete_cb(http_parser *parser)
        {
            auto *self = get_instance(parser);
            if (self)
            {
                self->currentEvent_ = RequestParserEvent::MessageComplete;
                return self->eventHandler_.onMessageComplete();
            }
            return 0;
        }

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
                currentEvent_ = RequestParserEvent::Error;
                eventHandler_.onError(PipelineParserError(static_cast<enum http_errno>(parser_.http_errno)));
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