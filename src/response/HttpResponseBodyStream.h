#pragma once
#include <memory>
#include "../streams/ByteStream.h"

namespace HttpServerAdvanced
{

  class HttpResponseBodyStream : public IByteSource
  {
  protected:
    std::unique_ptr<IByteSource> innerSource_;

  public:
    explicit HttpResponseBodyStream(std::unique_ptr<IByteSource> innerSource);

    AvailableResult available() override;
    size_t read(HttpServerAdvanced::span<uint8_t> buffer) override;
    size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override;

    static std::unique_ptr<HttpResponseBodyStream> create(std::unique_ptr<IByteSource> innerSource) {
      return std::make_unique<HttpResponseBodyStream>(std::move(innerSource));
    }

  };

  // ========== Implementations ==========

  // HttpResponseBodyStream constructor
  inline HttpResponseBodyStream::HttpResponseBodyStream(std::unique_ptr<IByteSource> innerSource)
      : innerSource_(std::move(innerSource)) {}

  inline AvailableResult HttpResponseBodyStream::available()
  {
    return innerSource_ ? innerSource_->available() : ExhaustedResult();
  }

  inline size_t HttpResponseBodyStream::read(HttpServerAdvanced::span<uint8_t> buffer)
  {
    return innerSource_ ? innerSource_->read(buffer) : 0;
  }

  inline size_t HttpResponseBodyStream::peek(HttpServerAdvanced::span<uint8_t> buffer)
  {
    return innerSource_ ? innerSource_->peek(buffer) : 0;
  }

} // namespace HttpServerAdvanced


