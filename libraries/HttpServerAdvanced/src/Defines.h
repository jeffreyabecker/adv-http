#pragma once
#include <Arduino.h>
#include <IPAddress.h>

namespace HttpServerAdvanced
{


    constexpr std::size_t ETHERNET_FRAME_BUFFER_SIZE = 1436; // Standard Ethernet MTU minus headers

// no direct equivalent in WebServer as it does a bunch of dynamic allocations in parsing.h
#ifndef HTTPSERVER_REQUEST_BUFFER_SIZE
    constexpr std::size_t REQUEST_BUFFER_SIZE = ETHERNET_FRAME_BUFFER_SIZE; // default to 1 ethernet packet worth of data
#else
    constexpr std::size_t REQUEST_BUFFER_SIZE = HTTPSERVER_REQUEST_BUFFER_SIZE;
#endif

// corresponds to HTTP_RAW_BUFLEN
#ifndef HTTPSERVER_REQUEST_BODY_BUFFER_SIZE
    constexpr std::size_t REQUEST_BODY_BUFFER_SIZE = ETHERNET_FRAME_BUFFER_SIZE; // default to same as request buffer size 
#else
    constexpr std::size_t REQUEST_BODY_BUFFER_SIZE = HTTPSERVER_REQUEST_BODY_BUFFER_SIZE;
#endif

// no direct equivalent in WebServer as it does a bunch of dynamic allocations in HttpServer.cpp
// eg all the response headers are built into a single String
// this framework does a shell game of streams to minimize allocations
#ifndef HTTPSERVER_CHUNKED_RESPONSE_BUFFER_SIZE
    constexpr std::size_t CHUNKED_RESPONSE_BUFFER_SIZE = REQUEST_BUFFER_SIZE;
#else
    constexpr std::size_t CHUNKED_RESPONSE_BUFFER_SIZE = HTTPSERVER_CHUNKED_RESPONSE_BUFFER_SIZE;
#endif
    static_assert(CHUNKED_RESPONSE_BUFFER_SIZE >= ETHERNET_FRAME_BUFFER_SIZE, "CHUNKED_RESPONSE_BUFFER_SIZE must be at least ETHERNET_FRAME_BUFFER_SIZE and should probably be an integer multiple of that size.");


/**
 * @brief Maximum total time allowed for a complete HTTP request in a pipeline
 */
#ifndef HTTPSERVER_PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS
    constexpr uint32_t PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS = 5000UL; // Default: 5 seconds
#else
    constexpr uint32_t PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS = HTTPSERVER_PIPELINE_MAX_TOTAL_REQUEST_LENGTH_MS;
#endif

/**
 * @brief Timeout for activity on the pipeline (resets on each successful read/write)
 */
#ifndef HTTPSERVER_PIPELINE_ACTIVITY_TIMEOUT
    constexpr uint32_t PIPELINE_ACTIVITY_TIMEOUT = 1000UL; // Default: 1 second
#else
    constexpr uint32_t PIPELINE_ACTIVITY_TIMEOUT = HTTPSERVER_PIPELINE_ACTIVITY_TIMEOUT;
#endif

/**
 * @brief Timeout for a single client->read call in the pipeline
 */
#ifndef HTTPSERVER_PIPELINE_READ_TIMEOUT
    constexpr uint32_t PIPELINE_READ_TIMEOUT = 500UL; // Default: 500 ms
#else
    constexpr uint32_t PIPELINE_READ_TIMEOUT = HTTPSERVER_PIPELINE_READ_TIMEOUT;
#endif
    

} // namespace HttpServerAdvanced
