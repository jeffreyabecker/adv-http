#pragma once

#include <array>
#include <cstdint>

#ifdef ARDUINO
#include <IPAddress.h>
#endif

namespace HttpServerAdvanced
{
    namespace Compat
    {
#ifdef ARDUINO
        using IpAddress = ::IPAddress;

        static constexpr uint32_t IpAddressAny = IPADDR_ANY;
#else
        class IpAddress
        {
        public:
            constexpr IpAddress() = default;

            constexpr explicit IpAddress(uint32_t address)
                : octets_{
                    static_cast<uint8_t>((address >> 24) & 0xFFU),
                    static_cast<uint8_t>((address >> 16) & 0xFFU),
                    static_cast<uint8_t>((address >> 8) & 0xFFU),
                    static_cast<uint8_t>(address & 0xFFU)}
            {
            }

            constexpr IpAddress(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth)
                : octets_{first, second, third, fourth}
            {
            }

            constexpr uint8_t operator[](std::size_t index) const
            {
                return octets_[index];
            }

            constexpr bool operator==(const IpAddress &other) const = default;

        private:
            std::array<uint8_t, 4> octets_{};
        };

        static constexpr uint32_t IpAddressAny = 0;
#endif
    }

    using IPAddress = Compat::IpAddress;
}