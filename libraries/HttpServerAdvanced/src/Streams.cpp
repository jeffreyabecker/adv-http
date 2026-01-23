#include "Streams.h"
#include <cstring>

namespace HttpServerAdvanced
{
    // ReadStream
    size_t ReadStream::write(uint8_t)
    {
        return 0;
    }

    // EmptyReadStream
    int EmptyReadStream::available()
    {
        return 0;
    }

    int EmptyReadStream::read()
    {
        return -1;
    }

    int EmptyReadStream::peek()
    {
        return -1;
    }

    // OctetsStream
    OctetsStream::OctetsStream(const char *cstr)
        : data_(reinterpret_cast<const uint8_t *>(cstr)), length_(strlen(cstr)), ownsData_(false)
    {
    }

    OctetsStream::OctetsStream(const uint8_t *data, size_t length, bool ownsData)
        : data_(data), length_(length), ownsData_(ownsData)
    {
    }

    OctetsStream::OctetsStream(const String &str)
        : data_(reinterpret_cast<const uint8_t *>(str.c_str())), length_(str.length()), ownsData_(false)
    {
    }

    OctetsStream::~OctetsStream()
    {
        if (ownsData_)
        {
            delete[] data_;
        }
    }

    int OctetsStream::available()
    {
        return static_cast<int>(length_ - position_);
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

    // StringStream
    StringStream::StringStream(String &&str)
        : OctetsStream(str), str_(std::move(str))
    {
    }

    StringStream::StringStream(const String &str)
        : OctetsStream(str), str_() // intentionally empty so we dont copy the stream
    {
    }

    // LazyStreamAdapter
    LazyStreamAdapter::LazyStreamAdapter(std::function<std::unique_ptr<Stream>()> factory)
        : streamFactory_(std::move(factory))
    {
    }



    int LazyStreamAdapter::available()
    {
        if (!stream_)
        {
            stream_ = streamFactory_();
        }
        return stream_->available();
    }

    int LazyStreamAdapter::read()
    {
        if (!stream_)
        {
            stream_ = streamFactory_();
        }
        return stream_->read();
    }

    int LazyStreamAdapter::peek()
    {
        if (!stream_)
        {
            stream_ = streamFactory_();
        }
        return stream_->peek();
    }

    // NonOwningMemoryStream
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
        return static_cast<int>(count_);
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
        return static_cast<int>(size_ - count_);
    }

    // MemoryStream
    MemoryStream::MemoryStream(size_t size)
        : NonOwningMemoryStream(new uint8_t[size], size), dynamicBuffer_(buffer_)
    {
    }

    MemoryStream::~MemoryStream()
    {
        delete[] dynamicBuffer_;
    }

    // RefBufferedReadStreamWrapper
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

    RefBufferedReadStreamWrapper::RefBufferedReadStreamWrapper(std::unique_ptr<ReadStream> innerStream, uint8_t *buffer, size_t size)
        : buffer_(buffer), size_(size), innerStream_(std::move(innerStream))
    {
    }

    int RefBufferedReadStreamWrapper::available()
    {
        return static_cast<int>(count_);
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


    String ReadAsString(Stream &stream, size_t maxLength)
    {
        String result;
        int availableBytes = stream.available();
        if (availableBytes == 0)
        {
            return result;
        }
        else if (availableBytes > 0 && static_cast<size_t>(availableBytes) < maxLength)
        {
            result.reserve(availableBytes);
            maxLength = availableBytes;
        }

        size_t bytesRead = 0;
        while (bytesRead < maxLength)
        {
            int byte = stream.read();
            if (byte == -1)
            {
                break; // End of stream
            }
            result += static_cast<char>(byte);
            bytesRead++;
        }
        return result;
    }

    std::vector<uint8_t> ReadAsVector(Stream &stream, size_t maxLength){
        std::vector<uint8_t> result;
        int availableBytes = stream.available();
        if (availableBytes == 0)
        {
            return result;
        }
        else if (availableBytes > 0 && static_cast<size_t>(availableBytes) < maxLength)
        {
            result.reserve(availableBytes);
            maxLength = availableBytes;
        }

        size_t bytesRead = 0;
        while (bytesRead < maxLength)
        {
            int byte = stream.read();
            if (byte == -1)
            {
                break; // End of stream
            }
            result.push_back(static_cast<uint8_t>(byte));
            bytesRead++;
        }
        return result;
    }    
} // namespace HttpServerAdvanced
