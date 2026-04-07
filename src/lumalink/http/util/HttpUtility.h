#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "KeyValuePairView.h"

namespace lumalink::http::util
{
    class WebUtility
    {
    public:
        using QueryParameter = std::pair<std::string, std::string>;
        using QueryParameters = KeyValuePairView<std::string, std::string>;

        static QueryParameters ParseQueryParameters(const char *query, std::size_t length);
        static QueryParameters ParseQueryParameters(std::string_view query);

        static std::vector<QueryParameter> ParseQueryString(const char *query, std::size_t length);
        static std::vector<QueryParameter> ParseQueryString(std::string_view query);

        static std::string DecodeURIComponent(const char *str, std::size_t length);
        static std::string DecodeURIComponent(std::string_view str);

        static std::string EncodeURIComponent(const char *str, std::size_t length);
        static std::string EncodeURIComponent(std::string_view str);

        static std::string HtmlEncode(const char *str, std::size_t length);
        static std::string HtmlEncode(const char *input);
        static std::string HtmlEncode(std::string_view str);

        static std::string HtmlAttributeEncode(const char *input, std::size_t length);
        static std::string HtmlAttributeEncode(const char *input);
        static std::string HtmlAttributeEncode(std::string_view str);

        static std::string JavaScriptStringEncode(std::string_view input, bool includeQuotes = true);

        static std::string Base64Encode(const uint8_t *data, std::size_t length, bool urlCompatible = false);
        static std::string Base64Encode(std::string_view input, bool urlCompatible = false);
        static std::vector<uint8_t> Base64Decode(std::string_view input, bool urlCompatible = false);
        static std::string Base64DecodeToString(const char *data, std::size_t length, bool urlCompatible = false);
        static std::string Base64DecodeToString(std::string_view input, bool urlCompatible = false);
    };

} // namespace lumalink::http::util
