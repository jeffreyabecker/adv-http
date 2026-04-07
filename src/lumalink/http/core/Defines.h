#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace lumalink::http::core
{

    constexpr std::size_t ETHERNET_FRAME_BUFFER_SIZE = 1436; // Standard Ethernet MTU minus headers

// no direct equivalent in WebServer as it does a bunch of dynamic allocations in parsing.h
#ifndef LUMALINK_HTTP_PIPELINE_STACK_BUFFER_SIZE
    constexpr std::size_t PIPELINE_STACK_BUFFER_SIZE = 256; // the internal wifi classes use 256 byte buffers for stream copying
#else
    constexpr std::size_t PIPELINE_STACK_BUFFER_SIZE = LUMALINK_HTTP_PIPELINE_STACK_BUFFER_SIZE;
#endif

#ifndef LUMALINK_HTTP_MULTIPART_FORM_DATABUFFER_SIZE
    constexpr std::size_t MULTIPART_FORM_DATA_BUFFER_SIZE = 1436; // Standard Ethernet MTU minus headers
#else
    constexpr std::size_t MULTIPART_FORM_DATA_BUFFER_SIZE = LUMALINK_HTTP_MULTIPART_FORM_DATABUFFER_SIZE;
#endif



/**
 * @brief Maximum total time allowed for a complete HTTP request in a pipeline
 */
#ifndef LUMALINK_HTTP_PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS
    constexpr uint32_t PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS = 5000UL; // Default: 5 seconds
#else
    constexpr uint32_t PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS = LUMALINK_HTTP_PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS;
#endif

/**
 * @brief Timeout for activity on the pipeline (resets on each successful read/write)
 */
#ifndef LUMALINK_HTTP_PIPELINE_ACTIVITY_TIMEOUT
    constexpr uint32_t PIPELINE_ACTIVITY_TIMEOUT = 1000UL; // Default: 1 second
#else
    constexpr uint32_t PIPELINE_ACTIVITY_TIMEOUT = LUMALINK_HTTP_PIPELINE_ACTIVITY_TIMEOUT;
#endif

/**
 * @brief Timeout for a single client->read call in the pipeline
 */
#ifndef LUMALINK_HTTP_PIPELINE_READ_TIMEOUT
    constexpr uint32_t PIPELINE_READ_TIMEOUT = 500UL; // Default: 500 ms
#else
    constexpr uint32_t PIPELINE_READ_TIMEOUT = LUMALINK_HTTP_PIPELINE_READ_TIMEOUT;
#endif

#ifndef LUMALINK_HTTP_MAX_REQUEST_URI_LENGTH
    constexpr size_t MAX_REQUEST_URI_LENGTH = 1024; // 1 KB
#else
    constexpr size_t MAX_REQUEST_URI_LENGTH = LUMALINK_HTTP_MAX_REQUEST_URI_LENGTH;
#endif

#ifndef LUMALINK_HTTP_MAX_REQUEST_METHOD_LENGTH
    constexpr size_t MAX_REQUEST_METHOD_LENGTH = 11;
#else
    constexpr size_t MAX_REQUEST_METHOD_LENGTH = LUMALINK_HTTP_MAX_REQUEST_METHOD_LENGTH;
#endif

#ifndef LUMALINK_HTTP_MAX_REQUEST_HEADER_COUNT
    constexpr size_t MAX_REQUEST_HEADER_COUNT = 32;
#else
    constexpr size_t MAX_REQUEST_HEADER_COUNT = LUMALINK_HTTP_MAX_REQUEST_HEADER_COUNT;
#endif

// limits for header name/value lengths
// the longest canned header is  "Attribution-Reporting-Register-Trigger", length 38 characters.
// 64 is slightly less than double that, allowing some room for custom headers.
#ifndef LUMALINK_HTTP_MAX_REQUEST_HEADER_NAME_LENGTH
    constexpr size_t MAX_REQUEST_HEADER_NAME_LENGTH = 64;
#else
    constexpr size_t MAX_REQUEST_HEADER_NAME_LENGTH = LUMALINK_HTTP_MAX_REQUEST_HEADER_NAME_LENGTH;
#endif
#ifndef LUMALINK_HTTP_MAX_REQUEST_HEADER_VALUE_LENGTH
    constexpr size_t MAX_REQUEST_HEADER_VALUE_LENGTH = 256; // 256 bytes
#else
    constexpr size_t MAX_REQUEST_HEADER_VALUE_LENGTH = LUMALINK_HTTP_MAX_REQUEST_HEADER_VALUE_LENGTH;
#endif

#ifndef LUMALINK_HTTP_REQUEST_PARSER_BUFFER_LENGTH
    constexpr size_t REQUEST_PARSER_BUFFER_LENGTH =
        std::max(MAX_REQUEST_URI_LENGTH + MAX_REQUEST_METHOD_LENGTH + 32,
                 MAX_REQUEST_HEADER_NAME_LENGTH + MAX_REQUEST_HEADER_VALUE_LENGTH); // shared parser token buffer
#else
    constexpr size_t REQUEST_PARSER_BUFFER_LENGTH = std::max(
        LUMALINK_HTTP_REQUEST_PARSER_BUFFER_LENGTH,
        std::max(MAX_REQUEST_URI_LENGTH + MAX_REQUEST_METHOD_LENGTH + 32,
                 MAX_REQUEST_HEADER_NAME_LENGTH + MAX_REQUEST_HEADER_VALUE_LENGTH));

#endif

#ifndef LUMALINK_HTTP_MAX_BUFFERED_BODY_LENGTH
    constexpr std::ptrdiff_t MAX_BUFFERED_BODY_LENGTH = 2048; // 2 KB
#else
    constexpr std::ptrdiff_t MAX_BUFFERED_BODY_LENGTH = LUMALINK_HTTP_MAX_BUFFERED_BODY_LENGTH;
#endif

#ifndef LUMALINK_HTTP_MAX_BUFFERED_FORM_BODY_LENGTH
    constexpr std::ptrdiff_t MAX_BUFFERED_FORM_BODY_LENGTH = MAX_BUFFERED_BODY_LENGTH; // 2 KB
#else
    constexpr std::ptrdiff_t MAX_BUFFERED_FORM_BODY_LENGTH = LUMALINK_HTTP_MAX_BUFFERED_FORM_BODY_LENGTH;
#endif

#ifndef LUMALINK_HTTP_MAX_BUFFERED_JSON_BODY_LENGTH
    constexpr std::ptrdiff_t MAX_BUFFERED_JSON_BODY_LENGTH = MAX_BUFFERED_BODY_LENGTH; // 2 KB
#else
    constexpr std::ptrdiff_t MAX_BUFFERED_JSON_BODY_LENGTH = LUMALINK_HTTP_MAX_BUFFERED_JSON_BODY_LENGTH;
#endif

#ifndef LUMALINK_HTTP_MAX_CONCURRENT_CONNECTIONS
    constexpr size_t MAX_CONCURRENT_CONNECTIONS = 1; // Default: 1 simultaneous pipeline
#else
    constexpr size_t MAX_CONCURRENT_CONNECTIONS = LUMALINK_HTTP_MAX_CONCURRENT_CONNECTIONS;
#endif

#ifndef LUMALINK_HTTP_WEBSOCKET_MAX_FRAME_PAYLOAD_SIZE
#define LUMALINK_HTTP_WEBSOCKET_MAX_FRAME_PAYLOAD_SIZE 4096
#endif
    static constexpr std::size_t WsMaxFramePayloadSize = LUMALINK_HTTP_WEBSOCKET_MAX_FRAME_PAYLOAD_SIZE;

#ifndef LUMALINK_HTTP_WEBSOCKET_MAX_MESSAGE_SIZE
#define LUMALINK_HTTP_WEBSOCKET_MAX_MESSAGE_SIZE 8192
#endif
    static constexpr std::size_t WsMaxMessageSize = LUMALINK_HTTP_WEBSOCKET_MAX_MESSAGE_SIZE;

#ifndef LUMALINK_HTTP_WEBSOCKET_IDLE_TIMEOUT_MS
#define LUMALINK_HTTP_WEBSOCKET_IDLE_TIMEOUT_MS 30000UL
#endif
    static constexpr std::uint32_t WsIdleTimeoutMs = LUMALINK_HTTP_WEBSOCKET_IDLE_TIMEOUT_MS;

#ifndef LUMALINK_HTTP_WEBSOCKET_CLOSE_TIMEOUT_MS
#define LUMALINK_HTTP_WEBSOCKET_CLOSE_TIMEOUT_MS 2000UL
#endif
    static constexpr std::uint32_t WsCloseTimeoutMs = LUMALINK_HTTP_WEBSOCKET_CLOSE_TIMEOUT_MS;

#ifndef LUMALINK_HTTP_ENABLE_ARDUINO_JSON
#if defined(__has_include)
    #if __has_include(<ArduinoJson.h>)
        #define LUMALINK_HTTP_ENABLE_ARDUINO_JSON 1
    #else
        #define LUMALINK_HTTP_ENABLE_ARDUINO_JSON 0
    #endif
#else
    #define LUMALINK_HTTP_ENABLE_ARDUINO_JSON 0
#endif
#endif

    static constexpr bool EnableArduinoJson = LUMALINK_HTTP_ENABLE_ARDUINO_JSON != 0;
} // namespace lumalink::http::core


