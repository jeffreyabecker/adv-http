#pragma once
#include <memory>
#include "../transport/ByteStream.h"
#include "../core/Defines.h"

namespace httpadv::v1::response
{
  using httpadv::v1::transport::AvailableResult;
  using httpadv::v1::transport::IByteSource;


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
  class ChunkedHttpResponseBodyStream : public IByteSource
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

    static constexpr size_t chunkDataSize_ = httpadv::v1::core::ETHERNET_FRAME_BUFFER_SIZE;
    static constexpr const char trailer_[3] = "\r\n";
    static constexpr const char finalChunk_[6] = "0\r\n\r\n";

    void prepareHeader();
    int peekInner() const;
    int readSingleByte();
    int peekSingleByte();

  protected:
    std::unique_ptr<IByteSource> innerSource_;

  public:
    explicit ChunkedHttpResponseBodyStream(std::unique_ptr<IByteSource> innerSource);
    static std::unique_ptr<IByteSource> create(std::unique_ptr<IByteSource> innerSource);
    AvailableResult available() override;
    size_t read(httpadv::v1::util::span<uint8_t> buffer) override;
    size_t peek(httpadv::v1::util::span<uint8_t> buffer) override;
  };

} // namespace httpadv::v1::response

