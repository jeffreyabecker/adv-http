#include "HttpResponse.h"
#include "./Streams.h"
#include "./HttpContentTypes.h"
namespace HttpServerAdvanced
{



    HttpResponse::HttpResponse(HttpStatus status, std::unique_ptr<Stream> body, HttpHeaderCollection &&headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(std::move(body)))
    {
    }

    std::unique_ptr<IHttpResponse> HttpResponse::create(HttpStatus status, const String &body, std::initializer_list<HttpHeader> headers)
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
            headersCollection.set(HttpHeader(HttpHeaderNames::ContentLength, String(body.length())));
        }
        auto bodyStream = std::make_unique<StringStream>(body);
        return std::make_unique<HttpResponse>(status, std::move(bodyStream), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> HttpResponse::create(HttpStatus status, String &&body, std::initializer_list<HttpHeader> headers)
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
            headersCollection.set(HttpHeader(HttpHeaderNames::ContentLength, String(body.length())));
        }
        auto bodyStream = std::make_unique<StringStream>(std::move(body));
        return std::make_unique<HttpResponse>(status, std::move(bodyStream), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> HttpResponse::create(HttpStatus status, const char *body, std::initializer_list<HttpHeader> headers)
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
            headersCollection.set(HttpHeader(HttpHeaderNames::ContentLength, String(strlen(body))));
        }
        auto bodyStream = std::make_unique<StringStream>(body);
        return std::make_unique<HttpResponse>(status, std::move(bodyStream), std::move(headersCollection));
    }

    std::unique_ptr<IHttpResponse> HttpResponse::create(HttpStatus status, const uint8_t *body, size_t length, std::initializer_list<HttpHeader> headers)
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
            headersCollection.set(HttpHeader(HttpHeaderNames::ContentLength, String(length)));
        }
        auto bodyStream = std::make_unique<OctetsStream>(body, length, false);
        return std::make_unique<HttpResponse>(status, std::move(bodyStream), std::move(headersCollection));
    }


    HttpStatus HttpResponse::status() const
    {
        return status_;
    }

    HttpHeaderCollection &HttpResponse::headers()
    {
        return headers_;
    }

    std::unique_ptr<HttpResponseBodyStream> HttpResponse::getBody()
    {
        // Extract the body stream for use in response writing.
        // The body is moved only once to the HttpPipelineResponseStream.
        return std::move(body_);
    }

} // namespace HttpServerAdvanced
