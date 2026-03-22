#pragma once

#include "../compat/Availability.h"
#include "../compat/Span.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace HttpServerAdvanced
{
    inline int LegacyAvailableFromResult(const AvailableResult &result)
    {
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

    class IByteSource
    {
    public:
        virtual ~IByteSource() = default;

        virtual AvailableResult available() = 0;
        virtual size_t read(HttpServerAdvanced::span<uint8_t> buffer) = 0;
        virtual size_t peek(HttpServerAdvanced::span<uint8_t> buffer) = 0;
    };

    class IByteSink
    {
    public:
        virtual ~IByteSink() = default;

        virtual std::size_t write(std::uint8_t byte)
        {
            return write(HttpServerAdvanced::span<const uint8_t>(&byte, 1));
        }

        virtual std::size_t write(HttpServerAdvanced::span<const uint8_t> buffer) = 0;

        std::size_t write(std::string_view buffer)
        {
            return write(HttpServerAdvanced::span<const uint8_t>(reinterpret_cast<const std::uint8_t *>(buffer.data()), buffer.size()));
        }

        virtual void flush() = 0;
    };

    class IByteChannel : public IByteSource, public IByteSink
    {
    public:
        ~IByteChannel() override = default;
    };

    inline int ReadByte(IByteSource &source)
    {
        uint8_t byte = 0;
        return source.read(HttpServerAdvanced::span<uint8_t>(&byte, 1)) == 0 ? -1 : byte;
    }

    inline int PeekByte(IByteSource &source)
    {
        uint8_t byte = 0;
        return source.peek(HttpServerAdvanced::span<uint8_t>(&byte, 1)) == 0 ? -1 : byte;
    }

    class SpanByteSource : public IByteSource
    {
    public:
        SpanByteSource(const uint8_t *data, size_t size)
            : data_(data), size_(size)
        {
        }

        explicit SpanByteSource(std::string_view data)
            : SpanByteSource(reinterpret_cast<const uint8_t *>(data.data()), data.size())
        {
        }

        AvailableResult available() override
        {
            if (position_ >= size_)
            {
                return ExhaustedResult();
            }

            return AvailableBytes(size_ - position_);
        }

        size_t read(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            const size_t copied = peek(buffer);
            position_ += copied;
            return copied;
        }

        size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            if (buffer.empty() || position_ >= size_ || data_ == nullptr)
            {
                return 0;
            }

            const size_t copied = std::min<size_t>(static_cast<size_t>(buffer.size()), size_ - position_);
            std::copy_n(data_ + position_, copied, buffer.data());
            return copied;
        }

    protected:
        const uint8_t *data_ = nullptr;
        size_t size_ = 0;
        size_t position_ = 0;
    };

    class StdStringByteSource : public SpanByteSource
    {
    public:
        explicit StdStringByteSource(std::string data)
            : SpanByteSource(nullptr, 0), storage_(std::move(data))
        {
            data_ = reinterpret_cast<const uint8_t *>(storage_.data());
            size_ = storage_.size();
        }

    private:
        std::string storage_;
    };

    class VectorByteSource : public SpanByteSource
    {
    public:
        explicit VectorByteSource(std::vector<uint8_t> data)
            : SpanByteSource(nullptr, 0), storage_(std::move(data))
        {
            data_ = storage_.data();
            size_ = storage_.size();
        }

    private:
        std::vector<uint8_t> storage_;
    };

    class ConcatByteSource : public IByteSource
    {
    public:
        explicit ConcatByteSource(std::vector<std::unique_ptr<IByteSource>> sources)
            : sources_(std::move(sources))
        {
        }

        AvailableResult available() override
        {
            return currentAvailability();
        }

        size_t read(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            size_t totalRead = 0;
            while (totalRead < buffer.size())
            {
                const AvailableResult availableResult = currentAvailability();
                if (!availableResult.hasBytes())
                {
                    break;
                }

                HttpServerAdvanced::span<uint8_t> destination(buffer.data() + totalRead, buffer.size() - totalRead);
                const size_t bytesRead = sources_[currentIndex_]->read(destination);
                if (bytesRead == 0)
                {
                    break;
                }

                totalRead += bytesRead;
            }

            return totalRead;
        }

        size_t peek(HttpServerAdvanced::span<uint8_t> buffer) override
        {
            const AvailableResult availableResult = currentAvailability();
            if (!availableResult.hasBytes() || buffer.empty())
            {
                return 0;
            }

            return sources_[currentIndex_]->peek(buffer);
        }

    private:
        AvailableResult currentAvailability()
        {
            while (currentIndex_ < sources_.size())
            {
                AvailableResult result = sources_[currentIndex_]->available();
                if (result.isExhausted())
                {
                    ++currentIndex_;
                    continue;
                }

                return result;
            }

            return ExhaustedResult();
        }

        std::vector<std::unique_ptr<IByteSource>> sources_;
        size_t currentIndex_ = 0;
    };
}