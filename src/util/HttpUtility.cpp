#include "HttpUtility.h"

#include <Arduino.h>
#include <string_view>

#include "StringView.h"
#include "../streams/Base64Stream.h"
#include "../streams/Streams.h"
#include "../streams/UriStream.h"

namespace HttpServerAdvanced
{
    namespace
    {
        std::vector<std::pair<String, String>> toArduinoQueryPairs(const WebUtility::QueryParameters &params)
        {
            std::vector<std::pair<String, String>> converted;
            converted.reserve(params.pairs().size());
            for (const auto &pair : params.pairs())
            {
                converted.emplace_back(String(pair.first.c_str()), String(pair.second.c_str()));
            }
            return converted;
        }
    }

    WebUtility::QueryParameters WebUtility::ParseQueryParameters(const char *query, std::size_t length)
    {
        std::vector<QueryParameter> params;
        if (query == nullptr || length == 0)
        {
            return QueryParameters(std::move(params));
        }

        size_t pos = 0;
        while (pos < length)
        {
            std::string_view remaining(query + pos, length - pos);
            const size_t ampOffset = remaining.find('&');
            const size_t eqOffset = remaining.find('=');
            const bool hasAmp = ampOffset != std::string_view::npos;
            const bool hasEq = eqOffset != std::string_view::npos;
            const size_t ampPos = hasAmp ? pos + ampOffset : length;
            if (!hasEq || (hasAmp && ampOffset < eqOffset))
            {
                params.emplace_back(DecodeURIComponentToString(query + pos, ampPos - pos), std::string());
            }
            else
            {
                const size_t eqPos = pos + eqOffset;
                const size_t valueStart = eqPos + 1;
                const size_t valueLength = ampPos > valueStart ? ampPos - valueStart : 0;
                params.emplace_back(
                    DecodeURIComponentToString(query + pos, eqPos - pos),
                    DecodeURIComponentToString(query + valueStart, valueLength));
            }

            if (!hasAmp)
            {
                break;
            }
            pos = ampPos + 1;
        }
        return QueryParameters(std::move(params));
    }

    WebUtility::QueryParameters WebUtility::ParseQueryParameters(std::string_view query)
    {
        return ParseQueryParameters(query.data(), query.size());
    }

    std::vector<std::pair<String, String>> WebUtility::ParseQueryString(const char *query, std::size_t length)
    {
        return toArduinoQueryPairs(ParseQueryParameters(query, length));
    }

    std::vector<std::pair<String, String>> WebUtility::ParseQueryString(const String &query)
    {
        return ParseQueryString(query.c_str(), query.length());
    }

    std::vector<std::pair<String, String>> WebUtility::ParseQueryString(const StringView &query)
    {
        return ParseQueryString(query.begin(), query.length());
    }

    std::string WebUtility::DecodeURIComponentToString(const char *str, std::size_t length)
    {
        UriDecodingStream uriStream(reinterpret_cast<const uint8_t *>(str), length);
        auto decoded = ReadAsVector(uriStream);
        return std::string(decoded.begin(), decoded.end());
    }

    std::string WebUtility::DecodeURIComponentToString(std::string_view str)
    {
        return DecodeURIComponentToString(str.data(), str.size());
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
        const std::string decoded = DecodeURIComponentToString(str, length);
        return String(decoded.c_str());
    }

    std::string WebUtility::EncodeURIComponentToString(const char *str, std::size_t length)
    {
        UriEncodingStream uriStream(reinterpret_cast<const uint8_t *>(str), length);
        auto encoded = ReadAsVector(uriStream);
        return std::string(encoded.begin(), encoded.end());
    }

    std::string WebUtility::EncodeURIComponentToString(std::string_view str)
    {
        return EncodeURIComponentToString(str.data(), str.size());
    }

    String WebUtility::EncodeURIComponent(const char *str, std::size_t length)
    {
        const std::string encoded = EncodeURIComponentToString(str, length);
        return String(encoded.c_str());
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
