#pragma once

#include <cstddef>
#include <cstdint>

#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace HttpServerAdvanced
{
    namespace Compat
    {
#ifdef ARDUINO
        using Stream = ::Stream;
#else
        class Stream
        {
        public:
            virtual ~Stream() = default;

            virtual int available() = 0;
            virtual int read() = 0;
            virtual int peek() = 0;
            virtual std::size_t write(uint8_t byte) = 0;
            virtual void flush() = 0;

            // Arduino's Print base exposes this, and one in-tree stream type
            // overrides it today even though the HTTP pipeline does not yet
            // depend on it directly.
            virtual int availableForWrite()
            {
                return 0;
            }
        };
#endif
    }

    using Stream = Compat::Stream;
}