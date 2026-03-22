#pragma once

#include <array>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

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

            std::string toString() const
            {
                return std::to_string(octets_[0]) + "." +
                       std::to_string(octets_[1]) + "." +
                       std::to_string(octets_[2]) + "." +
                       std::to_string(octets_[3]);
            }

            static std::optional<IpAddress> tryParse(std::string_view text)
            {
                std::array<uint8_t, 4> octets{};
                std::size_t octetIndex = 0;
                std::size_t start = 0;

                while (start <= text.size() && octetIndex < octets.size())
                {
                    const std::size_t end = text.find('.', start);
                    const std::size_t length = (end == std::string_view::npos ? text.size() : end) - start;
                    if (length == 0)
                    {
                        return std::nullopt;
                    }

                    unsigned value = 0;
                    const char *begin = text.data() + start;
                    const char *finish = begin + length;
                    const auto result = std::from_chars(begin, finish, value);
                    if (result.ec != std::errc() || result.ptr != finish || value > 255U)
                    {
                        return std::nullopt;
                    }

                    octets[octetIndex++] = static_cast<uint8_t>(value);
                    if (end == std::string_view::npos)
                    {
                        break;
                    }

                    start = end + 1;
                }

                if (octetIndex != octets.size())
                {
                    return std::nullopt;
                }

                return IpAddress(octets[0], octets[1], octets[2], octets[3]);
            }

        private:
            std::array<uint8_t, 4> octets_{};
        };

        static constexpr uint32_t IpAddressAny = 0;
    }

    using IPAddress = Compat::IpAddress;
}