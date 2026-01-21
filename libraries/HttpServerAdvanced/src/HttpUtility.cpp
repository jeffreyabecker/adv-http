#include "./HttpUtility.h"
#include <optional>

namespace HttpServerAdvanced
{

    std::vector<std::pair<String, String>> HttpUtility::ParseQueryString(const char *query, std::size_t length)
    {
        std::vector<std::pair<String, String>> params;
        size_t pos = 0;
        while (pos < length)
        {
            size_t amp_pos = StringUtil::indexOf(query, length, "&", 1, pos);
            size_t eq_pos = StringUtil::indexOf(query, length, "=", 1, pos);
            if (eq_pos == -1 || (amp_pos != -1 && amp_pos < eq_pos))
            {
                // No '=' found, or '&' comes before '='
                String key = DecodeURIComponent(query + pos, (amp_pos == -1 ? length : amp_pos) - pos);
                params.emplace_back(key, String());
            }
            else
            {
                String key = DecodeURIComponent(query + pos, eq_pos - pos);
                String value;
                if (amp_pos == -1)
                {
                    value = DecodeURIComponent(query + eq_pos + 1, length - (eq_pos + 1));
                }
                else
                {
                    value = DecodeURIComponent(query + eq_pos + 1, amp_pos - (eq_pos + 1));
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

    std::vector<std::pair<String, String>> HttpUtility::ParseQueryString(const String &query)
    {
        return ParseQueryString(query.c_str(), query.length());
    }

    std::vector<std::pair<String, String>> HttpUtility::ParseQueryString(const StringView &query)
    {
        return ParseQueryString(query.begin(), query.length());
    }

    String HttpUtility::DecodeURIComponent(const String &str)
    {
        return DecodeURIComponent(str.c_str(), str.length());
    }

    String HttpUtility::DecodeURIComponent(const StringView &str)
    {
        return DecodeURIComponent(str.begin(), str.length());
    }

    String HttpUtility::DecodeURIComponent(const char *str, std::size_t length)
    {
        String ret;
        for (size_t i = 0; i < length; ++i)
        {
            if (str[i] == '+')
            {
                ret += ' ';
            }
            else if (str[i] == '%' && i + 2 < length)
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

    String HttpUtility::EncodeURIComponent(const char *str, std::size_t length)
    {
        String ret;
        const char *hex = "0123456789ABCDEF";
        for (size_t i = 0; i < length; ++i)
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

    String HttpUtility::EncodeURIComponent(const String &str)
    {
        return EncodeURIComponent(str.c_str(), str.length());
    }

    String HttpUtility::EncodeURIComponent(const StringView &str)
    {
        return EncodeURIComponent(str.begin(), str.length());
    }

    String HttpUtility::HtmlEncode(const char *str, std::size_t length)
    {
        if (str == nullptr || length == 0)
            return String();
        String output;
        output.reserve(length);
        for (size_t i = 0; i < length; ++i)
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

    String HttpUtility::HtmlEncode(const String &str)
    {
        return HtmlEncode(str.c_str(), str.length());
    }

    String HttpUtility::HtmlEncode(const char *input)
    {
        return HtmlEncode(String(input));
    }

    String HttpUtility::HtmlEncode(const StringView &str)
    {
        return HtmlEncode(str.begin(), str.length());
    }

    String HttpUtility::HtmlAttributeEncode(const char *input, std::size_t length)
    {
        if (input == nullptr || length == 0)
            return String();

        String output;
        output.reserve(length);
        for (size_t i = 0; i < length; ++i)
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

    String HttpUtility::HtmlAttributeEncode(const String &input)
    {
        return HtmlAttributeEncode(input.c_str(), input.length());
    }

    String HttpUtility::HtmlAttributeEncode(const char *input)
    {
        return HtmlAttributeEncode(String(input));
    }

    String HttpUtility::HtmlAttributeEncode(const StringView &str)
    {
        return HtmlAttributeEncode(str.begin(), str.length());
    }

    String HttpUtility::JavaScriptStringEncode(const String &input, bool includeQuotes)
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

    String HttpUtility::Base64Encode(const String &input, bool urlCompatible)
    {
        if (urlCompatible)
        {
            return Base64Url.encode(reinterpret_cast<const uint8_t *>(input.begin()), input.length());
        }
        else
        {
            return Base64.encode(reinterpret_cast<const uint8_t *>(input.begin()), input.length());
        }
    }

    String HttpUtility::Base64Encode(const uint8_t *data, std::size_t length, bool urlCompatible)
    {
        if (urlCompatible)
        {
            return Base64Url.encode(data, length);
        }
        else
        {
            return Base64.encode(data, length);
        }
    }

    std::vector<uint8_t> HttpUtility::Base64Decode(const String &input, bool urlCompatible)
    {
        if (urlCompatible)
        {
            return Base64Url.decode(reinterpret_cast<const uint8_t *>(input.begin()), input.length());
        }
        else
        {
            return Base64.decode(reinterpret_cast<const uint8_t *>(input.begin()), input.length());
        }
    }

    std::vector<uint8_t> HttpUtility::Base64Decode(const StringView &input, bool urlCompatible)
    {
        if (urlCompatible)
        {
            return Base64Url.decode(reinterpret_cast<const uint8_t *>(input.begin()), input.length());
        }
        else
        {
            return Base64.decode(reinterpret_cast<const uint8_t *>(input.begin()), input.length());
        }
    }

    String HttpUtility::Base64Encode(const StringView &input, bool urlCompatible)
    {
        return Base64Encode(reinterpret_cast<const uint8_t *>(input.begin()), input.length(), urlCompatible);
    }

    String HttpUtility::Base64DecodeToString(const String &input, bool urlCompatible)
    {
        return Base64DecodeToString(input.begin(), input.length(), urlCompatible);
    }

    String HttpUtility::Base64DecodeToString(const char *data, std::size_t length, bool urlCompatible)
    {
        auto decodedBytes = Base64Decode(String(data, length), urlCompatible);
        return String(reinterpret_cast<const char *>(decodedBytes.data()), static_cast<size_t>(decodedBytes.size()));
    }

    String HttpUtility::Base64DecodeToString(const StringView &input, bool urlCompatible)
    {
        return Base64DecodeToString(input.begin(), input.length(), urlCompatible);
    }

    std::vector<std::pair<String, std::optional<String>>> HttpUtility::ParseHeaderDirectives(const StringView &val)
    {
        std::vector<std::pair<String, std::optional<String>>> directives;
        if (val.isEmpty())
        {
            return directives;
        }

        auto parseSingleDirective = [&](const StringView &directive)
        {
            int eqPos = directive.indexOf('=');
            if (eqPos == -1)
            {
                directives.emplace_back(directive, std::nullopt);
            }
            else
            {
                StringView key = directive.substring(0, eqPos);
                StringView value = directive.substring(eqPos + 1);

                directives.emplace_back(key.trim().toString(), value.trim().toString());
            }
        };

        // Split by comma
        int start = 0;
        int end = val.indexOf(',');
        while (end != -1)
        {
            StringView directive = val.substring(start, end);
            directive.trim();
            parseSingleDirective(directive);
            start = end + 1;
            end = val.indexOf(',', start);
        }
        // Last one
        StringView directive = val.substring(start);

        parseSingleDirective(directive);

        return directives;
    }

} // namespace HttpServerAdvanced
