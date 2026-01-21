#pragma once
#include <Arduino.h>
#include <functional>
#include <memory>
#include <iterator>
#include <type_traits>
#include <algorithm>
#include <array>
#include "./Defines.h"

namespace HttpServerAdvanced
{
    /**
     * @brief An abstract base class for read-only streams.
     */
    class ReadStream : public Stream
    {
    public:
        virtual ~ReadStream() = default;
        virtual size_t write(uint8_t b) override { return 0; }
    };

    class EmptyReadStream : public ReadStream
    {
    public:
        virtual int available() override { return 0; }
        virtual int read() override { return -1; }
        virtual int peek() override { return -1; }
    };
    /**
     * @brief A stream that reads from a constant C-style string.
     */
    class OctetsStream : public ReadStream
    {
    private:
        const uint8_t *data_;
        size_t length_;
        size_t position_ = 0;
        bool ownsData_ = false;

    public:
        OctetsStream(const char *cstr)
            : data_(reinterpret_cast<const uint8_t *>(cstr)), length_(strlen(cstr)), ownsData_(false) {}
        OctetsStream(const uint8_t *data, size_t length, bool ownsData = false)
            : data_(data), length_(length), ownsData_(ownsData) {}
        OctetsStream(const String& str)
            : data_(reinterpret_cast<const uint8_t *>(str.c_str())), length_(str.length()), ownsData_(false) {}
        virtual ~OctetsStream()
        {
            if (ownsData_)
            {
                delete[] data_;
            }
        }
        virtual int available() override
        {
            return length_ - position_;
        }
        virtual int read() override
        {
            if (position_ >= length_)
            {
                return -1;
            }
            return data_[position_++];
        }
        virtual int peek() override
        {
            if (position_ >= length_)
            {
                return -1;
            }
            return data_[position_];
        }
    };



    /**
     * @brief A stream that reads from a String.
     */
    class StringStream : public OctetsStream
    {
    private:
        String str_;
    public:
        StringStream(String &&str) : OctetsStream(str), str_(std::move(str)) {}
        StringStream(const String &str) : OctetsStream(str), str_(/* intentionally empty so we dont copy the stream */) {}
    };

    class LazyStreamAdapter : public ReadStream
    {
    private:
        std::function<std::unique_ptr<ReadStream>()> streamFactory_;
        std::unique_ptr<ReadStream> stream_;

    public:
        LazyStreamAdapter(std::function<std::unique_ptr<ReadStream>()> factory) : streamFactory_(factory) {}
        LazyStreamAdapter(std::function<ReadStream *()> factory) : streamFactory_([factory]()
                                                                                  { return std::unique_ptr<ReadStream>(factory()); }) {}
        virtual ~LazyStreamAdapter() = default;
        virtual int available() override
        {
            if (!stream_)
                stream_ = streamFactory_();
            return stream_->available();
        }
        virtual int read() override
        {
            if (!stream_)
                stream_ = streamFactory_();
            return stream_->read();
        }
        virtual int peek() override
        {
            if (!stream_)
                stream_ = streamFactory_();
            return stream_->peek();
        }
    };


    /**
     * @brief A stream that concatenates multiple ReadStreams into one continuous stream.
     */
    template<typename ForwardIt, typename Sentinel = ForwardIt>
    class IndefiniteConcatStream : public ReadStream
    {
    private:
        ForwardIt current_;
        Sentinel end_;

    public:
        template<typename FIt = ForwardIt, typename S = Sentinel,
                 typename = std::enable_if_t<std::is_same<typename std::iterator_traits<FIt>::value_type, std::unique_ptr<Stream>>::value &&
                                             std::is_base_of<std::forward_iterator_tag, typename std::iterator_traits<FIt>::iterator_category>::value>>
        IndefiniteConcatStream(FIt first, S last) : current_(first), end_(last) {}

        virtual int available() override
        {
            while (current_ != end_ && (*current_)->available() == 0)
            {
                ++current_;
            }
            if (current_ != end_)
            {
                return (*current_)->available();
            }
            return 0;
        }

