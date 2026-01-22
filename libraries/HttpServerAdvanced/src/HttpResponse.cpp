#include "HttpResponse.h"

namespace HttpServerAdvanced
{
    void HttpResponse::init(size_t contentLength)
    {
        if (contentLength >= 0)
        {
            headers_.set(HttpHeaderNames::ContentLength, String(contentLength));
            if (!headers_.exists(HttpHeaderNames::ContentType))
            {
                headers_.set(HttpHeaderNames::ContentType, defaultContentType);
            }
        }
        if (contentLength < 0)
        {
            headers_.remove(HttpHeaderNames::ContentLength);
            headers_.set(HttpHeaderNames::TransferEncoding, "chunked");
        }
    }

    HttpResponse::HttpResponse(HttpStatus status, String &&body, HttpHeadersCollection &&headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(std::move(body)))
    {
        init(body_->available());
    }

    HttpResponse::HttpResponse(HttpStatus status, const String &body, HttpHeadersCollection &&headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(body))
    {
        init(body_->available());
    }

    HttpResponse::HttpResponse(HttpStatus status, const char *body, HttpHeadersCollection &&headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(body))
    {
        init(body_->available());
    }

    HttpResponse::HttpResponse(HttpStatus status, const uint8_t *body, size_t length, HttpHeadersCollection &&headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(body, length))
    {
        init(body_->available());
    }

    HttpResponse::HttpResponse(HttpStatus status, std::unique_ptr<Stream> body, HttpHeadersCollection &&headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(std::move(body)))
    {
        init(body_->available());
    }

    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String &body, std::initializer_list<HttpHeader> headers)
    {
        HttpHeadersCollection headersCollection;
        for (const auto &header : headers)
        {
            headersCollection.set(header);
        }
        return std::make_unique<HttpResponse>(status, body, std::move(headersCollection));
    }

    static std::unique_ptr<IHttpResponse> create(HttpStatus status, String &&body, std::initializer_list<HttpHeader> headers)
    {
        HttpHeadersCollection headersCollection;
        for (const auto &header : headers)
        {
            headersCollection.set(header);
        }
        return std::make_unique<HttpResponse>(status, std::move(body), std::move(headersCollection));
    }

    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const char *body, std::initializer_list<HttpHeader> headers)
    {
        HttpHeadersCollection headersCollection;
        for (const auto &header : headers)
        {
            headersCollection.set(header);
        }
        return std::make_unique<HttpResponse>(status, body, std::move(headersCollection));
    }

    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const uint8_t *body, size_t length, std::initializer_list<HttpHeader> headers)
    {
        HttpHeadersCollection headersCollection;
        for (const auto &header : headers)
        {
            headersCollection.set(header);
        }
        return std::make_unique<HttpResponse>(status, body, length, std::move(headersCollection));
    }

    static std::unique_ptr<IHttpResponse> create(HttpStatus status, std::unique_ptr<Stream> body, std::initializer_list<HttpHeader> headers)
    {
        HttpHeadersCollection headersCollection;
        for (const auto &header : headers)
        {
            headersCollection.set(header);
        }
        return std::make_unique<HttpResponse>(status, std::move(body), std::move(headersCollection));
    }

    HttpStatus HttpResponse::status() const
    {
        return status_;
    }

    HttpHeadersCollection &HttpResponse::headers()
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
