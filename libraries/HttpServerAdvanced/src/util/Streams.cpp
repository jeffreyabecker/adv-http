#include "./Util.h"
#include "./Streams.h"
#include <algorithm>

namespace HttpServerAdvanced
{


    StringStream::StringStream(String &&str)
        : OctetsStream(str), str_(std::move(str))
    {
    }
    StringStream::StringStream(const String &str)
        : OctetsStream(str), str_(/* intentionally empty so we dont copy the stream */)
    {
    }

    void NonOwningMemoryStream::compactBuffer()
    {
        if (readPos_ > 0)
        {
            std::copy(buffer_ + readPos_, buffer_ + readPos_ + count_, buffer_);
            readPos_ = 0;
            writePos_ = count_;
        }
    }

    NonOwningMemoryStream::NonOwningMemoryStream(uint8_t *buffer, size_t size)
        : buffer_(buffer), size_(size)
    {
    }

    int NonOwningMemoryStream::available()
    {
        return count_;
    }

    int NonOwningMemoryStream::read()
    {
        if (count_ == 0)
        {
            return -1;
        }
        uint8_t b = buffer_[readPos_];
        readPos_++;
        count_--;
        return b;
    }

    int NonOwningMemoryStream::peek()
    {
        if (count_ == 0)
        {
            return -1;
        }
        return buffer_[readPos_];
    }

    size_t NonOwningMemoryStream::write(uint8_t b)
    {
        if (count_ >= size_)
        {
            return 0;
        }
        if (writePos_ >= size_)
        {
            compactBuffer();
        }
        buffer_[writePos_] = b;
        writePos_++;
        count_++;
        return 1;
    }

    void NonOwningMemoryStream::flush()
    {
        // No-op for buffered stream
    }

    int NonOwningMemoryStream::availableForWrite()
    {
        return size_ - count_;
    }

    int OctetsStream::available()
    {
        return length_ - position_;
    }

    int OctetsStream::read()
    {
        if (position_ >= length_)
        {
            return -1;
        }
        return data_[position_++];
    }

    int OctetsStream::peek()
    {
        if (position_ >= length_)
        {
            return -1;
        }
        return data_[position_];
    }

    int LazyStreamAdapter::available()
    {
        if (!stream_)
            stream_ = streamFactory_();
        return stream_->available();
    }

    int LazyStreamAdapter::read()
    {
        if (!stream_)
            stream_ = streamFactory_();
        return stream_->read();
    }

    int LazyStreamAdapter::peek()
    {
        if (!stream_)
            stream_ = streamFactory_();
        return stream_->peek();
    }

    void RefBufferedReadStreamWrapper::consume()
    {
        if (count_ > 0)
        {
            readPos_ = (readPos_ + 1) % size_;
            count_--;
        }
    }

    bool RefBufferedReadStreamWrapper::fillBuffer()
    {
        if (readPos_ != 0)
        {
            // Compact the buffer
            size_t unreadBytes = count_;
            for (size_t i = 0; i < unreadBytes; ++i)
            {
                buffer_[i] = buffer_[(readPos_ + i) % size_];
            }
            readPos_ = 0;
            writePos_ = unreadBytes;
            count_ = unreadBytes;
        }
        while (count_ < size_)
        {
            int val = innerStream_->read();
            if (val == -1)
            {
                break;
            }
            buffer_[writePos_] = static_cast<uint8_t>(val);
            writePos_ = (writePos_ + 1) % size_;
            count_++;
        }
        return count_ > 0;
    }

    int RefBufferedReadStreamWrapper::available()
    {
        return count_;
    }

    int RefBufferedReadStreamWrapper::read()
    {
        int val = peek();
        if (val != -1)
        {
            consume();
        }
        return val;
    }

    int RefBufferedReadStreamWrapper::peek()
    {
        if (count_ == 0)
        {
            if (!fillBuffer())
            {
                return -1;
            }
        }
        return buffer_[readPos_];
    }
}