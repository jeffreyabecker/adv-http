#include "HttpResponse.h"

namespace HttpServerAdvanced
{
    void HttpResponse::init(size_t contentLength) 
    {
        if (contentLength >= 0) 
        {
            headers_.set(HttpHeader::ContentLength, String(contentLength));
            if (!headers_.exists(HttpHeader::ContentType)) 
            {
                headers_.set(HttpHeader::ContentType, defaultContentType);
            }
        }
        if (contentLength < 0) 
        {
            headers_.remove(HttpHeader::ContentLength);
            headers_.set(HttpHeader::TransferEncoding, "chunked");
        }
    }

    HttpResponse::HttpResponse(HttpStatus status, String &&body, HttpHeadersCollection headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(std::move(body))) 
    {
        init(body_->available());
    }

    HttpResponse::HttpResponse(HttpStatus status, const String &body, HttpHeadersCollection headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(body)) 
    {
        init(body_->available());
    }

    HttpResponse::HttpResponse(HttpStatus status, const char *body, HttpHeadersCollection headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(body)) 
    {
        init(body_->available());
    }

    HttpResponse::HttpResponse(HttpStatus status, const uint8_t *body, size_t length, HttpHeadersCollection headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(body, length)) 
    {
        init(body_->available());
    }

    HttpResponse::HttpResponse(HttpStatus status, std::unique_ptr<Stream> body, HttpHeadersCollection headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(std::move(body))) 
    {
        init(body_->available());
    }

    std::unique_ptr<IHttpResponse> HttpResponse::create(HttpStatus status, const String &body, HttpHeadersCollection headers)
    {
        return std::make_unique<HttpResponse>(status, body, headers);
    }

    std::unique_ptr<IHttpResponse> HttpResponse::create(HttpStatus status, String &&body, HttpHeadersCollection headers)
    {
        return std::make_unique<HttpResponse>(status, std::move(body), headers);
    }

    std::unique_ptr<IHttpResponse> HttpResponse::create(HttpStatus status, const char *body, HttpHeadersCollection headers)
    {
        return std::make_unique<HttpResponse>(status, body, headers);
    }

    std::unique_ptr<IHttpResponse> HttpResponse::create(HttpStatus status, const uint8_t *body, size_t length, HttpHeadersCollection headers)
    {
        return std::make_unique<HttpResponse>(status, body, length, headers);
    }

    std::unique_ptr<IHttpResponse> HttpResponse::create(HttpStatus status, std::unique_ptr<Stream> body, HttpHeadersCollection headers)
    {
        return std::make_unique<HttpResponse>(status, std::move(body), headers);
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
