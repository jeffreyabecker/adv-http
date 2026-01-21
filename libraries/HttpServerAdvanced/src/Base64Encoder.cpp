#include "./Base64Encoder.h"
#include <cstring>

namespace HttpServerAdvanced {

// Define static constexpr arrays
constexpr const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr const char base64_url_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

String Base64EncoderImpl::encode(const uint8_t *input, std::size_t length) {
    return encode(input, length, defaultUsePadding_);
}

String Base64EncoderImpl::encode(const uint8_t *input, std::size_t length, bool usePadding) {
    std::size_t outputSize = ((length + 2) / 3) * 4;
    if (!usePadding) {
        std::size_t remainder = length % 3;
        if (remainder > 0) {
            outputSize -= (3 - remainder);
        }
    }

    String result;
    result.reserve(outputSize);
    std::size_t outPos = 0;

    int val = 0, valb = -6;
    for (std::size_t i = 0; i < length; i++) {
        val = (val << 8) + input[i];
        valb += 8;
        while (valb >= 0) {
            result.setCharAt(outPos++, dictionary_[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        result.setCharAt(outPos++, dictionary_[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    if (!usePadding) {
        while (outPos % 4) {
            result.setCharAt(outPos++, '=');
        }
    } else {
        result = result.substring(0, outPos);
    }

    return result;
}

String Base64EncoderImpl::encode(const String &input) {
    return encode(reinterpret_cast<const uint8_t *>(input.c_str()), input.length(), defaultUsePadding_);
}

String Base64EncoderImpl::encode(const String &input, bool usePadding) {
    return encode(reinterpret_cast<const uint8_t *>(input.c_str()), input.length(), usePadding);
}

std::vector<uint8_t> Base64EncoderImpl::decode(const uint8_t *input, std::size_t length) {
    std::size_t maxOutputSize = (length * 3) / 4 + 3;
    std::vector<uint8_t> result(maxOutputSize);
    result.reserve(maxOutputSize);

    std::size_t outPos = 0;

    int val = 0, valb = -8;

    for (std::size_t i = 0; i < length; i++) {
        if (input[i] == '=')
            break;

        const char *pos = strchr(dictionary_, input[i]);
        if (pos == nullptr)
            continue;

        val = (val << 6) + (pos - dictionary_);
        valb += 6;
        if (valb >= 0) {
            result[outPos++] = (val >> valb) & 0xFF;
            valb -= 8;
        }
    }

    if (valb > -8) {
        std::size_t paddingNeeded = (4 - (length % 4)) % 4;

        for (std::size_t p = 0; p < paddingNeeded; p++) {
            val = val << 6;
            valb += 6;
            if (valb >= 0) {
                result[outPos++] = (val >> valb) & 0xFF;
                valb -= 8;
            }
        }
    }
    result.resize(outPos);
    return result;
}

std::vector<uint8_t> Base64EncoderImpl::decode(const String &input) {
    return decode(reinterpret_cast<const uint8_t *>(input.c_str()), static_cast<std::size_t>(input.length()));
}
Base64EncoderImpl::Base64EncoderImpl(const char *dictionary, bool defaultUsePadding)
    : dictionary_(dictionary == nullptr ? base64_chars : dictionary), defaultUsePadding_(defaultUsePadding) {}
// Define global instances
Base64EncoderImpl Base64(base64_chars, false);
Base64EncoderImpl Base64Url(base64_url_chars, true);

} // namespace HttpServerAdvanced
