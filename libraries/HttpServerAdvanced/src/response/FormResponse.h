#pragma once
#include <initializer_list>
#include <vector>
#include <map>
#include <utility>
#include "../core/HttpStatus.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "./HttpResponse.h"
#include "../streams/UriStream.h"
#include <Arduino.h>
#include <memory>

namespace HttpServerAdvanced
{
    class FormResponse
    {
    public:
        /**
         * @brief Create a form URL-encoded response.
         * @param status HTTP status code
         * @param data Form field key-value pairs to be URL-encoded
         * @param headers Optional additional headers (Content-Type and Content-Length are set automatically)
         * @return A unique_ptr to an IHttpResponse with form-encoded body
         */
        static std::unique_ptr<IHttpResponse> create(
            HttpStatus status,
            std::vector<std::pair<String, String>> &&data,
            std::initializer_list<HttpHeader> headers = {}
        );

        /**
         * @brief Create a form URL-encoded response (const reference overload).
         * @param status HTTP status code
         * @param data Form field key-value pairs to be URL-encoded
         * @param headers Optional additional headers (Content-Type and Content-Length are set automatically)
         * @return A unique_ptr to an IHttpResponse with form-encoded body
         */
        static std::unique_ptr<IHttpResponse> create(
            HttpStatus status,
            const std::vector<std::pair<String, String>> &data,
            std::initializer_list<HttpHeader> headers = {}
        );

        /**
         * @brief Create a form URL-encoded response from a map.
         * @param status HTTP status code
         * @param data Map of form field key-value pairs to be URL-encoded
         * @param headers Optional additional headers (Content-Type and Content-Length are set automatically)
         * @return A unique_ptr to an IHttpResponse with form-encoded body
         */
        static std::unique_ptr<IHttpResponse> create(
            HttpStatus status,
            std::map<String, String> &&data,
            std::initializer_list<HttpHeader> headers = {}
        );

        /**
         * @brief Create a form URL-encoded response from a map (const reference overload).
         * @param status HTTP status code
         * @param data Map of form field key-value pairs to be URL-encoded
         * @param headers Optional additional headers (Content-Type and Content-Length are set automatically)
         * @return A unique_ptr to an IHttpResponse with form-encoded body
         */
        static std::unique_ptr<IHttpResponse> create(
            HttpStatus status,
            const std::map<String, String> &data,
            std::initializer_list<HttpHeader> headers = {}
        );
    };
}


