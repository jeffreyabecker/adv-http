#pragma once
#include <Arduino.h>
#include <IPAddress.h>

namespace HTTPServer
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
    class HttpServerBase;

    namespace Concepts
    {

        template <typename T>
        struct IsReadableType
        {
        private:
            template <typename U>
            auto test(int) -> decltype(
                // Available() -> int
                std::is_convertible<decltype(std::declval<U>().available()), int>{},
                // Read(rbuf, size) -> int
                std::is_convertible<decltype(std::declval<U>().read(std::declval<uint8_t *>(), std::declval<std::size_t>())), int>{},
                std::true_type{});

            template <typename>
            std::false_type test(...);

        public:
            static constexpr bool value = decltype(test<T>(0))::value;
        };

        template <typename T>
        struct IsWritableType
        {
        private:
            template <typename U>
            auto test(int) -> decltype(
                // Write(buf, size) -> std::size_t
                std::is_convertible<decltype(std::declval<U>().write(std::declval<const uint8_t *>(), std::declval<std::size_t>())), std::size_t>{},
                // Flush() -> void
                std::is_same<decltype(std::declval<U>().flush()), void>{},
                std::true_type{});

            template <typename>
            std::false_type test(...);

        public:
            static constexpr bool value = decltype(test<T>(0))::value;
        };

        /// @brief Many arduino types such as File, overload bool() to indicate if they are valid or not.
        /// This trait checks if a type has an overloaded operator bool() that returns a boolean value
        /// @tparam T
        template <typename T>
        struct IsTestableType
        {
        private:
            template <typename U>
            auto test(int) -> decltype(
                // operator bool()
                std::is_convertible<decltype(std::declval<U>().operator bool()), bool>{},
                std::true_type{});

            template <typename>
            std::false_type test(...);

        public:
            static constexpr bool value = decltype(test<T>(0))::value;
        };

        template <typename T>
        class HasStaticName
        {
        private:
            template <typename U>
            static auto test(int) -> decltype(std::is_same<decltype(U::NAME), const char *>{},
                                              std::true_type{});

            template <typename>
            static std::false_type test(...);

        public:
            static constexpr bool value = decltype(test<T>(0))::value;
        };

    } // namespace Concepts

} // namespace HTTPServer
