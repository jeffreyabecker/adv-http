#include "StringResponse.h"

namespace HttpServerAdvanced
{

    HttpHeaderCollection buildHeaders(std::initializer_list<HttpHeader> headers, size_t contentLength)
    {
        HttpHeaderCollection headersCollection;
        for (const auto &header : headers)
        {
            headersCollection.set(header);
        }
        if (!headersCollection.exists(HttpHeaderNames::ContentType))
        {
            headersCollection.set(HttpHeader(HttpHeaderNames::ContentType, HttpContentTypes::TextPlain));
        }
        if (!headersCollection.exists(HttpHeaderNames::ContentLength))
        {
            headersCollection.set(HttpHeader(HttpHeaderNames::ContentLength, String(contentLength)));
        }
        return headersCollection;
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, const String &body, std::initializer_list<HttpHeader> headers)
    {
        auto headersCollection = buildHeaders(headers, body.length());
        auto bodyStream = std::make_unique<StringStream>(body);
        return std::make_unique<HttpResponse>(status, std::move(bodyStream), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, const String &body)
    {
        return create(status, body, {});
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, String &&body, std::initializer_list<HttpHeader> headers)
    {
        auto headersCollection = buildHeaders(headers, body.length());
        auto bodyStream = std::make_unique<StringStream>(std::move(body));
        return std::make_unique<HttpResponse>(status, std::move(bodyStream), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, String &&body)
    {
        return create(status, std::move(body), {});
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, const char *body, std::initializer_list<HttpHeader> headers)
    {
        size_t len = strlen(body);
        auto headersCollection = buildHeaders(headers, len);
        auto bodyStream = std::make_unique<StringStream>(body);
        return std::make_unique<HttpResponse>(status, std::move(bodyStream), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, const char *body)
    {
        return create(status, body, {});
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, const uint8_t *body, size_t length, std::initializer_list<HttpHeader> headers)
    {
        auto headersCollection = buildHeaders(headers, length);
        auto bodyStream = std::make_unique<OctetsStream>(body, length, false);
        return std::make_unique<HttpResponse>(status, std::move(bodyStream), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, const String &contentType, const String &body)
    {
        return create(status, body, {HttpHeader::ContentType(contentType)});
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, const String &contentType, String &&body)
    {
        return create(status, std::move(body), {HttpHeader::ContentType(contentType)});
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, const String &contentType, const char *body)
    {
        return create(status, body, {HttpHeader::ContentType(contentType)});
    }

} // namespace HttpServerAdvanced
