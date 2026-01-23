#pragma once
#include <initializer_list>
#include "./HttpStatus.h"
#include "./HttpHeader.h"
#include "./HttpHeaderCollection.h"
#include "./HttpResponse.h"
#include "./Streams.h"
#include "./HttpContentTypes.h"
#include <Arduino.h>
#include <memory>

namespace HttpServerAdvanced
{
    class StringResponse
    {
    public:
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String &body, std::initializer_list<HttpHeader> headers = {});
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, String &&body, std::initializer_list<HttpHeader> headers = {});
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const char *body, std::initializer_list<HttpHeader> headers = {});
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const uint8_t *body, size_t length, std::initializer_list<HttpHeader> headers = {});

        // Convenience overloads that use the default content type
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String &body);
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, String &&body);
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const char *body);

        // Convenience overloads that accept a content-type string
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String &contentType, const String &body);
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String &contentType, String &&body);
        static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String &contentType, const char *body);
    };
}
