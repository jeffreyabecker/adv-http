#pragma once
#include <Arduino.h>

namespace HttpServerAdvanced
{

    constexpr std::size_t ETHERNET_FRAME_BUFFER_SIZE = 1436; // Standard Ethernet MTU minus headers

// no direct equivalent in WebServer as it does a bunch of dynamic allocations in parsing.h
#ifndef HTTPSERVER_ADVANCED_PIPELINE_STACK_BUFFER_SIZE
    constexpr std::size_t PIPELINE_STACK_BUFFER_SIZE = 256; // the internal wifi classes use 256 byte buffers for stream copying
#else
    constexpr std::size_t PIPELINE_STACK_BUFFER_SIZE = HTTPSERVER_ADVANCED_PIPELINE_STACK_BUFFER_SIZE;
#endif

#ifndef HTTPSERVER_ADVANCED_MULTIPART_FORM_DATABUFFER_SIZE
    constexpr std::size_t MULTIPART_FORM_DATA_BUFFER_SIZE = 1436; // Standard Ethernet MTU minus headers
#else
    constexpr std::size_t MULTIPART_FORM_DATA_BUFFER_SIZE = HTTPSERVER_ADVANCED_MULTIPART_FORM_DATABUFFER_SIZE;
#endif



/**
 * @brief Maximum total time allowed for a complete HTTP request in a pipeline
 */
#ifndef HTTPSERVER_ADVANCED_PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS
    constexpr uint32_t PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS = 5000UL; // Default: 5 seconds
#else
    constexpr uint32_t PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS = HTTPSERVER_ADVANCED_PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS;
#endif

/**
 * @brief Timeout for activity on the pipeline (resets on each successful read/write)
 */
#ifndef HTTPSERVER_ADVANCED_PIPELINE_ACTIVITY_TIMEOUT
    constexpr uint32_t PIPELINE_ACTIVITY_TIMEOUT = 1000UL; // Default: 1 second
#else
    constexpr uint32_t PIPELINE_ACTIVITY_TIMEOUT = HTTPSERVER_ADVANCED_PIPELINE_ACTIVITY_TIMEOUT;
#endif

/**
 * @brief Timeout for a single client->read call in the pipeline
 */
#ifndef HTTPSERVER_ADVANCED_PIPELINE_READ_TIMEOUT
    constexpr uint32_t PIPELINE_READ_TIMEOUT = 500UL; // Default: 500 ms
#else
    constexpr uint32_t PIPELINE_READ_TIMEOUT = HTTPSERVER_ADVANCED_PIPELINE_READ_TIMEOUT;
#endif

#ifndef HTTPSERVER_ADVANCED_REQUEST_MATCHER_PATH_WILDCARD_CHAR
    constexpr char REQUEST_MATCHER_PATH_WILDCARD_CHAR = '?';
#else
    constexpr char REQUEST_MATCHER_PATH_WILDCARD_CHAR = HTTPSERVER_ADVANCED_REQUEST_MATCHER_PATH_WILDCARD_CHAR;
#endif

#ifndef HTTPSERVER_ADVANCED_MAX_REQUEST_URI_LENGTH
    constexpr size_t MAX_REQUEST_URI_LENGTH = 1024; // 1 KB
#else
    constexpr size_t MAX_REQUEST_URI_LENGTH = HTTPSERVER_ADVANCED_MAX_REQUEST_URI_LENGTH;
#endif

#ifndef HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_COUNT
    constexpr size_t MAX_REQUEST_HEADER_COUNT = 32;
#else
    constexpr size_t MAX_REQUEST_HEADER_COUNT = HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_COUNT;
#endif

// limits for header name/value lengths
// the longest canned header is  "Attribution-Reporting-Register-Trigger", length 38 characters.
// 64 is slightly less than double that, allowing some room for custom headers.
#ifndef HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_NAME_LENGTH
    constexpr size_t MAX_REQUEST_HEADER_NAME_LENGTH = 64;
#else
    constexpr size_t MAX_REQUEST_HEADER_NAME_LENGTH = HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_NAME_LENGTH;
#endif
#ifndef HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_VALUE_LENGTH
    constexpr size_t MAX_REQUEST_HEADER_VALUE_LENGTH = 256; // 256 bytes
#else
    constexpr size_t MAX_REQUEST_HEADER_VALUE_LENGTH = HTTPSERVER_ADVANCED_MAX_REQUEST_HEADER_VALUE_LENGTH;
#endif

#ifndef HTTPSERVER_ADVANCED_REQUEST_PARSER_BUFFER_LENGTH
    constexpr size_t REQUEST_PARSER_BUFFER_LENGTH =
        std::max(MAX_REQUEST_URI_LENGTH,
                 MAX_REQUEST_HEADER_NAME_LENGTH + MAX_REQUEST_HEADER_VALUE_LENGTH); // 2 KB
#else
    constexpr size_t REQUEST_PARSER_BUFFER_LENGTH = std::max(HTTPSERVER_ADVANCED_REQUEST_PARSER_BUFFER_LENGTH, MAX_REQUEST_URI_LENGTH,
                                                      MAX_REQUEST_HEADER_NAME_LENGTH + MAX_REQUEST_HEADER_VALUE_LENGTH);

#endif

#ifndef HTTPSERVER_ADVANCED_MAX_BUFFERED_BODY_LENGTH
    constexpr ssize_t MAX_BUFFERED_BODY_LENGTH = 2048; // 2 KB
#else
    constexpr ssize_t MAX_BUFFERED_BODY_LENGTH = HTTPSERVER_ADVANCED_MAX_BUFFERED_BODY_LENGTH;
#endif

#ifndef HTTPSERVER_ADVANCED_MAX_BUFFERED_FORM_BODY_LENGTH
    constexpr ssize_t MAX_BUFFERED_FORM_BODY_LENGTH = MAX_BUFFERED_BODY_LENGTH; // 2 KB
#else
    constexpr ssize_t MAX_BUFFERED_FORM_BODY_LENGTH = HTTPSERVER_ADVANCED_MAX_BUFFERED_FORM_BODY_LENGTH;
#endif

#ifndef HTTPSERVER_ADVANCED_MAX_BUFFERED_JSON_BODY_LENGTH
    constexpr ssize_t MAX_BUFFERED_JSON_BODY_LENGTH = MAX_BUFFERED_BODY_LENGTH; // 2 KB
#else
    constexpr ssize_t MAX_BUFFERED_JSON_BODY_LENGTH = HTTPSERVER_ADVANCED_MAX_BUFFERED_JSON_BODY_LENGTH;
#endif

#ifndef HTTPSERVER_ADVANCED_MAX_CONCURRENT_CONNECTIONS
    constexpr size_t MAX_CONCURRENT_CONNECTIONS = 1; // Default: 1 simultaneous pipeline
#else
    constexpr size_t MAX_CONCURRENT_CONNECTIONS = HTTPSERVER_ADVANCED_MAX_CONCURRENT_CONNECTIONS;
#endif

#ifndef HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON
#ifdef ARDUINOJSON_VERSION
    #define HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON 1
#else
    #define HTTPSERVER_ADVANCED_ENABLE_ARDUINO_JSON 0
#endif
#endif
} // namespace HttpServerAdvanced


