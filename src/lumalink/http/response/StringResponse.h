#pragma once
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <exception>
#include "../core/HttpStatus.h"
#include "../core/HttpHeader.h"
#include "../core/HttpHeaderCollection.h"
#include "LumaLinkPlatform.h"
#include "HttpResponse.h"
#include "../core/HttpContentTypes.h"

namespace lumalink::http::response
{
    using lumalink::http::core::HttpHeader;
    using lumalink::http::core::HttpStatus;

    class StringResponse
    {
    public:
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, std::string body, std::initializer_list<HttpHeader> headers = {});
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, std::string_view body, std::initializer_list<HttpHeader> headers = {});
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const char *body, std::initializer_list<HttpHeader> headers = {});
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const uint8_t *body, size_t length, std::initializer_list<HttpHeader> headers = {});

        // Convenience overloads that use the default content type
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, std::string body);
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, std::string_view body);
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const char *body);

        // Convenience overloads that accept a content-type string
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, std::string_view contentType, std::string body);
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, std::string_view contentType, std::string_view body);
    };


} // namespace lumalink::http::response


