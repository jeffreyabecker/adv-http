#pragma once
#include "../compat/Stream.h"
#include "ByteStream.h"
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
    // Legacy Stream contract as currently used by the repository:
    //  - available()  > 0 : that many bytes are readable now.
    //  - available() == 0 : terminal exhaustion for finite sources and the
    //                       value concat helpers skip past.
    //  - available()  < 0 : adapter/wrapper-only "temporarily unavailable";
    //                       existing helpers do not preserve this state
    //                       consistently.
    // read() and peek() return the next byte or -1 when no byte is produced
    // for the current call. Most current callers treat -1 as end-of-stream.

    /**
     * @brief An abstract base class for read-only streams.
     */
    class ReadStream : public SingleByteSource
    {
    public:
        virtual ~ReadStream() = default;
    };

    class EmptyReadStream : public ReadStream
    {
    public:
        AvailableResult available() override;

    protected:
        int readSingleByte() override;
        int peekSingleByte() override;
    };
    /**
     * @brief A stream that reads from a constant C-style string.
     */
    class OctetsStream : public ReadStream
    {
    protected:
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
        AvailableResult available() override;

    protected:
        int readSingleByte() override;
        int peekSingleByte() override;
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
        AvailableResult available() override;

    protected:
        int readSingleByte() override;
        int peekSingleByte() override;
    };

    class LazyStreamAdapter : public ReadStream
    {
    private:
        std::function<std::unique_ptr<IByteSource>()> streamFactory_;
        std::unique_ptr<IByteSource> stream_;
        IByteSource *ensureStream();

    public:
        LazyStreamAdapter(std::function<std::unique_ptr<IByteSource>()> factory);
        virtual ~LazyStreamAdapter() = default;
        AvailableResult available() override;

    protected:
        int readSingleByte() override;
        int peekSingleByte() override;
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

        AvailableResult available() override
        {
            while (current_ != end_)
            {
                const AvailableResult currentAvailable = (*current_)->available();
                if (currentAvailable.isExhausted())
                {
                    ++current_;
                    continue;
                }

                return currentAvailable;
            }

            return ExhaustedResult();
        }

    protected:
        int readSingleByte() override
        {
            while (current_ != end_)
            {
                int val = ReadByte(*(*current_));
                if (val != -1)
                {
                    return val;
                }
                ++current_;
            }
            return -1;
        }

        int peekSingleByte() override
        {
            while (current_ != end_)
            {
                int val = PeekByte(*(*current_));
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
    class ConcatStream : public IndefiniteConcatStream<typename std::array<std::unique_ptr<IByteSource>, N>::iterator>
    {
    private:
        std::array<std::unique_ptr<IByteSource>, N> streams_;

    public:
        ConcatStream(std::array<std::unique_ptr<IByteSource>, N> streams)
            : streams_(std::move(streams))
        {
            this->resetRange(streams_.begin(), streams_.end());
        }

        ConcatStream(std::initializer_list<std::unique_ptr<IByteSource>>) = delete;
    };

    /**
     * @brief Legacy duplex stream backed by caller-owned storage.
     *
     * This is intentionally the only retained in-tree writable stream helper.
     * Owning memory-stream variants were removed once the response pipeline
     * stopped depending on them internally.
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

    class RefBufferedReadStreamWrapper : public ReadStream
    {
    private:
        uint8_t *buffer_;
        size_t size_;
        size_t readPos_ = 0;
        size_t writePos_ = 0;
        size_t count_ = 0;
        std::unique_ptr<IByteSource> innerStream_;

        void consume();
        bool fillBuffer();

    public:
        RefBufferedReadStreamWrapper(std::unique_ptr<IByteSource> innerStream, uint8_t *buffer, size_t size);
        AvailableResult available() override;

    protected:
        int readSingleByte() override;
        int peekSingleByte() override;
    };

    template <size_t N>
    class StaticBufferedReadStreamWrapper : public RefBufferedReadStreamWrapper
    {
    private:
        uint8_t buffer_[N];

    public:
        StaticBufferedReadStreamWrapper(std::unique_ptr<IByteSource> innerStream)
            : RefBufferedReadStreamWrapper(std::move(innerStream), buffer_, N) {}
    };

    String ReadAsString(IByteSource &stream, size_t maxLength = SIZE_MAX);
}
