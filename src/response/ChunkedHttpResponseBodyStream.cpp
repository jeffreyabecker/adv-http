#include "ChunkedHttpResponseBodyStream.h"
#include <cstdio>
#include <algorithm>

namespace HttpServerAdvanced
{

    ChunkedHttpResponseBodyStream::ChunkedHttpResponseBodyStream(std::unique_ptr<IByteSource> innerSource)
        : HttpResponseBodyStream(std::move(innerSource)) {}

    ChunkedHttpResponseBodyStream::ChunkedHttpResponseBodyStream(std::unique_ptr<Stream> innerStream)
        : HttpResponseBodyStream(std::move(innerStream)) {}

    std::unique_ptr<HttpResponseBodyStream> ChunkedHttpResponseBodyStream::create(std::unique_ptr<IByteSource> innerSource)
    {
        return std::make_unique<ChunkedHttpResponseBodyStream>(std::move(innerSource));
    }

    std::unique_ptr<HttpResponseBodyStream> ChunkedHttpResponseBodyStream::create(std::unique_ptr<Stream> innerStream)
    {
        return std::make_unique<ChunkedHttpResponseBodyStream>(std::move(innerStream));
    }

    void ChunkedHttpResponseBodyStream::prepareHeader()
    {
        // Determine how many bytes we can promise from the inner stream (up to chunkDataSize_)
        const AvailableResult innerAvail = innerSource_->available();
        if (innerAvail <= 0)
        {
            // No data or awaiting more; transition to final chunk if truly ended
            if (innerAvail.isExhausted())
            {
                state_ = State::Final;
                finalPos_ = 0;
            }
            // If temporarily unavailable or errored, we stay in Header and available() will return -1.
            return;
        }
        chunkRemaining_ = std::min(innerAvail.count, chunkDataSize_);
        headerLen_ = static_cast<size_t>(std::snprintf(headerBuf_, sizeof(headerBuf_), "%zx\r\n", chunkRemaining_));
        headerPos_ = 0;
        state_ = State::Header;
    }

    int ChunkedHttpResponseBodyStream::peekInner() const
    {
        return innerSource_->peek();
    }

    int ChunkedHttpResponseBodyStream::available()
    {
        switch (state_)
        {
        case State::Header:
            if (headerLen_ == 0)
            {
                prepareHeader();
            }
            if (state_ == State::Final)
            {
                return static_cast<int>(sizeof(finalChunk_) - 1 - finalPos_);
            }
            if (headerLen_ == 0)
            {
                // Inner stream returned -1 (awaiting)
                return -1;
            }
            return static_cast<int>(headerLen_ - headerPos_);
        case State::Body:
            return static_cast<int>(chunkRemaining_);
        case State::Trailer:
            return static_cast<int>(sizeof(trailer_) - 1 - trailerPos_);
        case State::Final:
            return static_cast<int>(sizeof(finalChunk_) - 1 - finalPos_);
        case State::Done:
        default:
            return 0;
        }
    }

    int ChunkedHttpResponseBodyStream::peek()
    {
        switch (state_)
        {
        case State::Header:
            if (headerLen_ == 0)
            {
                prepareHeader();
            }
            if (state_ == State::Final)
            {
                return static_cast<uint8_t>(finalChunk_[finalPos_]);
            }
            if (headerLen_ == 0)
            {
                return -1;
            }
            return static_cast<uint8_t>(headerBuf_[headerPos_]);
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

    int ChunkedHttpResponseBodyStream::read()
    {
        switch (state_)
        {
        case State::Header:
            if (headerLen_ == 0)
            {
                prepareHeader();
            }
            if (state_ == State::Final)
            {
                return read(); // recurse into Final state
            }
            if (headerLen_ == 0)
            {
                return -1;
            }
            {
                int c = static_cast<uint8_t>(headerBuf_[headerPos_++]);
                if (headerPos_ >= headerLen_)
                {
                    state_ = State::Body;
                }
                return c;
            }
        case State::Body:
        {
            int c = innerSource_->read();
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
                headerLen_ = 0;
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

    size_t ChunkedHttpResponseBodyStream::write(uint8_t b)
    {
        (void)b;
        return 0;
    }

} // namespace HttpServerAdvanced

