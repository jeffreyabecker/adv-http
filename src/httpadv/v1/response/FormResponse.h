#pragma once
#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "../core/HttpStatus.h"
#include "../streams/UriStream.h"
#include "HttpResponse.h"

namespace httpadv::v1::response
{
    class FormResponse
    {
    public:
        using Field = std::pair<std::string, std::string>;
        using FieldCollection = std::vector<Field>;
        using FieldMap = std::map<std::string, std::string>;

        /**
         * @brief Create a form URL-encoded response.
         * @param status HTTP status code
         * @param data Form field key-value pairs to be URL-encoded
         * @param headers Optional additional headers (Content-Type and Content-Length are set automatically)
         * @return A unique_ptr to an IHttpResponse with form-encoded body
         */
        static std::unique_ptr<IHttpResponse> create(
            HttpStatus status,
            FieldCollection &&data,
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
            const FieldCollection &data,
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
            FieldMap &&data,
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
            const FieldMap &data,
            std::initializer_list<HttpHeader> headers = {}
        );

    };
}


