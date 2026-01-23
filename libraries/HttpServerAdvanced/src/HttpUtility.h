#pragma once
#include <Arduino.h>
#include "./StringUtility.h"
#include "./StringView.h"
#include <vector>
#include <map>
#include <initializer_list>
#include <unordered_map>
#include <algorithm>
#include <optional>

namespace HttpServerAdvanced
{
    class WebUtility
    {

    public:
        static std::vector<std::pair<String, String>> ParseQueryString(const char *query, std::size_t length);
        static std::vector<std::pair<String, String>> ParseQueryString(const String &query);
        static std::vector<std::pair<String, String>> ParseQueryString(const StringView &query);
        static String DecodeURIComponent(const String &str);
        static String DecodeURIComponent(const char *str, std::size_t length);
        static String DecodeURIComponent(const StringView &str);


        static String EncodeURIComponent(const char *str, std::size_t length);
        static String EncodeURIComponent(const String &str);
        static String EncodeURIComponent(const StringView &str);

        static String HtmlEncode(const char *str, std::size_t length);
        static String HtmlEncode(const String &str);
        static String HtmlEncode(const char *input);
        static String HtmlEncode(const StringView &str);

        static String HtmlAttributeEncode(const char *input, std::size_t length);
        static String HtmlAttributeEncode(const String &input);
        static String HtmlAttributeEncode(const char *input);
        static String HtmlAttributeEncode(const StringView &str);

        static String JavaScriptStringEncode(const String &input, bool includeQuotes = true);

        static String Base64Encode(const String &input, bool urlCompatible = false);
        static String Base64Encode(const uint8_t *data, std::size_t length, bool urlCompatible = false);
        static String Base64Encode(const StringView &input, bool urlCompatible = false);
        static std::vector<uint8_t> Base64Decode(const String &input, bool urlCompatible = false);
        static std::vector<uint8_t> Base64Decode(const StringView &input, bool urlCompatible = false);
        static String Base64DecodeToString(const String &input, bool urlCompatible = false);
        static String Base64DecodeToString(const char *data, std::size_t length, bool urlCompatible = false);
        static String Base64DecodeToString(const StringView &input, bool urlCompatible = false);


    };

} // namespace HttpServerAdvanced
