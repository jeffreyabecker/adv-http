#pragma once
#include <initializer_list>
#include <cstring>
#include "./HttpStatus.h"
#include "./HttpHeader.h"
#include "./HttpHeaderCollection.h"
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
    HttpHeaderCollection headers_;
    std::unique_ptr<HttpResponseBodyStream> body_;

    static constexpr const char *defaultContentType = "text/plain";
    void init(size_t contentLength);

  public:
    HttpResponse(HttpStatus status, String &&body, HttpHeaderCollection && headers);
    HttpResponse(HttpStatus status, const String &body, HttpHeaderCollection && headers);
    HttpResponse(HttpStatus status, const char *body, HttpHeaderCollection && headers);
    HttpResponse(HttpStatus status, const uint8_t *body, size_t length, HttpHeaderCollection && headers);
    HttpResponse(HttpStatus status, std::unique_ptr<Stream> body, HttpHeaderCollection && headers);

    // static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String &body, std::initializer_list<std::pair<const char *, const char *>> headers);
    // static std::unique_ptr<IHttpResponse> create(HttpStatus status, String &&body, std::initializer_list<std::pair<const char *, const char *>> headers);
    // static std::unique_ptr<IHttpResponse> create(HttpStatus status, const char *body, std::initializer_list<std::pair<const char *, const char *>> headers);
    // static std::unique_ptr<IHttpResponse> create(HttpStatus status, const uint8_t *body, size_t length, std::initializer_list<std::pair<const char *, const char *>> headers);
    // static std::unique_ptr<IHttpResponse> create(HttpStatus status, std::unique_ptr<Stream> body, std::initializer_list<std::pair<const char *, const char *>> headers);

    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String &body, std::initializer_list<HttpHeader> headers);
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, String &&body, std::initializer_list<HttpHeader> headers);
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const char *body, std::initializer_list<HttpHeader> headers);
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const uint8_t *body, size_t length, std::initializer_list<HttpHeader> headers);
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, std::unique_ptr<Stream> body, std::initializer_list<HttpHeader> headers);

    ~HttpResponse() override = default;

    HttpStatus status() const override;
    HttpHeaderCollection &headers() override;
    std::unique_ptr<HttpResponseBodyStream> getBody() override;
  };
    // Stream contract used throughout the HTTP pipeline:
    //  - available() == 0   : end of stream (no more bytes)
    //  - available()  > 0   : that many bytes are buffered and readable now
    //  - available() == -1  : buffer empty but more data is expected (non-final)
    // read() should return -1 only at end of stream; peek() mirrors read() without consuming.
  std::unique_ptr<Stream> CreateResponseStream(std::unique_ptr<IHttpResponse> response);

} // namespace HttpServerAdvanced
