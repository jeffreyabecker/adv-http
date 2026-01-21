#pragma once
#include <array>
#include <memory>
#include "./HttpResponseBodyStream.h"
#include "./Defines.h"

namespace HttpServerAdvanced
{

  class ChunkedHttpResponseBodyStream : public HttpResponseBodyStream
  {
  private:
    std::unique_ptr<Stream> innerStream_;
    std::array<uint8_t, HttpServerAdvanced::CHUNKED_RESPONSE_BUFFER_SIZE> buffer_;
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

  // ========== Implementations ==========

  // ChunkedHttpResponseBodyStream implementation
  inline void ChunkedHttpResponseBodyStream::consume()
  {
    if (length_ > 0)
    {
      --length_;
      ++head_;
      if (length_ == 0)
      {
        head_ = 0;
      }
    }
  }

  inline constexpr std::size_t ChunkedHttpResponseBodyStream::hex_length(std::uint64_t n)
  {
    // At least one digit, even for 0.
    std::size_t len = 1;
    while (n >= 0x10)
    {
      n /= 0x10;
      ++len;
    }
    return len;
  }

  inline void ChunkedHttpResponseBodyStream::bufferNextChunk()
  {
    if (haveBufferedTerminalChunk_)
    {
      return;
    }

    size_t dataOffset = hex_length(HttpServerAdvanced::CHUNKED_RESPONSE_BUFFER_SIZE) + 3;          // +3 for \r\n and null terminator
    size_t maxInnerReadLength = HttpServerAdvanced::CHUNKED_RESPONSE_BUFFER_SIZE - dataOffset - 2; // -2 for trailing CRLF
    size_t bytesRead = 0;
    while (bytesRead < maxInnerReadLength)
    {
      int byte = innerStream_->read();
      if (byte == -1)
      {
        break;
      }
      buffer_[bytesRead] = static_cast<uint8_t>(byte);
      ++bytesRead;
    }
    if (bytesRead == 0)
    {
      // Buffer final chunk
      for (size_t i = 0; i < finalChunkLength; i++)
      {
        buffer_[i] = finalChunk[i];
      }
      length_ = finalChunkLength;
      head_ = 0;
      haveBufferedTerminalChunk_ = true;
      return;
    }
    else
    {
      auto printed = snprintf(reinterpret_cast<char *>(buffer_.data()), dataOffset, "%zx\r\n", bytesRead);
      auto toRemove = dataOffset - printed;
      if (toRemove > 0)
      {
        // Shift data to remove unused space
        for (size_t i = 0; i < bytesRead; ++i)
        {
          buffer_[printed + i] = buffer_[dataOffset + i];
        }
        length_ = printed + bytesRead + 2; // +2 for trailing CRLF
        head_ = 0;
      }
      else
      {
        length_ = dataOffset + bytesRead + 2; // +2 for trailing CRLF
        head_ = 0;
      }
    }
  }

  inline ChunkedHttpResponseBodyStream::ChunkedHttpResponseBodyStream(std::unique_ptr<Stream> innerStream)
      : HttpResponseBodyStream(std::move(innerStream))
  {
  }

  inline int ChunkedHttpResponseBodyStream::available()
  {
    if (done_)
    {
      return 0;
    }
    return -1;
  }

  inline int ChunkedHttpResponseBodyStream::read()
  {
    int result = peek();
    consume();
    return result;
  }

  inline int ChunkedHttpResponseBodyStream::peek()
  {
    if (done_)
    {
      return -1;
    }
    if (length_ == 0)
    {
      bufferNextChunk();
    }
    if (length_ == 0)
    {
      done_ = true;
      return -1;
    }
    return buffer_[head_];
  }

  inline size_t ChunkedHttpResponseBodyStream::write(uint8_t b)
  {
    return 0;
  }

  inline std::unique_ptr<HttpResponseBodyStream> ChunkedHttpResponseBodyStream::create(std::unique_ptr<Stream> innerStream)
  {
    return std::make_unique<ChunkedHttpResponseBodyStream>(std::move(innerStream));
  }

} // namespace HttpServerAdvanced
