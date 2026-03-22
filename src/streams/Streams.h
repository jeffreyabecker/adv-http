#pragma once
#include "../compat/Stream.h"
#include <Arduino.h>
#include <functional>
#include <memory>
#include <iterator>
#include <type_traits>
#include <algorithm>
#include <array>
#include <string>
#include "../core/Defines.h"

namespace HttpServerAdvanced
{
    // Stream contract used throughout the HTTP pipeline:
    //  - available() == 0   : end of stream (no more bytes)
    //  - available()  > 0   : that many bytes are buffered and readable now
    //  - available() == -1  : buffer empty but more data is expected (non-final)
    // read() should return -1 only at end of stream; peek() mirrors read() without consuming.

    /**
     * @brief An abstract base class for read-only streams.
     */
    class ReadStream : public Stream
    {
    public:
        using Stream::write;

        virtual ~ReadStream() = default;
        virtual size_t write(uint8_t b) override;
    };

    class EmptyReadStream : public ReadStream
    {
    public:
        virtual int available() override;
        virtual int read() override;
        virtual int peek() override;
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
        OctetsStream(const char *cstr);
        OctetsStream(const uint8_t *data, size_t length, bool ownsData = false);
        OctetsStream(const char *data, size_t length, bool ownsData = false) : OctetsStream(reinterpret_cast<const uint8_t *>(data), length, ownsData) {}
        OctetsStream(const String &str);
        virtual ~OctetsStream();
        virtual int available() override;
        virtual int read() override;
        virtual int peek() override;
    };

    /**
     * @brief A stream that reads from a String.
     */
    class StringStream : public OctetsStream
    {
    private:
        String str_;

    public:
        StringStream(String &&str);
        StringStream(const String &str);
    };

    class StdStringStream : public ReadStream
    {
    private:
        std::string str_;
        size_t position_ = 0;

    public:
        explicit StdStringStream(std::string str);
        virtual int available() override;
        virtual int read() override;
        virtual int peek() override;
    };

    class LazyStreamAdapter : public ReadStream
    {
    private:
        std::function<std::unique_ptr<Stream>()> streamFactory_;
        std::unique_ptr<Stream> stream_;

    public:
        LazyStreamAdapter(std::function<std::unique_ptr<Stream>()> factory);
        virtual ~LazyStreamAdapter() = default;
        virtual int available() override;
        virtual int read() override;
        virtual int peek() override;
    };

    /**
     * @brief A stream that concatenates multiple ReadStreams into one continuous stream.
     */
    template <typename ForwardIt, typename Sentinel = ForwardIt>
    class IndefiniteConcatStream : public ReadStream
    {
    private:
        ForwardIt current_;
        Sentinel end_;

    public:
        IndefiniteConcatStream() = default;

        template <typename FIt = ForwardIt, typename S = Sentinel>
        IndefiniteConcatStream(FIt first, S last) : current_(first), end_(last) {}

    protected:
        void resetRange(ForwardIt first, Sentinel last)
        {
            current_ = first;
            end_ = last;
        }

    public:

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
    template <size_t N>
    class ConcatStream : public IndefiniteConcatStream<typename std::array<std::unique_ptr<Stream>, N>::iterator>
    {
    private:
        std::array<std::unique_ptr<Stream>, N> streams_;

    public:
        ConcatStream(std::array<std::unique_ptr<Stream>, N> streams)
            : streams_(std::move(streams))
        {
            this->resetRange(streams_.begin(), streams_.end());
        }

        ConcatStream(std::initializer_list<std::unique_ptr<Stream>>) = delete;
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

        void compactBuffer();

    public:
        using Stream::write;

        NonOwningMemoryStream(uint8_t *buffer, size_t size);
        virtual ~NonOwningMemoryStream() = default;
        virtual int available() override;
        virtual int read() override;
        virtual int peek() override;
        virtual size_t write(uint8_t b) override;
        virtual void flush() override;
        virtual int availableForWrite() override;
    };

    class MemoryStream : public NonOwningMemoryStream
    {
    private:
        uint8_t *dynamicBuffer_;

    public:
        MemoryStream(size_t size);
        virtual ~MemoryStream() override;
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

        void consume();
        bool fillBuffer();

    public:
        RefBufferedReadStreamWrapper(std::unique_ptr<ReadStream> innerStream, uint8_t *buffer, size_t size);
        virtual int available() override;
        virtual int read() override;
        virtual int peek() override;
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

    String ReadAsString(Stream &stream, size_t maxLength = SIZE_MAX);

    std::vector<uint8_t> ReadAsVector(Stream &stream, size_t maxLength = SIZE_MAX);
}
