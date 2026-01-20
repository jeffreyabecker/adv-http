#pragma once
#include <Arduino.h>
#include "./util/Util.h"
#include <vector>
#include <map>
#include <initializer_list>
#include <unordered_map>
#include <algorithm>


namespace HttpServerAdvanced
{
    class HttpUtility
    {

    public:
        static std::vector<std::pair<String, String>> parseQueryString(const StringView &query)
        {
            std::vector<std::pair<String, String>> params;
            size_t pos = 0;
            while (pos < query.length())
            {
                size_t amp_pos = query.indexOf('&', pos);
                size_t eq_pos = query.indexOf('=', pos);
                if (eq_pos == -1 || (amp_pos != -1 && amp_pos < eq_pos))
                {
                    // No '=' found, or '&' comes before '='
                    String key = decodeURIComponent(query.substring(pos, amp_pos == -1 ? query.length() : amp_pos));
                    params.emplace_back(key, String());
                }
                else
                {
                    String key = decodeURIComponent(query.substring(pos, eq_pos));
                    String value;
                    if (amp_pos == -1)
                    {
                        value = decodeURIComponent(query.substring(eq_pos + 1));
                    }
                    else
                    {
                        value = decodeURIComponent(query.substring(eq_pos + 1, amp_pos));
                    }
                    params.emplace_back(key, value);
                }
                if (amp_pos == -1)
                {
                    break;
                }
                pos = amp_pos + 1;
            }
            return params;
        }
        static String decodeURIComponent(const StringView &str)
        {
            String ret;
            for (size_t i = 0; i < str.length(); ++i)
            {
                if (str[i] == '+')
                {
                    ret += ' ';
                }
                else if (str[i] == '%' && i + 2 < str.length())
                {
                    char hex[3] = {str[i + 1], str[i + 2], '\0'};
                    char decoded = static_cast<char>(strtol(hex, nullptr, 16));
                    ret += decoded;
                    i += 2;
                }
                else
                {
                    ret += str[i];
                }
            }
            return ret;
        }

        // Decodes a URL-encoded string - const char* overload delegates to StringView implementation
        static String decodeURIComponent(const char *str, std::size_t length)
        {
            return decodeURIComponent(StringView(str, length));
        }

        // Decodes a URL-encoded string - String overload delegates to StringView implementation
        static String decodeURIComponent(const String &str)
        {
            return decodeURIComponent(StringView(str));
        }

        // Serializes any container of key-value pairs into a URL-encoded string
        template <typename Container>
        static String serializeQueryString(const Container &params)
        {
            String result;
            std::vector<std::pair<StringView, StringView>> param_list;
            for (const auto &kv : params) {
                param_list.emplace_back(StringView(kv.first), StringView(kv.second));
            }
            size_t total_size = 0;
            for (const auto &kv : param_list) {
                String encoded_key = encodeURIComponent(kv.first);
                String encoded_value = encodeURIComponent(kv.second);
                total_size += encoded_key.length() + encoded_value.length() + 1; // +1 for '='
            }
            if (param_list.size() > 1) {
                total_size += param_list.size() - 1; // for '&'
            }
            result.reserve(total_size);
            bool first = true;
            for (const auto &kv : param_list)
            {
                if (!first)
                    result += '&';
                result += encodeURIComponent(kv.first) + '=' + encodeURIComponent(kv.second);
                first = false;
            }
            return result;
        }
        // Overload for initializer_list
        static String serializeQueryString(std::initializer_list<std::pair<String, String>> params)
        {
            return serializeQueryString<std::initializer_list<std::pair<String, String>>>(params);
        }

