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
            virtual std::size_t write(const uint8_t *buffer, std::size_t size)
            {
                if (buffer == nullptr)
                {
                    return 0;
                }

                std::size_t written = 0;
                while (size-- > 0)
                {
                    if (write(*buffer++) == 0)
                    {
                        break;
                    }
                    ++written;
                }
                return written;
            }

            std::size_t write(const char *buffer, std::size_t size)
            {
                return write(reinterpret_cast<const uint8_t *>(buffer), size);
            }

            virtual void flush()
            {
            }

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