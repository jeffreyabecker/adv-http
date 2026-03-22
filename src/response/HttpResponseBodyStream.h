#pragma once
#include <memory>
#include "../streams/ByteStream.h"
#include "../compat/Stream.h"
#include <Arduino.h>
#include "../streams/Streams.h"

namespace HttpServerAdvanced
{

  class HttpResponseBodyStream : public ReadStream
  {
  protected:
    std::unique_ptr<IByteSource> innerSource_;

  public:
    using ReadStream::write;

    explicit HttpResponseBodyStream(std::unique_ptr<IByteSource> innerSource);
    explicit HttpResponseBodyStream(std::unique_ptr<Stream> innerStream);
    HttpResponseBodyStream(const String &str) : HttpResponseBodyStream(std::make_unique<OwningStreamByteSourceAdapter>(std::make_unique<HttpServerAdvanced::StringStream>(str))) {}
    HttpResponseBodyStream(String &&str) : HttpResponseBodyStream(std::make_unique<OwningStreamByteSourceAdapter>(std::make_unique<HttpServerAdvanced::StringStream>(std::move(str)))) {}
    HttpResponseBodyStream(const char *cstr) : HttpResponseBodyStream(std::make_unique<OwningStreamByteSourceAdapter>(std::make_unique<HttpServerAdvanced::OctetsStream>(cstr))) {}
    HttpResponseBodyStream(const uint8_t *data, size_t length, bool ownsData = false) : HttpResponseBodyStream(std::make_unique<OwningStreamByteSourceAdapter>(std::make_unique<HttpServerAdvanced::OctetsStream>(data, length, ownsData))) {}

    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual size_t write(uint8_t b) override;

    static std::unique_ptr<HttpResponseBodyStream> create(std::unique_ptr<IByteSource> innerSource) {
      return std::make_unique<HttpResponseBodyStream>(std::move(innerSource));
    }
    static std::unique_ptr<HttpResponseBodyStream> create(std::unique_ptr<Stream> innerStream) {
      return std::make_unique<HttpResponseBodyStream>(std::move(innerStream));
    }
    static std::unique_ptr<HttpResponseBodyStream> create(const String &str) {
      return std::make_unique<HttpResponseBodyStream>(str);
    }
    static std::unique_ptr<HttpResponseBodyStream> create(String &&str) {
      return std::make_unique<HttpResponseBodyStream>(std::move(str));
    }
    static std::unique_ptr<HttpResponseBodyStream> create(const char *cstr) {
      return std::make_unique<HttpResponseBodyStream>(cstr);
    }
    static std::unique_ptr<HttpResponseBodyStream> create(const uint8_t *data, size_t length, bool ownsData = false) {
      return std::make_unique<HttpResponseBodyStream>(data, length, ownsData);
    }

  };

  // ========== Implementations ==========

  // HttpResponseBodyStream constructor
  inline HttpResponseBodyStream::HttpResponseBodyStream(std::unique_ptr<IByteSource> innerSource)
      : innerSource_(std::move(innerSource)) {}

  inline HttpResponseBodyStream::HttpResponseBodyStream(std::unique_ptr<Stream> innerStream)
      : innerSource_(std::make_unique<OwningStreamByteSourceAdapter>(std::move(innerStream))) {}

  inline int HttpResponseBodyStream::available()
  {
    return innerSource_ ? LegacyAvailableFromResult(innerSource_->available()) : 0;
  }

  inline int HttpResponseBodyStream::read()
  {
    return innerSource_ ? innerSource_->read() : -1;
  }

  inline int HttpResponseBodyStream::peek()
  {
    return innerSource_ ? innerSource_->peek() : -1;
  }

  inline size_t HttpResponseBodyStream::write(uint8_t b)
  {
    return 0;
  }

} // namespace HttpServerAdvanced


