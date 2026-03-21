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

#ifdef ARDUINO
            explicit IpAddress(const ::IPAddress &address)
                : octets_{address[0], address[1], address[2], address[3]}
            {
            }
#endif

            constexpr uint8_t operator[](std::size_t index) const
            {
                return octets_[index];
            }

            constexpr const std::array<uint8_t, 4> &octets() const
            {
                return octets_;
            }

            constexpr uint32_t toUint32() const
            {
                return (static_cast<uint32_t>(octets_[0]) << 24) |
                       (static_cast<uint32_t>(octets_[1]) << 16) |
                       (static_cast<uint32_t>(octets_[2]) << 8) |
                       static_cast<uint32_t>(octets_[3]);
            }

#ifdef ARDUINO
            operator ::IPAddress() const
            {
                return ::IPAddress(octets_[0], octets_[1], octets_[2], octets_[3]);
            }
#endif

            constexpr bool operator==(const IpAddress &other) const
            {
                return octets_ == other.octets_;
            }

            constexpr bool operator!=(const IpAddress &other) const
            {
                return !(*this == other);
            }

        private:
            std::array<uint8_t, 4> octets_{};
        };

        static constexpr uint32_t IpAddressAny = 0;
    }

    using IPAddress = Compat::IpAddress;
}