        // Encodes a string for URL encoding - StringView implementation (core logic)
        static String encodeURIComponent(const StringView &str)
        {
            String ret;
            const char *hex = "0123456789ABCDEF";
            for (size_t i = 0; i < str.length(); ++i)
            {
                char c = str[i];
                if ((c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') ||
                    c == '-' || c == '_' || c == '.' || c == '~')
                {
                    ret += c;
                }
                else if (c == ' ')
                {
                    ret += '+';
                }
                else
                {
                    unsigned char byte = static_cast<unsigned char>(c);
                    ret += '%';
                    ret += hex[(byte >> 4) & 0xF];
                    ret += hex[byte & 0xF];
                }
            }
            return ret;
        }

        // Encodes a string for URL encoding - primary implementation (delegates to StringView)
        static String encodeURIComponent(const char *str, std::size_t length)
        {
            return encodeURIComponent(StringView(str, length));
        }


        static String htmlEncode(const char *str, std::size_t length)
        {
            return htmlEncode(StringView(str, length));
        }
        static String htmlEncode(StringView str)
        {
            if (str.isEmpty())
                return String();

            String output;
            output.reserve(str.length());
            for (size_t i = 0; i < str.length(); ++i)
            {
                char c = str[i];
                // Encode special HTML characters
                if (c == '&')
                {
                    output += F("&amp;");
                }
                else if (c == '<')
                {
                    output += F("&lt;");
                }
                else if (c == '>')
                {
                    output += F("&gt;");
                }
                else if (c == '"')
                {
                    output += F("&quot;");
                }
                else if (c == '\'')
                {
                    output += F("&#39;");
                }
                else
                {
                    output += c;
                }
            }
            return output;
        }

        // Overload for String - delegates to main implementation

        // Overload for const char* - delegates to main implementation
        static String htmlEncode(const char *input)
        {
            return htmlEncode(StringView(input));
        }
        static String htmlAttributeEncode(const char *input, std::size_t length)
        {
            return htmlAttributeEncode(StringView(input, length));
        }
        static String htmlAttributeEncode(StringView input)
        {
            if (input.isEmpty())
                return String();

            String output;
            output.reserve(input.length());
            for (size_t i = 0; i < input.length(); ++i)
            {
                char c = input[i];
                // Encode special HTML attribute characters
                if (c == '&')
                {
                    output += F("&amp;");
                }
                else if (c == '"')
                {
                    output += F("&quot;");
                }
                else if (c == '\'')
                {
                    output += F("&#39;");
                }
                else if (c == '<')
                {
                    output += F("&lt;");
                }
                else if (c == '>')
                {
                    output += F("&gt;");
                }
                else
                {
                    output += c;
                }
            }
            return output;
        }

        // Overload for null-terminated string
        static String htmlAttributeEncode(const char *input)
        {
            return htmlAttributeEncode(StringView(input));
        }

        // Overload for String




        static String javascriptStringEncode(StringView input, bool includeQuotes = true)
        {
            if (input.isEmpty())
                return includeQuotes ? String("\"\"") : String();

            String output;
            output.reserve(input.length() + (includeQuotes ? 2 : 0));
            if (includeQuotes)
                output += '\"';

            for (char c : input)
            {
                switch (c)
                {
                case '\"':
                    output += "\\\"";
                    break;
                case '\\':
                    output += "\\\\";
                    break;
                case '\b':
                    output += "\\b";
                    break;
                case '\f':
                    output += "\\f";
                    break;
                case '\n':
                    output += "\\n";
                    break;
                case '\r':
                    output += "\\r";
                    break;
                case '\t':
                    output += "\\t";
                    break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20 || static_cast<unsigned char>(c) > 0x7E)
                    {
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned char>(c));
                        output += buf;
                    }
                    else
                    {
                        output += c;
                    }
                    break;
                }
            }

            if (includeQuotes)
                output += '\"';

            return output;
        }

        static String base64Encode(StringView input, bool urlCompatible = false)
        {
            if (urlCompatible) {
                return Util::Base64Url.encode(reinterpret_cast<const uint8_t *>(input.begin()), input.length());
            } else {
                return Util::Base64.encode(reinterpret_cast<const uint8_t *>(input.begin()), input.length());
            }
        }
        static String base64Encode(const uint8_t *data, std::size_t length, bool urlCompatible = false)
        {
            if (urlCompatible) {
                return Util::Base64Url.encode(data, length);
            } else {
                return Util::Base64.encode(data, length);
            }
        }
        static std::vector<uint8_t> base64Decode(StringView input, bool urlCompatible = false)
        {
            if (urlCompatible) {
                return Util::Base64Url.decode(reinterpret_cast<const uint8_t *>(input.begin()), input.length());
            } else {
                return Util::Base64.decode(reinterpret_cast<const uint8_t *>(input.begin()), input.length());
            }
        }
        static String base64DecodeToString(StringView input, bool urlCompatible = false)
        {
            return base64DecodeToString(input.begin(), input.length(), urlCompatible);
        }
        static String base64DecodeToString(const char *data, std::size_t length, bool urlCompatible = false)
        {
            auto decodedBytes = base64Decode(StringView(data, length), urlCompatible);
            return String(reinterpret_cast<const char *>(decodedBytes.data()), decodedBytes.size());
        }
    };

} // namespace ExtendedHttp
