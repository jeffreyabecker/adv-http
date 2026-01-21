#pragma once
#include <Arduino.h>
#include "./StringUtility.h"
#include "./StringView.h"
#include <vector>
#include <map>
#include <initializer_list>
#include <unordered_map>
#include <algorithm>
#include "./Base64Encoder.h"

namespace HttpServerAdvanced
{
    class HttpUtility
    {

    public:
        static std::vector<std::pair<String, String>> ParseQueryString(const char *query, std::size_t length);
        static std::vector<std::pair<String, String>> ParseQueryString(const String &query);
        static std::vector<std::pair<String, String>> ParseQueryString(const StringView &query);
        static String DecodeURIComponent(const String &str);
        static String DecodeURIComponent(const char *str, std::size_t length);
        static String DecodeURIComponent(const StringView &str);

        // // Serializes any container of key-value pairs into a URL-encoded string
        // template <typename Container>
        // static String SerializeQueryString(const Container &params)
        // {
        //     String result;
        //     std::vector<std::pair<String, String>> param_list;
        //     for (const auto &kv : params)
        //     {
        //         param_list.emplace_back(String(kv.first), String(kv.second));
        //     }
        //     size_t total_size = 0;
        //     for (const auto &kv : param_list)
        //     {
        //         String encoded_key = EncodeURIComponent(kv.first);
        //         String encoded_value = EncodeURIComponent(kv.second);
        //         total_size += encoded_key.length() + encoded_value.length() + 1; // +1 for '='
        //     }
        //     if (param_list.size() > 1)
        //     {
        //         total_size += param_list.size() - 1; // for '&'
        //     }
        //     result.reserve(total_size);
        //     bool first = true;
        //     for (const auto &kv : param_list)
        //     {
        //         if (!first)
        //             result += '&';
        //         result += EncodeURIComponent(kv.first) + '=' + EncodeURIComponent(kv.second);
        //         first = false;
        //     }
        //     return result;
        // }
        // Overload for initializer_list
        static String SerializeQueryString(std::initializer_list<std::pair<String, String>> params);

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

        static std::vector<std::pair<String, std::optional<String>>> ParseHeaderDirectives(const StringView &val);
    };

} // namespace HttpServerAdvanced
