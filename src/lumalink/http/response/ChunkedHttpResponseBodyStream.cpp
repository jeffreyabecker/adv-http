#include "ChunkedHttpResponseBodyStream.h"
#include <span>
#include <algorithm>
#include <format>

namespace lumalink::http::response
{
    using lumalink::platform::buffers::AvailableByteCount;
    using lumalink::platform::buffers::HasAvailableBytes;
    using lumalink::platform::buffers::IsExhausted;

    ChunkedHttpResponseBodyStream::ChunkedHttpResponseBodyStream(std::unique_ptr<IByteSource> innerSource)
        : innerSource_(std::move(innerSource)) {}

    std::unique_ptr<IByteSource> ChunkedHttpResponseBodyStream::create(std::unique_ptr<IByteSource> innerSource)
    {
        return std::make_unique<ChunkedHttpResponseBodyStream>(std::move(innerSource));
    }

    void ChunkedHttpResponseBodyStream::prepareHeader()
    {
        // Determine how many bytes we can promise from the inner stream (up to chunkDataSize_)
        const ByteAvailability innerAvail = innerSource_->available();
        if (!HasAvailableBytes(innerAvail))
        {
            // No data or awaiting more; transition to final chunk if truly ended
            if (IsExhausted(innerAvail))
            {
                state_ = State::Final;
                finalPos_ = 0;
                currentChunkIsLast_ = false;
            }
            // If temporarily unavailable or errored, we stay in Header and available() will return -1.
            return;
        }
        chunkRemaining_ = std::min(AvailableByteCount(innerAvail), chunkDataSize_);
        currentChunkIsLast_ = AvailableByteCount(innerAvail) <= chunkDataSize_;
        header_ = std::format("{:x}\r\n", chunkRemaining_);
        headerPos_ = 0;
        state_ = State::Header;
    }

    int ChunkedHttpResponseBodyStream::peekInner() const
    {
        return innerSource_ ? PeekByte(*innerSource_) : -1;
    }

    ByteAvailability ChunkedHttpResponseBodyStream::available()
    {
        switch (state_)
        {
        case State::Header:
            if (header_.empty())
            {
                prepareHeader();
            }
            if (state_ == State::Final)
            {
                return lumalink::platform::buffers::AvailableBytes(sizeof(finalChunk_) - 1 - finalPos_);
            }
            if (header_.empty())
            {
                return lumalink::platform::buffers::TemporarilyUnavailableResult();
            }
            return lumalink::platform::buffers::AvailableBytes((header_.size() - headerPos_) + chunkRemaining_ + (sizeof(trailer_) - 1) + (currentChunkIsLast_ ? (sizeof(finalChunk_) - 1) : 0));
        case State::Body:
            return lumalink::platform::buffers::AvailableBytes(chunkRemaining_ + (sizeof(trailer_) - 1 - trailerPos_) + (currentChunkIsLast_ ? (sizeof(finalChunk_) - 1) : 0));
        case State::Trailer:
            return lumalink::platform::buffers::AvailableBytes((sizeof(trailer_) - 1 - trailerPos_) + (currentChunkIsLast_ ? (sizeof(finalChunk_) - 1) : 0));
        case State::Final:
            return lumalink::platform::buffers::AvailableBytes(sizeof(finalChunk_) - 1 - finalPos_);
        case State::Done:
        default:
            return lumalink::platform::buffers::ExhaustedResult();
        }
    }

    int ChunkedHttpResponseBodyStream::peekSingleByte()
    {
        switch (state_)
        {
        case State::Header:
            if (header_.empty())
            {
                prepareHeader();
            }
            if (state_ == State::Final)
            {
                return static_cast<uint8_t>(finalChunk_[finalPos_]);
            }
            if (header_.empty())
            {
                return -1;
            }
            return static_cast<uint8_t>(header_[headerPos_]);
        case State::Body:
            return peekInner();
        case State::Trailer:
            return static_cast<uint8_t>(trailer_[trailerPos_]);
        case State::Final:
            return static_cast<uint8_t>(finalChunk_[finalPos_]);
        case State::Done:
        default:
            return -1;
        }
    }

    int ChunkedHttpResponseBodyStream::readSingleByte()
    {
        switch (state_)
        {
        case State::Header:
            if (header_.empty())
            {
                prepareHeader();
            }
            if (state_ == State::Final)
            {
                return readSingleByte();
            }
            if (header_.empty())
            {
                return -1;
            }
            {
                int c = static_cast<uint8_t>(header_[headerPos_++]);
                if (headerPos_ >= header_.size())
                {
                    state_ = State::Body;
                }
                return c;
            }
        case State::Body:
        {
            const int c = innerSource_ ? ReadByte(*innerSource_) : -1;
            if (c >= 0 && chunkRemaining_ > 0)
            {
                --chunkRemaining_;
            }
            if (chunkRemaining_ == 0)
            {
                state_ = State::Trailer;
                trailerPos_ = 0;
            }
            return c;
        }
        case State::Trailer:
        {
            int c = static_cast<uint8_t>(trailer_[trailerPos_++]);
            if (trailerPos_ >= sizeof(trailer_) - 1)
            {
                // Prepare next chunk header
                header_.clear();
                state_ = State::Header;
                prepareHeader();
            }
            return c;
        }
        case State::Final:
        {
            int c = static_cast<uint8_t>(finalChunk_[finalPos_++]);
            if (finalPos_ >= sizeof(finalChunk_) - 1)
            {
                state_ = State::Done;
            }
            return c;
        }
        case State::Done:
        default:
            return -1;
        }
    }

    size_t ChunkedHttpResponseBodyStream::read(std::span<uint8_t> buffer)
    {
        size_t totalRead = 0;
        while (totalRead < buffer.size())
        {
            const int value = readSingleByte();
            if (value < 0)
            {
                break;
            }

            buffer[totalRead++] = static_cast<uint8_t>(value);
        }

        return totalRead;
    }

    size_t ChunkedHttpResponseBodyStream::peek(std::span<uint8_t> buffer)
    {
        if (buffer.empty())
        {
            return 0;
        }

        const int value = peekSingleByte();
        if (value < 0)
        {
            return 0;
        }

        buffer[0] = static_cast<uint8_t>(value);
        return 1;
    }

} // namespace lumalink::http::response

