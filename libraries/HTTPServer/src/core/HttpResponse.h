#pragma once
#include <initializer_list>
#include <cstring>
#include "./HttpStatus.h"
#include "./HttpHeader.h"
#include "../util/Util.h"
#include <Arduino.h>
#include <memory>
namespace HTTPServer::Core
{

  class HttpResponseBodyStream : public ReadStream
  {
  protected:
    std::unique_ptr<Stream> innerStream_;

  public:
    HttpResponseBodyStream(std::unique_ptr<Stream> innerStream);
    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual size_t write(uint8_t b) override;

    template <typename TStream, typename... TArgs>
    static std::unique_ptr<HttpResponseBodyStream> create(TArgs &&...args)
    {
      return std::make_unique<HttpResponseBodyStream>(std::make_unique<TStream>(std::forward<TArgs>(args)...));
    }
    static std::unique_ptr<HttpResponseBodyStream> create(const String &str)
    {
      return create<StringStream>(str);
    }
    static std::unique_ptr<HttpResponseBodyStream> create(String &&str)
    {
      return create<StringStream>(std::move(str));
    }
    static std::unique_ptr<HttpResponseBodyStream> create(const char *cstr)
    {
      return create<OctetsStream>(cstr);
    }
    static std::unique_ptr<HttpResponseBodyStream> create(const uint8_t *data, size_t length, bool ownsData = false)
    {
      return create<OctetsStream>(data, length, ownsData);
    }
    static std::unique_ptr<HttpResponseBodyStream> empty()
    {
      return std::make_unique<HttpResponseBodyStream>(std::make_unique<EmptyReadStream>());
    }
    static std::unique_ptr<HttpResponseBodyStream> create(std::unique_ptr<Stream> stream)
    {
      return std::make_unique<HttpResponseBodyStream>(std::move(stream));
    }
  };

  class IHttpResponse
  {
  public:
    using ResponseFilter = std::function<std::unique_ptr<IHttpResponse>(std::unique_ptr<IHttpResponse>)>;
    virtual ~IHttpResponse() = default;
    virtual HttpStatus status() const = 0;
    virtual HttpHeadersCollection &headers() = 0;
    virtual std::unique_ptr<HttpResponseBodyStream> getBody() = 0;
  };

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

    template <typename... Headers>
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, String &&body, Headers &&...headers)
    {
      return std::make_unique<HttpResponse>(status, std::move(body), HttpHeadersCollection{std::forward<Headers>(headers)...});
    }

    template <typename... Headers>
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const String &body, Headers &&...headers)
    {
      return std::make_unique<HttpResponse>(status, body, HttpHeadersCollection{std::forward<Headers>(headers)...});
    }
    template <typename... Headers>
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const char *body, Headers &&...headers)
    {
      return std::make_unique<HttpResponse>(status, body, HttpHeadersCollection{std::forward<Headers>(headers)...});
    }
    template <typename... Headers>
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, const uint8_t *body, size_t length, Headers &&...headers)
    {
      return std::make_unique<HttpResponse>(status, body, length, HttpHeadersCollection{std::forward<Headers>(headers)...});
    }
    template <typename... Headers>
    static std::unique_ptr<IHttpResponse> create(HttpStatus status, std::unique_ptr<Stream> body, Headers &&...headers)
    {
      return std::make_unique<HttpResponse>(status, std::move(body), HttpHeadersCollection{std::forward<Headers>(headers)...});
    }

    ~HttpResponse() override = default;

    // void setStatus(HttpStatus status);
    // void setBody(const String &body, const String &contentType = defaultContentType);

    HttpStatus status() const override;
    HttpHeadersCollection &headers() override;
    std::unique_ptr<HttpResponseBodyStream> getBody() override;
  };

  class ChunkedHttpResponseBodyStream : public HttpResponseBodyStream
  {
  private:
    std::unique_ptr<Stream> innerStream_;
    std::array<uint8_t, HTTPServer::CHUNKED_RESPONSE_BUFFER_SIZE> buffer_;
    size_t head_ = 0;
    size_t length_ = 0;
    bool done_ = false;
    bool haveBufferedTerminalChunk_ = false;
    static constexpr size_t finalChunkLength = 6;
    static constexpr const char finalChunk[finalChunkLength] = "0\r\n\r\n";

    void consume();
    constexpr std::size_t hex_length(std::uint64_t n);
    void bufferNextChunk();

  public:
    ChunkedHttpResponseBodyStream(std::unique_ptr<Stream> innerStream);
    static std::unique_ptr<HttpResponseBodyStream> create(std::unique_ptr<Stream> innerStream);
    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual size_t write(uint8_t b) override;
  };

  std::unique_ptr<ReadStream> CreateResponseStream(std::unique_ptr<IHttpResponse> response);

} // namespace HTTPServer
