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

  // Implementations moved to .cpp

} // namespace HttpServerAdvanced
