#pragma once
#include <initializer_list>
#include <cstring>
#include "./HttpStatus.h"
#include "./HttpHeader.h"
#include "./Streams.h"
#include "./Iterators.h"
#include "./HttpResponseBodyStream.h"
#include "./ChunkedHttpResponseBodyStream.h"
#include "./HttpResponseIterators.h"
#include "./IHttpResponse.h"
#include <Arduino.h>
#include <memory>
namespace HttpServerAdvanced
{

  class HttpResponse : public IHttpResponse
  {
  private:
    HttpStatus status_;
    HttpHeadersCollection headers_;
    std::unique_ptr<HttpResponseBodyStream> body_;

    static constexpr const char *defaultContentType = "text/plain";
    
    void init(size_t contentLength) {
      if (contentLength >= 0) {
        headers_.set(HttpHeader::ContentLength, String(contentLength));
        if (!headers_.exists(HttpHeader::ContentType)) {
          headers_.set(HttpHeader::ContentType, defaultContentType);
        }
      }
      if (contentLength < 0) {
        headers_.remove(HttpHeader::ContentLength);
        headers_.set(HttpHeader::TransferEncoding, "chunked");
      }
    }

  public:
    HttpResponse(HttpStatus status, String &&body, HttpHeadersCollection headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(std::move(body))) {
      init(body_->available());
    }
    
    HttpResponse(HttpStatus status, const String &body, HttpHeadersCollection headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(body)) {
      init(body_->available());
    }
    
    HttpResponse(HttpStatus status, const char *body, HttpHeadersCollection headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(body)) {
      init(body_->available());
    }
    
    HttpResponse(HttpStatus status, const uint8_t *body, size_t length, HttpHeadersCollection headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(body, length)) {
      init(body_->available());
    }
    
    HttpResponse(HttpStatus status, std::unique_ptr<Stream> body, HttpHeadersCollection headers)
        : status_(status), headers_(std::move(headers)), body_(HttpResponseBodyStream::create(std::move(body))) {
      init(body_->available());
    }

    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String &body, HttpHeadersCollection headers = HttpHeadersCollection()){
      return std::make_unique<HttpResponse>(status, body, headers);
    }
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, String &&body, HttpHeadersCollection headers = HttpHeadersCollection()){
      return std::make_unique<HttpResponse>(status, std::move(body), headers);
    }
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const char *body, HttpHeadersCollection headers = HttpHeadersCollection()){
      return std::make_unique<HttpResponse>(status, body, headers);
    }
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const uint8_t *body, size_t length, HttpHeadersCollection headers = HttpHeadersCollection()){
      return std::make_unique<HttpResponse>(status, body, length, headers);
    }
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, std::unique_ptr<Stream> body, HttpHeadersCollection headers = HttpHeadersCollection()){
      return std::make_unique<HttpResponse>(status, std::move(body), headers);
    }

    ~HttpResponse() override = default;

    HttpStatus status() const override {
      return status_;
    }
    
    HttpHeadersCollection &headers() override {
      return headers_;
    }
    
    std::unique_ptr<HttpResponseBodyStream> getBody() override {
      return std::move(body_);
    }
  };

  std::unique_ptr<Stream> CreateResponseStream(std::unique_ptr<IHttpResponse> response);

} // namespace HttpServerAdvanced
