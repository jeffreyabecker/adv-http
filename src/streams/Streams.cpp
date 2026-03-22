#include "Streams.h"
#include <cstring>

namespace HttpServerAdvanced
{
    // EmptyReadStream
    AvailableResult EmptyReadStream::available()
    {
        return ExhaustedResult();
    }

    int EmptyReadStream::readSingleByte()
    {
        return -1;
    }

    int EmptyReadStream::peekSingleByte()
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

    AvailableResult OctetsStream::available()
    {
        if (position_ >= length_)
        {
            return ExhaustedResult();
        }

        return AvailableBytes(length_ - position_);
    }

    int OctetsStream::readSingleByte()
    {
        if (position_ >= length_)
        {
            return -1;
        }
        return data_[position_++];
    }

    int OctetsStream::peekSingleByte()
    {
        if (position_ >= length_)
        {
            return -1;
        }
        return data_[position_];
    }

    // StringStream
    StringStream::StringStream(String &&str)
        : OctetsStream(static_cast<const uint8_t *>(nullptr), 0), str_(std::move(str))
    {
        data_ = reinterpret_cast<const uint8_t *>(str_.c_str());
        length_ = str_.length();
    }

    StringStream::StringStream(const String &str)
        : OctetsStream(static_cast<const uint8_t *>(nullptr), 0), str_(str)
    {
        data_ = reinterpret_cast<const uint8_t *>(str_.c_str());
        length_ = str_.length();
    }

    StdStringStream::StdStringStream(std::string str)
        : str_(std::move(str))
    {
    }

    AvailableResult StdStringStream::available()
    {
        if (position_ >= str_.size())
        {
            return ExhaustedResult();
        }

        return AvailableBytes(str_.size() - position_);
    }

    int StdStringStream::readSingleByte()
    {
        if (position_ >= str_.size())
        {
            return -1;
        }

        return static_cast<uint8_t>(str_[position_++]);
    }

    int StdStringStream::peekSingleByte()
    {
        if (position_ >= str_.size())
        {
            return -1;
        }

        return static_cast<uint8_t>(str_[position_]);
    }

    // LazyStreamAdapter
    LazyStreamAdapter::LazyStreamAdapter(std::function<std::unique_ptr<IByteSource>()> factory)
        : streamFactory_(std::move(factory))
    {
    }

    IByteSource *LazyStreamAdapter::ensureStream()
    {
        if (!stream_)
        {
            stream_ = streamFactory_();
        }

        return stream_.get();
    }

    AvailableResult LazyStreamAdapter::available()
    {
        IByteSource *source = ensureStream();
        return source != nullptr ? source->available() : ExhaustedResult();
    }

    int LazyStreamAdapter::readSingleByte()
    {
        IByteSource *source = ensureStream();
        return source != nullptr ? ReadByte(*source) : -1;
    }

    int LazyStreamAdapter::peekSingleByte()
    {
        IByteSource *source = ensureStream();
        return source != nullptr ? PeekByte(*source) : -1;
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
            int val = ReadByte(*innerStream_);
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

    RefBufferedReadStreamWrapper::RefBufferedReadStreamWrapper(std::unique_ptr<IByteSource> innerStream, uint8_t *buffer, size_t size)
        : buffer_(buffer), size_(size), innerStream_(std::move(innerStream))
    {
    }

    AvailableResult RefBufferedReadStreamWrapper::available()
    {
        return count_ > 0 ? AvailableBytes(count_) : ExhaustedResult();
    }

    int RefBufferedReadStreamWrapper::readSingleByte()
    {
        int val = peekSingleByte();
        if (val != -1)
        {
            consume();
        }
        return val;
    }

    int RefBufferedReadStreamWrapper::peekSingleByte()
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


    String ReadAsString(IByteSource &stream, size_t maxLength)
    {
        String result;
        AvailableResult availableBytes = stream.available();
        if (availableBytes.isExhausted())
        {
            return result;
        }
        else if (availableBytes.hasBytes() && availableBytes.count < maxLength)
        {
            result.reserve(availableBytes.count);
            maxLength = availableBytes.count;
        }

        size_t bytesRead = 0;
        uint8_t buffer[256] = {};
        while (bytesRead < maxLength)
        {
            const size_t chunkSize = (std::min)(sizeof(buffer), maxLength - bytesRead);
            const size_t readNow = stream.read(HttpServerAdvanced::span<uint8_t>(buffer, chunkSize));
            if (readNow == 0)
            {
                break; // End of stream
            }

            for (size_t index = 0; index < readNow; ++index)
            {
                result += static_cast<char>(buffer[index]);
            }

            bytesRead += readNow;
        }
        return result;
    }
} // namespace HttpServerAdvanced
