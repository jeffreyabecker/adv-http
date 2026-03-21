#pragma once

#include "../compat/Availability.h"
#include "../compat/Stream.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

namespace HttpServerAdvanced
{
    class IByteSource
    {
    public:
        virtual ~IByteSource() = default;

        virtual AvailableResult available() = 0;
        virtual int read() = 0;
        virtual int peek() = 0;
    };

    class IByteSink
    {
    public:
        virtual ~IByteSink() = default;

        virtual std::size_t write(std::uint8_t byte) = 0;

        virtual std::size_t write(const std::uint8_t *buffer, std::size_t size)
        {
            if (buffer == nullptr)
            {
                return 0;
            }

            std::size_t written = 0;
            while (written < size)
            {
                if (write(buffer[written]) == 0)
                {
                    break;
                }

                ++written;
            }

            return written;
        }

        std::size_t write(const char *buffer, std::size_t size)
        {
            return write(reinterpret_cast<const std::uint8_t *>(buffer), size);
        }

        virtual void flush() = 0;
    };

    class IByteChannel : public IByteSource, public IByteSink
    {
    public:
        ~IByteChannel() override = default;
    };

    class StreamByteSourceAdapter : public IByteSource
    {
    public:
        explicit StreamByteSourceAdapter(Stream &stream)
            : stream_(stream)
        {
        }

        AvailableResult available() override
        {
            const int result = stream_.available();
            if (result > 0)
            {
                return AvailableBytes(static_cast<std::size_t>(result));
            }

            if (result == 0)
            {
                return ExhaustedResult();
            }

            return TemporarilyUnavailableResult();
        }

        int read() override
        {
            return stream_.read();
        }

        int peek() override
        {
            return stream_.peek();
        }

    private:
        Stream &stream_;
    };

    class OwningStreamByteSourceAdapter : public IByteSource
    {
    public:
        explicit OwningStreamByteSourceAdapter(std::unique_ptr<Stream> stream)
            : stream_(std::move(stream))
        {
        }

        AvailableResult available() override
        {
            if (!stream_)
            {
                return ExhaustedResult();
            }

            const int result = stream_->available();
            if (result > 0)
            {
                return AvailableBytes(static_cast<std::size_t>(result));
            }

            if (result == 0)
            {
                return ExhaustedResult();
            }

            return TemporarilyUnavailableResult();
        }

        int read() override
        {
            return stream_ ? stream_->read() : -1;
        }

        int peek() override
        {
            return stream_ ? stream_->peek() : -1;
        }

        Stream *get() const
        {
            return stream_.get();
        }

        std::unique_ptr<Stream> release()
        {
            return std::move(stream_);
        }

    private:
        std::unique_ptr<Stream> stream_;
    };

    class StreamByteSinkAdapter : public IByteSink
    {
    public:
        explicit StreamByteSinkAdapter(Stream &stream)
            : stream_(stream)
        {
        }

        std::size_t write(std::uint8_t byte) override
        {
            return stream_.write(byte);
        }

        std::size_t write(const std::uint8_t *buffer, std::size_t size) override
        {
            return stream_.write(buffer, size);
        }

        void flush() override
        {
            stream_.flush();
        }

    private:
        Stream &stream_;
    };

    class ByteSourceStreamAdapter : public Stream
    {
    public:
        explicit ByteSourceStreamAdapter(std::unique_ptr<IByteSource> source)
            : source_(std::move(source))
        {
        }

        int available() override
        {
            if (!source_)
            {
                return 0;
            }

            const AvailableResult result = source_->available();
            switch (result.state)
            {
            case AvailabilityState::HasBytes:
                return static_cast<int>(std::min<std::size_t>(result.count, static_cast<std::size_t>(std::numeric_limits<int>::max())));

            case AvailabilityState::Exhausted:
                return 0;

            case AvailabilityState::TemporarilyUnavailable:
            case AvailabilityState::Error:
            default:
                return -1;
            }
        }

        int read() override
        {
            return source_ ? source_->read() : -1;
        }

        int peek() override
        {
            return source_ ? source_->peek() : -1;
        }

        std::size_t write(std::uint8_t) override
        {
            return 0;
        }

        IByteSource *get() const
        {
            return source_.get();
        }

        std::unique_ptr<IByteSource> release()
        {
            return std::move(source_);
        }

    private:
        std::unique_ptr<IByteSource> source_;
    };
}