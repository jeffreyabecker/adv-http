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
    void init(size_t contentLength);

  public:
    HttpResponse(HttpStatus status, String &&body, HttpHeadersCollection headers);
    HttpResponse(HttpStatus status, const String &body, HttpHeadersCollection headers);
    HttpResponse(HttpStatus status, const char *body, HttpHeadersCollection headers);
    HttpResponse(HttpStatus status, const uint8_t *body, size_t length, HttpHeadersCollection headers);
    HttpResponse(HttpStatus status, std::unique_ptr<Stream> body, HttpHeadersCollection headers);

    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String &body, HttpHeadersCollection headers = HttpHeadersCollection());

    // Convenience overloads to allow direct brace-initializer usage in sketches: create(..., {{"Name","Value"}})
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String &body, std::initializer_list<std::pair<const char *, const char *>> headers);
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, String &&body, HttpHeadersCollection headers = HttpHeadersCollection());
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const char *body, HttpHeadersCollection headers = HttpHeadersCollection());
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const uint8_t *body, size_t length, HttpHeadersCollection headers = HttpHeadersCollection());
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, std::unique_ptr<Stream> body, HttpHeadersCollection headers = HttpHeadersCollection());

    ~HttpResponse() override = default;

    HttpStatus status() const override;
    HttpHeadersCollection &headers() override;
    std::unique_ptr<HttpResponseBodyStream> getBody() override;
  };
    // Stream contract used throughout the HTTP pipeline:
    //  - available() == 0   : end of stream (no more bytes)
    //  - available()  > 0   : that many bytes are buffered and readable now
    //  - available() == -1  : buffer empty but more data is expected (non-final)
    // read() should return -1 only at end of stream; peek() mirrors read() without consuming.
  std::unique_ptr<Stream> CreateResponseStream(std::unique_ptr<IHttpResponse> response);

} // namespace HttpServerAdvanced
