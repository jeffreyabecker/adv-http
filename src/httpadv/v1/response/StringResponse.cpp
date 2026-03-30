#include "StringResponse.h"

#include <cstring>
#include <string>
#include <string_view>

namespace httpadv::v1::response
{
    namespace
    {
        HttpHeaderCollection buildHeaders(std::initializer_list<HttpHeader> headers, size_t contentLength, std::string_view defaultContentType = httpadv::v1::core::HttpContentTypes::TextPlain)
        {
            HttpHeaderCollection headersCollection;
            for (const auto &header : headers)
            {
                headersCollection.set(header);
            }
            if (!headersCollection.exists(HttpHeaderNames::ContentType))
            {
                headersCollection.set(HttpHeader(HttpHeaderNames::ContentType, defaultContentType));
            }
            if (!headersCollection.exists(HttpHeaderNames::ContentLength))
            {
                headersCollection.set(HttpHeader(std::string_view(HttpHeaderNames::ContentLength), std::to_string(contentLength)));
            }
            return headersCollection;
        }
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, std::string body, std::initializer_list<HttpHeader> headers)
    {
        auto headersCollection = buildHeaders(headers, body.size());
        auto bodySource = std::make_unique<StdStringByteSource>(std::move(body));
        return std::make_unique<HttpResponse>(status, std::move(bodySource), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, std::string_view body, std::initializer_list<HttpHeader> headers)
    {
        return create(status, std::string(body), headers);
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, std::string body)
    {
        return create(status, std::move(body), {});
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, std::string_view body)
    {
        return create(status, body, {});
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, const char *body, std::initializer_list<HttpHeader> headers)
    {
        size_t len = strlen(body);
        auto headersCollection = buildHeaders(headers, len);
        auto bodySource = std::make_unique<StdStringByteSource>(std::string(body, len));
        return std::make_unique<HttpResponse>(status, std::move(bodySource), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, const char *body)
    {
        return create(status, body, {});
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, const uint8_t *body, size_t length, std::initializer_list<HttpHeader> headers)
    {
        auto headersCollection = buildHeaders(headers, length);
        auto bodySource = std::make_unique<httpadv::v1::transport::VectorByteSource>(std::vector<uint8_t>(body, body + length));
        return std::make_unique<HttpResponse>(status, std::move(bodySource), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, std::string_view contentType, std::string body)
    {
        return create(status, std::move(body), {HttpHeader(HttpHeaderNames::ContentType, contentType)});
    }

    std::unique_ptr<IHttpResponse> StringResponse::create(HttpStatus status, std::string_view contentType, std::string_view body)
    {
        return create(status, std::string(body), {HttpHeader(HttpHeaderNames::ContentType, contentType)});
    }

} // namespace HttpServerAdvanced