        virtual int read() override
        {
            while (current_ != end_)
            {
                int val = (*current_)->read();
                if (val != -1)
                {
                    return val;
                }
                ++current_;
            }
            return -1;
        }

        virtual int peek() override
        {
            while (current_ != end_)
            {
                int val = (*current_)->peek();
                if (val != -1)
                {
                    return val;
                }
                ++current_;
            }
            return -1;
        }
    };
    /**
     * @brief A convenience class for concatenating streams from a vector of ReadStreams.
     */
    template<size_t N>
    class ConcatStream : public IndefiniteConcatStream<std::array<std::unique_ptr<Stream>, N>::iterator>
    {
    private:
        std::array<std::unique_ptr<Stream>> streams_;

    public:
        ConcatStream(std::array<std::unique_ptr<Stream>> streams)
            : IndefiniteConcatStream<std::array<std::unique_ptr<Stream>>::iterator>(streams.begin(), streams.end()),
              streams_(std::move(streams)) {}
    };

    /**
     * @brief A stream that uses a user-provided buffer for read and write operations.
     */
    class NonOwningMemoryStream : public Stream
    {
    protected:
        uint8_t *buffer_;
        size_t size_;
        size_t readPos_ = 0;
        size_t writePos_ = 0;
        size_t count_ = 0;

        void compactBuffer()
        {
            if (readPos_ > 0)
            {
                std::copy(buffer_ + readPos_, buffer_ + readPos_ + count_, buffer_);
                readPos_ = 0;
                writePos_ = count_;
            }
        }

    public:
        NonOwningMemoryStream(uint8_t *buffer, size_t size)
            : buffer_(buffer), size_(size) {}
        virtual ~NonOwningMemoryStream() = default;
        virtual int available() override
        {
            return count_;
        }
        virtual int read() override
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
        virtual int peek() override
        {
            if (count_ == 0)
            {
                return -1;
            }
            return buffer_[readPos_];
        }
        virtual size_t write(uint8_t b) override
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
        virtual void flush() override
        {
            // No-op for buffered stream
        }
        virtual int availableForWrite() override
        {
            return size_ - count_;
        }
    };

    class MemoryStream : public NonOwningMemoryStream
    {
    private:
        uint8_t *dynamicBuffer_;

    public:
        MemoryStream(size_t size) : NonOwningMemoryStream(new uint8_t[size], size), dynamicBuffer_(buffer_) {}
        virtual ~MemoryStream() override
        {
            delete[] dynamicBuffer_;
        }
    };

    /**
     * @brief A stream that uses a statically allocated buffer for read and write operations.
     */
    template <size_t N>
    class StaticMemoryStream : public NonOwningMemoryStream
    {
    private:
        uint8_t staticBuffer_[N];

    public:
        StaticMemoryStream() : NonOwningMemoryStream(staticBuffer_, N) {}
    };

    class RefBufferedReadStreamWrapper : public ReadStream
    {
    private:
        uint8_t *buffer_;
        size_t size_;
        size_t readPos_ = 0;
        size_t writePos_ = 0;
        size_t count_ = 0;
        std::unique_ptr<ReadStream> innerStream_;

        void consume()
        {
            if (count_ > 0)
            {
                readPos_ = (readPos_ + 1) % size_;
                count_--;
            }
        }
        bool fillBuffer()
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

    public:
        RefBufferedReadStreamWrapper(std::unique_ptr<ReadStream> innerStream, uint8_t *buffer, size_t size)
            : innerStream_(std::move(innerStream)), buffer_(buffer), size_(size) {}
        virtual int available() override
        {
            return count_;
        }
        virtual int read() override
        {
            int val = peek();
            if (val != -1)
            {
                consume();
            }
            return val;
        }
        virtual int peek() override
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
    };

    template <size_t N>
    class StaticBufferedReadStreamWrapper : public RefBufferedReadStreamWrapper
    {
    private:
        uint8_t buffer_[N];

    public:
        StaticBufferedReadStreamWrapper(std::unique_ptr<ReadStream> innerStream)
            : RefBufferedReadStreamWrapper(std::move(innerStream), buffer_, N) {}
    };
}