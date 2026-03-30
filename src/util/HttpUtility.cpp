#include "HttpUtility.h"

#include <cstdio>
#include <cstring>
#include <string_view>

#include "../compat/ByteStream.h"
#include "../streams/Base64Stream.h"
#include "../streams/UriStream.h"

namespace httpadv::v1::util
{
    namespace
    {
        static constexpr const char *HtmlAmp = "&amp;";
        static constexpr const char *HtmlLt = "&lt;";
        static constexpr const char *HtmlGt = "&gt;";
        static constexpr const char *HtmlQuot = "&quot;";
        static constexpr const char *HtmlApos = "&#39;";

        std::vector<WebUtility::QueryParameter> toOwnedQueryPairs(const WebUtility::QueryParameters &params)
        {
            std::vector<WebUtility::QueryParameter> converted;
            converted.reserve(params.pairs().size());
            for (const auto &pair : params.pairs())
            {
                converted.emplace_back(pair.first, pair.second);
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
                params.emplace_back(DecodeURIComponent(query + pos, ampPos - pos), std::string());
            }
            else
            {
                const size_t eqPos = pos + eqOffset;
                const size_t valueStart = eqPos + 1;
                const size_t valueLength = ampPos > valueStart ? ampPos - valueStart : 0;
                params.emplace_back(
                    DecodeURIComponent(query + pos, eqPos - pos),
                    DecodeURIComponent(query + valueStart, valueLength));
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

    std::vector<WebUtility::QueryParameter> WebUtility::ParseQueryString(const char *query, std::size_t length)
    {
        return toOwnedQueryPairs(ParseQueryParameters(query, length));
    }

    std::vector<WebUtility::QueryParameter> WebUtility::ParseQueryString(std::string_view query)
    {
        return ParseQueryString(query.data(), query.size());
    }

    std::string WebUtility::DecodeURIComponent(const char *str, std::size_t length)
    {
        httpadv::v1::streams::UriDecodingStream uriStream(reinterpret_cast<const uint8_t *>(str), length);
        auto decoded = httpadv::v1::transport::ReadAsVector(uriStream);
        return std::string(decoded.begin(), decoded.end());
    }

    std::string WebUtility::DecodeURIComponent(std::string_view str)
    {
        return DecodeURIComponent(str.data(), str.size());
    }

    std::string WebUtility::EncodeURIComponent(const char *str, std::size_t length)
    {
        httpadv::v1::streams::UriEncodingStream uriStream(reinterpret_cast<const uint8_t *>(str), length);
        auto encoded = httpadv::v1::transport::ReadAsVector(uriStream);
        return std::string(encoded.begin(), encoded.end());
    }

    std::string WebUtility::EncodeURIComponent(std::string_view str)
    {
        return EncodeURIComponent(str.data(), str.size());
    }

    std::string WebUtility::HtmlEncode(const char *str, std::size_t length)
    {
        if (str == nullptr || length == 0)
        {
            return std::string();
        }

        std::string output;
        output.reserve(length);
        for (size_t i = 0; i < length; ++i)
        {
            const char c = str[i];
            if (c == '&')
            {
                output += HtmlAmp;
            }
            else if (c == '<')
            {
                output += HtmlLt;
            }
            else if (c == '>')
            {
                output += HtmlGt;
            }
            else if (c == '"')
            {
                output += HtmlQuot;
            }
            else if (c == '\'')
            {
                output += HtmlApos;
            }
            else
            {
                output += c;
            }
        }
        return output;
    }

    std::string WebUtility::HtmlEncode(const char *input)
    {
        return input != nullptr ? HtmlEncode(input, std::strlen(input)) : std::string();
    }

    std::string WebUtility::HtmlEncode(std::string_view str)
    {
        return HtmlEncode(str.data(), str.size());
    }

    std::string WebUtility::HtmlAttributeEncode(const char *input, std::size_t length)
    {
        if (input == nullptr || length == 0)
        {
            return std::string();
        }

        std::string output;
        output.reserve(length);
        for (size_t i = 0; i < length; ++i)
        {
            const char c = input[i];
            if (c == '&')
            {
                output += HtmlAmp;
            }
            else if (c == '"')
            {
                output += HtmlQuot;
            }
            else if (c == '\'')
            {
                output += HtmlApos;
            }
            else if (c == '<')
            {
                output += HtmlLt;
            }
            else if (c == '>')
            {
                output += HtmlGt;
            }
            else
            {
                output += c;
            }
        }
        return output;
    }

    std::string WebUtility::HtmlAttributeEncode(const char *input)
    {
        return input != nullptr ? HtmlAttributeEncode(input, std::strlen(input)) : std::string();
    }

    std::string WebUtility::HtmlAttributeEncode(std::string_view str)
    {
        return HtmlAttributeEncode(str.data(), str.size());
    }

    std::string WebUtility::JavaScriptStringEncode(std::string_view input, bool includeQuotes)
    {
        if (input.empty())
        {
            return includeQuotes ? std::string("\"\"") : std::string();
        }

        std::string output;
        output.reserve(input.length() + (includeQuotes ? 2 : 0));
        if (includeQuotes)
        {
            output += '"';
        }

        for (const char c : input)
        {
            switch (c)
            {
            case '"':
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
                    std::snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned char>(c));
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
        {
            output += '"';
        }

        return output;
    }

    std::string WebUtility::Base64Encode(const uint8_t *data, std::size_t length, bool urlCompatible)
    {
        httpadv::v1::streams::Base64EncoderStream base64Stream = httpadv::v1::streams::Base64EncoderStream::create(data, length, urlCompatible);
        return httpadv::v1::transport::ReadAsStdString(base64Stream);
    }

    std::string WebUtility::Base64Encode(std::string_view input, bool urlCompatible)
    {
        return Base64Encode(reinterpret_cast<const uint8_t *>(input.data()), input.size(), urlCompatible);
    }

    std::vector<uint8_t> WebUtility::Base64Decode(std::string_view input, bool urlCompatible)
    {
        httpadv::v1::streams::Base64DecoderStream base64Stream = httpadv::v1::streams::Base64DecoderStream::create(reinterpret_cast<const uint8_t *>(input.data()), input.size(), urlCompatible);
        return httpadv::v1::transport::ReadAsVector(base64Stream);
    }

    std::string WebUtility::Base64DecodeToString(const char *data, std::size_t length, bool urlCompatible)
    {
        httpadv::v1::streams::Base64DecoderStream base64Stream = httpadv::v1::streams::Base64DecoderStream::create(reinterpret_cast<const uint8_t *>(data), length, urlCompatible);
        return httpadv::v1::transport::ReadAsStdString(base64Stream);
    }

    std::string WebUtility::Base64DecodeToString(std::string_view input, bool urlCompatible)
    {
        return Base64DecodeToString(input.data(), input.size(), urlCompatible);
    }
} // namespace HttpServerAdvanced
