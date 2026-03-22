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

    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual size_t write(uint8_t b) override;

    static std::unique_ptr<HttpResponseBodyStream> create(std::unique_ptr<IByteSource> innerSource) {
      return std::make_unique<HttpResponseBodyStream>(std::move(innerSource));
    }

  };

  // ========== Implementations ==========

  // HttpResponseBodyStream constructor
  inline HttpResponseBodyStream::HttpResponseBodyStream(std::unique_ptr<IByteSource> innerSource)
      : innerSource_(std::move(innerSource)) {}

  inline int HttpResponseBodyStream::available()
  {
    return innerSource_ ? LegacyAvailableFromResult(innerSource_->available()) : 0;
  }

  inline int HttpResponseBodyStream::read()
  {
    if (!innerSource_)
    {
      return -1;
    }

    uint8_t buffer[1] = {};
    return innerSource_->read(buffer) == 0 ? -1 : buffer[0];
  }

  inline int HttpResponseBodyStream::peek()
  {
    if (!innerSource_)
    {
      return -1;
    }

    uint8_t buffer[1] = {};
    return innerSource_->peek(buffer) == 0 ? -1 : buffer[0];
  }

  inline size_t HttpResponseBodyStream::write(uint8_t b)
  {
    return 0;
  }

} // namespace HttpServerAdvanced


