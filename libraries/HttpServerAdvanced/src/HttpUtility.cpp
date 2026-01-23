#include "./HttpUtility.h"
#include <optional>
#include "./HttpUtility.h"
#include "./Base64Stream.h"
#include "./Streams.h"
#include "./UriStream.h"
namespace HttpServerAdvanced
{

    std::vector<std::pair<String, String>> WebUtility::ParseQueryString(const char *query, std::size_t length)
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

    std::vector<std::pair<String, String>> WebUtility::ParseQueryString(const String &query)
    {
        return ParseQueryString(query.c_str(), query.length());
    }

    std::vector<std::pair<String, String>> WebUtility::ParseQueryString(const StringView &query)
    {
        return ParseQueryString(query.begin(), query.length());
    }

    String WebUtility::DecodeURIComponent(const String &str)
    {
        return DecodeURIComponent(str.c_str(), str.length());
    }

    String WebUtility::DecodeURIComponent(const StringView &str)
    {
        return DecodeURIComponent(str.begin(), str.length());
    }

    String WebUtility::DecodeURIComponent(const char *str, std::size_t length)
    {
        UriDecodingStream uriStream(reinterpret_cast<const uint8_t *>(str), length);
        return ReadAsString(uriStream);
    }

    String WebUtility::EncodeURIComponent(const char *str, std::size_t length)
    {
        UriEncodingStream uriStream(reinterpret_cast<const uint8_t *>(str), length);
        return ReadAsString(uriStream);
    }

    String WebUtility::EncodeURIComponent(const String &str)
    {
        return EncodeURIComponent(str.c_str(), str.length());
    }

    String WebUtility::EncodeURIComponent(const StringView &str)
    {
        return EncodeURIComponent(str.begin(), str.length());
    }

    String WebUtility::HtmlEncode(const char *str, std::size_t length)
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

    String WebUtility::HtmlEncode(const String &str)
    {
        return HtmlEncode(str.c_str(), str.length());
    }

    String WebUtility::HtmlEncode(const char *input)
    {
        return HtmlEncode(String(input));
    }

    String WebUtility::HtmlEncode(const StringView &str)
    {
        return HtmlEncode(str.begin(), str.length());
    }

    String WebUtility::HtmlAttributeEncode(const char *input, std::size_t length)
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

    String WebUtility::HtmlAttributeEncode(const String &input)
    {
        return HtmlAttributeEncode(input.c_str(), input.length());
    }

    String WebUtility::HtmlAttributeEncode(const char *input)
    {
        return HtmlAttributeEncode(String(input));
    }

    String WebUtility::HtmlAttributeEncode(const StringView &str)
    {
        return HtmlAttributeEncode(str.begin(), str.length());
    }

    String WebUtility::JavaScriptStringEncode(const String &input, bool includeQuotes)
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

    String WebUtility::Base64Encode(const String &input, bool urlCompatible)
    {
        Base64EncoderStream base64Stream = Base64EncoderStream::create(input, urlCompatible);
        return ReadAsString(base64Stream);
    }

    String WebUtility::Base64Encode(const uint8_t *data, std::size_t length, bool urlCompatible)
    {
        Base64EncoderStream base64Stream = Base64EncoderStream::create(data, length, urlCompatible);
        return ReadAsString(base64Stream);
    }

    std::vector<uint8_t> WebUtility::Base64Decode(const String &input, bool urlCompatible)
    {
        Base64DecoderStream base64Stream = Base64DecoderStream::create(input, urlCompatible);
        return ReadAsVector(base64Stream);
    }

    std::vector<uint8_t> WebUtility::Base64Decode(const StringView &input, bool urlCompatible)
    {
        Base64DecoderStream base64Stream = Base64DecoderStream::create(reinterpret_cast<const uint8_t *>(input.c_str()), input.length(), urlCompatible);
        return ReadAsVector(base64Stream);
    }

    String WebUtility::Base64Encode(const StringView &input, bool urlCompatible)
    {
        return Base64Encode(reinterpret_cast<const uint8_t *>(input.begin()), input.length(), urlCompatible);
    }

    String WebUtility::Base64DecodeToString(const String &input, bool urlCompatible)
    {
        Base64DecoderStream base64Stream = Base64DecoderStream::create(input, urlCompatible);
        return ReadAsString(base64Stream);
    }

    String WebUtility::Base64DecodeToString(const char *data, std::size_t length, bool urlCompatible)
    {
        Base64DecoderStream base64Stream = Base64DecoderStream::create(reinterpret_cast<const uint8_t *>(data), length, urlCompatible);
        return ReadAsString(base64Stream);
    }

    String WebUtility::Base64DecodeToString(const StringView &input, bool urlCompatible)
    {
        return Base64DecodeToString(input.begin(), input.length(), urlCompatible);
    }


} // namespace HttpServerAdvanced
