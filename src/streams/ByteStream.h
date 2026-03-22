#pragma once

#include "../compat/Availability.h"
#include "../compat/Span.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string_view>
#include <utility>

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
}