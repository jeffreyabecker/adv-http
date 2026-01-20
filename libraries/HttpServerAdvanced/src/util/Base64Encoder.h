#pragma once
#include <Arduino.h>
#include <vector>
#include <map>
#include <initializer_list>
#include <unordered_map>
#include <algorithm>
#include <cstring>

namespace HttpServerAdvanced::Util
{
    class Base64EncoderImpl
    {
    private:
        const char *dictionary_;
        const bool defaultUsePadding_ = false; // Default behavior is to not use padding

    public:
        static constexpr const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        static constexpr const char base64_url_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

        explicit Base64EncoderImpl(const char *dictionary, bool defaultUsePadding = false)
            : dictionary_(dictionary == nullptr ? base64_chars : dictionary), defaultUsePadding_(defaultUsePadding) {}

        // Binary encode methods returning String
        String encode(const uint8_t *input, std::size_t length)
        {
            return encode(input, length, defaultUsePadding_);
        }

        String encode(const uint8_t *input, std::size_t length, bool usePadding)
        {
            // Calculate output size
            std::size_t outputSize = ((length + 2) / 3) * 4;
            if (!usePadding)
            {
                // Adjust for dropped padding
                std::size_t remainder = length % 3;
                if (remainder > 0)
                {
                    outputSize -= (3 - remainder);
                }
            }

            String result;
            result.reserve(outputSize);
            // uint8_t* outData = reinterpret_cast<uint8_t*>(const_cast<char*>(result.c_str()));
            std::size_t outPos = 0;

            int val = 0, valb = -6;
            for (std::size_t i = 0; i < length; i++)
            {
                val = (val << 8) + input[i];
                valb += 8;
                while (valb >= 0)
                {
                    result.setCharAt(outPos++, dictionary_[(val >> valb) & 0x3F]);
                    valb -= 6;
                }
            }
            if (valb > -6)
            {
                result.setCharAt(outPos++, dictionary_[((val << 8) >> (valb + 8)) & 0x3F]);
            }

            // Only add padding if dropPadding is false
            if (!usePadding)
            {
                while (outPos % 4)
                {
                    result.setCharAt(outPos++, '=');
                }
            }
            else
            {
                // Resize result to actual output size
                result = result.substring(0, outPos);
            }

            return result;
        }

        String encode(const String &input)
        {
            return encode(reinterpret_cast<const uint8_t *>(input.c_str()), input.length(), defaultUsePadding_);
        }

        String encode(const String &input, bool usePadding)
        {
            return encode(reinterpret_cast<const uint8_t *>(input.c_str()), input.length(), usePadding);
        }

        // Binary decode methods returning String
        std::vector<uint8_t> decode(const uint8_t *input, std::size_t length)
        {
            // Estimate output size (will be exact or slightly larger)
            std::size_t maxOutputSize = (length * 3) / 4 + 3;
            std::vector<uint8_t> result(maxOutputSize);
            result.reserve(maxOutputSize);

            std::size_t outPos = 0;

            int val = 0, valb = -8;

            for (std::size_t i = 0; i < length; i++)
            {
                if (input[i] == '=')
                    break; // Padding character

                const char *pos = strchr(dictionary_, input[i]);
                if (pos == nullptr)
                    continue; // Skip invalid characters

                val = (val << 6) + (pos - dictionary_);
                valb += 6;
                if (valb >= 0)
                {
                    result[outPos++] = (val >> valb) & 0xFF;
                    valb -= 8;
                }
            }

            // Handle remaining bits if we're allowing no padding
            if (valb > -8)
            {
                // Calculate how many padding characters would be needed
                std::size_t paddingNeeded = (4 - (length % 4)) % 4;

                // Process virtual padding
                for (std::size_t p = 0; p < paddingNeeded; p++)
                {
                    val = val << 6; // Shift as if we read a padding character (0)
                    valb += 6;
                    if (valb >= 0)
                    {
                        result[outPos++] = (val >> valb) & 0xFF;
                        valb -= 8;
                    }
                }
            }
            result.resize(outPos);
            return result;
        }

        std::vector<uint8_t> decode(const String &input)
        {
            return decode(reinterpret_cast<const uint8_t *>(input.c_str()), reinterpret_cast<std::size_t>(input.length()));
        }

    };

    Base64EncoderImpl Base64(Base64EncoderImpl::base64_chars, false);
    Base64EncoderImpl Base64Url(Base64EncoderImpl::base64_url_chars, true);
}