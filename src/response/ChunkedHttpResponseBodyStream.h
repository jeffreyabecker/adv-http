#pragma once
#include <memory>
#include "HttpResponseBodyStream.h"
#include "../core/Defines.h"

namespace HttpServerAdvanced
{

  /**
   * @brief Wraps a stream and emits HTTP chunked transfer encoding on-the-fly.
   *
   * This implementation avoids buffering entire chunks. It reads a fixed-size
   * "window" of inner stream bytes and emits: hex-length CRLF <data> CRLF,
   * repeating until the inner stream is exhausted, then emits the final "0\r\n\r\n".
   *
   * State machine:
   *   Header  -> emitting "XX\r\n" (chunk size in hex + CRLF)
   *   Body    -> streaming inner bytes (chunkRemaining_ bytes)
   *   Trailer -> emitting "\r\n"
   *   Final   -> emitting "0\r\n\r\n"
   *   Done    -> available() == 0
   */
  class ChunkedHttpResponseBodyStream : public HttpResponseBodyStream
  {
  private:
    enum class State { Header, Body, Trailer, Final, Done };

    State state_ = State::Header;
    size_t chunkRemaining_ = 0;       ///< bytes left in current body phase
    bool currentChunkIsLast_ = false;
    char headerBuf_[12] = {};         ///< "XXXX\r\n" (max 8 hex digits + CRLF + NUL)
    size_t headerPos_ = 0;
    size_t headerLen_ = 0;
    size_t trailerPos_ = 0;
    size_t finalPos_ = 0;

    static constexpr size_t chunkDataSize_ = HttpServerAdvanced::ETHERNET_FRAME_BUFFER_SIZE;
    static constexpr const char trailer_[3] = "\r\n";
    static constexpr const char finalChunk_[6] = "0\r\n\r\n";

    void prepareHeader();
    int peekInner() const;

  public:
    using HttpResponseBodyStream::write;

    explicit ChunkedHttpResponseBodyStream(std::unique_ptr<IByteSource> innerSource);
    static std::unique_ptr<HttpResponseBodyStream> create(std::unique_ptr<IByteSource> innerSource);
    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual size_t write(uint8_t b) override;
  };

} // namespace HttpServerAdvanced

