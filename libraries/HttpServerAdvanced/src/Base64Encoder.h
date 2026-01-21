#pragma once
#include <Arduino.h>
#include <vector>
#include <map>
#include <initializer_list>
#include <unordered_map>
#include <algorithm>
#include <cstring>

namespace HttpServerAdvanced
{
    class Base64EncoderImpl
    {
    private:
        const char *dictionary_;
        const bool defaultUsePadding_ = false; // Default behavior is to not use padding

    public:

        Base64EncoderImpl(const char *dictionary, bool defaultUsePadding = false);


        // Binary encode methods returning String
        String encode(const uint8_t *input, std::size_t length);

        String encode(const uint8_t *input, std::size_t length, bool usePadding);

        String encode(const String &input);

        String encode(const String &input, bool usePadding);

        // Binary decode methods returning String
        std::vector<uint8_t> decode(const uint8_t *input, std::size_t length);

        std::vector<uint8_t> decode(const String &input);

    };
    
    extern Base64EncoderImpl Base64;
    extern Base64EncoderImpl Base64Url;
}